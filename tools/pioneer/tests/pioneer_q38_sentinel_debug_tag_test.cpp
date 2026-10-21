/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdio>
#include <initializer_list>
#define PIONEER_Q38_SENTINEL_REFERENCE_CLIENT
#include "../PioneerQ38SentinelDebugTag.h"
int main() {
	tt_pioneer_driver_reload good={};good.version=1;good.driver=55;good.guard_version=1;
	good.build_tag=
#ifdef PIONEER_Q38_Q4_DIRECT_REFERENCE_CLIENT
	UINT64_C(0x2026102100010027);
#elif defined(PIONEER_Q38_Q4_NIBBLE_REFERENCE_CLIENT)
	UINT64_C(0x2026102100010026);
#elif defined(PIONEER_Q38_PEER_PHASE_REFERENCE_CLIENT)
	UINT64_C(0x2026102100010025);
#elif defined(PIONEER_Q38_COMMAND_REFERENCE_CLIENT)
	UINT64_C(0x2026102100010024);
#elif defined(PIONEER_Q38_PREFETCH_REFERENCE_CLIENT)
	UINT64_C(0x2026102100010023);
#elif defined(PIONEER_Q38_RAW_CACHE_REFERENCE_CLIENT)
	UINT64_C(0x2026102100010022);
#elif defined(PIONEER_Q38_FRONTEND_REFERENCE_CLIENT)
	UINT64_C(0x2026102100010021);
#elif defined(PIONEER_Q38_BATCH_RAW_REFERENCE_CLIENT)
	UINT64_C(0x2026102100010020);
#elif defined(PIONEER_Q38_BATCH4_REFERENCE_CLIENT)
	UINT64_C(0x2026102100010019);
#elif defined(PIONEER_Q38_HOST_PHASE_REFERENCE_CLIENT)
	UINT64_C(0x2026102100010018);
#else
	UINT64_C(0x2026102100010017);
#endif
	good.generation=31;good.open_count=1;
	assert(PioneerQ38SentinelDebugTag(good));unsigned cases=0;
	for(unsigned bit=0;bit<64;bit++){auto r=good;r.build_tag^=UINT64_C(1)<<bit;assert(!PioneerQ38SentinelDebugTag(r));cases++;}
	for(unsigned field=0;field<7;field++)for(unsigned value:{0U,1U,2U,UINT32_MAX}) {
		auto r=good;uint32_t* targets[]={&r.version,&r.driver,&r.guard_version,&r.open_count,&r.quiesced,&r.cold,&r.active_owner};
		unsigned old=*targets[field];*targets[field]=value;
		assert(PioneerQ38SentinelDebugTag(r)==(value==old));cases++;
	}
	auto r=good;r.generation=0;assert(!PioneerQ38SentinelDebugTag(r));cases++;
	printf("PASS %u actual sentinel-root CPU tag/version/guard/generation/owner/open refusals;NO card/BEGIN/IO\n",cases);
}
