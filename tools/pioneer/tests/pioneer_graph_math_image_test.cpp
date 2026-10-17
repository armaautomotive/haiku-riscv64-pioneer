/* SPDX-License-Identifier: MIT */
#define PIONEER_COORDINATED_MACHINE_ONLY
#include "pioneer_coordinated_worker_test.cpp"
#include "../PioneerGraphNorm.h"
#include "../PioneerGraphRope.h"
#include <map>
#include <string>
struct ScalarMachine:Machine {
	std::vector<uint8_t> code;
	std::map<uint32_t,uint32_t> memory;
	uint32_t fp[32]={};
	unsigned n,mode,minStack=0x70000,outputWrites=0,operandWrites=0,scaleWrites=0;
	ScalarMachine(const char* root,unsigned count,unsigned kind):n(count),mode(kind) {
		std::string directory(root); code=Read((directory+"/code.bin").c_str()); auto boot=Read((directory+"/boot.bin").c_str());
		assert(boot.size()==4 && code.size()<32764 && !(code.size()&3)); l1[0]=Word(boot,0);
		for (unsigned i=0;i<code.size()/4;i++) l1[1+i]=Word(code,i*4);
		memory[0x74000]=0x31504c47; memory[0x74004]=n; memory[0x74008]=mode; memory[0x7400c]=mode==5?(n%1024?128:1024):0;
		memory[0x17ffc0]=memory[0x17ffc4]=memory[0x17ffc8]=0;
		for (unsigned i=0;i<3072;i++) {
			float a=mode==2 || mode==4?float(i+1)*.00137f:float(sin(i*.713)*12.7),b=mode==2?float(i+3)*.0193f:float(cos(i*.219)*3.1);
			memory[0x84000+i*4]=PioneerAttentionBits(a); memory[0x87000+i*4]=PioneerAttentionBits(b);
			memory[0x90000+i*4]=0xdeadbeef;
		}
		for (unsigned i=0;i<1536;i++) memory[0x80000+i*4]=0xdeadbeef;
		if (mode==6) for (unsigned i=0;i<64;i++) { memory[0x87000+i*8]=PioneerAttentionBits(float(cos(i*.193)));
			memory[0x87004+i*8]=PioneerAttentionBits(float(sin(i*.193))); }
		for (unsigned i=0;i<96;i++) memory[0x83000+i*4]=0xdeadbeef;
	}
	static bool Range(uint32_t a,uint32_t first,unsigned bytes) { return a>=first && a-first<bytes; }
	unsigned Index(uint32_t a) override {
		if (a==0x6000) return 0;
		assert(Range(a,0x64000,unsigned(code.size())) && !(a&3)); return 1+(a-0x64000)/4;
	}
	bool CodeAddress(uint32_t a) const override { return a==0x6000 || Range(a,0x64000,unsigned(code.size())); }
	unsigned StepLimit() const override { return 100000000; }
	uint32_t Load(uint32_t a) override {
		assert(!(a&3));
		if (Range(a,0x64000,unsigned(code.size()))) return l1[Index(a)];
		assert(Range(a,0x74000,16) || Range(a,0x6e000,8192) || Range(a,0x84000,n*4) || Range(a,0x87000,n*4)
			|| Range(a,0x90000,n*4) || Range(a,0x80000,n*2) || Range(a,0x83000,n/32*4) || Range(a,0x17ffc0,12));
		assert(memory.count(a)); return memory.at(a);
	}
	void Store(uint32_t a,uint32_t value) override {
		assert(!(a&3));
		assert(Range(a,0x6e000,8192) || Range(a,0x90000,n*4) || Range(a,0x80000,n*2)
			|| Range(a,0x83000,n/32*4) || Range(a,0x17ffc0,12)); // NO code/input/NoC/CSR/PCI/reset writes
		if (Range(a,0x6e000,8192)) minStack=std::min(minStack,unsigned(a));
		if (Range(a,0x90000,n*4)) outputWrites++;
		if (Range(a,0x80000,n*2)) operandWrites++;
		if (Range(a,0x83000,n/32*4)) scaleWrites++;
		memory[a]=value;
	}
	static float FlushScalar(float value) {
		uint32_t bits=PioneerAttentionBits(value);
		return bits&0x7f800000?value:PioneerAttentionFloat(bits&0x80000000);
	}
	void StepScalar() {
		uint32_t i=l1[Index(pc)],op=i&127;
		if (op!=7 && op!=0x27 && op!=0x53) { Machine::Step(); return; }
		assert(++steps<StepLimit());
		unsigned d=(i>>7)&31,f=(i>>12)&7,s=(i>>15)&31,t=(i>>20)&31,hi=i>>25;
		if (op==7) { assert(f==2); fp[d]=Load(regs[s]+Sign(i>>20,12)); }
		else if (op==0x27) { assert(f==2); Store(regs[s]+Sign((hi<<5)|d,12),fp[t]); }
		else {
			float a=FlushScalar(PioneerAttentionFloat(fp[s])),b=FlushScalar(PioneerAttentionFloat(fp[t]));
			switch (hi) {
				case 0: assert(f==7 || !f); fp[d]=PioneerAttentionBits(FlushScalar(a+b)); break;
				case 4: assert(f==7 || !f); fp[d]=PioneerAttentionBits(FlushScalar(a-b)); break;
				case 8: assert(f==7 || !f); fp[d]=PioneerAttentionBits(FlushScalar(a*b)); break;
				case 0x10: assert(f<=2); fp[d]=f==0?((fp[s]&0x7fffffff)|(fp[t]&0x80000000)):f==1?((fp[s]&0x7fffffff)|((~fp[t])&0x80000000)):(fp[s]^(fp[t]&0x80000000)); break;
				case 0x50: assert(f<=2); if (d) regs[d]=f==0?a<=b:f==1?a<b:a==b; break;
				case 0x68: assert(t<=1); fp[d]=PioneerAttentionBits(t?float(regs[s]):float(int32_t(regs[s]))); break;
				case 0x70: assert(!t && !f); if (d) regs[d]=fp[s]; break;
				case 0x78: assert(!t && !f); fp[d]=regs[s]; break;
				default: fprintf(stderr,"UNQUALIFIED scalar FP instruction%08x PC%08x\n",i,pc); assert(false);
			}
		}
		regs[0]=0; pc+=4;
	}
	bool RunScalar() {
		while (pc!=0x64038 && pc!=0x64058) StepScalar();
		assert(l1[Index(pc)]==0x0000006f && minStack>=0x6e000);
		if (pc==0x64058) { assert(memory.at(0x17ffc8)==0x46504c47 && !memory.at(0x17ffc4)); return false; }
		assert(memory.at(0x17ffc0)==0x6400c && memory.at(0x17ffc8)==0x31504c47);
		return memory.at(0x17ffc4)==n;
	}
	void Verify() {
		assert(outputWrites==n);
		if (mode==6) {
			for (unsigned head=0;head<n;head+=128) for (unsigned i=0;i<64;i++) {
				float a=PioneerAttentionFloat(memory.at(0x84000+(head+i)*4)),b=PioneerAttentionFloat(memory.at(0x84000+(head+64+i)*4));
				float c=PioneerAttentionFloat(memory.at(0x87000+i*8)),s=PioneerAttentionFloat(memory.at(0x87004+i*8));
				assert(memory.at(0x90000+(head+i)*4)==PioneerAttentionBits(fmaf(a,c,-(b*s))) && memory.at(0x90000+(head+64+i)*4)==PioneerAttentionBits(fmaf(a,s,b*c)));
			}
			assert(!operandWrites && !scaleWrites); return;
		}
		if (mode==5) {
			unsigned width=memory.at(0x7400c);
			for (unsigned start=0;start<n;start+=width) {
				double sum=0; for (unsigned i=0;i<width;i++) { float x=PioneerAttentionFloat(memory.at(0x84000+(start+i)*4)); sum+=double(x*x); }
				float mean=float(sum/width),scale=1.0f/sqrtf(mean+1e-6f);
				for (unsigned i=0;i<width;i++) assert(memory.at(0x90000+(start+i)*4)==PioneerAttentionBits(PioneerAttentionFloat(memory.at(0x84000+(start+i)*4))*scale));
			}
			assert(!operandWrites && !scaleWrites); return;
		}
		for (unsigned i=0;i<n;i++) {
			float a=PioneerAttentionFloat(memory.at(0x84000+i*4)),b=PioneerAttentionFloat(memory.at(0x87000+i*4));
			float actual=PioneerAttentionFloat(memory.at(0x90000+i*4));
			float reference=mode==4?sqrtf(a):mode==2?a/b:mode?(a/(1.0f+expf(-a)))*b:a;
			if (mode==2 || mode==4 || !mode) assert(PioneerAttentionBits(actual)==PioneerAttentionBits(reference));
			else assert(isfinite(actual) && fabsf(actual-reference)<3e-6f*(1+fabsf(reference)));
		}
		if (mode==2 || mode==4) { assert(!operandWrites && !scaleWrites); return; }
		assert(operandWrites==n/2 && scaleWrites==n/32);
		for (unsigned block=0;block<n/32;block++) {
			float maximum=0;
			for (unsigned i=0;i<32;i++) maximum=fmaxf(maximum,fabsf(PioneerAttentionFloat(memory.at(0x90000+(block*32+i)*4))));
			float d=maximum/127.0f,id=d?1.0f/d:0;
			assert(memory.at(0x83000+block*4)==PioneerAttentionBits(PioneerAttentionHalf(PioneerAttentionToHalf(d))));
			for (unsigned i=0;i<32;i++) {
				float value=PioneerAttentionFloat(memory.at(0x90000+(block*32+i)*4));
				uint32_t word=memory.at(0x80000+(block*16+i/2)*4);
				assert(uint16_t(word>>(16*(i&1)))==PioneerInt8Operand(int(roundf(value*id))));
			}
		}
	}
};
#ifndef PIONEER_GRAPH_MATH_MACHINE_ONLY
int main(int argc,char** argv)
{
	assert(argc==2);
	for (unsigned mode:{0U,1U,2U,3U,4U,5U,6U}) for (unsigned n:{32U,64U,128U,1024U,2048U,3072U}) {
		if (mode==5 && n<128) continue;
		if (mode==6 && n!=1024 && n!=2048) continue;
		ScalarMachine machine(argv[1],n,mode); assert(machine.RunScalar()); machine.Verify();
		printf("OFFLINE actual RV32IMF graph math mode%u n%u steps%u stack%08x PASS\n",mode,n,machine.steps,machine.minStack);
	}
	for (unsigned field=0;field<5;field++) {
		ScalarMachine m(argv[1],32,1);
		if (field==0) m.memory[0x74000]^=1; if (field==1) m.memory[0x74004]=0;
		if (field==2) m.memory[0x74004]=3104; if (field==3) m.memory[0x74008]=7; if (field==4) m.memory[0x7400c]=1;
		assert(!m.RunScalar() && !m.outputWrites && !m.operandWrites && !m.scaleWrites);
	}
	{ ScalarMachine m(argv[1],32,1); m.memory[0x84000]=0x7f800000; assert(!m.RunScalar() && !m.operandWrites && !m.scaleWrites); }
	for (unsigned field=0;field<4;field++) { ScalarMachine m(argv[1],128,5);
		if (field==0) m.memory[0x7400c]=64; if (field==1) m.memory[0x84000]=0x7f800000;
		if (field==2) m.memory[0x84000]=1; if (field==3) m.memory[0x84000]=0x47800000;
		assert(!m.RunScalar() && !m.outputWrites && !m.operandWrites && !m.scaleWrites); }
	puts("PASS OFFLINE actual scalar instruction image,SiLU/Q8/division/sqrt/RMS bounds,all used words,refused malformed/nonfinite input,no fdiv/fsqrt/NoC/control/reset/driver exposure");
}
#endif
