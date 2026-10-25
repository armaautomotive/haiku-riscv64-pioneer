/* SPDX-License-Identifier: MIT */
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentOutputPlan.h"
extern "C" bool pioneer_q38_resident_output_build_data(PioneerQ38ResidentOutputPlan& heap,const PioneerQ38CanonicalModelPlan& plan,
 unsigned layer,unsigned position,unsigned bank,unsigned slot,unsigned token,unsigned phase,const PioneerQ38GdnIdentity& id,const uint32_t* sources,const uint32_t* sequence){
 return heap.Build(plan,layer,position,bank,slot,token,phase,id,sources,sequence);
}
extern "C" bool pioneer_q38_resident_output_match_data(const PioneerQ38ResidentOutputPlan& heap,const PioneerQ38CanonicalModelPlan& plan,
 unsigned layer,unsigned position,unsigned bank,unsigned slot,unsigned token,unsigned phase,const PioneerQ38GdnIdentity& id,const uint32_t* sources,const uint32_t* sequence,
 const uint8_t* pages,unsigned count){
 return heap.Matches(plan,layer,position,bank,slot,token,phase,id,sources,sequence) && heap.LearnedPagesPresent(pages,count);
}
