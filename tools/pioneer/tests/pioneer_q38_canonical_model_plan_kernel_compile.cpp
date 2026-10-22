/* SPDX-License-Identifier: MIT */
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38GraphMutableBanks.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38FfnCanonicalJobs.h"
extern "C" bool pioneer_q38_canonical_jobs_build(PioneerQ38FfnCanonicalJobs& heapJobs,const PioneerQ38CanonicalModelPlan& heapPlan,unsigned layer,uint64_t token) {
 return heapJobs.Build(heapPlan,layer,token);
}
extern "C" bool pioneer_q38_canonical_plan_build(PioneerQ38CanonicalModelPlan& heapPlan) {return heapPlan.Build();}
extern "C" bool pioneer_q38_canonical_plan_pages(const PioneerQ38CanonicalModelPlan& heapPlan,unsigned layer,const uint8_t* learned,unsigned pages) {
 return heapPlan.FfnPagesPresent(layer,learned,pages);
}
extern "C" bool pioneer_q38_canonical_plan_match(const PioneerQ38GraphMutableBanks& heapOwner,const PioneerQ38GdnIdentity& identity,
 const PioneerQ38FfnCanonicalJobs& heapJobs,unsigned layer,uint64_t token,const uint8_t* learned,unsigned pages) {
 return heapOwner.MatchInitializedFfn(identity,heapJobs,layer,token,learned,pages);
}
extern "C" bool pioneer_q38_canonical_plan_bind(PioneerQ38GraphMutableBanks& heapOwner,PioneerResidentBanks& banks,
 const tt_dram_samples_result& proof,const PioneerQ38GdnIdentity& identity,const uint8_t* learned,unsigned pages,
 const PioneerQ38CanonicalModelPlan& heapPlan,unsigned bank) {
 return heapOwner.BindCanonical(banks,proof,identity,learned,pages,heapPlan,bank);
}
