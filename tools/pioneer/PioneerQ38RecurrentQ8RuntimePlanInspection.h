/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_RECURRENT_Q8_MATRIX_PLAN_INSPECTION) || PIONEER_Q38_RECURRENT_Q8_MATRIX_PLAN_INSPECTION != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Recurrent runtime plan is offline inspection only
#endif
#ifndef PIONEER_Q38_RECURRENT_Q8_RUNTIME_PLAN_INSPECTION_H
#define PIONEER_Q38_RECURRENT_Q8_RUNTIME_PLAN_INSPECTION_H
#include "PioneerQ38RecurrentQ8WorkspaceTransferInspection.h"

// DATA proposal only: does not write device memory or authorize execution.
class PioneerQ38RecurrentQ8RuntimePlanInspection {
 using Base=PioneerQ38TokenRuntimePlan;using Transfer=PioneerQ38RecurrentQ8WorkspaceTransferInspection;
 using R=PioneerQ38RecurrentQ8UploadReceiptInspection;
 Base base;Transfer transfer;R* receipt=nullptr;const PioneerQ38TokenStartupPlan* startup=nullptr;
 const PioneerQ38CanonicalModelPlan* canonical=nullptr;
 const void* context=nullptr;const void* cookie=nullptr;const uint32_t* hashes=nullptr;const uint8_t* ledger=nullptr;
 int64_t team=-1;uint64_t generation=0;bool attempted=false,ready=false;
 bool Current()const{return ready && receipt && receipt->Matches(context,cookie,hashes,ledger,team,generation);}
 bool Output(const void* output,unsigned bytes)const{
  const uintptr_t out=reinterpret_cast<uintptr_t>(output),self=reinterpret_cast<uintptr_t>(this),limit=~uintptr_t(0);
  const uintptr_t rp=reinterpret_cast<uintptr_t>(receipt),hp=reinterpret_cast<uintptr_t>(hashes),lp=reinterpret_cast<uintptr_t>(ledger);
  const uintptr_t cp=reinterpret_cast<uintptr_t>(context),fp=reinterpret_cast<uintptr_t>(cookie);
  const size_t hashBytes=size_t(PioneerResidentBanks::Pages)*sizeof(uint32_t);
  return output && bytes && out<=limit-bytes && self<=limit-sizeof(*this)
   && rp<=limit-sizeof(R) && hp<=limit-hashBytes && lp<=limit-PioneerResidentBanks::Pages
   && !(out<self+sizeof(*this) && self<out+bytes) && !(out<rp+sizeof(R) && rp<out+bytes)
   && !(out<hp+hashBytes && hp<out+bytes) && !(out<lp+PioneerResidentBanks::Pages && lp<out+bytes)
   && !(cp>=out && cp<out+bytes) && !(fp>=out && fp<out+bytes)
   && startup && startup->PublishedOutput(output,bytes);
 }
public:
 using Chunk=Base::Chunk;
 enum {Chunks=Base::Chunks+Transfer::Chunks,PayloadBytes=Base::PayloadBytes+PioneerQ38RecurrentQ8MatrixWorkspace::Bytes};
 PioneerQ38RecurrentQ8RuntimePlanInspection()=default;
 PioneerQ38RecurrentQ8RuntimePlanInspection(const PioneerQ38RecurrentQ8RuntimePlanInspection&)=delete;
 PioneerQ38RecurrentQ8RuntimePlanInspection& operator=(const PioneerQ38RecurrentQ8RuntimePlanInspection&)=delete;
 bool Bind(const PioneerQ38CanonicalModelPlan& c,const PioneerQ38TokenStartupPlan& s,
  const PioneerQ38GdnIdentity& id,const uint32_t* source,R& owner,const void* ctx,const void* fd,
  const uint32_t* h,const uint8_t* initialized,int64_t ownerTeam,uint64_t gen){
  if(attempted)return false;attempted=true;
  if(id.driverGeneration!=gen || !owner.MatchesLogical(c,ctx,fd,h,initialized,ownerTeam,gen)
   || !base.Bind(c,s,id,source) || !transfer.Bind(owner,ctx,fd,h,initialized,ownerTeam,gen,base.Bank(),base.Workspace()))return false;
  for(unsigned j=0;j<Transfer::Chunks;j++){
   Chunk extra;if(!transfer.Describe(j,extra))return false;
   const uint64_t begin=extra.address,end=begin+extra.bytes;
   for(unsigned i=0;i<Base::Chunks;i++){
    Chunk old;if(!base.Describe(i,old) || (old.bank==extra.bank && uint64_t(old.address)<end && begin<uint64_t(old.address)+old.bytes))return false;
   }
  }
  receipt=&owner;startup=&s;canonical=&c;context=ctx;cookie=fd;hashes=h;ledger=initialized;team=ownerTeam;generation=gen;ready=true;return true;
 }
 bool MatchesBinding(const PioneerQ38CanonicalModelPlan& c,const PioneerQ38TokenStartupPlan& s,
  const PioneerQ38GdnIdentity& id,const uint32_t* source)const{
  return Current() && canonical==&c && startup==&s && id.driverGeneration==generation && base.MatchesBinding(c,s,id,source);
 }
 // Expensive model/ledger verification is explicit at lifecycle boundaries;
 // it is not a per-token CPU model validation or device ownership proof.
 bool Revalidate(const PioneerQ38CanonicalModelPlan& c,const PioneerQ38TokenStartupPlan& s,
  const PioneerQ38GdnIdentity& id,const uint32_t* source){
  return MatchesBinding(c,s,id,source) && receipt->MatchesLogical(c,context,cookie,hashes,ledger,team,generation);
 }
 bool Describe(unsigned index,Chunk& out){
  if(!Current() || !startup || index>=Chunks)return false;
  return index<Base::Chunks?base.Describe(index,out):transfer.Describe(index-Base::Chunks,out);
 }
 bool Read(unsigned index,uint8_t* output,unsigned bytes){
  Chunk d;if(!Describe(index,d) || bytes!=d.bytes || !Output(output,bytes))return false;
  return index<Base::Chunks?base.Read(index,output,bytes):transfer.Read(index-Base::Chunks,output,bytes);
 }
 unsigned Bank()const{return Current()?base.Bank():7;}
 unsigned Workspace()const{return Current()?base.Workspace():0;}
};
#endif
