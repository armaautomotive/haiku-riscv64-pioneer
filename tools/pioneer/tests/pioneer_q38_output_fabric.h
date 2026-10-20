/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_OUTPUT_FABRIC_H
#define PIONEER_Q38_OUTPUT_FABRIC_H
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
		// Independently emitted helper MUST match every production saved/code
		// word too;the test fabric must not hide a stale baked prefix.
#ifdef PIONEER_Q38_WORK_OUTPUT_DMA
		for (unsigned a=0x70000;a<0x72000;a+=4)
			assert(PioneerQ38DispatchPrefixWord(0,a)==(a<0x70000+helper.size()?Word(helper,a-0x70000):0));
#endif
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
#endif
