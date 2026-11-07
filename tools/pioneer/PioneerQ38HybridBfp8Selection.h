/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_HYBRID_BFP8_SELECTION_H
#define PIONEER_Q38_HYBRID_BFP8_SELECTION_H
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Hybrid BFP8 selection is CPU preparation only
#endif
#include "PioneerQ38NativeBfp8WeightTiles.h"
// Select only the exact 39 BFP8 FFN tensors of the proposed hybrid. Numerical
// conversion remains the existing frozen all-BFP8 writer/codec, not a new
// recipe. This is selection data, not an uploader or model-quality claim.
struct PioneerQ38HybridBfp8Selection {
 static bool Match(const PioneerQ38Tensor& tensor){
  for(unsigned layer=48;layer<=60;layer++)for(unsigned role=2;role<=4;role++){
   PioneerQ38Tensor wanted;if(!PioneerQ38Expected(layer,role,wanted))return false;
   if(tensor.name==wanted.name)return tensor.type==wanted.type && tensor.rank==wanted.rank
    && tensor.inputs==wanted.inputs && tensor.outputs==wanted.outputs && tensor.bytes==wanted.bytes;
  }
  return false;
 }
};
#endif
