/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_MUTABLE_STATE
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38EmbeddingPlan.h"
extern "C" bool pioneer_embedding_build(PioneerQ38EmbeddingPlan& heap,const PioneerQ38CanonicalModelPlan& canonical,
 unsigned token,unsigned position,unsigned bank,const PioneerQ38GdnIdentity& id,const uint32_t* source,unsigned phase){
 return heap.Build(canonical,token,position,bank,id,source,phase);
}
extern "C" bool pioneer_embedding_match(const PioneerQ38EmbeddingPlan& heap,const PioneerQ38CanonicalModelPlan& canonical,
 unsigned token,unsigned position,unsigned bank,const PioneerQ38GdnIdentity& id,const uint32_t* source,unsigned phase){
 return heap.Matches(canonical,token,position,bank,id,source,phase);
}
extern "C" bool pioneer_embedding_learned(const PioneerQ38EmbeddingPlan& heap,const uint8_t* learned,unsigned pages){
 return heap.LearnedPagesPresent(learned,pages);
}
extern "C" bool pioneer_embedding_shape(const uint32_t* data){return PioneerQ38EmbeddingShape(data);}
extern "C" bool pioneer_embedding_summary(const uint32_t* summary,const uint32_t* data,const uint32_t* before,const uint32_t* after){
 return PioneerQ38EmbeddingSummary(summary,data,before,after);
}
