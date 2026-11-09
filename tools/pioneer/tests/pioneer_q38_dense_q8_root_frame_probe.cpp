/* SPDX-License-Identifier: MIT */
// Cross-build only: actual isolated frame builder, no device access.
#define PIONEER_ATTENTION_FREESTANDING 1
#define PIONEER_DRAM_SWEEP_FREESTANDING 1
#define PIONEER_Q38_DENSE_Q8_MATRIX_ROOT_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION 1
#define PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION 1
#define PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT 1
#include "../PioneerQ38DenseQ8MatrixRootFrame.h"
extern "C" unsigned q38_dense_q8_frame_probe(const uint32_t* tuple,const uint32_t* boot,unsigned layer,
 unsigned role,unsigned position,unsigned invocation,unsigned previous,uint32_t* output){
 return PioneerQ38DenseQ8MatrixRootFrame::Build(tuple,boot,layer,role,position,invocation,previous,output)?1:0;
}
