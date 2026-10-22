/* SPDX-License-Identifier: MIT */
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionJobs.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionGatherShape.h"
extern "C" bool pioneer_q38_projection_build(PioneerQ38ProjectionJobs& heapJobs,const PioneerQ38CanonicalModelPlan& heapPlan,
 unsigned layer,unsigned role,uint64_t token){return heapJobs.Build(heapPlan,layer,role,token);}
extern "C" bool pioneer_q38_projection_match(const PioneerQ38ProjectionJobs& heapJobs,const PioneerQ38CanonicalModelPlan& heapPlan,
 unsigned layer,unsigned role,uint64_t token,const uint8_t* learned,unsigned pages){
 return heapJobs.Matches(heapPlan,layer,role,token) && heapJobs.LearnedPagesPresent(learned,pages);
}
extern "C" bool pioneer_q38_projection_gather_data(const uint32_t* line,const uint32_t* descriptor,unsigned chunk,
 const uint32_t* summary,const uint32_t* before,const uint32_t* after){
 return PioneerQ38ProjectionGatherLine(line,descriptor,chunk)
  && PioneerQ38ProjectionGatherSummary(summary,descriptor,before,after);
}
