/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdint>
#include <cstdio>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerRuntimeHostStage.h"
struct HostStage {
	uint32_t words[1024]={};
	unsigned fences=0,stores=0,loads=0,drop=~0U,corrupt=~0U;
	void RuntimeHostStageFence() {
		fences++;
		assert((fences==1 && !stores && !loads) || (fences==2 && stores==1024 && !loads)
			|| (fences==3 && stores==1024 && loads==1024));
	}
	void RuntimeHostStageStore(unsigned w,uint32_t value) { assert(fences==1 && w==stores++); if (w!=drop) words[w]=value; }
	uint32_t RuntimeHostStageLoad(unsigned w) { assert(fences==2 && w==loads++); return words[w]^(w==corrupt); }
};
int main() {
	uint32_t data[1024]; for (unsigned w=0;w<1024;w++) data[w]=0x90000001+w*13;
	for (unsigned fault=0;fault<1024;fault++) {
		HostStage missing; missing.drop=fault; assert(!PioneerRuntimeHostStageBlock(missing,data)); assert(missing.fences==3);
		HostStage bad; bad.corrupt=fault; assert(!PioneerRuntimeHostStageBlock(bad,data)); assert(bad.fences==3);
	}
	HostStage good; assert(PioneerRuntimeHostStageBlock(good,data) && good.fences==3);
	for (unsigned w=0;w<1024;w++) data[w]=~data[w];
	good.fences=good.loads=good.stores=0; assert(PioneerRuntimeHostStageBlock(good,data));
	HostStage empty; assert(!PioneerRuntimeHostStageBlock(empty,nullptr) && !empty.fences);
	puts("PASS ordinary private host staging: three exact phase fences,all1024 dropped stores/all1024 corrupt reads,changed pages,full scans; NO MMIO fence changes");
}
