/* SPDX-License-Identifier: MIT */
// OFFLINE schedule/output-layout prerequisite; no driver or card access.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <algorithm>
#include <vector>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerPairedQkvPlan.h"
static unsigned Rows(unsigned t) { return t?1024:2048; }
static unsigned Base(unsigned t) { return t==0?0x87000:t==1?0x84000:0x86000; }
static unsigned Index(unsigned a) { assert(a>=0x83fc0 && a<0x8c000 && !(a&3)); return (a-0x83fc0)/4; }
static uint32_t Value(unsigned t,unsigned row) { return 0x3e000000+t*0x100000+row; }
static bool OutputMatches(const std::vector<uint32_t>& memory) {
	for (unsigned t=0;t<3;t++) for (unsigned row=0;row<Rows(t);row++)
		if (memory[Index(Base(t)+row*4)]!=Value(t,row)) return false;
	return true;
}
static bool Gather(std::vector<uint32_t>& memory,bool reverse,bool preserve,bool allReadersDone) {
	if (!allReadersDone) return false; // MUST precede the very first prefix write
	for (unsigned turn=0;turn<119;turn++) {
		unsigned worker=reverse?119-turn:turn+1; PioneerPairedQkvSpan span;
		assert(PioneerPairedQkvWorker(worker,span)); unsigned address=PioneerPairedQkvDestination(span);
		assert(address && address+span.rows*4<=0x89000);
		uint32_t old[16]; bool save=preserve && PioneerPairedQkvPreservePrefix(span);
		if (save) for (unsigned i=0;i<16;i++) old[i]=memory[Index(address-64+i*4)];
		for (unsigned i=0;i<16;i++) memory[Index(address-64+i*4)]=0xd00d0000+worker*16+i;
		for (unsigned row=0;row<span.rows;row++) memory[Index(address+row*4)]=Value(span.tensor,span.first+row);
		if (save) for (unsigned i=0;i<16;i++) memory[Index(address-64+i*4)]=old[i];
	}
	return true;
}
int main() {
	bool seen[3][2048]={}; unsigned workers[3]={},chunks[3]={},doubled=0;
	for (unsigned worker=1;worker<120;worker++) {
		PioneerPairedQkvSpan span={99,99,99}; assert(PioneerPairedQkvWorker(worker,span));
		assert(span.tensor<3 && !(span.first&31) && (span.rows==32 || span.rows==64));
		assert(span.first+span.rows<=Rows(span.tensor)); workers[span.tensor]++; chunks[span.tensor]+=span.rows/32; doubled+=span.rows==64;
		for (unsigned row=span.first;row<span.first+span.rows;row++) { assert(!seen[span.tensor][row]); seen[span.tensor][row]=true; }
		// Every span stays inside exactly ONE immutable K1024 tensor extent.
		uint64_t begin=uint64_t(span.first)*1152,end=begin+span.rows*1152;
		assert(!(begin&4095) && !(end&4095) && end<=uint64_t(Rows(span.tensor))*1152);
		assert(PioneerPairedQkvDestination(span)==Base(span.tensor)+span.first*4);
		assert(PioneerPairedQkvPreservePrefix(span)==(worker==1));
	}
	assert(workers[0]==60 && workers[1]==30 && workers[2]==29 && doubled==9);
	assert(chunks[0]==64 && chunks[1]==32 && chunks[2]==32);
	for (unsigned t=0;t<3;t++) for (unsigned row=0;row<2048;row++) assert(seen[t][row]==(row<Rows(t)));
	for (unsigned bad:{0U,120U,121U,UINT32_MAX}) { PioneerPairedQkvSpan span={7,8,9};
		assert(!PioneerPairedQkvWorker(bad,span) && span.tensor==7 && span.first==8 && span.rows==9); }
	for (PioneerPairedQkvSpan bad: {PioneerPairedQkvSpan{3,0,32},{0,UINT32_MAX,32},{0,2048,32},{1,1000,32},{2,0,0},{0,0,128}})
		assert(!PioneerPairedQkvDestination(bad) && !PioneerPairedQkvPreservePrefix(bad));
	std::vector<uint32_t> original((0x8c000-0x83fc0)/4,0xabcdef01),memory=original;
	assert(!Gather(memory,true,true,false) && memory==original);
	assert(Gather(memory,true,true,true) && OutputMatches(memory));
	std::vector<bool> touched(memory.size(),false);
	for (unsigned w=1;w<120;w++) { PioneerPairedQkvSpan s; assert(PioneerPairedQkvWorker(w,s)); unsigned a=PioneerPairedQkvDestination(s);
		for (unsigned i=0;i<s.rows*4+64;i+=4) touched[Index(a-64+i)]=true; }
	for (unsigned i=0;i<memory.size();i++) if (!touched[i]) assert(memory[i]==original[i]);
	memory=original; assert(Gather(memory,true,false,true) && !OutputMatches(memory)); // Q prefix corrupts K
	memory=original; assert(Gather(memory,false,true,true) && !OutputMatches(memory)); // forward prefixes corrupt completed rows
	puts("PASS OFFLINE QKV119-peer plan,all4096 rows exactly once/one-tensor32-64-row extents/page and scratch bounds,reverse gather/ALL-reader prerequisite/K-prefix preservation/ALL output bits/guard words;NOT driver-selected,compiled inference or native performance");
}
