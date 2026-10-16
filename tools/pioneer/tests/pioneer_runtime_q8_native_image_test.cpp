/* SPDX-License-Identifier: MIT */
#define PIONEER_COMPACT_Q8_MACHINE_ONLY
#include "pioneer_runtime_q8_compact_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeRuntimeQ8NativeImage.h"
struct NativeQ8Machine:CompactQ8Machine {
	static int NativeWeight(unsigned row,unsigned k) { return int((row*37+k*13+(k/32)*11)%255)-127; }
	NativeQ8Machine(unsigned k,unsigned n,unsigned t,uint32_t address=0x04000000):CompactQ8Machine(k,n,t,address) {
		for (unsigned w=0;w<1024;w++) l1[1+w]=BlackholeRuntimeQ8NativeCodeWord(w);
		l1[Index(0x70500)]=base; l1[Index(0x70504)]=k/16; l1[Index(0x70508)]=n/16;
		weights.assign(size_t(PioneerQ8CompactWeightBytes(k,n)/4),0);
		for (unsigned row=0;row<n;row++) for (unsigned block=0;block<k/32;block++) {
			uint64_t byte=PioneerQ8CompactScaleByte(k,block,row);
			weights[size_t(byte/4)]|=uint32_t(WeightScale(row,block))<<(8*(byte&3));
			for (unsigned i=0;i<32;i++) {
				byte=PioneerQ8CompactValueByte(k,block*32+i,row); uint8_t q=0;
				assert(PioneerInt8UnpackEncode(NativeWeight(row,block*32+i),q));
				weights[size_t(byte/4)]|=uint32_t(q)<<(8*(byte&3));
			}
		}
	}
	bool NativeInt8Unpack() const override { return true; }
	unsigned StepLimit() const override { return 2000000+1200*ExpectedFixtures(); }
	void Store16(uint32_t,uint16_t) override { assert(false && "native unpack must not use scalar byte expansion"); }
	void Store(uint32_t a,uint32_t v) override {
		if ((a>=0x706d0 && a<0x706e0) || (a>=0x707d0 && a<0x707e0)) {
			bool odd=a>=0x707d0;
			assert(drained && !pending && v==0 && odd==bool(regs[20]&1) && (!odd || matrixInstructions>=4));
			l1[Index(a)]=v; return;
		}
		CompactQ8Machine::Store(a,v);
	}
	uint32_t Load(uint32_t a) override {
		if (a==0xffb20040) {
			doorbellRead=true;
			assert((target&31)==(0x706c0&31)); // GDDR -> L1 congruence is mandatory
			if (pending && ++waits>delay && requests!=faultAt) {
				for (unsigned w=0;w<144;w++) l1[Index(0x706c0+w*4)]=weights[(requests-1)*144+w]^((corrupt && w==8)?0x01010101:0)^((corruptScale && w==0)?0x04000400:0);
				pending=false; responses++;
			}
			return pending?1:0;
		}
		return Q8Machine::Load(a);
	}
	bool CorrectNative() const {
		for (unsigned row=0;row<outputs;row++) {
			float sum=0,absolute=0;
			for (unsigned block=0;block<inputs/32;block++) {
				int dot=0; for (unsigned i=0;i<32;i++) dot+=NativeWeight(row,block*32+i)*Input(block*32+i,trial);
				float product=float(dot)*(Half(WeightScale(row,block))*InputScale(block,trial)); sum+=product; absolute+=fabsf(product);
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
	for (unsigned w=0;w<1024;w++) assert(Word(code,w*4)==BlackholeRuntimeQ8NativeCodeWord(w));
	for (int q=-512;q<=511;q++) {
		uint8_t encoded=0x55; bool valid=PioneerInt8UnpackEncode(q,encoded);
		assert(valid==(q>=-127 && q<=127));
		if (valid) assert(int(encoded&127)*((encoded&128)?-1:1)==q); else assert(encoded==0x55);
	}
	for (unsigned k:{1024U,2048U,3072U,5120U,6144U}) for (unsigned t=0;t<2;t++) {
		NativeQ8Machine m(k,32,t); m.delay=3; m.Run();
		assert(m.CorrectNative() && !m.expansions && m.requests==m.ExpectedFixtures()/2 && m.matrixInstructions==4*m.ExpectedFixtures());
	}
	NativeQ8Machine alternate(1024,64,1,0x04800000); alternate.Run(); assert(alternate.CorrectNative());
	NativeQ8Machine full(1024,4096,1); full.Run(); assert(full.CorrectNative());
	for (unsigned fault=0;fault<5;fault++) {
		NativeQ8Machine m(1024,32,1); m.omitCast=fault==0; m.omitScale=fault==1; m.omitAccumulate=fault==2;
		m.corrupt=fault==3; m.corruptScale=fault==4; m.Run(); assert(!m.CorrectNative());
	}
	for (unsigned fault:{1U,33U,64U}) {
		NativeQ8Machine m(1024,32,0); m.faultAt=fault; bool failed=false;
		try { m.Run(); } catch(uint32_t marker) { failed=marker==0x4e4f4346; }
		assert(failed && m.requests==fault && m.responses==fault-1 && !m.expansions);
	}
	puts("PASS: native signed INT8 unpack image, lossless SM7 encoding with -128 refusal, FP16-overlay full-range activations, all K/N including4096, exact half NoC counts, scaled references and bounded failure parks; NO hardware");
}
