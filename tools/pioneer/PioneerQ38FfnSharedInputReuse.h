/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_FFN_SHARED_INPUT_REUSE_H
#define PIONEER_Q38_FFN_SHARED_INPUT_REUSE_H
#if !defined(PIONEER_Q38_FFN_SHARED_INPUT_REUSE_CANDIDATE) \
 || PIONEER_Q38_FFN_SHARED_INPUT_REUSE_CANDIDATE != 1 \
 || (!defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE) && !defined(PIONEER_Q38_GLOBAL_WORKER_IMAGE) \
  && !defined(PIONEER_Q38_PUSH_COMPLETION_WORKER_IMAGE))
#error FFN shared-input reuse is a distinct Q4 full-card component/worker candidate
#endif
#if defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_TOKEN_VALIDATION_NATIVE) || defined(PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_NATIVE) \
 || defined(PIONEER_Q38_GLOBAL_CHAIN_BF16_FPU) || defined(PIONEER_Q38_GLOBAL_CHAIN_HYBRID_BFP)
#error FFN shared-input reuse is the independent Q4 all-device mode, not CPU-validation or hybrid BF16
#endif
#ifndef PIONEER_ATTENTION_FREESTANDING
#include <stdint.h>
#endif
// Pure policy for the isolated full-card FFN gate->up input-transfer candidate.
// The caller must establish adjacency in the same recurrent request before
// setting sameRequest; this helper grants nothing from a caller-supplied job.
namespace PioneerQ38FfnSharedInputReuse {
enum {GateRole=2,UpRole=3,InputBytes=5120*2,ScaleBytes=80,PaddedScaleBytes=128};
// Preconditions: the caller has authenticated both frames as consecutive
// Q4 gate/up recipes in the same sealed recurrent request. Their recipe
// grammar fixes roles, K and type; this checks only the physical input alias.
static inline bool Pair(const volatile uint32_t* previous,const volatile uint32_t* current){
 return previous && current && previous[53] && current[53]==previous[53];
}
// Recurrent Matrix() takes its previous descriptor from RecipeCopies, not
// from its prior materialized frame. Word 53 is reserved zero in that
// recipe. The caller has just sealed both recipes and sequentially run gate
// immediately before up; both materialized frames bind the same fixed input.
static inline bool PairFromRecipe(const volatile uint32_t* gate,const volatile uint32_t* up,unsigned input){
 if(!gate || !up || !input || (input&3) || gate[53] || up[53]!=input
  || gate[43]!=GateRole || up[43]!=UpRole || gate[14]!=5120 || up[14]!=5120
  || gate[15]!=12 || up[15]!=12 || !gate[6] || gate[6]>=0x7ffffffe || up[6]!=gate[6]+1)return false;
 for(unsigned i=40;i<=42;i++)if(gate[i]!=up[i])return false;
 if(gate[3]!=up[3] || gate[4]!=up[4] || gate[8]!=up[8] || gate[9]!=up[9])return false;
 for(unsigned i=16;i<40;i++)if(gate[i]!=up[i])return false;
 return true;
}
static inline unsigned InputReads(bool reuse,unsigned inputs){return reuse?0U:(inputs*2+16383)/16384+1;}
static inline unsigned InputFlits(bool reuse,unsigned inputs,unsigned blocks){
 return reuse?0U:(inputs*2+((blocks*4+63)&~63U))/64;
}
static_assert(InputBytes==10240 && ScaleBytes==80 && PaddedScaleBytes==128,"Qwen38 quantized hidden activation and padded Q4 scales");
}
#endif
