/* SPDX-License-Identifier: MIT */
// Real RISC-V kernel ABI DATA compile only;no native role/route/IO factory.
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38NextProjectionGraphPlan.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38NextProjectionGraphProof.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38NextProjectionGraphImages.h"
extern "C" bool pioneer_next_projection_build(PioneerQ38NextProjectionGraphPlan& heap,const PioneerQ38CanonicalModelPlan& plan,
 const PioneerQ38ProjectionJobs& jobs,unsigned layer,unsigned role,uint64_t token,unsigned bank,const PioneerQ38GdnIdentity& id,
 const uint32_t* root,const uint32_t sources[119][16],const uint32_t* sequence,unsigned phase,uint64_t input,bool stop){
 return heap.Build(plan,jobs,layer,role,token,bank,id,root,sources,sequence,phase,input,stop);
}
extern "C" bool pioneer_next_projection_match(const PioneerQ38NextProjectionGraphPlan& heap,const PioneerQ38CanonicalModelPlan& plan,
 const PioneerQ38ProjectionJobs& jobs,unsigned layer,unsigned role,uint64_t token,unsigned bank,const PioneerQ38GdnIdentity& id,
 const uint32_t* root,const uint32_t sources[119][16],const uint32_t* sequence,unsigned phase,uint64_t input,bool stop){
 return heap.Matches(plan,jobs,layer,role,token,bank,id,root,sources,sequence,phase,input,stop);
}
extern "C" bool pioneer_next_projection_receipt(const uint32_t* summary,const uint32_t* manifest,const uint32_t* before,const uint32_t* after){
 return PioneerQ38NextProjectionGraphReceipt(summary,manifest,before,after);
}
extern "C" uint32_t pioneer_next_projection_word(unsigned worker,uint32_t address){
 return PioneerQ38NextProjectionGraphProgramWord(worker,address);
}
