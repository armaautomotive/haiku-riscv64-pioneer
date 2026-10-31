/* SPDX-License-Identifier: MIT */
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(__FAST_MATH__)
#error HOST isolated card arithmetic diagnostic only; no CPU oracle or native roles
#endif
#define PIONEER_Q38_SILU_PRECISION_IMAGE 1
#define PIONEER_Q38_GDN_PREPARE_IMAGE 1
#define PIONEER_Q38_GDN_GATED_NORM_IMAGE 1
#define PIONEER_Q38_GDN_INPUT_COMBINED_STAGE 1
#include "../PioneerQ38ConvMath.h"
#include "../PioneerQ38GdnGatedNorm.h"
#include "pioneer_q38_layer29_math_bridge.h"
bool Q38Layer29CardConv(const float* input,const float* weights,float* output){
 float previous[48]={},next[48];return PioneerQ38Conv16(input,weights,previous,next,output);
}
bool Q38Layer29CardPrepare(const float* q,const float* k,float a,float b,float negativeA,float dt,float* outQ,float* outK,float* gates){
 return PioneerQ38PrepareHead(q,k,a,b,negativeA,dt,outQ,outK,gates);
}
bool Q38Layer29CardGated(const float* raw,const float* norm,const float* z,float* output){
 return PioneerQ38GdnGatedNormHalf(raw,norm,z,0,output) && PioneerQ38GdnGatedNormHalf(raw,norm,z,1,output+64);
}
