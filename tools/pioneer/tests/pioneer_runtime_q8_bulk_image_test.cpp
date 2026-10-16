/* SPDX-License-Identifier: MIT */
#define PIONEER_NATIVE_F32_Q8_MACHINE_ONLY
#include "pioneer_runtime_q8_native_f32_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeRuntimeQ8BulkImage.h"
struct BulkQ8Machine:NativeF32Q8Machine {
	unsigned skipWord=~0U;
	BulkQ8Machine(unsigned k,unsigned n,unsigned t,uint32_t address=0x04000000):NativeF32Q8Machine(k,n,t,address) {
		for (unsigned w=0;w<1024;w++) l1[1+w]=BlackholeRuntimeQ8BulkCodeWord(w);
		l1[Index(0x70500)]=base; l1[Index(0x70504)]=k/16; l1[Index(0x70508)]=n/16;
		for (unsigned w=0;w<4096;w++) l1[2048+w]=0xcafebabe;
	}
	unsigned Index(uint32_t a) override {
		if (a>=0x88000 && a<0x8c000) { assert(!(a&3)); return 2048+(a-0x88000)/4; }
		return NativeF32Q8Machine::Index(a);
	}
	uint32_t NativePayloadBase(unsigned half) const override {
		assert(!pending); return 0x88000+((regs[20]>>1)&15)*576+64+half*256;
	}
	void Store(uint32_t a,uint32_t v) override {
		if (a==0xffb20000) { assert(!pending && drained && !(regs[20]&31) && v==base+requests*9216); target=v; return; }
		if (a==0xffb20040) {
			assert(v==1 && !pending && drained && target==base+requests*9216 && requests<ExpectedFixtures()/32);
			requests++; pending=true; doorbellRead=false; waits=0; return;
		}
		if (a>=0x88000 && a<0x8c000) {
			unsigned offset=a-0x88000,half=regs[20]&1,within=offset%576,header=half?304:48;
			assert(drained && !pending && !v && offset/576==((regs[20]>>1)&15) && within>=header && within<header+16);
			l1[Index(a)]=v; return;
		}
		NativeF32Q8Machine::Store(a,v);
	}
	uint32_t Load(uint32_t a) override {
		if (a>=0x88000 && a<0x8c000) { assert(!pending); return l1[Index(a)]; }
		if (a==0xffb20040) {
			doorbellRead=true; assert((target&31)==(0x88000&31));
			if (pending && ++waits>delay && requests!=faultAt) {
				assert(size_t(requests)*2304<=weights.size());
				for (unsigned w=0;w<2304;w++) if (w!=skipWord)
					l1[Index(0x88000+w*4)]=weights[(requests-1)*2304+w]^((corrupt && w==16)?0x01010101:0)^((corruptScale && w==0)?0x04000400:0);
				pending=false; responses++;
			}
			return pending?1:0;
		}
		return NativeF32Q8Machine::Load(a);
	}
	bool Guards() const { for (unsigned w=2304;w<4096;w++) if (l1[2048+w]!=0xcafebabe) return false; return true; }
};
int main(int argc,char** argv) {
	assert(argc==2); auto code=Read(argv[1]); assert(code.size()==4096);
	for (unsigned w=0;w<1024;w++) assert(Word(code,w*4)==BlackholeRuntimeQ8BulkCodeWord(w));
	for (unsigned k:{1024U,2048U,3072U,5120U,6144U}) for (unsigned t=0;t<2;t++) {
		BulkQ8Machine m(k,32,t); m.delay=3;
		uint32_t activationBefore[4096]; memcpy(activationBefore,m.buffers,sizeof(activationBefore));
		m.Run();
		assert(m.CorrectIeee() && m.Guards() && m.requests==m.ExpectedFixtures()/32 && m.responses==m.requests && !m.expansions);
		assert(!memcmp(activationBefore,m.buffers,sizeof(activationBefore))); // entire input+scales region read-only
	}
	BulkQ8Machine full(1024,4096,1); full.Run(); assert(full.CorrectIeee() && full.Guards());
	BulkQ8Machine alternate(1024,64,1,0x04800000); alternate.Run(); assert(alternate.CorrectIeee() && alternate.Guards());
	NativeF32Q8Machine prior(1024,32,0); prior.Run(); BulkQ8Machine bulk(1024,32,0); bulk.Run();
	assert(prior.requests==16*bulk.requests && bulk.CorrectIeee());
	for (unsigned fault=0;fault<3;fault++) {
		BulkQ8Machine m(1024,32,1); m.corrupt=fault==0; m.corruptScale=fault==1; if (fault==2) m.skipWord=16;
		m.Run(); assert(!m.CorrectIeee() && m.Guards());
	}
	for (unsigned fault:{1U,3U,4U}) {
		BulkQ8Machine m(1024,32,0); m.faultAt=fault; bool failed=false;
		try { m.Run(); } catch (uint32_t marker) { failed=marker==0x4e4f4346; }
		assert(failed && m.requests==fault && m.responses==fault-1 && m.Guards());
	}
	printf("PASS: OFFLINE9216-byte/16-tile reader, all K/N4096 IEEE bits, data/header bounds and failures; model reads %u vs %u, RISC steps %u vs %u; NOT hardware/performance, no driver exposure\n",bulk.requests,prior.requests,bulk.steps,prior.steps);
}
