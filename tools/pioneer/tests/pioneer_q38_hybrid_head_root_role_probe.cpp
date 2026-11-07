/* SPDX-License-Identifier: MIT */
// Freestanding ROOT selection/receipt compile probe only; no program image.
#define PIONEER_ATTENTION_FREESTANDING
#define PIONEER_DRAM_SWEEP_FREESTANDING
typedef __UINT32_TYPE__ uint32_t;
#include "../PioneerQ38HybridHeadLayout.h"
#include "../PioneerQ38HybridMatrixReceipt.h"
static_assert(PioneerQ38SelectedGlobalWorker::Magic==0x38574751U,"QGW8 worker selected");
extern "C" bool head_root_role_probe(const uint32_t* p,const uint32_t* j,const uint32_t* b,const uint32_t* before,const uint32_t* after){
 return PioneerQ38HybridMatrixReceipt::Matrix(p,j,b,before,after);
}
