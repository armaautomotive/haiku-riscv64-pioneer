/* SPDX-License-Identifier: MIT */
// Separate preparation-only alpha/beta candidate. It does not authorize the
// existing 144-tensor cache, a driver, device execution or Work mode.
#if !defined(PIONEER_Q38_RECURRENT_SMALL_BFP8_PREPARER) || PIONEER_Q38_RECURRENT_SMALL_BFP8_PREPARER != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_RECURRENT_Q8_BFP8_PREPARER) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(__FAST_MATH__)
#error Recurrent alpha/beta BFP8 selection is separate preparation-only
#endif
#ifndef PIONEER_Q38_RECURRENT_SMALL_BFP8_SELECTION_H
#define PIONEER_Q38_RECURRENT_SMALL_BFP8_SELECTION_H
#include "PioneerQ38NativeBfp8WeightTiles.h"
struct PioneerQ38RecurrentSmallBfp8Selection {
 static bool Match(const PioneerQ38Tensor& tensor){
  for(unsigned layer=0;layer<64;layer++)if(layer%4!=3)for(unsigned role:{10U,11U}){
   PioneerQ38Tensor expected;if(!PioneerQ38Expected(layer,role,expected))return false;
   if(tensor.name==expected.name)return tensor.type==8 && tensor.type==expected.type && tensor.rank==expected.rank
    && tensor.inputs==expected.inputs && tensor.outputs==expected.outputs && tensor.bytes==expected.bytes;
  }
  return false;
 }
};
#endif
