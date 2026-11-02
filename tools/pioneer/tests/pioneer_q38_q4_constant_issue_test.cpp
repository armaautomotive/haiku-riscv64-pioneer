/* SPDX-License-Identifier: MIT */
#include <array>
#include <cassert>
#include <cstdio>
#include <vector>
#include "../PioneerQ38VectorPanel.h"
struct Recorder {
	std::array<uint32_t,32> replay{};
	std::array<bool,32> initialized{};
	std::vector<uint32_t> issued, expanded;
	unsigned remaining=0, at=0;
	void Op(uint32_t word) {
		issued.push_back(word);
		if (remaining) {
			assert((word>>24)!=4 && at<32);
			replay[at]=word; initialized[at++]=true;
			remaining--; expanded.push_back(word); return;
		}
		if ((word>>24)!=4) { expanded.push_back(word); return; }
		unsigned index=(word>>14)&31, count=(word>>4)&63;
		assert(count && index+count<=32 && !(word&0x00f83c0c));
		if (word&1) { assert((word&3)==3); remaining=count; at=index; }
		else {
			assert(!(word&3));
			for (unsigned n=0;n<count;n++) {
				assert(initialized[index+n]); expanded.push_back(replay[index+n]);
			}
		}
	}
};
// Original generic algorithm independently called with actual type/panel;
// never use the new constant-transform helper to derive expected words.
static bool Original(Recorder& e, unsigned type, unsigned panel) {
	PioneerQ38VectorPanelSpan span;
	if (!PioneerQ38VectorPanel(type,panel,span)) return false;
	unsigned count=PioneerQ38OperandReplayCount(type,panel);
	if (!count) return false;
	unsigned replay=0x04000000|(count<<4);
	if (!PioneerQ38VectorChunk<Recorder,0,0>(e,type,panel,span.second,replay)
	 || !PioneerQ38VectorChunk<Recorder,0,2>(e,type,panel,span.second,replay)
	 || !PioneerQ38VectorChunk<Recorder,4,0>(e,type,panel,span.second,replay)
	 || !PioneerQ38VectorChunk<Recorder,4,2>(e,type,panel,span.second,replay)
	 || !PioneerQ38VectorChunk<Recorder,8,0>(e,type,panel,span.second,replay)
	 || !PioneerQ38VectorChunk<Recorder,8,2>(e,type,panel,span.second,replay)
	 || !PioneerQ38VectorChunk<Recorder,12,0>(e,type,panel,span.second,replay)
	 || !PioneerQ38VectorChunk<Recorder,12,2>(e,type,panel,span.second,replay)) return false;
	for (uint32_t word:{0x08002040U,0x08082044U,0x08102048U,0x0818204cU}) e.Op(word);
	return true;
}
int main() {
	Recorder old,next; unsigned cases=0,refusals=0;
	for (unsigned pass=0;pass<16;pass++) for (unsigned type:{8U,12U,14U,12U,8U})
	for (unsigned p=0;p<(type==8?2U:16U);p++) {
		old.issued.clear(); next.issued.clear(); old.expanded.clear(); next.expanded.clear();
		assert(Original(old,type,p) && PioneerQ38VectorTransform(next,type,p));
		assert(old.issued==next.issued && old.expanded==next.expanded);
		assert(old.replay==next.replay && old.initialized==next.initialized);
		assert(!old.remaining && !next.remaining); cases++;
	}
	for (unsigned type:{0U,8U,12U,14U,15U,UINT32_MAX})
	for (unsigned panel:{0U,2U,15U,16U,UINT32_MAX}) {
		if ((type==8 && panel<2) || ((type==12 || type==14) && panel<16)) continue;
		Recorder bad;
		assert(!PioneerQ38VectorTransform(bad,type,panel) && bad.issued.empty()); refusals++;
	}
	printf("PASS_CONSTANT_Q4_ISSUE issued+expanded streams/replay state bit-exact cases%u refusals%u;Q8/Q6/return-to-Q8 unchanged;NO hardware or speed claim\n",cases,refusals);
}
