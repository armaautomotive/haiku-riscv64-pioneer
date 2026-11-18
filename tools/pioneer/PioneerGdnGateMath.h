/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_GDN_GATE_MATH_H
#define PIONEER_GDN_GATE_MATH_H
#include "PioneerGraphMath.h"

static inline bool PioneerQ38PrepareNormal(float v){
 uint32_t a=PioneerAttentionBits(v)&0x7fffffff;
 return !a || (a>=0x00800000 && a<0x7f800000);
}
static inline float PioneerQ38PrepareFlush(float v){
 uint32_t bits=PioneerAttentionBits(v);
 return bits&0x7f800000?v:PioneerAttentionFloat(bits&0x80000000);
}
static inline bool PioneerQ38PrepareScalar(float v){
 return PioneerQ38PrepareNormal(v) && (PioneerAttentionBits(v)&0x7fffffff)<=0x477fe000;
}
static inline float PioneerQ38PrepareNegativeExp(float x){
 // GDN gates may multiply LARGE prior state. Do not use the old activation
 // cutoff at-80:exp(-80..-87) is still NORMAL and must not become zero.
 if(!(x<=0))return PioneerAttentionFloat(0x7fc00000);
 if(x> -80)return PioneerGraphExp(x);
 if(x<=-104)return 0; // below even binary32's subnormal rounding range
 float half=PioneerGraphExp(x*0.5f);
 return PioneerQ38PrepareFlush(half*half); // ONLY actual subnormals -> FTZ
}
static inline float PioneerQ38PrepareSoftplus(float x){
 // log(1+e),e=exp(-abs(x)) in[0,1]. z=e/(2+e)<=1/3;
 // 2*atanh(z)'s ten odd terms have truncation error<1.1e-11.
 // Stable for either sign;NO exp(large positive),log/fdiv/fsqrt/FMA.
 float e=PioneerQ38PrepareNegativeExp(-PioneerAttentionFloat(PioneerAttentionBits(x)&0x7fffffff));
 // Avoid an e/2 subnormal intermediate for tiny NORMAL exp results.
 // log1p(e)=e with relative error<1.2e-7 throughout this branch.
 if(e<2.384185791015625e-7f)return x>0?x+e:e;
 float z=PioneerGraphDividePositive(e,2.0f+e),zz=z*z,p=1.0f/19.0f;
 p=p*zz+1.0f/17.0f;p=p*zz+1.0f/15.0f;p=p*zz+1.0f/13.0f;
 p=p*zz+1.0f/11.0f;p=p*zz+1.0f/9.0f;p=p*zz+1.0f/7.0f;
 p=p*zz+1.0f/5.0f;p=p*zz+1.0f/3.0f;p=p*zz+1.0f;
 float logarithm=(2.0f*z)*p;
 return x>0?x+logarithm:logarithm;
}
#endif
