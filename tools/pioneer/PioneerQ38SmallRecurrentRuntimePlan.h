/* SPDX-License-Identifier: MIT */
#include "PioneerQ38RecurrentKernelCompileRole.h"
#if defined(PIONEER_Q38_SMALL_RECURRENT_JOURNAL_INSPECTION) && (PIONEER_Q38_SMALL_RECURRENT_JOURNAL_INSPECTION != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT))
#error Small recurrent runtime journal is simulated inspection only
#endif
#if (!defined(PIONEER_Q38_RECURRENT_KERNEL_COMPILE_ONLY) && !defined(PIONEER_Q38_SMALL_RECURRENT_JOURNAL_INSPECTION)) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Small recurrent runtime plan is isolated inspection only
#endif
#ifndef PIONEER_Q38_SMALL_RECURRENT_RUNTIME_PLAN_H
#define PIONEER_Q38_SMALL_RECURRENT_RUNTIME_PLAN_H
#include "PioneerQ38SmallRecurrentUploadReceiptInspection.h"
#include "PioneerQ38TokenRuntimePlan.h"

// Bounded startup packet plan. It neither writes the device nor grants RUN.
class PioneerQ38SmallRecurrentRuntimePlan {
 using Base=PioneerQ38TokenRuntimePlan;using R=PioneerQ38SmallRecurrentUploadReceiptInspection;
 enum : unsigned {LargeBytes=R::RecurrentWords*4,SmallBytes=96*32*4,SmallOffset=0x3004a00};
 static_assert(R::RecurrentOffset+LargeBytes==SmallOffset && SmallOffset+SmallBytes<=PioneerQ38TokenLayout::SessionOffset,
  "recurrent records must not overlap session workspace");
 Base base;R* receipt=nullptr;const PioneerQ38TokenStartupPlan* startup=nullptr;const PioneerQ38CanonicalModelPlan* canonical=nullptr;
 const void* context=nullptr;const void* cookie=nullptr;const uint32_t* hashes=nullptr;const uint8_t* ledger=nullptr;
 uint32_t records[R::RecurrentWords+96*32]={};int64_t team=-1;uint64_t generation=0;bool attempted=false,ready=false;
 bool Current()const{return ready && receipt && receipt->Matches(context,cookie,hashes,ledger,team,generation);}
 bool Output(const void* output,unsigned bytes)const{
  const uintptr_t out=reinterpret_cast<uintptr_t>(output),self=reinterpret_cast<uintptr_t>(this),limit=~uintptr_t(0);
  const uintptr_t rp=reinterpret_cast<uintptr_t>(receipt),hp=reinterpret_cast<uintptr_t>(hashes),lp=reinterpret_cast<uintptr_t>(ledger);
  const uintptr_t cp=reinterpret_cast<uintptr_t>(context),fp=reinterpret_cast<uintptr_t>(cookie);
  const size_t hashBytes=size_t(PioneerResidentBanks::Pages)*sizeof(uint32_t);
  return output && bytes && out<=limit-bytes && self<=limit-sizeof(*this) && rp<=limit-sizeof(R)
   && hp<=limit-hashBytes && lp<=limit-PioneerResidentBanks::Pages
   && !(out<self+sizeof(*this) && self<out+bytes) && !(out<rp+sizeof(R) && rp<out+bytes)
   && !(out<hp+hashBytes && hp<out+bytes) && !(out<lp+PioneerResidentBanks::Pages && lp<out+bytes)
   && !(cp>=out && cp<out+bytes) && !(fp>=out && fp<out+bytes) && startup && startup->PublishedOutput(output,bytes);
 }
public:
 using Chunk=Base::Chunk;
 enum : unsigned {Chunks=Base::Chunks+3,PayloadBytes=Base::PayloadBytes+LargeBytes+SmallBytes};
 bool Bind(const PioneerQ38CanonicalModelPlan& c,const PioneerQ38TokenStartupPlan& s,
  const PioneerQ38GdnIdentity& id,const uint32_t* source,R& owner,const void* ctx,const void* fd,
  const uint32_t* h,const uint8_t* initialized,int64_t ownerTeam,uint64_t gen){
  if(attempted)return false;attempted=true;
  if(id.driverGeneration!=gen || !owner.MatchesLogical(c,ctx,fd,h,initialized,ownerTeam,gen)
   || !base.Bind(c,s,id,source) || base.Bank()>=7 || base.Workspace()>UINT32_MAX-SmallOffset-SmallBytes
   || !owner.RecurrentWorkspace(records,R::RecurrentWords,ctx,fd,h,initialized,ownerTeam,gen)
   || !owner.BuildWorkspace(ctx,fd,h,initialized,ownerTeam,gen,records+R::RecurrentWords,96*32))return false;
  for(unsigned i=0;i<Base::Chunks;i++){
   Chunk old;if(!base.Describe(i,old))return false;
   const uint64_t begin=uint64_t(base.Workspace())+R::RecurrentOffset,end=uint64_t(base.Workspace())+SmallOffset+SmallBytes;
   if(old.bank==base.Bank() && uint64_t(old.address)<end && begin<uint64_t(old.address)+old.bytes)return false;
  }
  receipt=&owner;startup=&s;canonical=&c;context=ctx;cookie=fd;hashes=h;ledger=initialized;team=ownerTeam;generation=gen;ready=true;return true;
 }
 bool MatchesBinding(const PioneerQ38CanonicalModelPlan& c,const PioneerQ38TokenStartupPlan& s,
  const PioneerQ38GdnIdentity& id,const uint32_t* source)const{
  return Current() && canonical==&c && startup==&s && id.driverGeneration==generation && base.MatchesBinding(c,s,id,source);
 }
 bool Revalidate(const PioneerQ38CanonicalModelPlan& c,const PioneerQ38TokenStartupPlan& s,
  const PioneerQ38GdnIdentity& id,const uint32_t* source){
  return MatchesBinding(c,s,id,source) && receipt->MatchesLogical(c,context,cookie,hashes,ledger,team,generation);
 }
 bool Describe(unsigned index,Chunk& out){
  if(!Current() || !startup || index>=Chunks)return false;
  if(index<Base::Chunks)return base.Describe(index,out);
  const unsigned n=index-Base::Chunks,offset=n==0?0U:n==1?16384U:unsigned(LargeBytes),bytes=n==0?16384U:n==1?unsigned(LargeBytes)-16384U:unsigned(SmallBytes);
  out={n==2?8U:7U,64,offset,base.Bank(),base.Workspace()+R::RecurrentOffset+offset,bytes};return true;
 }
 bool Read(unsigned index,uint8_t* output,unsigned bytes){
  Chunk d;if(!Describe(index,d) || bytes!=d.bytes || !Output(output,bytes))return false;
  if(index<Base::Chunks)return base.Read(index,output,bytes);
  for(unsigned i=0;i<bytes;i++){const unsigned p=d.offset+i;output[i]=uint8_t(records[p/4]>>((p&3)*8));}return true;
 }
 unsigned Bank()const{return Current()?base.Bank():7;}
 unsigned Workspace()const{return Current()?base.Workspace():0;}
};
#endif
