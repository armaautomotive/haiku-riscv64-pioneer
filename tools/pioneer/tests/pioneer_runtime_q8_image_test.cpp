/* SPDX-License-Identifier: MIT */
#define PIONEER_RUNTIME_MACHINE_ONLY
#include "pioneer_runtime_matvec_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeRuntimeQ8MatvecImage.h"
struct Q8Machine:RuntimeMachine {
	uint32_t lanes[10][32]={};
	unsigned casts=0,mads=0,scales=0;
	bool omitCast=false,omitScale=false,omitAccumulate=false;
	unsigned trial;
	static uint16_t WeightScale(unsigned row,unsigned block) {
		const uint16_t values[]={0x2001,0xa301,0x0001,0x83ff,0,0x8000,0x3503,0xb602};
		return values[(row+block)%8];
	}
	static float Half(uint16_t x) {
		unsigned e=(x>>10)&31; assert(e!=31);
		return std::ldexp(float(e?1024+(x&1023):(x&1023)),e?int(e)-25:-24)*((x&0x8000)?-1:1);
	}
	static float InputScale(unsigned block,unsigned t) { return (float(int((block*7+t*3)%31)-15))*.0009765625f; }
	static int Weight(unsigned row,unsigned k) { return int((row*37+k*13+(k/32)*11)%256)-128; }
	static int Input(unsigned k,unsigned t) { return int((k*17+t*29)%256)-128; }
	Q8Machine(unsigned k,unsigned n,unsigned t):RuntimeMachine(k,n,t,0x04000000,false,true,true),trial(t) {
		for (unsigned w=0;w<1024;w++) l1[1+w]=BlackholeRuntimeQ8MatvecCodeWord(w);
		l1[Index(0x70500)]=base; l1[Index(0x70504)]=k/16; l1[Index(0x70508)]=n/16;
		for (unsigned i=0;i<k;i++) {
			vector[i]=PioneerInt8Operand(Input(i,t));
			if (!(i&1)) buffers[i/2]=0;
			buffers[i/2]|=uint32_t(vector[i])<<(16*(i&1));
		}
		for (unsigned block=0;block<k/32;block++) buffers[0x3000/4+block]=Bits(InputScale(block,t));
		for (auto& word:weights) word=0;
		for (unsigned row=0;row<n;row++) for (unsigned i=0;i<k;i++) {
			uint64_t byte=PioneerMatvecWeightElement(k,i,row);
			weights[byte/4]|=uint32_t(PioneerInt8Operand(Weight(row,i)))<<(8*(byte&3));
		}
		for (unsigned row=0;row<n;row++) for (unsigned block=0;block<k/32;block++) {
			uint64_t byte=(uint64_t(row/16)*(k/16)+block*2+1)*576+544+(row%16)*2;
			weights[byte/4]|=uint32_t(WeightScale(row,block))<<(8*(byte&3));
		}
	}
	bool Int8ConvertedRead() const override { return false; }
	uint32_t ParkPc() const override { return TT_RUNTIME_Q8_PARK_PC; }
	uint32_t FinalPhase() const override { return TT_RUNTIME_Q8_FINAL_PHASE; }
	bool CodeAddress(uint32_t a) const override {
		return a==0x6000 || (a>=0x70000 && a<0x70500) || (a>=0x70540 && a<0x70580)
			|| (a>=0x70600 && a<0x70660) || (a>=0x70a40 && a<0x71000);
	}
	void Store(uint32_t a,uint32_t v) override {
		if (a>=0x70680 && a<0x706a0) { l1[Index(a)]=v; return; }
		if (a>=0xffbd8000 && a<0xffbd8140) {
			assert(drained && config[3]==0);
			unsigned i=(a-0xffbd8000)/4; dst[i/16][i%16]=Float(v); valid[i/16]=true; scales++; return;
		}
		RuntimeMachine::Store(a,v);
	}
	uint32_t Load(uint32_t a) override {
		if (a>=0xffbd8000 && a<0xffbd8140) {
			assert(drained && config[3]==0); unsigned i=(a-0xffbd8000)/4;
			return Bits(valid[i/16]?dst[i/16][i%16]:0);
		}
		return RuntimeMachine::Load(a);
	}
	void Instruction(uint32_t op) override {
		unsigned code=op>>24;
		if (code!=0x70 && code!=0x72 && code!=0x90 && code!=0x84) { Machine::Instruction(op); return; }
		if (code==0x70 || code==0x72) {
			unsigned reg=(op>>20)&15,mode=(op>>16)&15,address=op&1023;
			assert(reg<8 && (op&0xfc00)==0 && (address&1)==0 && thread[1]==0 && thread[36]==0);
			for (unsigned lane=0;lane<32;lane++) {
				unsigned row=(address&~3U)+lane/8,col=(lane&7)*2+((address&2)!=0);
				if (code==0x72) {
					assert(mode==3 && row>=32 && row<36);
					dst[row][col]=Float(lanes[reg][lane]); valid[row]=true;
				} else if (mode==11) lanes[reg][lane]=0;
				else if (mode==4) {
					assert(row>=32 && row<36 && valid[row]); int32_t v=dstInt[row][col];
					lanes[reg][lane]=(v<0?0x80000000U:0)|uint32_t(v<0?-v:v);
				} else { assert(mode==3 && row<8); lanes[reg][lane]=Bits(valid[row]?dst[row][col]:0); }
			}
		} else if (code==0x90) {
			unsigned c=(op>>8)&15,d=(op>>4)&15;
			assert((op&0x00fff00f)==0 && c<8 && d<8); casts++;
			if (!omitCast) for (unsigned lane=0;lane<32;lane++) {
				uint32_t v=lanes[c][lane]; lanes[d][lane]=Bits(float(v&0x7fffffff)*((v>>31)?-1:1));
			}
		} else {
			unsigned a=(op>>16)&15,b=(op>>12)&15,c=(op>>8)&15,d=(op>>4)&15;
			assert((op&0x00f0000f)==0 && a<8 && b<8 && (c<8 || c==9) && d<8); mads++;
			for (unsigned lane=0;lane<32;lane++) {
				float x=Float(lanes[a][lane]),y=Float(lanes[b][lane]),z=Float(lanes[c][lane]);
				if (omitScale && c==9) y=1;
				if (omitAccumulate && c!=9) z=0;
				// Test with a fully fused FP32 reference; native acceptance is
				// independent and required because SFPMAD is only partly fused.
				lanes[d][lane]=Bits(fmaf(x,y,z));
			}
		}
		drained=false;
	}
	bool CorrectQ8() const {
		for (unsigned row=0;row<outputs;row++) {
			float sum=0,absolute=0;
			for (unsigned block=0;block<inputs/32;block++) {
				int dot=0;
				for (unsigned i=0;i<32;i++) dot+=Weight(row,block*32+i)*Input(block*32+i,trial);
				float product=float(dot)*(Half(WeightScale(row,block))*InputScale(block,trial));
				sum+=product; absolute+=fabsf(product);
			}
			float actual=Float(buffers[4096+row]);
			if (!isfinite(actual) || fabsf(actual-sum)>1e-6f+absolute*1e-6f) return false;
		}
		for (unsigned row=outputs;row<4096;row++) if (buffers[4096+row]!=0x7fc00000) return false;
		return true;
	}
};
int main(int argc,char** argv) {
	assert(argc==2); auto code=Read(argv[1]); assert(code.size()==4096);
	for (unsigned w=0;w<1024;w++) assert(Word(code,w*4)==BlackholeRuntimeQ8MatvecCodeWord(w));
	for (unsigned k:{1024U,2048U,3072U,5120U,6144U}) for (unsigned t=0;t<2;t++) {
		Q8Machine m(k,32,t); m.delay=3; m.Run();
		assert(m.CorrectQ8() && m.casts==2*(k/32)*2 && m.mads==4*(k/32)*2 && m.scales==32*(k/32)*2);
	}
	for (unsigned fault=0;fault<3;fault++) {
		Q8Machine m(1024,32,1); m.omitCast=fault==0; m.omitScale=fault==1; m.omitAccumulate=fault==2;
		m.Run(); assert(!m.CorrectQ8());
	}
	{
		Q8Machine m(1024,32,0); m.dropClear=true;
		for (auto& row:m.dstInt) for (auto& v:row) v=7;
		m.Run(); assert(!m.CorrectQ8());
	}
	for (unsigned fault:{1U,65U,128U}) {
		Q8Machine m(1024,32,0); m.faultAt=fault; bool failed=false;
		try { m.Run(); } catch(uint32_t marker) { failed=marker==0x4e4f4346; }
		assert(failed && m.requests==fault && m.responses==fault-1);
	}
	puts("PASS: assembled Q8 integer-block scaling/FP32 accumulation image, all K widths, signed/subnormal/zero scales, omission/fault rejection; NO hardware");
}
