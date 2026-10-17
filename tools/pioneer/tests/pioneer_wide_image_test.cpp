/* SPDX-License-Identifier: MIT */
#define PIONEER_COORDINATOR_MACHINE_ONLY
#include "pioneer_coordinator_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerWideOutput.h"
struct WideMachine:CoordinatorMachine {
	std::vector<uint32_t> wide=std::vector<uint32_t>(TT_WIDE_OUTPUT_WORDS,0xffffffff);
	bool comparePhase=false;
	unsigned comparedSource=0,comparedReturn=0;
	bool layer=false;
	WideMachine(const char* root,unsigned rows,unsigned peers=119,bool layerImage=false):CoordinatorMachine(root,layerImage,true),layer(layerImage) {
		SetupFused(1024,rows,peers,3); operation=6; memory[0x140004]=6; memory[0x140024]=TT_WIDE_OUTPUT_BYTES;
	}
	unsigned StepLimit() const override { return 100000000; }
	void Submit() override {
		bool write=command[7]==0x212; unsigned xy=write?command[5]:command[2];
		if (xy!=1555) { comparePhase=false; CoordinatorMachine::Submit(); return; }
		transfers++; if (transfers==failTransfer) { pending=true; return; }
		unsigned local=write?command[0]:command[3],remote=write?command[3]:command[0],bytes=command[8];
		assert(local==(write?0x84000U:0x144000U) && remote>=0x80000000 && !(remote&127) && bytes && bytes<=16384 && !(bytes&127)
			&& (write?command[4]:command[1])==1 && !(write?command[1]:command[4]) && (write?command[2]:command[5])==129
			&& !command[6] && !command[9] && (write?command[7]==0x212:!command[7]));
		unsigned first=(remote-0x80000000)/4,words=bytes/4; assert(first<n && words<=n-first);
		if (write) {
			hostWrites++;
			if (!dropPosted) for (unsigned i=0;i<words-(partialPosted?1:0);i++) wide[first+i]=memory.at(0x84000+i*4)^(first+i==corruptHost?1:0);
			if (pairedCorrupt) { wide[first]^=1; wide[first+1]^=1; }
		} else { hostReads++; for (unsigned i=0;i<words;i++) memory[0x144000+i*4]=wide[first+i]; comparePhase=true; }
		unsigned at=write?4:0; counts[at]++; counts[at+1]+=bytes/64; counts[2]++; counts[6]=counts[2]; counts[at+3]++;
		if (transfers==badCounter) counts[at+3]++;
	}
	uint32_t Load(uint32_t a) override {
		if (comparePhase) { if (Range(a,0x84000,16384)) comparedSource++; if (Range(a,0x144000,16384)) comparedReturn++; }
		return CoordinatorMachine::Load(a);
	}
	bool RunWide() {
		while (pc!=codeBase+0x38 && pc!=codeBase+0x58) Step();
		assert(l1[Index(pc)]==0x0000006f);
		if (pc==codeBase+0x58) { assert(memory.at(0x17ffc8)==TT_COORDINATOR_FAULT && !memory.at(0x17ffc4)); return false; }
		assert(memory.at(0x17ffc0)==codeBase+0xc && memory.at(0x17ffc8)==TT_COORDINATOR_MARKER);
		return memory.at(0x17ffc4)!=0;
	}
	void Verify() {
		assert(!pending && memory.at(0x17ffc4)==workers && hostReads==workers && hostWrites==workers && comparedSource==n && comparedReturn==n);
		for (unsigned row=0;row<n;row++) {
			float expected=0;
			for (unsigned b=0;b<k/32;b++) {
				int dot=0; for (unsigned i=0;i<32;i++) dot+=Q8Machine::Input(b*32+i,trial)*NativeQ8Machine::NativeWeight(row,b*32+i);
				expected=fmaf(float(dot),Q8Machine::Half(Q8Machine::WeightScale(row,b))*Q8Machine::InputScale(b,trial),expected);
			}
			assert(wide[row]==Bits(expected));
		}
		for (unsigned row=n;row<TT_WIDE_OUTPUT_WORDS;row++) assert(wide[row]==0xffffffff);
		uint32_t s[24],before[8],after[8]; for (unsigned i=0;i<24;i++) s[i]=memory.at(0x143000+i*4);
		for (unsigned i=0;i<8;i++) { before[i]=s[8+i]; after[i]=s[16+i]; }
		assert(PioneerWideOutputSummary(s,n,workers,nonce,before,after,layer));
		for (unsigned i=0;i<24;i++) { s[i]^=1; assert(!PioneerWideOutputSummary(s,n,workers,nonce,before,after,layer)); s[i]^=1; }
		printf("OFFLINE wide owned output rows%u peers%u all bits/physical counters,exact compared%u words,stack%08x PASS\n",n,workers,comparedSource,minStack); fflush(stdout);
	}
};
int main(int argc,char** argv)
{
	assert(argc==2 || (argc==3 && std::string(argv[2])=="--layer")); compactImage=true; bool layer=argc==3;
	for (unsigned n:{32U,1280U,6144U,32768U,151936U}) { WideMachine m(argv[1],n,119,layer); assert(m.RunWide()); m.Verify(); }
	for (unsigned fault=0;fault<6;fault++) {
		WideMachine m(argv[1],1280,119,layer); m.dropPosted=fault==0; m.partialPosted=fault==1; m.pairedCorrupt=fault==2;
		m.corruptHost=fault==3?0:fault==4?639:fault==5?1279:~0U; assert(!m.RunWide());
	}
	for (unsigned fault=0;fault<7;fault++) {
		WideMachine m(argv[1],64,119,layer);
		if (fault==0) m.memory[0x14001c]+=4096; if (fault==1) m.memory[0x140020]=0;
		if (fault==2) m.memory[0x140020]=0x10000; if (fault==3) m.memory[0x140024]=16384;
		if (fault==4) m.memory[0x140010]=262176; if (fault==5) m.memory[0x140010]=33;
		if (fault==6) m.memory[0x140004]=7;
		assert(!m.RunWide() && !m.transfers && !m.issued);
	}
	puts("PASS OFFLINE actual wide-output coordinator/unchanged worker bytes,all vocabulary words,private1MiB owner,chunk exact readback,corruption/bounds refusal; NO driver exposure/native speed claim");
}
