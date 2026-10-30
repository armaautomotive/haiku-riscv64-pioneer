/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_PRECISE_ACTIVATION_REFERENCE_H
#define PIONEER_Q38_PRECISE_ACTIVATION_REFERENCE_H
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "../PioneerQ38PreciseActivationChunk.h"
#include "pioneer_q38_projection_input_reference.h"

// TEST control reproduces the unchanged ROOT arithmetic, NOT independent
// libm or full-model reference. Quantized packing is checked independently
// with native host division/rounding/binary16/byte packing as a second test.
static bool CurrentRootActivation(const float* gate,const float* up,float* out){
 for(unsigned i=0;i<256;i++){
  float sigmoid;
  if(!PioneerQ38RmsDomain(gate[i]) || !PioneerQ38RmsDomain(up[i])
   || !PioneerQ38SigmoidPrecision(gate[i],sigmoid))return false;
  out[i]=(gate[i]*sigmoid)*up[i];
  if(!PioneerQ38RmsDomain(out[i]))return false;
 }
 return true;
}
static void PreciseChunkInputs(unsigned trial,float* gate,float* up){
 uint32_t random=0x51414331U+trial;
 for(unsigned i=0;i<256;i++){
  float g=0,u=1;
  if(trial==0){g=PioneerAttentionFloat((i&1)<<31);u=i&2?-1.f:1.f;}
  else if(trial==1){g=float(int(i)-128)*.125f;u=float(int(i%17)-8)*.25f;}
  else if(trial==3){
   const uint32_t edges[]={0,0x80000000,0x26800000,0xa6800000,0x477fe000,0xc77fe000,
    0x41a00000,0xc1a00000,0xc2d00000,0xc2cfcccc,0x32800000,0xb2800000};
   const unsigned bits=edges[i%12];g=PioneerAttentionFloat(bits);u=(bits&0x7fffffff)==0x26800000?2.f:1.f;
  }else if(trial==4){g=-110.f;u=65504.f;}
  else if(trial==5){g=0;u=i&1?PioneerAttentionFloat(0x26800000):65504.f;}
  else if(trial==6 || trial==7){if(i==255){g=trial==6?65504.f:PioneerAttentionFloat(0x26800000);u=g;}}
  else if(trial==8){g=PioneerAttentionFloat(0x26800000);u=2.f;}
  else if(trial==9){g=20.f;u=(i&1?-1.f:1.f)*float(i%128)*.05f;}
  else{
   random=random*1664525U+1013904223U;g=float(int(random%4097)-2048)*.00390625f;
   random=random*1664525U+1013904223U;u=float(int(random%2049)-1024)*.00390625f;
  }
  gate[i]=g;up[i]=u;
 }
}
#endif
