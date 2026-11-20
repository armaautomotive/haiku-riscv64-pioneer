/* SPDX-License-Identifier: MIT */
#include <assert.h>
#include <initializer_list>
#include <stdio.h>
#include "../PioneerQwen35TilePrefetchPlan.h"

int main()
{
	const unsigned shapes[][2]={{8,2560},{12,2560},{13,2560},{14,2560},
		{12,4096},{13,4096},{12,9216},{14,9216}};
	unsigned cases=0;
	for (const auto& shape:shapes) {
		const unsigned type=shape[0],inputs=shape[1];
		const unsigned blocks=inputs>>(type==8?5:8);
		const unsigned bytes=PioneerQwen35Projection::TileBytes(type);
		for (unsigned groups: {1U,7U,16U}) {
			for (unsigned group=0;group<groups;group++) {
				for (unsigned block=0;block<blocks;block++) {
					PioneerQ35TilePrefetchPlan p;
					assert(PioneerQ35TilePrefetchPlan::Describe(type,inputs,groups,group,block,
						0x08000000,p));
					assert(p.currentSource==0x08000000+(group*blocks+block)*bytes);
					assert(p.currentRaw==((block&1)?0x98000:0x90000));
					assert(p.bytes==bytes && p.next==(block+1<blocks));
					if (p.next) {
						assert(p.nextSource==p.currentSource+bytes);
						assert(p.nextRaw==((block&1)?0x90000:0x98000));
					} else {
						assert(!p.nextSource && !p.nextRaw);
					}
					cases++;
				}
			}
		}
	}
	PioneerQ35TilePrefetchPlan p;
	p.currentSource=1;p.currentRaw=2;p.nextSource=3;p.nextRaw=4;p.bytes=5;p.next=true;
	const auto unchanged=p;
	assert(!PioneerQ35TilePrefetchPlan::Describe(8,4096,1,0,0,0x08000000,p));
	assert(p.currentSource==unchanged.currentSource && p.currentRaw==unchanged.currentRaw
		&& p.nextSource==unchanged.nextSource && p.nextRaw==unchanged.nextRaw
		&& p.bytes==unchanged.bytes && p.next==unchanged.next);
	assert(!PioneerQ35TilePrefetchPlan::Describe(13,9216,1,0,0,0x08000000,p));
	assert(!PioneerQ35TilePrefetchPlan::Describe(12,2560,0,0,0,0x08000000,p));
	assert(!PioneerQ35TilePrefetchPlan::Describe(12,2560,17,0,0,0x08000000,p));
	assert(!PioneerQ35TilePrefetchPlan::Describe(12,2560,1,1,0,0x08000000,p));
	assert(!PioneerQ35TilePrefetchPlan::Describe(12,2560,1,0,10,0x08000000,p));
	assert(!PioneerQ35TilePrefetchPlan::Describe(12,2560,1,0,0,0x08000001,p));
	assert(!PioneerQ35TilePrefetchPlan::Describe(12,2560,16,15,9,0xc3ff0000,p));
	printf("PASS %u Qwen35-4B checked tile-prefetch plans across every supported input/type, exact alternating-buffer/last-block boundaries and refusal atomicity\n",cases);
	return 0;
}
