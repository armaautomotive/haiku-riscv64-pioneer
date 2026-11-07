/* SPDX-License-Identifier: MIT */
#include "../../../../../tools/pioneer/PioneerQ38Qgw8NativeInspectionRole.h"
#ifndef PIONEER_Q38_QGW8_KERNEL_LAYOUT_H
#define PIONEER_Q38_QGW8_KERNEL_LAYOUT_H
#if !defined(PIONEER_Q38_QGW8_LAYOUT_INSPECTION) || PIONEER_Q38_QGW8_LAYOUT_INSPECTION != 1 \
 || (defined(_KERNEL_MODE) && !defined(PIONEER_Q38_QGW8_NATIVE_INSPECTION) \
  && (!defined(PIONEER_Q38_HYBRID_KERNEL_COMPILE_ONLY) || PIONEER_Q38_HYBRID_KERNEL_COMPILE_ONLY != 1)) \
 || ((defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT)) && !defined(PIONEER_Q38_QGW8_NATIVE_INSPECTION)) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) \
 || defined(PIONEER_Q38_BF16_FULL_NATIVE) || defined(PIONEER_Q38_BF16_FULL_CLIENT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error QGW8 kernel layout is non-loadable inspection data only
#endif
#include "PioneerQ38CanonicalModelPlan.h"
#if !defined(PIONEER_Q38_QGW8_CANONICAL_PRESENT) || PIONEER_Q38_QGW8_CANONICAL_PRESENT != 1
#error QGW8 inspection role must be selected before the canonical model header
#endif

// Freestanding fixed-size QGW8 geometry for a future kernel receipt. Unlike
// the legacy canonical layout this records an explicit encoding for every
// part, including the BFP8 head. No source authentication, initialized
// ledger, bank ownership, device IO, worker selection or launch authority.
// Heap only. Each object builds once and refuses an incomplete source.
class PioneerQ38Qgw8KernelLayout {
public:
 enum : uint32_t {Original=0,Bfp4=4,Bfp8=8,HeadBfp8=9};
 struct Part {uint32_t original,bank,offset,bytes,allocated,encoding;};
private:
 Part parts[6144]={};uint64_t used[7]={};unsigned count=0;bool attempted=false,ready=false;
public:
 PioneerQ38Qgw8KernelLayout()=default;
 PioneerQ38Qgw8KernelLayout(const PioneerQ38Qgw8KernelLayout&)=delete;
 PioneerQ38Qgw8KernelLayout& operator=(const PioneerQ38Qgw8KernelLayout&)=delete;
 bool Build(const PioneerQ38CanonicalModelPlan& source){
  if(attempted)return false;attempted=true;
  if(!source.OriginalFormatLayout() || source.Count()!=4678 || source.RecurrentCount()!=672)return false;
  uint64_t old[7]={};unsigned four=0,eight=0,heads=0,rows=0,banks=0;
  for(unsigned i=0;i<source.Count();i++){
   const auto* p=source.Part(i);
   if(!p || i>=6144 || p->bank>=7 || !p->bytes || p->bytes>p->allocated || (p->offset&4095)
    || (p->allocated&4095) || p->allocated-p->bytes>=4096 || p->offset!=old[p->bank]
    || old[p->bank]>source.Capacity || p->allocated>source.Capacity-old[p->bank])return false;
   old[p->bank]+=p->allocated;uint64_t bytes=p->bytes,allocated=p->allocated;uint32_t encoding=Original;
   if(p->kind==source.WeightKind && p->layer<64 && p->role>=2 && p->role<=4){
    const uint64_t width=p->role==4?17408:5120;
    if(p->type!=12 || !p->rows || ((p->firstRow|p->rows)&31) || p->rowBytes!=width*9/16
     || uint64_t(p->rows)*p->rowBytes!=p->bytes)return false;
    if(p->layer>=50 && p->layer<=60){
     bytes=uint64_t(p->rows)*width*17/16;allocated=(bytes+4095)&~uint64_t(4095);encoding=Bfp8;eight++;
    }else{encoding=Bfp4;four++;}
   }else if(p->kind==source.WeightKind && p->layer==64 && p->role==1){
    if(p->type!=14 || p->firstRow!=rows || !p->rows || ((p->firstRow|p->rows)&63)
     || p->rowBytes!=4240 || uint64_t(p->rows)*4240!=p->bytes || (banks&(1U<<p->bank)))return false;
    bytes=uint64_t(p->rows)*5440;allocated=(bytes+4095)&~uint64_t(4095);encoding=HeadBfp8;
    rows+=p->rows;banks|=1U<<p->bank;heads++;
   }
   if(!bytes || bytes>UINT32_MAX || allocated>UINT32_MAX || used[p->bank]>source.Capacity
    || allocated>source.Capacity-used[p->bank])return false;
   parts[i]={i,p->bank,uint32_t(used[p->bank]),uint32_t(bytes),uint32_t(allocated),encoding};
   used[p->bank]+=allocated;count++;
  }
  if(count!=4678 || four!=1113 || eight!=231 || heads!=7 || rows!=248320 || banks!=127)return false;
  for(unsigned b=0;b<7;b++)if(old[b]!=source.Used(b))return false;
  ready=true;return true;
 }
 bool Ready()const{return ready;}
 unsigned Count()const{return ready?count:0;}
 const Part* At(unsigned i)const{return ready && i<count?&parts[i]:nullptr;}
 uint64_t Used(unsigned bank)const{return ready && bank<7?used[bank]:0;}
};
static_assert(sizeof(PioneerQ38Qgw8KernelLayout)>131072 && sizeof(PioneerQ38Qgw8KernelLayout)<262144,
 "QGW8 kernel layout is heap-only");
#endif
