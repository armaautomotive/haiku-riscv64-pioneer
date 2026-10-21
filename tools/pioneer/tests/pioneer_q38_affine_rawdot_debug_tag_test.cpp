/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdio>
#include <initializer_list>
#define PIONEER_Q38_AFFINE_RAWDOT_REFERENCE_CLIENT
#include "../PioneerQ38AffineRawDotDebugTag.h"
int main() {
	tt_pioneer_driver_reload good={};good.version=1;good.driver=55;good.guard_version=1;
	good.build_tag=UINT64_C(0x2026102100010015);good.generation=28;good.open_count=1;
	assert(PioneerQ38AffineRawDotDebugTag(good));unsigned cases=0;
	for(unsigned bit=0;bit<64;bit++){auto r=good;r.build_tag^=UINT64_C(1)<<bit;assert(!PioneerQ38AffineRawDotDebugTag(r));cases++;}
	for(unsigned field=0;field<7;field++)for(unsigned value:{0U,1U,2U,UINT32_MAX}) {
		auto r=good;uint32_t* targets[]={&r.version,&r.driver,&r.guard_version,&r.open_count,&r.quiesced,&r.cold,&r.active_owner};
		unsigned old=*targets[field];*targets[field]=value;
		assert(PioneerQ38AffineRawDotDebugTag(r)==(value==old));cases++;
	}
	auto r=good;r.generation=0;assert(!PioneerQ38AffineRawDotDebugTag(r));cases++;
	printf("PASS %u actual affine/raw-dot CPU tag/version/guard/generation/owner/open refusals;NO card/BEGIN/IO\n",cases);
}
