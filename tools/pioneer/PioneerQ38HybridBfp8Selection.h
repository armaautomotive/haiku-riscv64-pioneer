/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_HYBRID_BFP8_SELECTION_H
#define PIONEER_Q38_HYBRID_BFP8_SELECTION_H
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Hybrid BFP8 selection is CPU preparation only
#endif
#include "PioneerQ38NativeBfp8WeightTiles.h"
// Select the exact BFP8 FFN tensors of the chosen hybrid proposal. Numerical
// conversion remains the existing frozen all-BFP8 writer/codec, not a new
// recipe. This is selection data, not an uploader or model-quality claim.
struct PioneerQ38HybridBfp8Selection {
 static bool MatchFrom(const PioneerQ38Tensor& tensor,unsigned first){
  if(first!=48 && first!=50)return false;
  for(unsigned layer=first;layer<=60;layer++)for(unsigned role=2;role<=4;role++){
   PioneerQ38Tensor wanted;if(!PioneerQ38Expected(layer,role,wanted))return false;
   if(tensor.name==wanted.name)return tensor.type==wanted.type && tensor.rank==wanted.rank
    && tensor.inputs==wanted.inputs && tensor.outputs==wanted.outputs && tensor.bytes==wanted.bytes;
  }
  return false;
 }
 static bool Match(const PioneerQ38Tensor& tensor){return MatchFrom(tensor,48);}
 static bool MatchQgw8(const PioneerQ38Tensor& tensor){return MatchFrom(tensor,50);}
 static bool MatchDenseQ8(const PioneerQ38Tensor& tensor){
  for(unsigned layer=3;layer<64;layer+=4)for(unsigned role=5;role<=7;role++){
   PioneerQ38Tensor wanted;if(!PioneerQ38Expected(layer,role,wanted))return false;
   if(tensor.name==wanted.name)return tensor.type==8 && tensor.type==wanted.type && tensor.rank==wanted.rank
    && tensor.inputs==wanted.inputs && tensor.outputs==wanted.outputs && tensor.bytes==wanted.bytes;
  }
  return false;
 }
};
#endif
