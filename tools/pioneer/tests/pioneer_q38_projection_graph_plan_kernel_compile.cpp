/* SPDX-License-Identifier: MIT */
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionGraphPlan.h"
extern "C" bool pioneer_projection_parent_build(PioneerQ38ProjectionGraphPlan& heap,const PioneerQ38CanonicalModelPlan& plan,
 const PioneerQ38ProjectionJobs& jobs,unsigned layer,unsigned role,uint64_t token,unsigned bank,const PioneerQ38GdnIdentity& id,
 const uint32_t* root,const uint32_t sources[119][16],const uint32_t* sequence,unsigned phase,uint64_t input,bool stop){
 return heap.Build(plan,jobs,layer,role,token,bank,id,root,sources,sequence,phase,input,stop);
}
extern "C" bool pioneer_projection_parent_match(const PioneerQ38ProjectionGraphPlan& heap,const PioneerQ38CanonicalModelPlan& plan,
 const PioneerQ38ProjectionJobs& jobs,unsigned layer,unsigned role,uint64_t token,unsigned bank,const PioneerQ38GdnIdentity& id,
 const uint32_t* root,const uint32_t sources[119][16],const uint32_t* sequence,unsigned phase,uint64_t input,bool stop){
 return heap.Matches(plan,jobs,layer,role,token,bank,id,root,sources,sequence,phase,input,stop);
}
