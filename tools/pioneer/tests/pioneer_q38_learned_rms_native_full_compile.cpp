/* SPDX-License-Identifier: MIT */
// ACTUAL complete native driver + embedding wrapper,OBJECT ONLY.
#define PIONEER_Q38_LEARNED_RMS_NATIVE 1
#include "pioneer_q38_embedding_native_full_compile.cpp"
extern "C" bool pioneer_rms_native_kernel_run(PioneerStoreContext& c,PioneerQ38LearnedRmsResult& result){
 return RunQ38LearnedRmsModel(c,result);
}
