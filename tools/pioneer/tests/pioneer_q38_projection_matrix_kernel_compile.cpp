/* SPDX-License-Identifier: MIT */
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionJobs.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionMatrixShape.h"
extern "C" bool pioneer_q38_projection_wave_data(const uint32_t* frame,const uint32_t* header,const uint32_t* banks,
 const uint32_t parts[119][8],const uint32_t* first){return PioneerQ38ProjectionMatrixShape(frame,header,banks,parts,first);}
extern "C" unsigned pioneer_q38_projection_job_data(const PioneerQ38ProjectionJobs& heapJobs,const PioneerQ38CanonicalModelPlan& heapPlan,
 unsigned layer,unsigned role,uint64_t token){
 return heapJobs.Matches(heapPlan,layer,role,token) && heapJobs.Count()==PioneerQ38ProjectionJobCount(heapJobs.Outputs())?heapJobs.Waves():0;
}
