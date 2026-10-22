/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_MUTABLE_STATE
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38LearnedRmsPlan.h"
extern "C" bool pioneer_rms_build(PioneerQ38LearnedRmsPlan& heap,const PioneerQ38CanonicalModelPlan& canonical,
 unsigned layer,unsigned role,unsigned position,unsigned bank,const PioneerQ38GdnIdentity& id,const uint32_t* source,unsigned phase){
 return heap.Build(canonical,layer,role,position,bank,id,source,phase);
}
extern "C" bool pioneer_rms_match(const PioneerQ38LearnedRmsPlan& heap,const PioneerQ38CanonicalModelPlan& canonical,
 unsigned layer,unsigned role,unsigned position,unsigned bank,const PioneerQ38GdnIdentity& id,const uint32_t* source,unsigned phase){
 return heap.Matches(canonical,layer,role,position,bank,id,source,phase);
}
extern "C" bool pioneer_rms_learned(const PioneerQ38LearnedRmsPlan& heap,const uint8_t* learned,unsigned pages){return heap.LearnedPagesPresent(learned,pages);}
extern "C" bool pioneer_rms_shape(const uint32_t* data){return PioneerQ38LearnedRmsShape(data);}
extern "C" bool pioneer_rms_summary(const uint32_t* s,const uint32_t* d,const uint32_t* before,const uint32_t* after){return PioneerQ38LearnedRmsSummary(s,d,before,after);}
