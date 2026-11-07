/* SPDX-License-Identifier: MIT */
// Freestanding compilation check only; not a linked ROOT image or launcher.
#define PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION 1
#define PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT 1
#define PIONEER_ATTENTION_FREESTANDING
#define PIONEER_DRAM_SWEEP_FREESTANDING
typedef __UINT32_TYPE__ uint32_t;
#include "../PioneerQ38HybridHeadLayout.h"
extern "C" bool qgw8_head_shape(const uint32_t* h,const uint32_t* root,unsigned bank,unsigned workspace){
 return PioneerQ38HybridHeadLayout::Shape(h,root,bank,workspace);
}
extern "C" bool qgw8_head_job(const uint32_t* h,unsigned ordinal,unsigned previous,unsigned position,const uint32_t* boot,uint32_t* output){
 return PioneerQ38HybridHeadLayout::Job(h,ordinal,previous,position,boot,output);
}
