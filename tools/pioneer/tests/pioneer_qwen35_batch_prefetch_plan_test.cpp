/* SPDX-License-Identifier: MIT */
#include <assert.h>
#include <initializer_list>
#include <stdio.h>
#include "../PioneerQwen35BatchFetch.h"
#include "../PioneerQwen35BatchPrefetchPlan.h"

int main()
{
	const unsigned shapes[][2]={{8,2560},{12,2560},{13,2560},{14,2560},
		{12,4096},{13,4096},{12,9216},{14,9216}};
	unsigned cases=0;
	for (const auto& shape:shapes) {
		const unsigned type=shape[0],inputs=shape[1];
		const unsigned batch=PioneerQwen35FetchBatch(type);
		const unsigned blocks=inputs>>(type==8?5:8);
		const unsigned tile=PioneerQwen35Projection::TileBytes(type);
		for (unsigned groups: {1U,7U,16U}) {
			for (unsigned group=0;group<groups;group++) {
				for (unsigned block=0;block<blocks;block++) {
					PioneerQwen35BatchFetchPlan p;
					assert(PioneerQwen35BatchFetch(type,inputs,groups,group,block,
						0x08000000,p));
					const unsigned first=block&~(batch-1),ordinal=first/batch;
					const unsigned buffer=PioneerQwen35BatchPrefetchBuffer(ordinal);
					assert(p.source==0x08000000+(group*blocks+first)*tile);
					assert(p.destination==0x98000);
					assert(buffer+(p.copySource-p.destination)==buffer+(block-first)*tile);
					assert(p.bytes==batch*tile && p.issue==((block&(batch-1))==0));
					assert(p.bytes<=8192 && buffer+p.bytes<=0x9c000);
					if (p.issue && first+batch<blocks) {
						assert(p.source+p.bytes==0x08000000+(group*blocks+first+batch)*tile);
						assert(PioneerQwen35BatchPrefetchBuffer(ordinal+1)
							==(buffer==0x98000?0x9a000:0x98000));
					}
					cases++;
				}
			}
		}
	}
	PioneerQwen35BatchFetchPlan p;
	p.source=1;p.destination=2;p.bytes=3;p.copySource=4;p.copyWords=5;p.tileBytes=6;p.issue=true;
	const auto unchanged=p;
	assert(!PioneerQwen35BatchFetch(8,4096,1,0,0,0x08000000,p));
	assert(!PioneerQwen35BatchFetch(13,9216,1,0,0,0x08000000,p));
	assert(!PioneerQwen35BatchFetch(12,2560,0,0,0,0x08000000,p));
	assert(!PioneerQwen35BatchFetch(12,2560,17,0,0,0x08000000,p));
	assert(!PioneerQwen35BatchFetch(12,2560,1,1,0,0x08000000,p));
	assert(!PioneerQwen35BatchFetch(12,2560,1,0,10,0x08000000,p));
	assert(!PioneerQwen35BatchFetch(12,2560,1,0,0,0x08000001,p));
	assert(!PioneerQwen35BatchFetch(12,2560,16,15,9,0xc3ff0000,p));
	assert(p.source==unchanged.source && p.destination==unchanged.destination
		&& p.bytes==unchanged.bytes && p.copySource==unchanged.copySource
		&& p.copyWords==unchanged.copyWords && p.tileBytes==unchanged.tileBytes
		&& p.issue==unchanged.issue);
	assert(PioneerQwen35BatchPrefetchBuffer(0)==0x98000
		&& PioneerQwen35BatchPrefetchBuffer(1)==0x9a000
		&& PioneerQwen35BatchPrefetchBuffer(2)==0x98000);
	printf("PASS %u Qwen35-4B double-buffered batch-prefetch plans; preserved original tile addresses, two-slot ordering, final batch, and refusal atomicity\n",cases);
	return 0;
}
