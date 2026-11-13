/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RECURRENT_Q8_KERNEL_LAYOUT_INSPECTION_H
#define PIONEER_Q38_RECURRENT_Q8_KERNEL_LAYOUT_INSPECTION_H
#include "PioneerQ38RecurrentKernelCompileRole.h"
#if !defined(PIONEER_Q38_RECURRENT_Q8_MATRIX_PLAN_INSPECTION) || PIONEER_Q38_RECURRENT_Q8_MATRIX_PLAN_INSPECTION != 1 \
 || ((defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT)) && !defined(PIONEER_Q38_RECURRENT_KERNEL_COMPILE_ONLY)) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Recurrent Q8 fixed layout is isolated inspection only
#endif
#include "PioneerQ38DenseQ8KernelLayoutInspection.h"
// Independent fixed-size reconstruction; no native receipt or ownership.
class PioneerQ38RecurrentQ8KernelLayoutInspection {
public:
 using Parent=PioneerQ38DenseQ8KernelLayoutInspection;using Part=Parent::Part;
 enum : uint32_t {RecurrentQ8Bfp8=11};
private:
 Part parts[6144]={};uint64_t used[7]={};unsigned count=0;bool attempted=false,ready=false;
public:
 PioneerQ38RecurrentQ8KernelLayoutInspection()=default;
 PioneerQ38RecurrentQ8KernelLayoutInspection(const PioneerQ38RecurrentQ8KernelLayoutInspection&)=delete;
 PioneerQ38RecurrentQ8KernelLayoutInspection& operator=(const PioneerQ38RecurrentQ8KernelLayoutInspection&)=delete;
 bool Build(const PioneerQ38CanonicalModelPlan& source,const Parent& parent,const Parent::Parent& ancestorLayout){
  if(attempted)return false;attempted=true;
  if(!source.OriginalFormatLayout() || source.Count()!=4678 || !parent.Ready() || parent.Count()!=4678
   || !ancestorLayout.Ready() || ancestorLayout.Count()!=4678)return false;
  uint64_t next[7]={};unsigned converted=0,rows[64][3]={};
  for(unsigned i=0;i<source.Count();i++){
   const auto* original=source.Part(i);const auto* prior=parent.At(i);
   if(!original || !prior || i>=6144 || prior->original!=i || prior->bank>=7 || prior->bank!=original->bank
    || prior->offset!=next[prior->bank] || !prior->bytes || prior->bytes>prior->allocated
    || (prior->offset&4095) || (prior->allocated&4095) || prior->offset>source.Capacity
    || prior->allocated>source.Capacity-prior->offset)return false;
   // Preserve the ancestor's full reservation, not its shorter live payload.
   const auto* ancestor=ancestorLayout.At(i);
   if(!ancestor || ancestor->offset!=prior->offset || ancestor->bank!=prior->bank
    || ancestor->allocated<prior->allocated || ancestor->allocated>source.Capacity-prior->offset
    || (ancestor->allocated&4095))return false;
   next[prior->bank]+=ancestor->allocated;Part result=*prior;
   if(original->kind==source.WeightKind && original->layer<64 && original->layer%4!=3
    && (original->role==5 || original->role==6 || original->role==13)){
    const unsigned slot=original->role==5?0:original->role==6?1:2;
    const unsigned limit=slot==0?6144:slot==1?10240:5120,oldBytes=slot==2?6912:5760,newBytes=slot==2?6528:5440;
    if(original->type!=8 || original->rowBytes!=oldBytes || !original->rows || ((original->firstRow|original->rows)&31)
     || original->firstRow!=rows[original->layer][slot] || original->firstRow>=limit || original->rows>limit-original->firstRow
     || uint64_t(original->rows)*oldBytes!=original->bytes || prior->encoding!=Parent::Original
     || prior->bytes!=original->bytes || prior->allocated!=original->allocated)return false;
    const uint64_t bytes=uint64_t(original->rows)*newBytes,allocated=(bytes+4095)&~uint64_t(4095);
    if(!bytes || bytes>UINT32_MAX || allocated>prior->allocated)return false;
    result.bytes=uint32_t(bytes);result.allocated=uint32_t(allocated);result.encoding=RecurrentQ8Bfp8;
    rows[original->layer][slot]+=original->rows;converted++;
   }
   parts[count++]=result;
  }
  if(count!=4678 || converted!=1008)return false;
  for(unsigned l=0;l<64;l++)if(l%4!=3 && (rows[l][0]!=6144 || rows[l][1]!=10240 || rows[l][2]!=5120))return false;
  for(unsigned b=0;b<7;b++){if(next[b]!=parent.Used(b))return false;used[b]=next[b];}
  ready=true;return true;
 }
 bool Ready()const{return ready;}
 unsigned Count()const{return ready?count:0;}
 const Part* At(unsigned i)const{return ready && i<count?&parts[i]:nullptr;}
 uint64_t Used(unsigned b)const{return ready && b<7?used[b]:0;}
};
#endif
