/* SPDX-License-Identifier: MIT */
#include "PioneerQ38RecurrentKernelCompileRole.h"
#if !defined(PIONEER_Q38_SMALL_RECURRENT_BFP8_PLAN_INSPECTION) || PIONEER_Q38_SMALL_RECURRENT_BFP8_PLAN_INSPECTION != 1 \
 || ((defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT)) && !defined(PIONEER_Q38_RECURRENT_KERNEL_COMPILE_ONLY)) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Small recurrent fixed layout is isolated inspection only
#endif
#ifndef PIONEER_Q38_SMALL_RECURRENT_KERNEL_LAYOUT_INSPECTION_H
#define PIONEER_Q38_SMALL_RECURRENT_KERNEL_LAYOUT_INSPECTION_H
#include "PioneerQ38RecurrentQ8KernelLayoutInspection.h"

// Fixed-size independent reconstruction, not a native upload grant. The
// parent is independently reconstructed from canonical metadata first.
class PioneerQ38SmallRecurrentKernelLayoutInspection {
public:
 using Parent=PioneerQ38RecurrentQ8KernelLayoutInspection;using Part=Parent::Part;
 enum : uint32_t {SmallRecurrentBfp8=12,Bytes=348160};
private:
 Parent rebuilt;Part parts[6144]={};uint64_t used[7]={};unsigned count=0;bool attempted=false,ready=false;
public:
 PioneerQ38SmallRecurrentKernelLayoutInspection()=default;
 PioneerQ38SmallRecurrentKernelLayoutInspection(const PioneerQ38SmallRecurrentKernelLayoutInspection&)=delete;
 PioneerQ38SmallRecurrentKernelLayoutInspection& operator=(const PioneerQ38SmallRecurrentKernelLayoutInspection&)=delete;
 bool Build(const PioneerQ38CanonicalModelPlan& source,const Parent& parent,const Parent::Parent& dense,const Parent::Parent::Parent& ancestor){
  if(attempted)return false;attempted=true;
  if(!rebuilt.Build(source,dense,ancestor) || !parent.Ready() || parent.Count()!=source.Count()
   || rebuilt.Count()!=source.Count())return false;
  uint64_t next[7];for(unsigned b=0;b<7;b++){
   if(parent.Used(b)!=rebuilt.Used(b) || (parent.Used(b)&4095) || parent.Used(b)>source.Capacity)return false;
   next[b]=parent.Used(b);
  }
  bool seen[64][2]={};unsigned converted=0;
  for(unsigned i=0;i<source.Count();i++){
   const auto* p=source.Part(i);const auto* a=parent.At(i);const auto* b=rebuilt.At(i);
   if(!p || !a || !b || a->original!=b->original || a->bank!=b->bank || a->offset!=b->offset
    || a->bytes!=b->bytes || a->allocated!=b->allocated || a->encoding!=b->encoding || a->bank>=7)return false;
   Part result=*a;
   if(p->kind==source.WeightKind && p->layer<64 && p->layer%4!=3 && (p->role==10 || p->role==11)){
    const unsigned role=p->role-10,bank=p->bank;
    if(bank>=7 || a->bank!=bank || seen[p->layer][role] || p->type!=8 || p->firstRow || p->rows!=48
     || p->rowBytes!=5760 || p->bytes!=48*5760 || a->encoding!=0 || a->bytes!=p->bytes
     || next[bank]>source.Capacity || Bytes>source.Capacity-next[bank] || next[bank]>UINT32_MAX-Bytes)return false;
    result.offset=uint32_t(next[bank]);result.bytes=Bytes;result.allocated=Bytes;result.encoding=SmallRecurrentBfp8;
    next[bank]+=Bytes;seen[p->layer][role]=true;converted++;
   }
   parts[count++]=result;
  }
  if(converted!=96 || count!=4678)return false;
  for(unsigned l=0;l<64;l++)if(l%4!=3 && (!seen[l][0] || !seen[l][1]))return false;
  for(unsigned b=0;b<7;b++)used[b]=next[b];ready=true;return true;
 }
 bool Ready()const{return ready;}
 unsigned Count()const{return ready?count:0;}
 const Part* At(unsigned i)const{return ready && i<count?&parts[i]:nullptr;}
 uint64_t Used(unsigned b)const{return ready && b<7?used[b]:0;}
};
static_assert(sizeof(PioneerQ38SmallRecurrentKernelLayoutInspection)>200000,"fixed reconstruction is HEAP ONLY");
#endif
