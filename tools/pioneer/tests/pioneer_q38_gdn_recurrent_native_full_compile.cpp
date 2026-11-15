/* SPDX-License-Identifier: MIT */
// Full ACTUAL SAME native init/CBO/QRJ1 source/lifetime/END,COMPILE ONLY48.
#define PIONEER_Q38_GDN_RECURRENT_NATIVE 1
#include "pioneer_q38_gdn_state_init_native_full_compile.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnRedistributedRecurrentPlan.h"
extern "C" bool pioneer_gdn_recurrent_native_run(PioneerStoreContext& c,PioneerQ38ResidentDecoderResult& r){return RunQ38ResidentGdnRecurrent(c,r);}
__attribute__((used)) static bool pioneer_gdn_redistributed_plan_compile_probe(
	const PioneerQ38CanonicalModelPlan& canonical,
	const PioneerQ38GdnBanks& state,
	const PioneerQ38GdnIdentity& identity,
	const uint32_t* source)
{
	PioneerQ38GdnRedistributedRecurrentPlan candidate;
	return candidate.Build(canonical,state,0,0,0,0,identity,source,1)
		&& candidate.Matches(canonical,state,0,0,0,0,identity,source,1);
}
