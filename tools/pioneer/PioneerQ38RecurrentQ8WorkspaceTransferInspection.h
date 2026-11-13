/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_RECURRENT_Q8_MATRIX_PLAN_INSPECTION) || PIONEER_Q38_RECURRENT_Q8_MATRIX_PLAN_INSPECTION != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Recurrent workspace transfer is offline inspection only
#endif
#ifndef PIONEER_Q38_RECURRENT_Q8_WORKSPACE_TRANSFER_INSPECTION_H
#define PIONEER_Q38_RECURRENT_Q8_WORKSPACE_TRANSFER_INSPECTION_H
#include "PioneerQ38RecurrentQ8UploadReceiptInspection.h"
#include "PioneerQ38TokenRuntimePlan.h"

// Describes bytes for the existing bounded packet interface. It does not
// execute packets, grant ownership, prove readback or authorize RUN.
class PioneerQ38RecurrentQ8WorkspaceTransferInspection {
 using W=PioneerQ38RecurrentQ8MatrixWorkspace;using R=PioneerQ38RecurrentQ8UploadReceiptInspection;
 R* receipt=nullptr;const void* context=nullptr;const void* cookie=nullptr;
 const uint32_t* hashes=nullptr;const uint8_t* ledger=nullptr;
 int64_t team=-1;uint64_t generation=0;unsigned bank=7,workspace=0;
 std::array<uint32_t,W::Words> data={};bool attempted=false,ready=false;
 bool Current(){return ready && receipt && receipt->Revalidate(context,cookie,hashes,ledger,team,generation);}
public:
 enum : unsigned {Chunks=2,FirstBytes=16384,LastBytes=W::Bytes-FirstBytes};
 static_assert(LastBytes==2048,"recurrent metadata requires two aligned bounded packets");
 PioneerQ38RecurrentQ8WorkspaceTransferInspection()=default;
 PioneerQ38RecurrentQ8WorkspaceTransferInspection(const PioneerQ38RecurrentQ8WorkspaceTransferInspection&)=delete;
 PioneerQ38RecurrentQ8WorkspaceTransferInspection& operator=(const PioneerQ38RecurrentQ8WorkspaceTransferInspection&)=delete;
 bool Bind(R& source,const void* c,const void* fd,const uint32_t* h,const uint8_t* initialized,
  int64_t owner,uint64_t gen,unsigned targetBank,unsigned targetWorkspace){
  if(attempted)return false;attempted=true;
  if(targetBank>=7 || (targetWorkspace&63) || targetWorkspace>UINT32_MAX-W::Offset-W::Bytes
   || !source.Workspace(data.data(),data.size(),c,fd,h,initialized,owner,gen))return false;
  receipt=&source;context=c;cookie=fd;hashes=h;ledger=initialized;team=owner;generation=gen;
  bank=targetBank;workspace=targetWorkspace;ready=true;return true;
 }
 bool Describe(unsigned index,PioneerQ38TokenRuntimePlan::Chunk& out){
  if(!Current() || index>=Chunks)return false;
  PioneerQ38TokenRuntimePlan::Chunk staged={7,64,index?FirstBytes:0,bank,
   workspace+W::Offset+(index?FirstBytes:0),index?LastBytes:FirstBytes};
  out=staged;return true;
 }
 bool Read(unsigned index,uint8_t* output,unsigned bytes){
  PioneerQ38TokenRuntimePlan::Chunk chunk;
  if(!output || !Describe(index,chunk) || bytes!=chunk.bytes)return false;
  const uintptr_t out=reinterpret_cast<uintptr_t>(output),self=reinterpret_cast<uintptr_t>(this),limit=~uintptr_t(0);
  const uintptr_t hp=reinterpret_cast<uintptr_t>(hashes),lp=reinterpret_cast<uintptr_t>(ledger);
  const uintptr_t rp=reinterpret_cast<uintptr_t>(receipt),cp=reinterpret_cast<uintptr_t>(context),fp=reinterpret_cast<uintptr_t>(cookie);
  const size_t hashBytes=size_t(PioneerResidentBanks::Pages)*sizeof(uint32_t);
  if(out>limit-bytes || self>limit-sizeof(*this) || rp>limit-sizeof(R) || hp>limit-hashBytes || lp>limit-PioneerResidentBanks::Pages
   || (out<self+sizeof(*this) && self<out+bytes) || (out<hp+hashBytes && hp<out+bytes)
   || (out<lp+PioneerResidentBanks::Pages && lp<out+bytes) || (out<rp+sizeof(R) && rp<out+bytes)
   || (cp>=out && cp<out+bytes) || (fp>=out && fp<out+bytes))return false;
  for(unsigned i=0;i<bytes;i++){const unsigned p=chunk.offset+i;output[i]=uint8_t(data[p/4]>>((p&3)*8));}
  return true;
 }
};
#endif
