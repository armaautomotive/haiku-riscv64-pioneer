/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>
#include <array>
#include "../PioneerQ38OutputTransfer.h"
struct Fabric {
	std::array<uint32_t,32> d={};
	std::array<uint32_t,10> command={};
	std::array<uint32_t,8> counters={};
	std::vector<uint32_t> source,host,posted,scratch;
	unsigned io=0,packets=0,first=0,length=0,fail=0,badCounter=0,dropWrite=0,corruptRead=0,sourceChange=0;
	unsigned dropControl=0,writes=0;
	unsigned hostLow=0,hostHigh=0;
	bool changeDescriptor=false;
	bool pending=false;
	static unsigned SourceOffset(unsigned i) {
		const unsigned offsets[]={0x100,0x118,0x11c,0x120,0x124,0x128,0x12c,0x130,0x134,0x138,0x13c,0x140,0x144,0x150,0x154,0x16c};
		return offsets[i];
	}
	Fabric(unsigned peers,bool edge=false) {
		d[0]=PIONEER_Q38_OUTPUT_MAGIC; d[1]=1; d[2]=peers; d[3]=19;
		d[4]=edge?0xfff00000:0x80000000; d[5]=edge?0xffff:1; d[6]=0x100000; d[7]=peers*1024;
		hostLow=d[4]; hostHigh=d[5];
		for (unsigned i=0;i<16;i++) d[16+i]=0xa5000000+i*0x13579;
		for (auto& n:counters) n=0xfffffff8;
		command.fill(0xdeadbeef);
		source.resize(d[7]/4); host.assign(source.size(),0xdeadbeef); scratch.assign(1024,0xdeadbeef);
		for (unsigned i=0;i<source.size();i++) source[i]=i*0x9e3779b9+0x7fc00000; // ALL raw bits,not only finite floats
	}
	void Fence() { io++; }
	uint32_t ReadReg(unsigned a) {
		io++;
		if (a<40) return command[a/4];
		for (unsigned i=0;i<4;i++) if (a==0x40+i*0x800) return i?0:unsigned(pending);
		if (a>=0x240 && a<0x2c0) return 0;
		for (unsigned i=0;i<16;i++) if (a==SourceOffset(i)) return d[16+i]^(sourceChange && packets>=sourceChange?1:0);
		const unsigned offsets[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};
		for (unsigned i=0;i<8;i++) if (a==offsets[i]) return counters[i];
		assert(false); return 0;
	}
	uint32_t ReadL1(unsigned a) {
		io++;
		if (a>=0x80000 && a<0x80000+d[7]) return source[(a-0x80000)/4];
		assert(a>=0x75000 && a<0x76000); return scratch[(a-0x75000)/4];
	}
	void WriteReg(unsigned a,uint32_t value) {
		io++; assert(!pending);
		if (a<40) { if (++writes!=dropControl) command[a/4]=value; return; }
		assert(a==0x40 && value==1); packets++;
		if (packets==fail) { pending=true; return; }
		bool write=command[7]==0x212; unsigned remote=write?command[3]:command[0];
		unsigned at=remote-hostLow,bytes=command[8],words=bytes/4;
		assert(bytes && bytes<=4096 && !(at&4095) && !(bytes&1023) && at<d[7] && bytes<=d[7]-at);
		assert(!command[6] && !command[9] && (write?command[1]:command[4])==0);
		assert((write?command[4]:command[1])==hostHigh && command[2]==(write?129U:1555U) && command[5]==(write?1555U:129U));
		if (write) {
			assert(!posted.size() && command[0]==0x80000+at && (packets&1));
			if (packets!=dropWrite) { first=at/4; length=words; posted.assign(source.begin()+first,source.begin()+first+words); }
			// ACK deliberately precedes host visibility. CPU must NOT publish.
		} else {
			assert(command[7]==0 && command[3]==0x75000 && !(packets&1));
			// Ordered PCIe read completion drains prior posted writes. The
			// helper additionally compares ALL readback bits,not only ACK.
			for (unsigned i=0;i<posted.size();i++) host[first+i]=posted[i];
			posted.clear();
			for (unsigned i=0;i<words;i++) scratch[i]=host[at/4+i]^(packets==corruptRead && !i?1:0);
		}
		unsigned c=write?4:0; counters[c]++; counters[c+1]+=bytes/64; counters[2]++; counters[6]=counters[2]; counters[c+3]++;
		if (packets==badCounter) counters[c+3]++;
		if (changeDescriptor && packets==1) d[4]^=0x100000; // never redirect a later packet
	}
};
#ifndef PIONEER_Q38_OUTPUT_TRANSFER_MODEL_ONLY
int main()
{
	for (unsigned peers:{1U,2U,4U,7U,119U}) for (bool edge:{false,true}) {
		Fabric f(peers,edge); PioneerQ38OutputTransfer<Fabric> transfer(f);
		assert(transfer.Run(f.d.data()) && transfer.Complete() && !transfer.Unsafe());
		assert(f.host==f.source && f.posted.empty() && f.packets==2*((peers+3)/4));
		unsigned old=f.io; assert(!transfer.Run(f.d.data()) && f.io==old);
	}
	for (unsigned field=0;field<16;field++) {
		Fabric f(7); if (field==3) f.d[field]=0; else f.d[field]^=1;
		PioneerQ38OutputTransfer<Fabric> transfer(f);
		assert(!transfer.Run(f.d.data()) && !f.io && !f.packets);
	}
	for (unsigned packet=1;packet<=60;packet++) for (unsigned fault=0;fault<5;fault++) {
		if ((fault==2 && !(packet&1)) || (fault==3 && (packet&1))) continue;
		Fabric f(119); if (!fault) f.fail=packet; else if (fault==1) f.badCounter=packet;
		else if (fault==2) f.dropWrite=packet; else if (fault==3) f.corruptRead=packet; else f.sourceChange=packet;
		PioneerQ38OutputTransfer<Fabric> transfer(f); assert(!transfer.Run(f.d.data()) && transfer.Unsafe() && !transfer.Complete());
		assert(f.packets==(fault==2?packet+1:packet));
		unsigned old=f.io; assert(!transfer.Run(f.d.data()) && f.io==old);
	}
	for (unsigned word=1;word<=10;word++) {
		Fabric f(119); f.dropControl=word; PioneerQ38OutputTransfer<Fabric> transfer(f);
		assert(!transfer.Run(f.d.data()) && transfer.Unsafe() && !f.packets);
	}
	{
		Fabric f(119); f.changeDescriptor=true; PioneerQ38OutputTransfer<Fabric> transfer(f);
		assert(!transfer.Run(f.d.data()) && transfer.Unsafe() && f.host==f.source && !transfer.Complete());
		unsigned old=f.io; assert(!transfer.Run(f.d.data()) && f.io==old);
	}
	puts("PASS OFFLINE bounded output transfer: all raw bits/1..119 peers/high48-bit owner/wrapped counters/delayed posted writes/ordered full readback, malformed shape ZERO IO, every packet timeout/counter/source/drop/corruption sticky with ZERO later IO;NOT driver/native integration");
}
#endif
