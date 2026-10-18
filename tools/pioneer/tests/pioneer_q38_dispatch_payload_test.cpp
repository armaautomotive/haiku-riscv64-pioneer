/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_PAYLOAD_MOCK_ONLY
#include "pioneer_q38_payload_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38DispatchRun.h"
int main()
{
	std::vector<uint32_t> input(8704),output(256); for (unsigned w=0;w<input.size();w++) input[w]=0x12345678^(w*0x713);
	struct Range { uint32_t a; unsigned words; };
	const Range spans[]={{0xa0000,2560},{0xa0000,3072},{0xa0000,8704},{0xab000,32},{0xab000,80},{0xab000,160},{0xab000,192},{0xab000,544},{0x74000,16},{0x74080,8},{0x74f40,8}};
	for (auto r:spans) {
		IO io; assert(PioneerQ38DispatchStage(io,r.a,input.data(),r.words,0) && io.writes==r.words && io.reads==r.words && io.fences==3);
		unsigned first=(r.a-0x6e000)/4;
		assert(io.data[first-1]==0xa5a5a5a5 && io.data[first+r.words]==0xa5a5a5a5);
		for (unsigned w=0;w<r.words;w++) {
			IO bad; bad.drop=w+1; assert(!PioneerQ38DispatchStage(bad,r.a,input.data(),r.words,0) && bad.reads==r.words);
		}
		if (r.a!=0xa0000 && r.a!=0xab000) continue;
		for (unsigned w=0;w<r.words;w++) {
			io.corrupt=first+w; io.reads=io.fences=0;
			assert(!PioneerQ38DispatchInputCompare(io,r.a,input.data(),r.words,0) && io.reads==r.words && io.fences==2);
		}
	}
	for (unsigned peer=0;peer<119;peer++) for (auto r:{Range{0x76000+peer*256,64},Range{0x80000+peer*1024,256}}) {
		IO io; unsigned first=(r.a-0x6e000)/4;
		for (unsigned w=0;w<r.words;w++) io.data[first+w]=input[w];
		assert(PioneerQ38DispatchRead(io,r.a,output.data(),r.words,0) && !io.writes && io.reads==r.words && io.fences==2);
		for (unsigned w=0;w<r.words;w++) assert(output[w]==input[w]);
	}
	for (auto r:{Range{0x72000,16},Range{0xffb20000,16},Range{0x74100,16},Range{0xa0004,2560},Range{0xab000,33},Range{0x74f60,8}}) {
		IO io; assert(!PioneerQ38DispatchStage(io,r.a,input.data(),r.words,0) && !io.views && !io.fences);
	}
	for (auto r:{Range{0x7d700,64},Range{0x9dc00,16},Range{0x80000,257},Range{0x76004,64}}) {
		IO io; assert(!PioneerQ38DispatchRead(io,r.a,output.data(),r.words,0) && !io.views && !io.fences);
	}
	IO io; io.stable=false;
	assert(!PioneerQ38DispatchStage(io,0xa0000,input.data(),2560,0) && !PioneerQ38DispatchInputCompare(io,0xa0000,input.data(),2560,0)
		&& !PioneerQ38DispatchRead(io,0x7f800,output.data(),64,0) && !io.views);
	puts("PASS OFFLINE54 paired payload:ALL exact input/AB-scale/header/task spans,EVERY dropped store/late corrupted input word detected,ALL119 proof/output slots,phase fences/full-bit comparisons/canaries,code/control/doorbell/overflow/unaligned/unstable mappings rejected BEFORE view;NO native qualification");
}
