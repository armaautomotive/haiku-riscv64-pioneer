/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_DENSE_Q8_KERNEL_LAYOUT_INSPECTION_H
#define PIONEER_Q38_DENSE_Q8_KERNEL_LAYOUT_INSPECTION_H
#if !defined(PIONEER_Q38_DENSE_Q8_KERNEL_LAYOUT_INSPECTION) || PIONEER_Q38_DENSE_Q8_KERNEL_LAYOUT_INSPECTION != 1 \
 || !defined(PIONEER_Q38_QGW8_LAYOUT_INSPECTION) || PIONEER_Q38_QGW8_LAYOUT_INSPECTION != 1 \
 || (defined(_KERNEL_MODE) && (!defined(PIONEER_Q38_HYBRID_KERNEL_COMPILE_ONLY) || PIONEER_Q38_HYBRID_KERNEL_COMPILE_ONLY != 1)) \
 || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Dense Q8 fixed layout is isolated inspection only
#endif
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38Qgw8KernelLayout.h"

// Fixed-size independent address/format reconstruction against the original
// canonical model and a separately built QGW8 layout. This is NOT a kernel
// receipt, source authentication, page ledger, memory owner or launch grant.
// The converted stripes occupy only the front of the old QGW8 reservations;
// unused tail pages are holes, not initialized or transferable model data.
class PioneerQ38DenseQ8KernelLayoutInspection {
public:
 using Parent=PioneerQ38Qgw8KernelLayout;using Part=Parent::Part;
 enum : uint32_t {Original=Parent::Original,Bfp4=Parent::Bfp4,Bfp8=Parent::Bfp8,
  HeadBfp8=Parent::HeadBfp8,DenseQ8Bfp8=10};
private:
 Part parts[6144]={};uint64_t used[7]={};unsigned count=0;bool attempted=false,ready=false;
public:
 PioneerQ38DenseQ8KernelLayoutInspection()=default;
 PioneerQ38DenseQ8KernelLayoutInspection(const PioneerQ38DenseQ8KernelLayoutInspection&)=delete;
 PioneerQ38DenseQ8KernelLayoutInspection& operator=(const PioneerQ38DenseQ8KernelLayoutInspection&)=delete;
 bool Build(const PioneerQ38CanonicalModelPlan& source,const Parent& parent){
  if(attempted)return false;attempted=true;
  if(!source.OriginalFormatLayout() || source.Count()!=4678 || !parent.Ready() || parent.Count()!=4678)return false;
  uint64_t next[7]={};unsigned converted=0,rows[16][3]={};
  for(unsigned i=0;i<source.Count();i++){
   const auto* original=source.Part(i);const auto* prior=parent.At(i);
   if(!original || !prior || i>=6144 || prior->original!=i || original->bank>=7 || prior->bank!=original->bank
    || prior->offset!=next[prior->bank] || !prior->bytes || prior->bytes>prior->allocated
    || (prior->offset&4095) || (prior->allocated&4095) || prior->allocated-prior->bytes>=4096
    || prior->offset>source.Capacity || prior->allocated>source.Capacity-prior->offset)return false;
   next[prior->bank]+=prior->allocated;Part result=*prior;
   if(original->kind==source.WeightKind && original->layer<64 && original->layer%4==3 && original->role>=5 && original->role<=7){
    const unsigned layer=original->layer/4,role=original->role-5,limit=role?1024:12288;
    if(original->type!=8 || original->rowBytes!=5760 || !original->rows || ((original->firstRow|original->rows)&31)
     || original->firstRow!=rows[layer][role] || original->rows>limit-original->firstRow
     || uint64_t(original->rows)*5760!=original->bytes || prior->encoding!=Original
     || prior->bytes!=original->bytes || prior->allocated!=original->allocated)return false;
    const uint64_t bytes=uint64_t(original->rows)*5440,allocated=(bytes+4095)&~uint64_t(4095);
    if(!bytes || bytes>UINT32_MAX || allocated>prior->allocated)return false;
    result.bytes=uint32_t(bytes);result.allocated=uint32_t(allocated);result.encoding=DenseQ8Bfp8;
    rows[layer][role]+=original->rows;converted++;
   }
   parts[count++]=result;
  }
  if(count!=4678 || converted!=336)return false;
  for(unsigned l=0;l<16;l++)if(rows[l][0]!=12288 || rows[l][1]!=1024 || rows[l][2]!=1024)return false;
  for(unsigned b=0;b<7;b++){if(next[b]!=parent.Used(b))return false;used[b]=next[b];}
  ready=true;return true;
 }
 bool Ready()const{return ready;}
 unsigned Count()const{return ready?count:0;}
 const Part* At(unsigned i)const{return ready && i<count?&parts[i]:nullptr;}
 uint64_t Used(unsigned b)const{return ready && b<7?used[b]:0;}
};
static_assert(sizeof(PioneerQ38DenseQ8KernelLayoutInspection)>131072 && sizeof(PioneerQ38DenseQ8KernelLayoutInspection)<262144,
 "dense-Q8 fixed inspection layout is heap-only");
#endif
