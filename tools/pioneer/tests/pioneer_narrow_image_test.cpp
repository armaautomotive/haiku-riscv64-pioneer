/* SPDX-License-Identifier: MIT */
#define PIONEER_COORDINATOR_MACHINE_ONLY
#include "pioneer_coordinator_image_test.cpp"
struct NarrowMachine:CoordinatorMachine {
	unsigned StepLimit() const override { return 100000000; }
	NarrowMachine(const char* root,unsigned inputs,unsigned rows,unsigned peers=119):CoordinatorMachine(root,true,true) { SetupFused(inputs,rows,peers,3); }
	bool Run() {
		while (pc!=TT_LAYER_PARK && pc!=0x64058) Step();
		assert(l1[Index(pc)]==0x0000006f && minStack>=0x6e000);
		if (pc==0x64058) { assert(memory.at(0x17ffc8)==TT_COORDINATOR_FAULT && !memory.at(0x17ffc4)); return false; }
		assert(memory.at(0x17ffc0)==TT_LAYER_DONE && memory.at(0x17ffc8)==TT_COORDINATOR_MARKER);
		return memory.at(0x17ffc4)!=0;
	}
	void VerifyAll() {
		Verify(); assert(!pending && hostWrites==1 && hostReads==1 && comparedOriginal==n && comparedReturned==n);
		for (unsigned i=0;i<n;i++) assert(host[i]==memory.at(0x84000+i*4));
		for (unsigned i=n;i<4096;i++) assert(host[i]==0xffffffff);
		uint32_t s[24],before[8],after[8]; for (unsigned i=0;i<24;i++) s[i]=memory.at(0x143000+i*4);
		for (unsigned i=0;i<8;i++) { before[i]=s[8+i]; after[i]=s[16+i]; }
		assert(PioneerLayerNarrowSummary(s,n,workers,nonce,before,after));
		for (unsigned i=0;i<24;i++) { s[i]^=1; assert(!PioneerLayerNarrowSummary(s,n,workers,nonce,before,after)); s[i]^=1; }
		printf("OFFLINE actual narrow K%u N%u workers%u exact all output/counter/host words PASS\n",k,n,workers); fflush(stdout);
	}
};
int main(int argc,char** argv)
{
	assert(argc==2); compactImage=true;
	for (unsigned n:{32U,64U,1024U,2048U,3072U,4096U}) { NarrowMachine m(argv[1],1024,n); assert(m.Run()); m.VerifyAll(); }
	for (unsigned k:{2048U,3072U,5120U,6144U}) { NarrowMachine m(argv[1],k,64); assert(m.Run()); m.VerifyAll(); }
	for (unsigned peers:{1U,2U,32U,64U,119U}) { NarrowMachine m(argv[1],1024,1024,peers); assert(m.Run()); m.VerifyAll(); }
	for (unsigned fault=0;fault<6;fault++) {
		NarrowMachine m(argv[1],1024,64); m.dropPosted=fault==0; m.partialPosted=fault==1; m.pairedCorrupt=fault==2;
		m.corruptHost=fault==3?0:fault==4?32:fault==5?63:~0U; assert(!m.Run());
	}
	for (unsigned word=0;word<12;word++) { NarrowMachine m(argv[1],1024,64); m.corruptProof=word; assert(!m.Run()); }
	for (unsigned word=0;word<8;word++) { NarrowMachine m(argv[1],1024,64); m.dropDescriptor=word; assert(!m.Run()); }
	{ NarrowMachine m(argv[1],1024,64); m.dropDoorbell=true; assert(!m.Run()); }
	for (unsigned fault=0;fault<8;fault++) {
		NarrowMachine m(argv[1],1024,64);
		if (fault==0) m.memory[0x140020]=0; if (fault==1) m.memory[0x140020]=0x10000;
		if (fault==2) m.memory[0x14001c]=1; if (fault==3) m.memory[0x140024]=32768;
		if (fault==4) m.memory[0x140010]=8192; if (fault==5) m.memory[0x140004]=2;
		if (fault==6) m.memory[0x140000]^=1; if (fault==7) m.memory[0x140100]=0;
		assert(!m.Run() && !m.transfers && !m.issued);
	}
	unsigned transfers=0; { NarrowMachine m(argv[1],1024,64); assert(m.Run()); transfers=m.transfers; }
	for (unsigned transfer=1;transfer<=transfers;transfer++) for (bool bad:{false,true}) {
		NarrowMachine m(argv[1],1024,64); if (bad) m.badCounter=transfer; else m.failTransfer=transfer; assert(!m.Run());
	}
	puts("PASS OFFLINE actual narrow image and unchanged worker,all K/bounded N/cohorts,all words/counters,fresh worker/sequence proofs,drop/corrupt/host/shape faults;not native speed");
}
