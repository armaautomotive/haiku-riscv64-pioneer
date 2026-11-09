/* SPDX-License-Identifier: MIT */
// A separately compiled offline root receipt predicate, not a ROOT program.
#include <stdint.h>
#ifdef PIONEER_Q38_TEST_OLD_ROOT_RECEIPT
#define PioneerQ38HybridMatrixReceipt PioneerQ38QgwaOldRootReceipt
#else
#define PioneerQ38HybridMatrixReceipt PioneerQ38QgwaBatchRootReceipt
#endif
#include "../PioneerQ38HybridMatrixReceipt.h"
#ifdef PIONEER_Q38_TEST_OLD_ROOT_RECEIPT
extern "C" bool qgwa_old_root_receipt(const uint32_t* p,const uint32_t* j,const uint32_t* b,
 const uint32_t* before,const uint32_t* after){
 return PioneerQ38QgwaOldRootReceipt::Matrix(p,j,b,before,after);
}
#else
extern "C" bool qgwa_batch_root_receipt(const uint32_t* p,const uint32_t* j,const uint32_t* b,
 const uint32_t* before,const uint32_t* after){
 return PioneerQ38QgwaBatchRootReceipt::Matrix(p,j,b,before,after);
}
#endif
