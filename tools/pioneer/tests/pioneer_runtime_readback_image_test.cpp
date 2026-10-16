/* SPDX-License-Identifier: MIT */
#define PIONEER_BULK_Q8_MACHINE_ONLY
#include "pioneer_runtime_q8_bulk_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeRuntimeQ8ReadbackImage.h"
struct ReadbackMachine:BulkQ8Machine {
	const bool compare;
	unsigned expected=0,originalReads=0,readbackReads=0,metadataWrites=0;
	ReadbackMachine(unsigned k,unsigned n,unsigned trial=0,unsigned mode=1,unsigned groups=~0U)
		:BulkQ8Machine(k,mode?32:n,trial),compare(mode!=0) {
		for (unsigned w=0;w<1024;w++) l1[1+w]=BlackholeRuntimeQ8ReadbackCodeWord(w);
		l1[Index(0x70500)]=base; l1[Index(0x70504)]=k/16; l1[Index(0x70508)]=n/16;
		l1[0]=TT_Q8_READBACK_ENTRY; l1[Index(0x7050c)]=mode;
		if (compare) {
			assert(n<=2048); outputs=n; expected=mode==1?(groups==~0U?n:groups>=1 && groups<=128?groups*16:0):0;
			l1[Index(0x70508)]=groups==~0U?n/16:groups;
			for (unsigned w=0;w<n;w++) buffers[4096+w]=l1[Index(0x88000+w*4)]=0x3f000000+w*17;
		}
	}
	unsigned Index(uint32_t a) override { return a==0x17ffcc?1028:BulkQ8Machine::Index(a); }
	uint32_t ParkPc() const override { return compare?TT_Q8_READBACK_PARK_PC:TT_RUNTIME_Q8_PARK_PC; }
	uint32_t FinalPhase() const override { return compare?TT_Q8_READBACK_PHASE:TT_RUNTIME_Q8_FINAL_PHASE; }
	uint32_t FinalMarker() const override { return compare?TT_Q8_READBACK_MARKER:0x4d415431; }
	unsigned ExpectedFixtures() const override { return compare?expected:BulkQ8Machine::ExpectedFixtures(); }
	unsigned StepLimit() const override { return compare?20000:BulkQ8Machine::StepLimit(); }
	bool CodeAddress(uint32_t a) const override {
		return a==0x6000 || (a>=TT_Q8_READBACK_DISPATCH_PC && a<TT_Q8_READBACK_END)
			|| (!compare && BulkQ8Machine::CodeAddress(a));
	}
	uint32_t Load(uint32_t a) override {
		if (!compare) return BulkQ8Machine::Load(a);
		if (a==0x70508 || a==0x7050c || a==0x17ffc8) return l1[Index(a)];
		if (a>=0x84000 && a<0x84000+TT_Q8_READBACK_MAX_WORDS*4) { originalReads++; return buffers[(a-0x80000)/4]; }
		assert(a>=0x88000 && a<0x88000+TT_Q8_READBACK_MAX_WORDS*4); readbackReads++; return l1[Index(a)];
	}
	void Store(uint32_t a,uint32_t v) override {
		if (!compare) { BulkQ8Machine::Store(a,v); return; }
		assert(a>=0x17ffc0 && a<=0x17ffcc && !(a&3)); // no payload, NIU, SFPU, reset or DMA write
		metadataWrites++; l1[Index(a)]=v;
	}
	bool Accepted() {
		return BlackholeRuntimeReadbackProof(l1[Index(0x17ffc0)],l1[Index(0x17ffc4)],l1[Index(0x17ffc8)],l1[Index(0x17ffcc)],pc,pc,outputs);
	}
};
int main(int argc,char** argv)
{
	assert(argc==3); auto boot=Read(argv[1]),code=Read(argv[2]);
	assert(boot.size()==4 && Word(boot,0)==TT_Q8_READBACK_ENTRY && code.size()==4096);
	for (unsigned w=0;w<1024;w++) assert(Word(code,w*4)==BlackholeRuntimeQ8ReadbackCodeWord(w));
	for (unsigned w=0;w<0xf54/4;w++) assert(BlackholeRuntimeQ8ReadbackCodeWord(w)==BlackholeRuntimeQ8BulkCodeWord(w));
	assert(!BlackholeRuntimeQ8ReadbackCodeWord(1024));
	for (unsigned k:{1024U,2048U,3072U,5120U,6144U}) {
		ReadbackMachine math(k,32,1,0); math.Run(); assert(math.CorrectIeee() && math.Guards());
	}
	unsigned corruptionCases=0;
	for (unsigned n:{16U,32U,64U,1024U,2048U}) {
		ReadbackMachine good(1024,n); auto before=std::vector<uint32_t>(good.buffers,good.buffers+8192);
		good.Run(); assert(good.Accepted() && good.originalReads==n && good.readbackReads==n && good.metadataWrites==4);
		assert(!memcmp(before.data(),good.buffers,sizeof(good.buffers)) && !good.requests && !good.matrixInstructions && !good.configWrites);
		for (unsigned w=0;w<n;w++) {
			ReadbackMachine bad(1024,n); bad.l1[bad.Index(0x88000+w*4)]^=1U<<(w%32);
			bad.Run(); assert(!bad.Accepted() && !bad.requests && bad.originalReads==n && bad.readbackReads==n); corruptionCases++;
		}
		ReadbackMachine doubleFlip(1024,n);
		doubleFlip.l1[doubleFlip.Index(0x88000)]^=1; doubleFlip.l1[doubleFlip.Index(0x88004)]^=1;
		doubleFlip.Run(); assert(!doubleFlip.Accepted()); // OR, not collision-prone XOR/checksum
		for (unsigned field=0;field<6;field++) {
			uint32_t proof[]={TT_Q8_READBACK_PHASE,n,TT_Q8_READBACK_MARKER,0,TT_Q8_READBACK_PARK_PC,TT_Q8_READBACK_PARK_PC}; proof[field]^=1;
			assert(!BlackholeRuntimeReadbackProof(proof[0],proof[1],proof[2],proof[3],proof[4],proof[5],n));
		}
	}
	for (unsigned groups:{0U,129U,256U,0xffffffffU}) {
		// ~0 is the constructor default sentinel; overwrite it explicitly.
		ReadbackMachine bad(1024,32,0,1,groups); bad.expected=0; bad.l1[bad.Index(0x70508)]=groups;
		bad.Run(); assert(!bad.Accepted() && !bad.originalReads && !bad.readbackReads && !bad.requests);
	}
	for (unsigned mode:{2U,0xffffffffU}) {
		ReadbackMachine bad(1024,32,0,mode); bad.Run(); assert(!bad.Accepted() && !bad.originalReads && !bad.readbackReads);
	}
	for (unsigned groups:{1U,3U,128U}) {
		ReadbackMachine wrongCount(1024,32,0,1,groups); wrongCount.Run();
		assert(!wrongCount.Accepted() && wrongCount.originalReads==groups*16 && wrongCount.readbackReads==groups*16);
	}
	for (unsigned seed=1;seed<=4;seed++) {
		ReadbackMachine good(1024,64);
		for (unsigned reg=1;reg<32;reg++) good.regs[reg]=0x9e3779b9U*(reg+seed*37);
		good.Run(); assert(good.Accepted() && !good.requests && !good.configWrites);
	}
	{
		ReadbackMachine zero(1024,32); zero.buffers[4096]=0; zero.l1[zero.Index(0x88000)]=0x80000000;
		zero.Run(); assert(!zero.Accepted()); // +0/-0 must remain different bits
	}
	printf("PASS: OFFLINE exact device word compare, %u corruptions, all bounds/markers/no side effects, unchanged bulk math; NO native exposure or throughput claim\n",corruptionCases);
}
