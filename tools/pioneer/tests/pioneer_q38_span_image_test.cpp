/* SPDX-License-Identifier: MIT */
// ACTUAL compiled instructions;host/wire memory ONLY is simulated.
#define PIONEER_Q38_BLOCK_MACHINE_ONLY
#include "pioneer_qwen38_block_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38SpanShape.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38SpanImage.h"
#include <array>
struct Q38SpanMachine:BlockMachine {
	std::vector<uint32_t> golden,hostData,gddr;
	std::array<uint32_t,64> descriptor={};
	uint32_t command[10]={},counts[8]={},seals[16]={};
	unsigned bytes,bank,base,hostLow,hostHigh,transfers=0,controlWrites=0,mmioReads=0;
	unsigned failPacket=0,badCounter=0,dropControl=0,changeSourceAfter=0;
	unsigned mirrorStores=0,echoStores=0,dropMirror=0,dropEcho=0;
	int dropPacket=-1,corruptPacket=-1;
	bool pending=false;
	static unsigned SourceOffset(unsigned i) {
		const unsigned offsets[]={0x100,0x118,0x11c,0x120,0x124,0x128,0x12c,0x130,0x134,0x138,0x13c,0x140,0x144,0x150,0x154,0x16c};
		return offsets[i];
	}
	Q38SpanMachine(const char* image,unsigned count,unsigned b=0,bool edge=false)
		:BlockMachine(image,8,0,.01f,nullptr,true),bytes(count),bank(b),base(edge?0xc4000000-count:0x04000000),
		hostLow(edge?0xfff00000:0x80000000),hostHigh(edge?0xffff:1) {
		assert(code.size()==TT_Q38_SPAN_CODE_BYTES && l1[Index(0x6000)]==TT_Q38_SPAN_BOOT);
		for (unsigned i=0;i<2048;i++) assert(PioneerQ38SpanCodeWord(i)==(i<code.size()/4?Word(code,i*4):0));
		assert(PioneerQ38SpanCodeWord(2048)==UINT32_MAX);
		golden.resize(bytes/4); for (unsigned i=0;i<golden.size();i++) golden[i]=i?0x9e3779b9U*i+0x7fc00000:0x12345678;
		hostData=golden; gddr.assign(golden.size(),0xdeadbeef);
		for (unsigned a:{0x80000U,0x84000U,0x144000U}) for (unsigned i=0;i<4096;i++) memory[a+i*4]=0xdeadbeef;
		for (unsigned i=0;i<32;i++) memory[0x143000+i*4]=0xdeadbeef;
		for (auto& c:command) c=0xdeadbeef;
		for (unsigned i=0;i<8;i++) counts[i]=0xfffffff8;
		for (unsigned i=0;i<16;i++) seals[i]=0xa5000000+i*0x13579; seals[0]=0x16005;
		descriptor[0]=TT_Q38_SPAN_MARKER; descriptor[1]=descriptor[2]=1;
		descriptor[4]=bytes; descriptor[5]=base; descriptor[6]=17; descriptor[7]=hostLow; descriptor[8]=hostHigh;
		descriptor[9]=0x100000; descriptor[10]=bank; descriptor[11]=PioneerQ38SpanRoute(bank);
		for (unsigned i=0;i<16;i++) descriptor[16+i]=seals[i];
		for (unsigned i=0;i<64;i++) memory[0x140000+i*4]=descriptor[i];
		memory[0x17ffc0]=memory[0x17ffc4]=memory[0x17ffc8]=0;
	}
	unsigned StepLimit() const override { return 100000000; }
	uint32_t Load(uint32_t a) override {
		if (Range(a,0xffb20000,0x2000)) {
			mmioReads++; unsigned offset=a-0xffb20000;
			if (offset<40) return command[offset/4];
			for (unsigned i=0;i<4;i++) if (offset==0x40+i*0x800) return i?0:unsigned(pending);
			if (offset>=0x240 && offset<0x2c0) return 0;
			for (unsigned i=0;i<16;i++) if (offset==SourceOffset(i)) return seals[i];
			const unsigned offsets[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};
			for (unsigned i=0;i<8;i++) if (offset==offsets[i]) return counts[i];
			assert(false && "unqualified span MMIO read");
		}
		if (CodeAddress(a)) return BlockMachine::Load(a);
		assert(Range(a,0x6e000,8192) || Range(a,0x140000,256) || Range(a,0x143000,128)
			|| Range(a,0x80000,16384) || Range(a,0x84000,16384) || Range(a,0x144000,16384)
			|| Range(a,0x17ffc0,12));
		return memory.at(a);
	}
	void Submit();
	void Store(uint32_t a,uint32_t value) override {
		if (a>=0xffb20000 && a<0xffb20028) {
			assert(!pending); if (++controlWrites!=dropControl) command[(a-0xffb20000)/4]=value; return;
		}
		if (a==0xffb20040) { assert(value==1 && !pending); Submit(); return; }
		assert(Range(a,0x6e000,8192) || Range(a,0x143000,128) || Range(a,0x80000,16384)
			|| Range(a,0x84000,16384) || Range(a,0x17ffc0,12));
		if (Range(a,0x6e000,8192)) minStack=std::min(minStack,a);
		if (Range(a,0x80000,16384) && ++mirrorStores==dropMirror) return;
		if (Range(a,0x84000,16384) && ++echoStores==dropEcho) return;
		memory[a]=value;
	}
	bool Run() {
		while (pc!=0x72038 && pc!=0x7207c) StepBlock();
		assert(minStack>=0x6e000 && !matrixInstructions && !moveInstructions);
		if (pc==0x7207c) {
			assert(memory.at(0x17ffc8)==TT_Q38_SPAN_FAULT);
			unsigned old=transfers,reads=mmioReads;
			for (unsigned i=0;i<16;i++) StepBlock();
			assert(transfers==old && mmioReads==reads); return false;
		}
		assert(memory.at(0x17ffc0)==0x7200c && memory.at(0x17ffc8)==TT_Q38_SPAN_MARKER);
		return memory.at(0x17ffc4)==bytes/4096;
	}
	void Verify() {
		assert(!pending && hostData==golden && gddr==golden);
		uint32_t s[32],before[8],after[8];
		for (unsigned i=0;i<32;i++) s[i]=memory.at(0x143000+i*4);
		for (unsigned i=0;i<8;i++) { before[i]=s[8+i]; after[i]=s[16+i]; }
		assert(PioneerQ38SpanSummary(s,descriptor.data(),before,after));
		for (unsigned i=0;i<32;i++) { s[i]^=1; assert(!PioneerQ38SpanSummary(s,descriptor.data(),before,after)); s[i]^=1; }
	}
};
void Q38SpanMachine::Submit()
{
	bool fast=descriptor[2]==2; unsigned packet=transfers%(fast?2U:7U); transfers++;
	if (transfers==failPacket) { pending=true; return; }
	bool write=command[7]==0x212; assert(write || !command[7]);
	unsigned xy=write?command[5]:command[2],local=write?command[0]:command[3],remote=write?command[3]:command[0];
	unsigned words=command[8]/4; bool host=xy==1555;
	const unsigned routes[]={786,978,1170,785,977,1169,1361};
	assert(bank<7 && (host || xy==routes[bank]) && words && words<=4096 && !(words&1023)
		&& command[8]==words*4 && !command[6] && !command[9] && (write?command[2]:command[5])==129
		&& !(write?command[1]:command[4]) && (write?command[4]:command[1])==(host?hostHigh:0U));
	unsigned firstAddress=host?hostLow:base; assert(remote>=firstAddress && !((remote-firstAddress)&4095));
	unsigned first=(remote-firstAddress)/4; assert(first<golden.size() && words<=golden.size()-first);
	assert(local==(host?(write?0x84000U:0x144000U):(write?0x144000U:0x84000U)));
	assert((packet==0 && host && !write) || (packet==1 && !host && write) || (!fast && ((packet==2 && !host && !write)
		|| ((packet==3 || packet==5) && host && write) || ((packet==4 || packet==6) && host && !write))));
	auto& external=host?hostData:gddr;
	if (int(packet)!=dropPacket) for (unsigned i=0;i<words;i++) {
		if (write) external[first+i]=memory.at(local+i*4)^(int(packet)==corruptPacket && !i?1:0);
		else memory[local+i*4]=external[first+i]^(int(packet)==corruptPacket && !i?1:0);
	}
	unsigned offset=write?4:0; counts[offset]++; counts[offset+1]+=words/16; counts[2]++; counts[6]=counts[2]; counts[offset+3]++;
	if (transfers==badCounter) counts[offset+3]++;
	if (transfers==changeSourceAfter) seals[11]^=1;
}
#ifndef PIONEER_Q38_SPAN_MACHINE_ONLY
int main(int argc,char** argv)
{
	assert(argc==2);
	for (unsigned bank=0;bank<7;bank++) for (bool edge:{false,true}) {
		for (unsigned bytes:{4096U,12288U,16384U,20480U,65536U,1048576U}) {
			Q38SpanMachine m(argv[1],bytes,bank,edge); assert(m.Run()); m.Verify();
		}
		printf("PASS actual span bank%u edge%u:4KiB..1MiB,all raw bits and32 summary fields,wrapped counters\n",bank,unsigned(edge)); fflush(stdout);
	}
	for (unsigned packet=0;packet<7;packet++) for (bool corrupt:{false,true}) {
		Q38SpanMachine m(argv[1],4096);
		if (corrupt) m.corruptPacket=int(packet); else m.dropPacket=int(packet);
		bool completed=m.Run();
		// A corrupt INITIAL host read is observable only against the future
		// caller's independent frozen CPU bytes. It MUST NOT commit pages.
		if (!packet) assert(completed && m.hostData!=m.golden); else assert(!completed);
	}
	for (unsigned packet=1;packet<=7;packet++) for (unsigned fault=0;fault<3;fault++) {
		Q38SpanMachine m(argv[1],4096);
		if (!fault) m.failPacket=packet; else if (fault==1) m.badCounter=packet; else m.changeSourceAfter=packet;
		assert(!m.Run() && m.transfers==packet);
	}
	for (unsigned word=1;word<=10;word++) {
		Q38SpanMachine m(argv[1],4096); m.dropControl=word; assert(!m.Run() && !m.transfers);
	}
	for (unsigned word:{1U,512U,1024U}) {
		Q38SpanMachine m(argv[1],4096); m.dropMirror=word; assert(!m.Run());
	}
	for (unsigned word:{1U,512U,1024U,1025U,1536U,2048U}) {
		Q38SpanMachine m(argv[1],4096); m.dropEcho=word; assert(!m.Run());
	}
	for (unsigned source=0;source<16;source++) {
		Q38SpanMachine m(argv[1],4096); m.seals[source]^=1; assert(!m.Run() && !m.transfers);
	}
	for (unsigned slot:{0U,1U,2U,3U,12U,15U,32U,63U}) {
		Q38SpanMachine m(argv[1],4096); m.memory[0x140000+slot*4]^=1;
		assert(!m.Run() && !m.transfers && !m.mmioReads);
	}
	for (unsigned fault=0;fault<13;fault++) {
		Q38SpanMachine m(argv[1],4096);
		const unsigned slots[]={4,4,4,5,5,6,6,7,8,9,10,11,8};
		const unsigned values[]={0,4097,1048576+4096,0x04000001,0xc4000000,0,0x7fffffff,0x80001000,0,16384,7,0,0x10000};
		m.memory[0x140000+slots[fault]*4]=values[fault];
		assert(!m.Run() && !m.transfers && !m.mmioReads);
	}
	puts("PASS OFFLINE actual seven-bank span:bounds/source/packet/drop/corrupt/stale-echo faults,UNKNOWN inert park,no matrix arithmetic;kernel owner+CPU echo+native qualification STILL REQUIRED");
}
#endif
