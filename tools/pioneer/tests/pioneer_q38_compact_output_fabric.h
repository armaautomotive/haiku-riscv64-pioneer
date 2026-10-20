/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_COMPACT_OUTPUT_FABRIC_H
#define PIONEER_Q38_COMPACT_OUTPUT_FABRIC_H
#include "pioneer_q38_output_fabric.h"
// Independent endpoint observer. Exact groups come from the frozen private
// task array,not the proposed byte-count helper or returned GPU statistics.
template<class Base> struct CompactReturnMachine:OutputReturnMachine<Base> {
	using OutputReturnMachine<Base>::OutputReturnMachine;
	unsigned OutputBytes() const {
		unsigned words=0;
		for (unsigned w=0;w<this->active;w++) {
			unsigned groups=this->memory.at(0x74088+w*32); assert(groups && groups<=16);
			words+=groups*16;
		}
		assert(words>=this->active*16 && words<=this->active*256); return words*4;
	}
	void Packet() override {
		bool write=this->command[7]==0x212;
		bool pci=(write?this->command[5]:this->command[2])==1555;
		if (!pci) { Base::Packet(); return; }
		assert(this->AllFinished() && this->gathers==this->active && this->pending && !this->unknown);
		this->dmaPackets++;
		if (this->dmaPackets==this->failDma) { this->faultAt=this->packets; return; }
		unsigned total=OutputBytes(),remote=write?this->command[3]:this->command[0];
		unsigned bytes=this->command[8],at=remote-this->hostLow;
		assert(bytes && bytes<=4096 && !(bytes&63) && !(at&4095) && at<total && bytes<=total-at);
		assert(!this->command[6] && !this->command[9] && !(write?this->command[1]:this->command[4])
			&& (write?this->command[4]:this->command[1])==this->hostHigh && (write?this->command[2]:this->command[5])==129);
		if (write) {
			assert(this->command[0]==0x80000+at && this->posted.empty() && (this->dmaPackets&1));
			this->postedAt=at/4;
			if (this->dmaPackets!=this->dropDma) for (unsigned i=0;i<bytes/4;i++)
				this->posted.push_back(this->memory.at(0x80000+at+i*4));
		} else {
			assert(this->command[3]==0x75000 && !this->command[7] && !(this->dmaPackets&1));
			for (unsigned i=0;i<this->posted.size();i++) this->host[this->postedAt+i]=this->posted[i];
			this->posted.clear();
			for (unsigned i=0;i<bytes/4;i++) this->memory[0x75000+i*4]=this->host[at/4+i]
				^(this->dmaPackets==this->corruptDma && !i?1:0);
		}
		this->counts[write?4:0]++; this->counts[write?5:1]+=bytes/64;
		this->counts[write?7:3]++; this->counts[2]++; this->responses++; this->pending=false;
	}
	void VerifyCompactHost() {
		unsigned words=OutputBytes()/4,offset=0;
		assert(this->posted.empty() && this->dmaPackets==2*((words*4+4095)/4096));
		for (unsigned w=0;w<this->active;w++) for (unsigned row=0;row<this->memory.at(0x74088+w*32)*16;row++) {
			assert(this->host[offset]==this->memory.at(0x80000+offset*4)
				&& this->host[offset]==this->ExpectedOutput(w,row)); offset++;
		}
		assert(offset==words);
		for (unsigned i=words;i<this->host.size();i++) assert(this->host[i]==0xdeadbeef);
		assert(this->memory.at(0x7f8c4)==0x324f3851 && this->memory.at(0x7f8c8)==words*4
			&& this->memory.at(0x7f8cc)==this->memory.at(0x74018) && this->memory.at(0x7f8e0)==1);
		for (unsigned i=57;i<64;i++) assert(!this->memory.at(0x7f800+i*4));
	}
};
#endif
