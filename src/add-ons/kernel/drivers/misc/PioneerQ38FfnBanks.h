/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_FFN_BANKS_H
#define PIONEER_Q38_FFN_BANKS_H
#ifndef PIONEER_Q38_GDN_MUTABLE_STATE
#error FFN workspace requires the existing single-owner mutable reservation interlock
#endif
#include "PioneerResidentBanks.h"
#include "PioneerQ38GdnIdentity.h"
#include "PioneerQ38FfnResidentShape.h"
// The shared four nonzero model/session/sequence/driver epochs are DATA,
// not source authentication or a public compute capability.
using PioneerQ38FfnIdentity=PioneerQ38GdnIdentity;
struct PioneerQ38FfnExtent {uint32_t bank,address,route,index,pages,bytes;};
struct PioneerResidentLease;struct PioneerBulkUploadLease;struct PioneerOutputBufferLease;struct PioneerWideOutputLease;
struct PioneerQ38FfnZeroLease;struct PioneerQ38FfnZeroResult;
class PioneerQ38FfnBanks {
 template<class IO> friend bool PioneerQ38FfnZeroPrepare(IO&,const tt_dram_samples_result&,const tt_compute_noc_result&,
  PioneerQ38FfnBanks&,PioneerResidentBanks&,const PioneerQ38FfnIdentity&,const tt_host_page_result&,
  PioneerResidentLease&,PioneerBulkUploadLease&,PioneerOutputBufferLease&,PioneerWideOutputLease&,PioneerQ38FfnZeroLease&);
 template<class IO> friend bool PioneerQ38FfnZeroRun(IO&,const tt_dram_samples_result&,const tt_compute_noc_result&,
  PioneerQ38FfnBanks&,PioneerResidentBanks&,const tt_host_page_result&,PioneerResidentLease&,PioneerBulkUploadLease&,
  PioneerOutputBufferLease&,PioneerWideOutputLease&,PioneerQ38FfnZeroLease&,const PioneerQ38FfnIdentity&,
  unsigned,unsigned,unsigned,PioneerQ38FfnZeroResult&);
 template<class IO> friend bool PioneerQ38FfnZeroClose(IO&,const tt_dram_samples_result&,const tt_compute_noc_result&,
  PioneerQ38FfnBanks&,PioneerResidentBanks&,const tt_host_page_result&,PioneerResidentLease&,PioneerBulkUploadLease&,
  PioneerOutputBufferLease&,PioneerWideOutputLease&,PioneerQ38FfnZeroLease&,bool);
 PioneerResidentBanks* banks=nullptr;
 const tt_dram_samples_result* baseline=nullptr;
 PioneerQ38FfnIdentity identity={};
 const PioneerQ38FfnZeroLease* initializationLease=nullptr;
 uint32_t bank=0,offset=0;
 uint64_t initialized[2]={};
 bool unsafe=false,published=false;
 bool Matches(const PioneerQ38FfnIdentity& id) const {
  return banks && baseline && !unsafe && id==identity && banks->Matches(*baseline);
 }
 bool Recorded(unsigned page) const {return (initialized[page/64]>>(page%64))&1;}
 bool ClaimInitialization(const PioneerQ38FfnIdentity& id,const PioneerQ38FfnZeroLease* lease) {
  if(!lease || initializationLease || published || !Matches(id))return false;
  initializationLease=lease;return true;
 }
 bool CloseInitialization(const PioneerQ38FfnIdentity& id,const PioneerQ38FfnZeroLease* lease) {
  if(!lease || initializationLease!=lease || !Matches(id))return false;
  initializationLease=nullptr;return true; // ONLY complete known-clean CLOSE
 }
 bool CommitInitialization(const PioneerQ38FfnIdentity& id,unsigned region,unsigned first,unsigned count) {
  PioneerQ38FfnExtent e;
  if(!FreshInitializationSpan(id,region,first,count,e))return false;
  for(unsigned i=0;i<count;i++)initialized[(first+i)/64]|=UINT64_C(1)<<((first+i)%64);
  return true; // private CPU ledger ONLY,after actual controller proofs
 }
 bool PublishInitialization(const PioneerQ38FfnIdentity& id) {
  unsigned count=0;if(published || !InitializationProgress(id,count) || count!=Pages)return false;
  published=true;return true; // called ONLY after complete known-clean zero CLOSE
 }
public:
 static constexpr unsigned Pages=(TT_Q38_FFN_RESIDENT_BYTES+4095)/4096;
 static constexpr unsigned Bytes=Pages*4096;
 static_assert(Pages==74 && Bytes==303104,"complete resident-FFN workspace footprint");
 PioneerQ38FfnBanks()=default;
 PioneerQ38FfnBanks(const PioneerQ38FfnBanks&)=delete;
 PioneerQ38FfnBanks& operator=(const PioneerQ38FfnBanks&)=delete;
 // KERNEL-PRIVATE proposal from the authenticated canonical model layout,
 // NEVER caller NoC/physical addresses. Caller holds the SAME global store
 // lock across binding+all commands. The entire64MiB workspace must fit,
 // but ONLY its first74 pages are reserved. No allocation/range widening.
 // Retain this exact object until the bank context dies,including on fault.
 bool Bind(PioneerResidentBanks& owner,const tt_dram_samples_result& proof,const PioneerQ38FfnIdentity& id,
  const uint8_t* learned,unsigned learnedPages,unsigned logicalBank,uint64_t workspaceOffset) {
  if(banks || unsafe || !id.Valid() || logicalBank>=7 || (workspaceOffset&4095)
   || workspaceOffset>=owner.Capacity || UINT64_C(67108864)>owner.Capacity-workspaceOffset || !owner.Matches(proof))return false;
  const PioneerResidentMutableRange claim={logicalBank,uint32_t(workspaceOffset/4096),Pages};
  if(!owner.ReserveMutable(proof,this,&claim,1,learned,learnedPages))return false;
  banks=&owner;baseline=&proof;identity=id;bank=logicalBank;offset=uint32_t(workspaceOffset);return true;
 }
 bool MatchesInitializationOwner(const PioneerResidentBanks& owner,const tt_dram_samples_result& proof,const PioneerQ38FfnIdentity& id) const {
  return banks==&owner && baseline==&proof && Matches(id);
 }
 // Initialization address derivation ONLY,not a compute/read-ready API.
 // Every covered page is re-proved against the exact reservation owner.
 bool InitializeSpan(const PioneerQ38FfnIdentity& id,unsigned region,unsigned first,unsigned count,PioneerQ38FfnExtent& output) const {
  if(!Matches(id) || region || !count || first>=Pages || count>Pages-first)return false;
  PioneerQ38FfnExtent e={bank,0,0,0,count,count*4096};
  for(unsigned p=0;p<count;p++) {
   uint32_t a=0,r=0,i=0;
   if(!banks->MutablePage(*baseline,this,bank,uint64_t(offset)+uint64_t(first+p)*4096,a,r,i))return false;
   if(!p){e.address=a;e.route=r;e.index=i;}
   else if(a!=e.address+p*4096 || r!=e.route || i!=e.index+p)return false;
  }
  output=e;return true;
 }
 bool FreshInitializationSpan(const PioneerQ38FfnIdentity& id,unsigned region,unsigned first,unsigned count,PioneerQ38FfnExtent& output) const {
  if(published || region || !count || first>=Pages || count>Pages-first)return false;
  for(unsigned p=0;p<count;p++)if(Recorded(first+p))return false;
  return InitializeSpan(id,region,first,count,output);
 }
 bool InitializationProgress(const PioneerQ38FfnIdentity& id,unsigned& output) const {
  if(!Matches(id))return false;
  unsigned count=0;for(unsigned p=0;p<Pages;p++)count+=Recorded(p);output=count;return true;
 }
 bool InitializationPublished(const PioneerQ38FfnIdentity& id) const {return Matches(id) && published;}
 bool OwnsInitializationLease(const PioneerQ38FfnZeroLease* lease) const {return lease && initializationLease==lease;}
 void Fault(){unsafe=true;published=false;if(banks)banks->Quarantine();} // MEMORY ONLY,no release/reset/rebind
 bool Unsafe() const {return unsafe || (banks && banks->Quarantined());}
};
#endif
