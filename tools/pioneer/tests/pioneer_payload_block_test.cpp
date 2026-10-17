/* SPDX-License-Identifier: MIT */
#include "../../../src/add-ons/kernel/drivers/misc/PioneerPayloadBlock.h"
#include <assert.h>
#include <stdio.h>
#include <initializer_list>
struct IO {
	uint32_t words[8192]={};
	unsigned fences=0,reads=0,writes=0,drop=0,corrupt=UINT32_MAX;
	void PayloadFence() { fences++; }
	uint32_t PayloadRead(unsigned w) { assert(w<8192 && fences); reads++; return words[w]^(w==corrupt?1:0); }
	void PayloadWrite(unsigned w,uint32_t value) { assert(w<8192 && fences==1); if (++writes!=drop) words[w]=value; }
};
struct WideIO:IO {
	unsigned pairs=0;
	uint64_t PayloadReadPair(unsigned w) { assert(!(w&1)); pairs++; uint64_t low=PayloadRead(w); return low|(uint64_t(PayloadRead(w+1))<<32); }
	void PayloadWritePair(unsigned w,uint32_t low,uint32_t high) { assert(!(w&1)); PayloadWrite(w,low); PayloadWrite(w+1,high); }
};
int main()
{
	uint32_t input[4096],output[4096]; for (unsigned w=0;w<4096;w++) input[w]=0xdead0000+w;
	for (unsigned n:{1U,7U,8U,9U,32U,1024U,2048U,3072U,4096U}) {
		IO io; assert(PioneerPayloadStageBlock(io,n,4096,input) && io.fences==3 && io.reads==n && io.writes==n);
		for (unsigned w=0;w<n;w++) io.words[4096+w]=input[w];
		io.fences=io.reads=io.writes=0;
		assert(PioneerPayloadReadBlock(io,4096,n,8192,output) && io.fences==2 && io.reads==n && !io.writes);
		for (unsigned w=0;w<n;w++) assert(output[w]==input[w]);
	}
	for (unsigned w=0;w<3072;w++) {
		IO io; io.drop=w+1; assert(!PioneerPayloadStageBlock(io,3072,3072,input) && io.reads==3072 && io.fences==3);
		IO changed; changed.corrupt=w; assert(!PioneerPayloadStageBlock(changed,3072,3072,input) && changed.reads==3072 && changed.fences==3);
	}
	for (unsigned first:{4096U,4097U}) for (unsigned n:{1U,2U,7U,8U,9U,2048U,3072U}) {
		WideIO io; assert(PioneerPayloadStageBlock(io,n,4096,input));
		for (unsigned w=0;w<n;w++) io.words[first+w]=input[w];
		io.fences=io.reads=io.writes=io.pairs=0;
		assert(PioneerPayloadReadBlock(io,first,n,8192,output) && io.reads==n && io.fences==2);
		for (unsigned w=0;w<n;w++) assert(output[w]==input[w]);
	}
	for (unsigned w=0;w<3072;w++) {
		WideIO io; io.drop=w+1; assert(!PioneerPayloadStageBlock(io,3072,3072,input) && io.reads==3072 && io.fences==3);
		WideIO changed; changed.corrupt=w; assert(!PioneerPayloadStageBlock(changed,3072,3072,input) && changed.reads==3072 && changed.fences==3);
	}
	IO io;
	assert(!PioneerPayloadReadBlock(io,8192,1,8192,output) && !PioneerPayloadReadBlock(io,UINT32_MAX,2,8192,output)
		&& !PioneerPayloadReadBlock(io,0,0,8192,output) && !PioneerPayloadReadBlock(io,0,1,8192,nullptr)
		&& !PioneerPayloadStageBlock(io,4097,4096,input) && !io.reads && !io.writes && !io.fences);
	printf("PASS: bounded32/64-bit SRAM-data blocks; all3072 dropped stores/corrupt readbacks in both widths, aligned pairs/short tails, full read coverage and phase fences; no controls/NoC/reset writes\n");
}
