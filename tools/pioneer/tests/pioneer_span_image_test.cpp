/* SPDX-License-Identifier: MIT */
#define PIONEER_COORDINATOR_MACHINE_ONLY
#include "pioneer_coordinator_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerSpanProof.h"
struct SpanMachine:CoordinatorMachine {
	std::vector<uint32_t> golden,hostData,gddr;
	unsigned bytes,mirrorStores=0,dropMirror=0,echoStores=0,dropEcho=0;
	int dropPacket=-1,corruptPacket=-1;
	SpanMachine(const char* root,unsigned count):CoordinatorMachine(root,true,true),bytes(count) {
		golden.resize(count/4); for (unsigned i=0;i<golden.size();i++) golden[i]=i?0x9e3779b9U*i+0x7fc00000:0x12345678;
		hostData=golden; gddr.assign(golden.size(),0xdeadbeef);
		for (unsigned i=0;i<64;i++) memory[0x140000+i*4]=0;
		for (unsigned i=0;i<4096;i++) memory[0x80000+i*4]=memory[0x84000+i*4]=memory[0x144000+i*4]=0xdeadbeef;
		memory[0x17ffc0]=memory[0x17ffc4]=memory[0x17ffc8]=0;
		memory[0x140000]=0x31445243; memory[0x140004]=7; memory[0x140008]=1;
		memory[0x140010]=bytes; memory[0x140014]=0x04000000; memory[0x140018]=17;
		memory[0x14001c]=0x80000000; memory[0x140020]=1; memory[0x140024]=TT_WIDE_OUTPUT_BYTES;
	}
	unsigned StepLimit() const override { return 100000000; }
	void Submit() override {
		unsigned packet=transfers%7; transfers++; if (transfers==failTransfer) { pending=true; return; }
		bool write=command[7]==0x212; assert(write || !command[7]);
		unsigned xy=write?command[5]:command[2],local=write?command[0]:command[3],remote=write?command[3]:command[0],words=command[8]/4;
		assert(words && words<=4096 && !(words&1023) && !command[6] && !command[9] && (write?command[2]:command[5])==129 && !(write?command[1]:command[4]));
		bool host=xy==1555; assert(host || xy==786);
		assert((write?command[4]:command[1])==(host?1U:0U));
		unsigned base=host?0x80000000:0x04000000; assert(remote>=base && !((remote-base)&4095));
		unsigned first=(remote-base)/4; assert(first<golden.size() && words<=golden.size()-first);
		assert(host?(local==(write?0x84000U:0x144000U)):(local==(write?0x144000U:0x84000U)));
		assert((packet==0 && host && !write) || (packet==1 && !host && write) || (packet==2 && !host && !write)
			|| ((packet==3 || packet==5) && host && write) || ((packet==4 || packet==6) && host && !write));
		auto& external=host?hostData:gddr;
		if (int(packet)!=dropPacket) for (unsigned i=0;i<words;i++) {
			if (write) external[first+i]=memory.at(local+i*4)^(int(packet)==corruptPacket && !i?1:0);
			else memory[local+i*4]=external[first+i]^(int(packet)==corruptPacket && !i?1:0);
		}
		unsigned at=write?4:0; counts[at]++; counts[at+1]+=words/16; counts[2]++; counts[6]=counts[2]; counts[at+3]++;
		if (transfers==badCounter) counts[at+3]++;
	}
	uint32_t Load(uint32_t a) override { if (Range(a,0x80000,16384)) return memory.at(a); return CoordinatorMachine::Load(a); }
	void Store(uint32_t a,uint32_t value) override {
		if (Range(a,0x80000,16384)) { if (++mirrorStores!=dropMirror) memory[a]=value; return; }
		if (Range(a,0x84000,16384)) { if (++echoStores!=dropEcho) memory[a]=value; return; }
		CoordinatorMachine::Store(a,value);
	}
	bool RunSpan() {
		while (pc!=TT_LAYER_PARK && pc!=0x64058) Step();
		assert(l1[Index(pc)]==0x0000006f && minStack>=0x6e000);
		if (pc==0x64058) { assert(memory.at(0x17ffc8)==TT_COORDINATOR_FAULT && !memory.at(0x17ffc4)); return false; }
		assert(memory.at(0x17ffc0)==TT_LAYER_DONE && memory.at(0x17ffc8)==TT_COORDINATOR_MARKER);
		return memory.at(0x17ffc4)!=0;
	}
	void Verify() {
		assert(!pending && !issued && memory.at(0x17ffc4)==bytes/4096 && hostData==golden && gddr==golden);
		uint32_t s[24],before[8],after[8]; for (unsigned i=0;i<24;i++) s[i]=memory.at(0x143000+i*4);
		for (unsigned i=0;i<8;i++) { before[i]=s[8+i]; after[i]=s[16+i]; }
		assert(PioneerSpanSummary(s,bytes,17,before,after));
		for (unsigned i=0;i<24;i++) { s[i]^=1; assert(!PioneerSpanSummary(s,bytes,17,before,after)); s[i]^=1; }
		printf("OFFLINE actual span bytes%u,all GDDR/source/complement/original echo words,fresh full physical counters,stack%08x PASS\n",bytes,minStack); fflush(stdout);
	}
};
int main(int argc,char** argv)
{
	assert(argc==2);
	for (unsigned bytes:{4096U,12288U,16384U,20480U,65536U,1048576U}) { SpanMachine m(argv[1],bytes); assert(m.RunSpan()); m.Verify(); }
	for (unsigned packet=0;packet<7;packet++) for (bool corrupt:{false,true}) {
		SpanMachine m(argv[1],4096); if (corrupt) m.corruptPacket=int(packet); else m.dropPacket=int(packet);
		bool completed=m.RunSpan();
		// A corrupt/missing initial host READ is detected by the mandatory
		// frozen CPU echo comparison,not by pretending GPU knows CPU input.
		if (!packet) assert(completed && m.hostData!=m.golden); else assert(!completed);
	}
	for (unsigned packet=1;packet<=7;packet++) for (bool counter:{false,true}) {
		SpanMachine m(argv[1],4096); if (counter) m.badCounter=packet; else m.failTransfer=packet; assert(!m.RunSpan());
	}
	for (unsigned word:{1U,512U,1024U}) { SpanMachine m(argv[1],4096); m.dropMirror=word; assert(!m.RunSpan()); }
	for (unsigned word:{1U,512U,1024U,1025U,1536U,2048U}) { SpanMachine m(argv[1],4096); m.dropEcho=word; assert(!m.RunSpan()); }
	for (unsigned fault=0;fault<12;fault++) {
		SpanMachine m(argv[1],4096);
		if (fault==0) m.memory[0x140000]^=1; if (fault==1) m.memory[0x140008]=2;
		if (fault==2) m.memory[0x14000c]=1; if (fault==3) m.memory[0x140010]=0;
		if (fault==4) m.memory[0x140010]=1048576+4096; if (fault==5) m.memory[0x140010]=4097;
		if (fault==6) m.memory[0x140014]=0x82400000; if (fault==7) m.memory[0x140018]=0;
		if (fault==8) m.memory[0x14001c]+=4096; if (fault==9) m.memory[0x140020]=0;
		if (fault==10) m.memory[0x140024]=16384; if (fault==11) m.memory[0x140028]=1;
		assert(!m.RunSpan() && !m.transfers && !m.issued);
	}
	puts("PASS OFFLINE actual bounded span image,full1MiB,special/raw bits,host/GDDR/stale/drop/counter faults,mandatory independent frozen-CPU echo,no checksum; NO driver exposure/native speed claim");
}
