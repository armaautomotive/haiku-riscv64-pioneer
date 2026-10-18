/* SPDX-License-Identifier: MIT */
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38Payload.h"
#include <assert.h>
#include <stdio.h>
#include <vector>
struct IO {
	std::vector<uint32_t> data=std::vector<uint32_t>(0x42000/4,0xa5a5a5a5);
	unsigned fences=0,reads=0,writes=0,pairs=0,views=0,drop=0,corrupt=UINT32_MAX;
	bool stable=true,dirty=false;
	bool Q38PanelMappingStable() { return stable; }
	struct View {
		IO& io; unsigned first;
		void PayloadFence() { io.fences++; io.dirty=false; }
		uint32_t PayloadRead(unsigned w) { assert(!io.dirty && first+w<io.data.size()); io.reads++; return io.data[first+w]^((first+w==io.corrupt)?1:0); }
		void PayloadWrite(unsigned w,uint32_t value) { assert(first+w<io.data.size()); io.dirty=true; if (++io.writes!=io.drop) io.data[first+w]=value; }
		uint64_t PayloadReadPair(unsigned w) { assert(!((first+w)&1)); io.pairs++; uint64_t low=PayloadRead(w); return low|(uint64_t(PayloadRead(w+1))<<32); }
		void PayloadWritePair(unsigned w,uint32_t low,uint32_t high) { assert(!((first+w)&1)); io.pairs++; PayloadWrite(w,low); PayloadWrite(w+1,high); }
	};
	View Q38PayloadData(uint32_t a) { assert(!(a&7) && a>=0x6e000 && a<0xb0000); views++; return View{*this,(a-0x6e000)/4}; }
};
#ifndef PIONEER_Q38_PAYLOAD_MOCK_ONLY
int main()
{
	std::vector<uint32_t> input(8704),out(256); for (unsigned w=0;w<input.size();w++) input[w]=0x12345678^(w*0x713);
	struct Range { uint32_t a; unsigned words; };
	const Range allowed[]={{0xa0000,2560},{0xa0000,3072},{0xa0000,8704},{0x94000,32},{0x94000,80},{0x94000,160},{0x94000,192},{0x94000,544},{0x74000,10}};
	for (auto r:allowed) {
		IO io; assert(PioneerQ38PayloadStage(io,r.a,input.data(),r.words) && io.writes==r.words && io.reads==r.words && io.fences==3 && io.pairs==r.words);
		if (r.a!=0x74000) { io.reads=0; assert(PioneerQ38PayloadCompare(io,r.a,input.data(),r.words) && io.reads==r.words); }
		unsigned first=(r.a-0x6e000)/4;
		assert(io.data[first-1]==0xa5a5a5a5 && io.data[first+r.words]==0xa5a5a5a5);
		for (unsigned w=0;w<r.words;w++) {
			IO bad; bad.drop=w+1; assert(!PioneerQ38PayloadStage(bad,r.a,input.data(),r.words) && bad.reads==r.words);
			if (r.a!=0x74000) { bad.drop=0; assert(PioneerQ38PayloadStage(bad,r.a,input.data(),r.words)); bad.corrupt=first+w; assert(!PioneerQ38PayloadCompare(bad,r.a,input.data(),r.words)); }
		}
	}
	for (unsigned words=16;words<=256;words+=16) {
		IO io; assert(PioneerQ38PayloadStage(io,0x84000,input.data(),words));
		io.reads=io.writes=0; assert(PioneerQ38PayloadReadOutput(io,words,out.data()) && io.reads==words && !io.writes);
		for (unsigned w=0;w<words;w++) assert(out[w]==input[w]);
	}
	for (auto r:{Range{0x72000,10},Range{0xffb20000,10},Range{0xa0004,2560},Range{0xa0000,0},Range{0xa0000,8706},Range{0x94000,33},Range{0x74000,12},Range{0x84000,257}}) {
		IO io; assert(!PioneerQ38PayloadStage(io,r.a,input.data(),r.words) && !io.views && !io.fences && !io.reads && !io.writes);
	}
	IO io; io.stable=false; assert(!PioneerQ38PayloadStage(io,0xa0000,input.data(),2560) && !io.views);
	assert(!PioneerQ38PayloadReadOutput(io,256,out.data()) && !io.views);
	printf("PASS OFFLINE Q38 bounded phase-fenced DATA transport:ALL valid inputs/scales/descriptor/output spans,each dropped input store/corrupt compare,32/64-bit exact coverage/canaries,code/control/unaligned/overflow/unstable mapping refused BEFORE view;NO hardware\n");
}
#endif
