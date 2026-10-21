/* SPDX-License-Identifier: MIT */
#include <array>
#include <cassert>
#include <cstdio>
#include <vector>
#include "../PioneerQ38VectorPanel.h"
#ifndef PIONEER_Q38_WORK_Q4_DIRECT_DECODE
#error Separate OFFLINE direct Q4 decoder test ONLY
#endif
struct Recorder {
	std::array<uint32_t,32> buffer{};
	std::array<bool,32> initialized{};
	std::vector<uint32_t> expanded;
	unsigned remaining=0,at=0,records=0,expansions=0;
	void Op(uint32_t word) {
		if(remaining) {
			assert((word>>24)!=4 && at<32);
			buffer[at]=word;initialized[at++]=true;remaining--;expanded.push_back(word);return;
		}
		if((word>>24)!=4){expanded.push_back(word);return;}
		unsigned index=(word>>14)&31,count=(word>>4)&63;
		assert(count && index+count<=32 && !(word&0x00f83c0c));
		if(word&1){assert((word&3)==3);remaining=count;at=index;records++;}
		else {assert(!(word&3));expansions++;for(unsigned n=0;n<count;n++){assert(initialized[index+n]);expanded.push_back(buffer[index+n]);}}
	}
};
static bool Reference(Recorder& e,unsigned type,unsigned panel,bool work) {
	PioneerQ38VectorPanelSpan span;if(!PioneerQ38VectorPanel(type,panel,span))return false;
	bool nibble=work&&type==12;
	if(nibble&&!PioneerQ38Q4NibbleSetup(e,panel))return false;
	unsigned count=nibble?1:PioneerQ38OperandReplayCount(type,panel);
	unsigned replay=0x04000000|(count<<4);
	if(!PioneerQ38VectorChunk<Recorder,0,0>(e,type,panel,span.second,nibble,replay)
	 || !PioneerQ38VectorChunk<Recorder,0,2>(e,type,panel,span.second,nibble,replay)
	 || !PioneerQ38VectorChunk<Recorder,4,0>(e,type,panel,span.second,nibble,replay)
	 || !PioneerQ38VectorChunk<Recorder,4,2>(e,type,panel,span.second,nibble,replay)
	 || !PioneerQ38VectorChunk<Recorder,8,0>(e,type,panel,span.second,nibble,replay)
	 || !PioneerQ38VectorChunk<Recorder,8,2>(e,type,panel,span.second,nibble,replay)
	 || !PioneerQ38VectorChunk<Recorder,12,0>(e,type,panel,span.second,nibble,replay)
	 || !PioneerQ38VectorChunk<Recorder,12,2>(e,type,panel,span.second,nibble,replay))return false;
	for(uint32_t word:{0x08002040U,0x08082044U,0x08102048U,0x0818204cU})e.Op(word);
	return true;
}
int main() {
	Recorder old,next;unsigned cases=0;
	// Repeated changed quant modes prove no dependence on old operand slots.
	for(unsigned repeat=0;repeat<4;repeat++)for(unsigned type:{8U,12U,14U,8U})
	for(bool work:{false,true})for(unsigned p=0;p<(type==8?2U:16U);p++) {
		old.expanded.clear();next.expanded.clear();unsigned r=old.records,s=next.records,x=old.expansions,y=next.expansions;
		auto before=next.buffer;auto initialized=next.initialized;
		assert(Reference(old,type,p,work)&&PioneerQ38VectorTransform(next,type,p,work));
		assert(!old.remaining&&!next.remaining&&old.expanded==next.expanded);
		assert(old.records-r==1 && old.expansions-x==7);
		assert(next.records-s==(work&&type==12?0U:1U) && next.expansions-y==(work&&type==12?0U:7U));
		if(work&&type==12)for(unsigned slot=12;slot<32;slot++)assert(next.buffer[slot]==before[slot]&&next.initialized[slot]==initialized[slot]);
		cases++;
	}
	for(unsigned type:{0U,8U,12U,14U,UINT32_MAX})for(unsigned panel:{16U,UINT32_MAX}){
		Recorder invalid;assert(!PioneerQ38VectorTransform(invalid,type,panel,true)&&invalid.expanded.empty());cases++;
	}
	puts("PASS OFFLINE direct Q4 expanded stream EXACT:all original SFPLOAD/decode/SFPSTORE/MOV words,only8 replay commands removed per Work panel;affine slots untouched,checked/Q8/Q6 original,return-to-Q8 after changed modes,no stale slots");
	printf("cases%u;NO hardware/driver/throughput qualification\n",cases);
}
