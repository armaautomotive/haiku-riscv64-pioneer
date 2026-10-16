/* SPDX-License-Identifier: MIT */
#define PIONEER_NATIVE_Q8_MACHINE_ONLY
#include "pioneer_runtime_q8_native_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeRuntimeQ8NativeF32Image.h"
#include "../PioneerBlackholeFma.h"
struct NativeF32Q8Machine:NativeQ8Machine {
	unsigned scaleWords=0;
	bool conditional=false,flags[32]={};
	NativeF32Q8Machine(unsigned k,unsigned n,unsigned t,uint32_t address=0x04000000):NativeQ8Machine(k,n,t,address) {
		for (auto& value:lanes[10]) value=Bits(1);
		for (unsigned w=0;w<1024;w++) l1[1+w]=BlackholeRuntimeQ8NativeF32CodeWord(w);
		l1[Index(0x70500)]=base; l1[Index(0x70504)]=k/16; l1[Index(0x70508)]=n/16;
		weights.assign(size_t(PioneerQ8CompactWeightBytes(k,n)/4),0);
		for (unsigned row=0;row<n;row++) for (unsigned block=0;block<k/32;block++) {
			// Independent conversion reference, not the new bit helper.
			uint64_t byte=PioneerQ8NativeF32ScaleByte(k,block,row);
			weights[size_t(byte/4)]=Bits(Half(WeightScale(row,block)));
			for (unsigned i=0;i<32;i++) {
				byte=PioneerQ8NativeF32ValueByte(k,block*32+i,row); uint8_t q=0;
				assert(PioneerInt8UnpackEncode(NativeWeight(row,block*32+i),q));
				weights[size_t(byte/4)]|=uint32_t(q)<<(8*(byte&3));
			}
		}
	}
	uint32_t NativePayloadBase(unsigned half) const override { return 0x70700+half*256; }
	bool CodeAddress(uint32_t a) const override {
		return (a>=0x70d80 && a<0x70e40) || (a<0x70d80 && NativeQ8Machine::CodeAddress(a));
	}
	void Instruction(uint32_t op) override {
		unsigned code=op>>24,vc=(op>>8)&15,vd=(op>>4)&15,mod=op&15;
		if ((code==0x70 || code==0x72) && ((op>>16)&15)==3) {
			unsigned reg=(op>>20)&15,address=op&1023; assert(reg<8 && !(address&1));
			for (unsigned lane=0;lane<32;lane++) {
				assert(!conditional); unsigned row=(address&~3U)+lane/8,col=(lane&7)*2+((address&2)!=0);
				assert(row<36);
				if (code==0x72) { assert(row>=8); dst[row][col]=Float(lanes[reg][lane]); valid[row]=true; }
				else lanes[reg][lane]=Bits(valid[row]?dst[row][col]:0);
			}
			drained=false; return;
		}
		if (code==0x70 || code==0x72 || code==0x90 || code<0x70 || code>=0xa0) { Q8Machine::Instruction(op); return; }
		unsigned a=(op>>16)&15,b=(op>>12)&15; assert(vd<8 || code==0x71);
		for (unsigned lane=0;lane<32;lane++) {
			if (code==0x8a) { assert(mod==2); conditional=(op&0x1000)!=0; flags[lane]=true; continue; }
			if (conditional && !flags[lane]) continue;
			uint32_t& result=lanes[code==0x71?0:vd][lane];
			switch (code) {
				case 0x71: { unsigned reg=(op>>20)&15,mode=(op>>16)&15; assert(reg<8); auto& value=lanes[reg][lane];
					if (!mode) value=(op&0xffff)<<16;
					else if (mode==10) value=(value&0xffff0000)|(op&0xffff);
					else { assert(mode==2); value=op&0xffff; } break; }
				case 0x84: case 0x85: {
					assert(a<8 || a==10); assert(b<8 || b==10); assert(vc<8 || vc==9);
					float x=Float(lanes[a][lane]),y=Float(lanes[b][lane]),z=Float(lanes[vc][lane]);
					if (omitScale && vc==9) y=1;
					if (omitAccumulate && vc!=9) z=0;
					if (mod&1) x=-x;
					if (mod&2) z=-z;
					assert(!(mod&~3)); result=Bits(PioneerBlackholeFma(x,y,z)); mads++; break; }
				case 0x7e: assert(mod==1 && b<8 && vc<8); result=lanes[b][lane]&lanes[vc][lane]; break;
				case 0x7c: assert(!mod && vc<8); result=lanes[vc][lane]; break;
				case 0x8d: assert(!mod && vc<8); result^=lanes[vc][lane]; break;
				case 0x7d: assert(mod==1 && vc<8); result=lanes[vc][lane]&0x7fffffff; break;
				case 0x7a: { assert(mod==1 || mod==3); int amount=int(op<<8)>>20;
					if (amount>=0) result<<=unsigned(amount)&31;
					else if (mod&2) result=uint32_t(int32_t(result)>>(unsigned(-amount)&31));
					else result>>=unsigned(-amount)&31; break; }
				case 0x79: assert(mod==4 || mod==5); result=lanes[vc][lane]+((mod&1)?uint32_t(int(op<<8)>>20):result); break;
				case 0x7b: assert(conditional && (mod==2 || mod==6)); flags[lane]=mod==2?lanes[vc][lane]!=0:lanes[vc][lane]==0; break;
				default: fprintf(stderr,"Unexpected exact SFPU opcode %08x\n",op); assert(false);
			}
		}
		drained=false;
	}
	bool CorrectIeee() const {
		if (conditional) return false;
		for (unsigned row=0;row<outputs;row++) {
			float expected=0;
			for (unsigned block=0;block<inputs/32;block++) {
				int dot=0; for (unsigned i=0;i<32;i++) dot+=NativeWeight(row,block*32+i)*Input(block*32+i,trial);
				expected=fmaf(float(dot),Half(WeightScale(row,block))*InputScale(block,trial),expected);
			}
			if (Bits(expected)!=buffers[4096+row]) return false;
		}
		return true;
	}
	void Store(uint32_t a,uint32_t v) override {
		if (a>=0x70680 && a<0x706c0) { assert(!pending); l1[Index(a)]=v; scaleWords++; return; }
		if ((a>=0x706f0 && a<0x70700) || (a>=0x707f0 && a<0x70800)) {
			bool odd=a>=0x707f0;
			assert(drained && !pending && !v && odd==bool(regs[20]&1) && (!odd || matrixInstructions>=4));
			l1[Index(a)]=v; return;
		}
		Q8Machine::Store(a,v);
	}
	uint32_t Load(uint32_t a) override {
		if (a>=0x70ea8 && a<0x70f54) return l1[Index(a)]; // immutable instruction table, never executed by RISC
		if (a==0xffb20040) {
			doorbellRead=true; assert((target&31)==(0x706c0&31));
			if (pending && ++waits>delay && requests!=faultAt) {
				for (unsigned w=0;w<144;w++) l1[Index(0x706c0+w*4)]=weights[(requests-1)*144+w]^((corrupt && w==16)?0x01010101:0)^((corruptScale && w==0)?0x04000400:0);
				pending=false; responses++;
			}
			return pending?1:0;
		}
		return Q8Machine::Load(a);
	}
};
int main(int argc,char** argv) {
	assert(argc==2); auto code=Read(argv[1]); assert(code.size()==4096);
	for (unsigned w=0;w<1024;w++) assert(Word(code,w*4)==BlackholeRuntimeQ8NativeF32CodeWord(w));
	{
		// Execute the actual assembled SFPU table, with independent lanes,
		// random finite half-scale products and severe cancellation cases.
		NativeF32Q8Machine arithmetic(1024,32,0); uint32_t seed=0x12345678;
		auto random=[&]() { seed^=seed<<13; seed^=seed>>17; seed^=seed<<5; return seed; };
		for (unsigned trial=0;trial<8192;trial++) {
			float expected[32];
			for (unsigned lane=0;lane<32;lane++) {
				uint16_t a=uint16_t(random()),b=uint16_t(random());
				if ((a&0x7c00)==0x7c00) a^=0x400;
				if ((b&0x7c00)==0x7c00) b^=0x400;
				float dot=float(int(random()%1040385)-520192),scale=Q8Machine::Half(a)*Q8Machine::Half(b);
				float c=trial%3?-(dot*scale):std::ldexp(float(int(random()%1048576)-524288),int(random()%80)-50);
				if (trial%3==2 && c) c=std::nextafter(c,INFINITY);
				arithmetic.lanes[0][lane]=Bits(dot); arithmetic.lanes[1][lane]=Bits(scale); arithmetic.lanes[2][lane]=Bits(c);
				expected[lane]=fmaf(dot,scale,c);
			}
			for (unsigned offset=0xec0;offset<0xf54;offset+=4) arithmetic.Instruction(Word(code,offset));
			assert(!arithmetic.conditional);
			for (unsigned lane=0;lane<32;lane++) assert(arithmetic.lanes[0][lane]==Bits(expected[lane]));
		}
	}
	for (unsigned half=0;half<65536;half++) {
		unsigned exp=(half>>10)&31,m=half&1023; uint32_t actual=PioneerQ8HalfToFloatBits(uint16_t(half));
		if (exp==31) assert((actual&0x7f800000)==0x7f800000 && bool(actual&0x7fffff)==bool(m));
		else assert(actual==Bits(Q8Machine::Half(uint16_t(half))));
		assert((actual>>31)==(half>>15));
	}
	for (unsigned k:{1024U,2048U,3072U,5120U,6144U}) for (unsigned t=0;t<2;t++) {
		NativeF32Q8Machine m(k,32,t); m.delay=3; m.Run();
		assert(m.CorrectNative() && m.CorrectIeee() && !m.expansions && m.requests==m.ExpectedFixtures()/2);
		assert(m.scaleWords==16*m.requests && m.matrixInstructions==4*m.ExpectedFixtures());
		// Every tile byte is either one exact FP32 scale or a quant byte.
		std::vector<bool> used(size_t(PioneerQ8CompactWeightBytes(k,32)));
		for (unsigned r=0;r<32;r++) for (unsigned b=0;b<k/32;b++) {
			uint64_t byte=PioneerQ8NativeF32ScaleByte(k,b,r);
			for (unsigned i=0;i<4;i++) { assert(!used[size_t(byte+i)]); used[size_t(byte+i)]=true; }
			for (unsigned i=0;i<32;i++) { byte=PioneerQ8NativeF32ValueByte(k,b*32+i,r); assert(!used[size_t(byte)]); used[size_t(byte)]=true; }
		}
		for (bool byte:used) assert(byte);
	}
	NativeF32Q8Machine full(1024,4096,1); full.Run(); assert(full.CorrectNative() && full.CorrectIeee());
	NativeF32Q8Machine alternate(1024,64,1,0x04800000); alternate.Run(); assert(alternate.CorrectNative() && alternate.CorrectIeee());
	NativeQ8Machine original(1024,32,0); original.Run();
	NativeF32Q8Machine exact(1024,32,0); exact.Run(); assert(exact.CorrectIeee());
	for (unsigned fault=0;fault<5;fault++) {
		NativeF32Q8Machine m(1024,32,1); m.omitCast=fault==0; m.omitScale=fault==1; m.omitAccumulate=fault==2;
		m.corrupt=fault==3; m.corruptScale=fault==4; m.Run(); assert(!m.CorrectNative());
	}
	for (unsigned fault:{1U,33U,64U}) {
		NativeF32Q8Machine m(1024,32,0); m.faultAt=fault; bool failed=false;
		try { m.Run(); } catch(uint32_t marker) { failed=marker==0x4e4f4346; }
		assert(failed && m.requests==fault && m.responses==fault-1 && !m.expansions);
	}
	printf("PASS: OFFLINE native FP32-scale + compensated-SFPU image/layout; exhaustive half bits incl signed zero/subnormal; all K/N4096 BIT-EXACT IEEE FMA and fault parks; RISC model steps %u vs original %u (NOT hardware timing); no driver/card exposure\n",exact.steps,original.steps);
}
