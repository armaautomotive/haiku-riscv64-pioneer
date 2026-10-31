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
#include "../PioneerQ38ProjectionInput.h"
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
bool Q38Layer29CardQuantize(unsigned type,const float* input,int* lanes,float* scales){
 if(!lanes || !scales)return false;
 PioneerQ38ProjectionInput packed;
 if(!PioneerQ38ProjectionQuantize(type,input,256,&packed))return false;
 for(unsigned i=0;i<256;i++){
  const unsigned word=(packed.operand[i/2]>>((i%2)*16))&65535;
  if((word&0x7f80)!=0x4000)return false;
  lanes[i]=(word&0x8000)?-int(word&127):int(word&127);
 }
 for(unsigned i=0;i<(type==8?8U:1U);i++)scales[i]=PioneerAttentionFloat(packed.scale[i]);
 return true;
}
