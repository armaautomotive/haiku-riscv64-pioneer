/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_RECURRENT_Q8_BFP8_PREPARER) || PIONEER_Q38_RECURRENT_Q8_BFP8_PREPARER != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_HYBRID_BFP8_PREPARER) || defined(PIONEER_Q38_DENSE_Q8_BFP8_PREPARER) \
 || defined(PIONEER_Q38_HYBRID_BFP8_WINDOW11) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(__FAST_MATH__)
#error Recurrent Q8 BFP8 selection is a separate CPU-only preparation role
#endif
#ifndef PIONEER_Q38_RECURRENT_BFP8_SELECTION_H
#define PIONEER_Q38_RECURRENT_BFP8_SELECTION_H
#include "PioneerQ38NativeBfp8WeightTiles.h"
// Selection only; the independently pinned writer authenticates the same
// original GGUF and applies its unchanged BFP8 recipe before publication.
struct PioneerQ38RecurrentBfp8Selection {
 static bool Match(const PioneerQ38Tensor& tensor){
  for(unsigned layer=0;layer<64;layer++)if(layer%4!=3){
   const unsigned roles[]={5,6,13};
   for(unsigned role:roles){
    PioneerQ38Tensor wanted;if(!PioneerQ38Expected(layer,role,wanted))return false;
    if(tensor.name==wanted.name)return tensor.type==8 && tensor.type==wanted.type && tensor.rank==wanted.rank
     && tensor.inputs==wanted.inputs && tensor.outputs==wanted.outputs && tensor.bytes==wanted.bytes;
   }
  }
  return false;
 }
};
#endif
