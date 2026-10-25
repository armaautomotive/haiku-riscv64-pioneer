/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GDN_BANKS_H
#define PIONEER_Q38_GDN_BANKS_H
#ifndef PIONEER_Q38_GDN_MUTABLE_STATE
#error GDN mutable banks require the separately gated reservation interlock
#endif
#include "PioneerResidentBanks.h"
#include "PioneerQ38GdnIdentity.h"
#include "PioneerQ38GdnZeroProgress.h"
#ifdef PIONEER_Q38_NEXT_GDN_NATIVE
#include "PioneerQ38GdnLayerProgress.h"
#endif
struct PioneerResidentLease;
struct PioneerBulkUploadLease;
struct PioneerOutputBufferLease;
struct PioneerWideOutputLease;
struct PioneerQ38StateZeroLease;
struct PioneerQ38StateZeroResult;
// Kernel-private frozen logical placement. No caller-provided NoC route,
// physical address, or GGML data pointer. It is NOT a public ioctl ABI.
struct PioneerQ38GdnRegion { uint32_t layer,slot,firstHead,heads,bank,offset; };
struct PioneerQ38GdnBankExtent { uint32_t bank,address,route,index,pages,bytes; };
class PioneerQ38GdnBanks {
 friend class PioneerQ38GraphMutableBanks;
 friend class PioneerQ38ProjectionMutableBanks;
#ifdef PIONEER_Q38_RESIDENT_LAYER_NATIVE_COMPONENT
 friend class PioneerQ38ResidentLayerOwner;
 const class PioneerQ38ResidentLayerOwner* residentLayerOwner=nullptr;
 unsigned residentLayerPhase=0;bool residentLayerNextPublished=false;
#endif
#ifdef PIONEER_Q38_RESIDENT_OUTPUT_OWNER
 friend class PioneerQ38ResidentOutputOwner;
 const class PioneerQ38ResidentOutputOwner* residentOutputOwner=nullptr;
 unsigned residentOutputPhase=0;bool residentOutputNextPublished=false;
#endif
#ifdef PIONEER_Q38_GDN_RECURRENT_NATIVE
 friend class PioneerQ38ResidentDecoderOwner;
#endif
 template<class IO> friend bool PioneerQ38StateZeroRun(IO&,const tt_dram_samples_result&,const tt_compute_noc_result&,
  PioneerQ38GdnBanks&,PioneerResidentBanks&,const tt_host_page_result&,PioneerResidentLease&,
  PioneerBulkUploadLease&,PioneerOutputBufferLease&,PioneerWideOutputLease&,PioneerQ38StateZeroLease&,
  const PioneerQ38GdnIdentity&,unsigned,unsigned,unsigned,PioneerQ38StateZeroResult&);
 template<class IO> friend bool PioneerQ38StateZeroClose(IO&,const tt_dram_samples_result&,const tt_compute_noc_result&,
  PioneerQ38GdnBanks&,PioneerResidentBanks&,const tt_host_page_result&,PioneerResidentLease&,
  PioneerBulkUploadLease&,PioneerOutputBufferLease&,PioneerWideOutputLease&,PioneerQ38StateZeroLease&,bool);
 // Allocate this owner on the HEAP in a future native context: staging
 // arrays intentionally do not consume Haiku's small kernel stack.
 PioneerResidentBanks* banks=nullptr;
 const tt_dram_samples_result* baseline=nullptr;
 const void* reservationOwner=nullptr; // this OR the exact retained joint owner
 PioneerQ38GdnIdentity identity={};
 PioneerQ38GdnRegion regions[672]={};
 PioneerResidentMutableRange claims[672]={};
 uint16_t regionForHead[48][2][48]={}; // heap-only O(1) half-head lookup
 uint32_t regionFirstPage[672]={};
 PioneerQ38GdnZeroProgress zeroProgress;
 unsigned count=0;
 bool unsafe=false,initializationPublished=false;
#ifdef PIONEER_Q38_GDN_RECURRENT_NATIVE
 const void* layerOwner=nullptr;unsigned layerPhase=0;bool layerNextPublished=false;
 bool ClaimFirstLayer(const void* owner,const PioneerQ38GdnIdentity& id,unsigned phase){
  if(!owner || layerOwner || !phase || phase>=0x7fffffff || !InitializationPublished(id))return false;
  layerOwner=owner;layerPhase=phase;return true;
 }
 bool OwnsFirstLayer(const void* owner,const PioneerQ38GdnIdentity& id,unsigned phase)const{
  return owner && layerOwner==owner && layerPhase==phase && InitializationPublished(id);
 }
 bool PublishFirstLayer(const void* owner,const PioneerQ38GdnIdentity& id,unsigned phase){
  if(layerNextPublished || !OwnsFirstLayer(owner,id,phase))return false;
  layerNextPublished=true;return true; // private layer0/position0/slot1 ONLY;NEVER current-slot/token flip
 }
#ifdef PIONEER_Q38_NEXT_GDN_NATIVE
 PioneerQ38GdnLayerProgress nextLayers;bool nextLayersBound=false;
 bool ClaimNextLayer(const void* who,const PioneerQ38GdnIdentity& id,unsigned layer,unsigned phase){
  if(layer!=1 || phase<=layerPhase || phase>=0x7fffffff || !layerNextPublished || !OwnsFirstLayer(who,id,layerPhase))return false;
  if(!nextLayersBound){
   if(!nextLayers.BindAfterFirstLayer(who,id,layerPhase))return false;
   nextLayersBound=true;
  }
  return nextLayers.ClaimNext(who,id,layer,phase);
 }
 bool OwnsNextLayer(const void* who,const PioneerQ38GdnIdentity& id,unsigned layer,unsigned phase)const{
  return layer==1 && nextLayersBound && layerNextPublished && OwnsFirstLayer(who,id,layerPhase)
   && (nextLayers.OwnsPending(who,id,layer,phase) || nextLayers.Published(who,id,layer,phase));
 }
 bool PublishNextLayer(const void* who,const PioneerQ38GdnIdentity& id,unsigned layer,unsigned phase){
  return OwnsNextLayer(who,id,layer,phase) && nextLayers.PublishNext(who,id,layer,phase);
 }
 bool NextLayerPublished(const void* who,const PioneerQ38GdnIdentity& id,unsigned layer,unsigned phase)const{
  return OwnsNextLayer(who,id,layer,phase) && nextLayers.Published(who,id,layer,phase);
 }
#endif
#endif
 static unsigned Ordinal(unsigned layer) { return layer<64 && layer%4!=3?layer-layer/4:~0U; }
 bool Resolve(unsigned region,unsigned firstPage,unsigned pages,PioneerQ38GdnBankExtent& output) const {
  if (region>=count || !pages || pages>256) return false;
  const auto& r=regions[region]; unsigned total=r.heads*16;
  if (firstPage>=total || pages>total-firstPage) return false;
  uint64_t offset=uint64_t(r.offset)+firstPage*4096;
  PioneerQ38GdnBankExtent e={r.bank,0,0,0,pages,pages*4096};
  for (unsigned p=0;p<pages;p++) {
   uint32_t address=0,route=0,index=0;
   if (!banks->MutablePage(*baseline,reservationOwner,r.bank,offset+p*4096,address,route,index)) return false;
   if (!p) { e.address=address; e.route=route; e.index=index; }
   else if (address!=e.address+p*4096 || route!=e.route || index!=e.index+p) return false;
  }
  output=e; return true;
 }
 bool Matches(const PioneerQ38GdnIdentity& id) const { return banks && !unsafe && id==identity && banks->Matches(*baseline); }
 bool CommitInitialization(const PioneerQ38GdnIdentity& id,unsigned region,unsigned firstPage,unsigned pages) {
  PioneerQ38GdnBankExtent extent;
  return Matches(id) && Resolve(region,firstPage,pages,extent)
   && zeroProgress.Record(id,regionFirstPage[region]+firstPage,pages);
 }
 bool PublishInitialization(const PioneerQ38GdnIdentity& id) {
  unsigned pages=0;
  if (!Matches(id) || !zeroProgress.Snapshot(id,pages) || pages!=PioneerQ38GdnZeroProgress::Pages) return false;
  initializationPublished=true; return true; // called ONLY after complete known-clean zero CLOSE proofs
 }
 // PRIVATE staging,not reservation/binding/readiness. The joint owner
 // uses the SAME complete region validator BEFORE its one atomic claim.
 bool StageRegions(const PioneerQ38GdnRegion* input,unsigned n) {
  if (banks || unsafe || !input || n<96 || n>672) return false;
  uint64_t heads[48][2]={}; unsigned totalPages=0;
  for (unsigned i=0;i<n;i++) {
   auto r=input[i]; unsigned ordinal=Ordinal(r.layer);
   if (ordinal>=48 || r.slot>=2 || r.firstHead>=48 || !r.heads || r.heads>48-r.firstHead || r.bank>=7
    || (r.offset&4095) || r.offset>=PioneerResidentBanks::Capacity
    || uint64_t(r.heads)*65536>PioneerResidentBanks::Capacity-r.offset) return false;
   uint64_t mask=((UINT64_C(1)<<r.heads)-1)<<r.firstHead;
   if (heads[ordinal][r.slot]&mask) return false;
   heads[ordinal][r.slot]|=mask; regions[i]=r; claims[i]={r.bank,r.offset/4096,r.heads*16};
   regionFirstPage[i]=totalPages; totalPages+=r.heads*16;
   for (unsigned h=r.firstHead;h<r.firstHead+r.heads;h++) regionForHead[ordinal][r.slot][h]=uint16_t(i);
  }
  for (unsigned layer=0;layer<48;layer++) for (unsigned slot=0;slot<2;slot++)
   if (heads[layer][slot]!=UINT64_C(0xffffffffffff)) return false;
  return totalPages==PioneerQ38GdnZeroProgress::Pages;
 }
 // Called ONLY after the complete independently checked reservation.
 // No caller-provided shared-owner/address hint is exposed publicly.
 bool AttachReservation(PioneerResidentBanks& owner,const tt_dram_samples_result& proof,
  const PioneerQ38GdnIdentity& id,const void* reservation,unsigned n) {
  banks=&owner; baseline=&proof; identity=id; count=n; reservationOwner=reservation;
  if (!zeroProgress.Bind(id)) { Fault(); return false; }
  return true;
 }
public:
 PioneerQ38GdnBanks()=default;
 PioneerQ38GdnBanks(const PioneerQ38GdnBanks&)=delete;
 PioneerQ38GdnBanks& operator=(const PioneerQ38GdnBanks&)=delete;
 // Exclusive global store lock is mandatory. Refusal before claim is
 // inert; successful ownership survives Close/fault until context death.
 bool Bind(PioneerResidentBanks& owner,const tt_dram_samples_result& proof,const PioneerQ38GdnIdentity& id,
  const uint8_t* initialized,unsigned initializedPages,const PioneerQ38GdnRegion* input,unsigned n) {
  if (banks || unsafe || !id.Valid() || !owner.Matches(proof) || !StageRegions(input,n)
   || !owner.ReserveMutable(proof,this,claims,n,initialized,initializedPages)) return false;
  return AttachReservation(owner,proof,id,this,n);
 }
 // Initialization-only private extent. The new owner has NO compute/read
 // readiness API yet. Actual zeroing/completion must be independently
 // qualified before a future native coordinator can make it ready.
 bool MatchesInitializationOwner(const PioneerResidentBanks& owner,const tt_dram_samples_result& proof,const PioneerQ38GdnIdentity& id) const {
  return banks==&owner && baseline==&proof && Matches(id); // CPU relation ONLY,no ready/address authority
 }
 bool InitializeHalf(const PioneerQ38GdnIdentity& id,unsigned layer,unsigned slot,unsigned worker,PioneerQ38GdnBankExtent& output) const {
  if (!Matches(id) || Ordinal(layer)>=48 || slot>=2 || worker>=96) return false;
  unsigned head=worker/2,half=worker%2;
  unsigned i=regionForHead[Ordinal(layer)][slot][head]; if (i>=count) return false;
  const auto& r=regions[i];
  if (r.layer!=layer || r.slot!=slot || head<r.firstHead || head-r.firstHead>=r.heads) return false;
  return Resolve(i,(head-r.firstHead)*16+half*8,8,output);
 }
 // Larger initialization chunks avoid a launch per half-head. Still only
 // the owner's exact private region, at most1MiB, with EVERY page proved.
 bool InitializationPages(const PioneerQ38GdnIdentity& id,unsigned region,unsigned& pages) const {
  if (!Matches(id) || region>=count) return false;
  pages=regions[region].heads*16; return true;
 }
 bool InitializationRegions(const PioneerQ38GdnIdentity& id,unsigned& regions) const {
  if (!Matches(id) || count<96 || count>672) return false;
  regions=count; return true; // private CPU geometry ONLY,no allocation/read/compute grant
 }
 bool InitializeSpan(const PioneerQ38GdnIdentity& id,unsigned region,unsigned firstPage,unsigned pages,PioneerQ38GdnBankExtent& output) const {
  return Matches(id) && Resolve(region,firstPage,pages,output);
 }
 // Memory ONLY. A future native controller must ALSO latch cold recovery
 // and retain its context/cookie/ALL host leases; this helper cannot do so.
 bool InitializationProgress(const PioneerQ38GdnIdentity& id,unsigned& pages) const {
  return Matches(id) && zeroProgress.Snapshot(id,pages); // CPU count ONLY,no compute/read-ready API
 }
 bool InitializationPublished(const PioneerQ38GdnIdentity& id) const {
  return Matches(id) && initializationPublished; // zero-init/CLOSE seal ONLY,NOT compute/read/model authority
 }
 void Fault() {
#ifdef PIONEER_Q38_NEXT_GDN_NATIVE
  nextLayers.Fault();
#endif
  unsafe=true; initializationPublished=false; zeroProgress.Fault(); if (banks) banks->Quarantine();
 } // NO release/reset/I/O
 bool Unsafe() const { return unsafe || (banks && banks->Quarantined()); }
};
#endif
