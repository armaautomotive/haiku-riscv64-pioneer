/* SPDX-License-Identifier: MIT */
#define PIONEER_NATIVE_Q8_MACHINE_ONLY
#include "pioneer_runtime_q8_native_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeRuntimeQ8NativeF32Image.h"
struct NativeF32Q8Machine:NativeQ8Machine {
	unsigned scaleWords=0;
	NativeF32Q8Machine(unsigned k,unsigned n,unsigned t,uint32_t address=0x04000000):NativeQ8Machine(k,n,t,address) {
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
	for (unsigned half=0;half<65536;half++) {
		unsigned exp=(half>>10)&31,m=half&1023; uint32_t actual=PioneerQ8HalfToFloatBits(uint16_t(half));
		if (exp==31) assert((actual&0x7f800000)==0x7f800000 && bool(actual&0x7fffff)==bool(m));
		else assert(actual==Bits(Q8Machine::Half(uint16_t(half))));
		assert((actual>>31)==(half>>15));
	}
	for (unsigned k:{1024U,2048U,3072U,5120U,6144U}) for (unsigned t=0;t<2;t++) {
		NativeF32Q8Machine m(k,32,t); m.delay=3; m.Run();
		assert(m.CorrectNative() && !m.expansions && m.requests==m.ExpectedFixtures()/2);
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
	NativeF32Q8Machine full(1024,4096,1); full.Run(); assert(full.CorrectNative());
	NativeF32Q8Machine alternate(1024,64,1,0x04800000); alternate.Run(); assert(alternate.CorrectNative());
	NativeQ8Machine original(1024,32,0); original.Run();
	NativeF32Q8Machine faster(1024,32,0); faster.Run(); assert(faster.steps<original.steps);
	for (unsigned fault=0;fault<5;fault++) {
		NativeF32Q8Machine m(1024,32,1); m.omitCast=fault==0; m.omitScale=fault==1; m.omitAccumulate=fault==2;
		m.corrupt=fault==3; m.corruptScale=fault==4; m.Run(); assert(!m.CorrectNative());
	}
	for (unsigned fault:{1U,33U,64U}) {
		NativeF32Q8Machine m(1024,32,0); m.faultAt=fault; bool failed=false;
		try { m.Run(); } catch(uint32_t marker) { failed=marker==0x4e4f4346; }
		assert(failed && m.requests==fault && m.responses==fault-1 && !m.expansions);
	}
	printf("PASS: OFFLINE native FP32-scale image/layout; exhaustive half bits incl signed zero/subnormal; all K/N4096 and fault parks; RISC model steps %u vs %u (NOT hardware timing); no driver/card exposure\n",faster.steps,original.steps);
}
