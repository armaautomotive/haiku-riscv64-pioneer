/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_SMALL_RECURRENT_BFP8_PLAN_INSPECTION) || PIONEER_Q38_SMALL_RECURRENT_BFP8_PLAN_INSPECTION != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Small recurrent workspace receipt is isolated offline data only
#endif
#ifndef PIONEER_Q38_SMALL_RECURRENT_WORKSPACE_RECEIPT_H
#define PIONEER_Q38_SMALL_RECURRENT_WORKSPACE_RECEIPT_H
#include "PioneerQ38SmallRecurrentWorkspace.h"
#include "PioneerQ38SmallRecurrentUploadReceiptInspection.h"

// Metadata-only bridge: the receipt and physical-order ledger are checked
// before and after construction. It performs no card write or readback.
struct PioneerQ38SmallRecurrentWorkspaceReceipt {
 using W=PioneerQ38SmallRecurrentWorkspace;using Receipt=PioneerQ38SmallRecurrentUploadReceiptInspection;
 static bool Build(Receipt& receipt,const W::Canonical& source,const W::Plan::Proposal& proposal,
  const void* context,const void* cookie,const uint32_t* hashes,const uint8_t* ledger,int64_t owner,uint64_t generation,
  uint32_t* output,unsigned words){
  if(!output || words!=W::Words || (reinterpret_cast<uintptr_t>(output)&3) || !hashes || !ledger)return false;
  const uintptr_t out=reinterpret_cast<uintptr_t>(output),self=reinterpret_cast<uintptr_t>(&receipt),limit=~uintptr_t(0);
  const uintptr_t hp=reinterpret_cast<uintptr_t>(hashes),lp=reinterpret_cast<uintptr_t>(ledger);
  const uintptr_t cp=reinterpret_cast<uintptr_t>(context),fp=reinterpret_cast<uintptr_t>(cookie);
  const uintptr_t src=reinterpret_cast<uintptr_t>(&source),plan=reinterpret_cast<uintptr_t>(&proposal);
  const uintptr_t parts=reinterpret_cast<uintptr_t>(proposal.parts.data());
  if(proposal.parts.size()>SIZE_MAX/sizeof(proposal.parts[0]))return false;
  const size_t partBytes=proposal.parts.size()*sizeof(proposal.parts[0]);
  const size_t hashBytes=size_t(PioneerResidentBanks::Pages)*sizeof(uint32_t);
  if(out>limit-W::Bytes || self>limit-sizeof(receipt) || hp>limit-hashBytes || lp>limit-PioneerResidentBanks::Pages
   || src>limit-sizeof(source) || plan>limit-sizeof(proposal) || parts>limit-partBytes
   || (out<self+sizeof(receipt) && self<out+W::Bytes) || (out<hp+hashBytes && hp<out+W::Bytes)
   || (out<lp+PioneerResidentBanks::Pages && lp<out+W::Bytes)
   || (out<src+sizeof(source) && src<out+W::Bytes) || (out<plan+sizeof(proposal) && plan<out+W::Bytes)
   || (partBytes && out<parts+partBytes && parts<out+W::Bytes)
   || (cp>=out && cp<out+W::Bytes) || (fp>=out && fp<out+W::Bytes))return false;
  std::array<uint32_t,W::Words> staged={};
  if(!receipt.MatchesLayout(source,proposal,context,cookie,hashes,ledger,owner,generation)
   || !W::Build(source,proposal,staged.data(),staged.size())
   || !receipt.MatchesLayout(source,proposal,context,cookie,hashes,ledger,owner,generation))return false;
  for(unsigned i=0;i<W::Words;i++)output[i]=staged[i];return true;
 }
};
#endif
