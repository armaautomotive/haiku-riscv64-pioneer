/* SPDX-License-Identifier: MIT */
// UNSELECTED prototype: explicit independent image hashes in the runner.
#define PIONEER_Q38_TEST_UNSELECTED_OUTPUT_ROOT
#define PIONEER_Q38_VECTOR_COOPERATIVE
#define PIONEER_Q38_COOPERATIVE_MOCK_ONLY
#include "pioneer_qwen38_dispatch_cooperative_test.cpp"
#include "../PioneerQ38OutputTransfer.h"
#include <utility>
template<class Base> struct OutputReturnMachine:Base {
	std::vector<uint8_t> helper;
	std::vector<uint32_t> host,posted;
	uint32_t hostLow=0x80000000,hostHigh=1;
	unsigned dmaPackets=0,nocReads=0,postedAt=0,failDma=0,dropDma=0,corruptDma=0,changeSourceDma=0;
	template<class... Args> OutputReturnMachine(const char* root,Args&&... args):Base(root,std::forward<Args>(args)...),
		helper(Read((std::string(root)+"/output.bin").c_str())),host(this->active*256,0xdeadbeef) {
		assert(!helper.empty() && helper.size()<=8192 && !(helper.size()&3));
		for (unsigned w=0;w<helper.size()/4;w++) this->l1[this->Index(0x70000+w*4)]=Word(helper,w*4);
		DmaHeader();
	}
	void DmaHeader() {
		this->memory[0x74020]=1; this->memory[0x74024]=PIONEER_Q38_OUTPUT_MAGIC;
		this->memory[0x74028]=hostLow; this->memory[0x7402c]=hostHigh; this->memory[0x74030]=0x100000;
	}
	bool CodeAddress(uint32_t a) const override { return Base::Range(a,0x70000,unsigned(helper.size())) || Base::CodeAddress(a); }
	uint32_t Load(uint32_t a) override {
		if (Base::Range(a,0x70000,unsigned(helper.size()))) return this->l1[this->Index(a)];
		if (Base::Range(a,0xffb20000,0x2000)) {
			nocReads++;
			if (changeSourceDma && dmaPackets>=changeSourceDma && a==0xffb20100) return 0x16004;
		}
		if (Base::Range(a,0x75000,4096)) return this->memory.at(a);
		return Base::Load(a);
	}
	void Packet() override {
		bool write=this->command[7]==0x212;
		bool pci=(write?this->command[5]:this->command[2])==1555;
		if (!pci) { Base::Packet(); return; }
		assert(this->AllFinished() && this->gathers==this->active && this->pending && !this->unknown);
		dmaPackets++;
		if (dmaPackets==failDma) { this->faultAt=this->packets; return; }
		unsigned remote=write?this->command[3]:this->command[0],bytes=this->command[8],at=remote-hostLow;
		assert(bytes && bytes<=4096 && !(bytes&1023) && !(at&4095) && at<this->active*1024 && bytes<=this->active*1024-at);
		assert(!this->command[6] && !this->command[9] && !(write?this->command[1]:this->command[4])
			&& (write?this->command[4]:this->command[1])==hostHigh && (write?this->command[2]:this->command[5])==129);
		if (write) {
			assert(this->command[0]==0x80000+at && posted.empty() && (dmaPackets&1));
			postedAt=at/4;
			if (dmaPackets!=dropDma) for (unsigned i=0;i<bytes/4;i++) posted.push_back(this->memory.at(0x80000+at+i*4));
		} else {
			assert(this->command[3]==0x75000 && !this->command[7] && !(dmaPackets&1));
			// Host writes remain invisible after NoC ACK. Only this ordered
			// read completion drains them; the actual root compares ALL bits.
			for (unsigned i=0;i<posted.size();i++) host[postedAt+i]=posted[i];
			posted.clear();
			for (unsigned i=0;i<bytes/4;i++) this->memory[0x75000+i*4]=host[at/4+i]^(dmaPackets==corruptDma && !i?1:0);
		}
		this->counts[write?4:0]++; this->counts[write?5:1]+=bytes/64;
		this->counts[write?7:3]++; this->counts[2]++;
		this->responses++; this->pending=false;
	}
	void VerifyHost() {
		assert(posted.empty() && dmaPackets==2*((this->active+3)/4));
		for (unsigned w=0;w<this->active;w++) for (unsigned r=0;r<256;r++)
			assert(host[w*256+r]==this->memory.at(0x80000+w*1024+r*4));
		assert(this->memory.at(0x7f8c4)==PIONEER_Q38_OUTPUT_MAGIC && this->memory.at(0x7f8c8)==this->active*1024
			&& this->memory.at(0x7f8cc)==this->memory.at(0x74018) && this->memory.at(0x7f8e0)==1);
		for (unsigned i=57;i<64;i++) assert(!this->memory.at(0x7f800+i*4));
	}
	void VerifyFaultInert() {
		unsigned old=nocReads,packets=this->packets;
		for (unsigned i=0;i<100;i++) this->StepBlock();
		assert(nocReads==old && this->packets==packets);
	}
};
int main(int argc,char** argv)
{
	assert(argc==3);
	for (unsigned workers:{1U,2U,7U,119U}) for (unsigned type:{8U,12U,14U}) {
		OutputReturnMachine<DispatchMachine> m(argv[1],workers,type,17408);
		assert(m.RunRoot()); m.VerifyHost();
		m.Configure(2); m.dmaPackets=0; m.DmaHeader(); assert(m.RunRoot()); m.VerifyHost();
		m.Configure(3); assert(m.RunRoot() && m.dmaPackets==2*((workers+3)/4)); // CHECKED: no extra host DMA
	}
	for (unsigned type:{8U,12U,14U}) for (unsigned k:{5120U,6144U,17408U}) {
		OutputReturnMachine<CooperativeDispatch> m(argv[1],argv[2],2,type,k);
		assert(m.RunRoot()); m.VerifyHost();
		for (auto& p:m.actual) assert(p->CorrectRows() && !p->dotProbeReads);
	}
	for (unsigned packet:{1U,30U,60U}) for (unsigned fault=0;fault<4;fault++) {
		if ((fault==1 && !(packet&1)) || (fault==2 && (packet&1))) continue;
		OutputReturnMachine<DispatchMachine> m(argv[1],119,12,5120);
		if (!fault) m.failDma=packet; else if (fault==1) m.dropDma=packet;
		else if (fault==2) m.corruptDma=packet; else m.changeSourceDma=packet;
		assert(!m.RunRoot() && m.unknown && !m.memory.at(0x7f8e0)); m.VerifyFaultInert();
	}
	for (unsigned field=9;field<16;field++) {
		OutputReturnMachine<DispatchMachine> m(argv[1],7); m.memory[0x74000+field*4]^=1;
		assert(!m.RunRoot() && !m.packets && !m.dmaPackets);
	}
	for (bool pc:{false,true}) {
		OutputReturnMachine<DispatchMachine> m(argv[1],7); if (pc) m.badPcWorker=7; else m.nonfiniteWorker=7;
		assert(!m.RunRoot() && m.unknown && !m.dmaPackets); m.VerifyFaultInert();
	}
	puts("PASS OFFLINE ACTUAL unselected root+real peers: all peer physical/source/counter/output proofs BEFORE bounded host DMA, repeated/checked mode isolation, delayed posted writes/full ordered bit comparison, every fault parks without later NoC IO;NO kernel/BME/host-owner/native integration");
}
