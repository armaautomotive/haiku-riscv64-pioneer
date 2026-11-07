/* SPDX-License-Identifier: MIT */
// Cross-compile probe for the same receipt predicate exercised by simulation.
// This is not a ROOT, native driver, or hardware image.
#define PIONEER_ATTENTION_FREESTANDING 1
#define PIONEER_DRAM_SWEEP_FREESTANDING 1
typedef __UINT32_TYPE__ uint32_t;
#include "../PioneerQ38HybridMatrixReceipt.h"
extern "C" bool q38_hybrid_receipt_probe(const uint32_t* p,const uint32_t* j,const uint32_t* b,
 const uint32_t* before,const uint32_t* after){
 return PioneerQ38HybridMatrixReceipt::Matrix(p,j,b,before,after);
}
