/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RESIDUAL_RMS_H
#define PIONEER_Q38_RESIDUAL_RMS_H
#include "PioneerQ38LearnedRms.h"
// Device arithmetic,not an independent CPU validation oracle/capability.
// Local scratch may be partial on refusal;caller MUST NOT publish any
// GDDR output until the COMPLETE operation succeeds.
static __attribute__((noinline)) bool PioneerQ38ResidualRms(const float* projected,const float* raw,const float* weight,float* residual,float* normalized){
 using Address=__UINTPTR_TYPE__;
 const Address spans[]={reinterpret_cast<Address>(projected),reinterpret_cast<Address>(raw),reinterpret_cast<Address>(weight),
  reinterpret_cast<Address>(residual),reinterpret_cast<Address>(normalized)};
 for(unsigned i=0;i<5;i++){
  if(!spans[i] || (spans[i]&3) || spans[i]>~Address(0)-20480)return false;
  for(unsigned j=0;j<i;j++)if(spans[i]<spans[j]+20480 && spans[j]<spans[i]+20480)return false;
 }
 for(unsigned i=0;i<5120;i++){
  if(!PioneerQ38RmsDomain(projected[i]) || !PioneerQ38RmsDomain(raw[i]))return false;
  float sum=raw[i]+projected[i];if(!PioneerQ38RmsDomain(sum))return false;
  residual[i]=sum; // one rounded F32 addition,NO clipping/subnormal substitution
 }
 return PioneerQ38LearnedRms(residual,weight,normalized);
}
#endif
