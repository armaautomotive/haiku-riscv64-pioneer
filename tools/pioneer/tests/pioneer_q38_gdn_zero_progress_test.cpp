/* SPDX-License-Identifier: MIT */
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnZeroProgress.h"
#include <cassert>
#include <cstdio>
#include <memory>
int main()
{
	const PioneerQ38GdnIdentity id={1,2,3,4};
	auto progress=std::make_unique<PioneerQ38GdnZeroProgress>();
	unsigned count=UINT32_MAX;
	assert(!progress->Snapshot(id,count) && count==UINT32_MAX && !progress->Record(id,0,1));
	assert(progress->Bind(id) && !progress->Bind(id) && progress->Snapshot(id,count) && !count);
	for (unsigned first:{0U,31U,32U,255U,256U,73727U}) {
		assert(progress->Record(id,first,1) && progress->Record(id,first,1));
	}
	assert(progress->Snapshot(id,count) && count==6);
	for (unsigned field=0;field<4;field++) {
		auto wrong=id;
		if (!field) wrong.modelEpoch++; else if (field==1) wrong.sequence++;
		else if (field==2) wrong.session++; else wrong.driverGeneration++;
		assert(!progress->Record(wrong,0,256)); count=UINT32_MAX;
		assert(!progress->Snapshot(wrong,count) && count==UINT32_MAX);
	}
	for (auto range:{std::pair<unsigned,unsigned>{0,0},{0,257},{73728,1},{73727,2},{UINT32_MAX,1},{1,UINT32_MAX}})
		assert(!progress->Record(id,range.first,range.second));
	assert(progress->Snapshot(id,count) && count==6);
	// Independent coverage,all73728 pages in a permuted non-overlapping
	// traversal. Repeated/overlapping chunks never manufacture progress.
	for (unsigned ordinal=0;ordinal<288;ordinal++) {
		unsigned first=((ordinal*131)%288)*256;
		assert(progress->Record(id,first,256) && progress->Record(id,first,256));
	}
	assert(progress->Snapshot(id,count) && count==PioneerQ38GdnZeroProgress::Pages);
	progress->Fault(); count=UINT32_MAX;
	assert(!progress->Snapshot(id,count) && count==UINT32_MAX && !progress->Record(id,0,1) && !progress->Bind(id));
	progress=std::make_unique<PioneerQ38GdnZeroProgress>(); assert(progress->Bind(id));
	for (unsigned first=0;first<73727;) {
		unsigned pages=73727-first; if (pages>256) pages=256;
		assert(progress->Record(id,first,pages)); first+=pages;
	}
	assert(progress->Snapshot(id,count) && count==73727);
	assert(progress->Record(id,0,256) && progress->Snapshot(id,count) && count==73727);
	assert(progress->Record(id,73727,1) && progress->Snapshot(id,count) && count==73728);
	puts("PASS CPU GDN zero ledger:all73728 pages,permuted/overlapping/duplicate chunks,word/whole-owner boundaries,last-hole refusal,all4 foreign epochs,inert bounds,sticky fault;count ONLY,NO device/init/compute/read/model-ready authority");
}
