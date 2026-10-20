/* SPDX-License-Identifier: MIT */
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnFreshPlan.h"
extern "C" bool pioneer_q38_gdn_fresh_plan_compile(PioneerQ38GdnFreshPlan& heapPlan,const uint8_t* initialized,unsigned pages) {
	return heapPlan.Build(initialized,pages);
}
