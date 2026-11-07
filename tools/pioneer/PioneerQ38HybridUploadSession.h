/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_HYBRID_UPLOAD_SESSION_H
#define PIONEER_Q38_HYBRID_UPLOAD_SESSION_H
#if !defined(PIONEER_Q38_HYBRID_UPLOAD_DIAGNOSTIC) || PIONEER_Q38_HYBRID_UPLOAD_DIAGNOSTIC != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) \
 || defined(PIONEER_Q38_BF16_FULL_CLIENT) || defined(PIONEER_Q38_BF16_FULL_NATIVE) \
 || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_DRIVER_BUILD_TAG) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Hybrid upload session is an isolated diagnostic, not a native or Work inference capability
#endif
#include "PioneerQ38HybridSpanUpload.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
#ifdef PIONEER_Q38_HYBRID_TRACKED_UPLOAD_CLIENT
#if PIONEER_Q38_HYBRID_TRACKED_UPLOAD_CLIENT != 1
#error Hybrid tracked upload client requires its distinct diagnostic selection
#endif
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38HybridUploadProtocol.h"
#endif
#ifdef PIONEER_Q38_HYBRID_CANDIDATE_UPLOAD_CLIENT
#if PIONEER_Q38_HYBRID_CANDIDATE_UPLOAD_CLIENT != 1 || !defined(PIONEER_Q38_HYBRID_TRACKED_UPLOAD_CLIENT) \
 || PIONEER_Q38_HYBRID_TRACKED_UPLOAD_CLIENT != 1
#error Hybrid candidate uploader requires the distinct tracked client
#endif
#endif
#if defined(PIONEER_Q38_HYBRID_FIXED_126) && (!defined(PIONEER_Q38_HYBRID_CANDIDATE_UPLOAD_CLIENT) \
 || PIONEER_Q38_HYBRID_CANDIDATE_UPLOAD_CLIENT != 1 || PIONEER_Q38_HYBRID_FIXED_126 != 1)
#error Fixed tag126 client requires the distinct candidate uploader
#endif
#ifdef PIONEER_Q38_HYBRID_FULL_CLIENT
#if PIONEER_Q38_HYBRID_FULL_CLIENT != 1 || !defined(PIONEER_Q38_HYBRID_CANDIDATE_UPLOAD_CLIENT) \
 || PIONEER_Q38_HYBRID_CANDIDATE_UPLOAD_CLIENT != 1
#error Hybrid full client requires its explicit candidate uploader
#endif
template<class Pages,class IO> class PioneerQ38HybridFullSession;
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38HybridFullProtocol.h"
#endif

// A single upload-only diagnostic session around the existing seven-bank
// owner and span transport. The source must remain authenticated and ALL
// converted cache entries preverified throughout. No token/compute START,
// validation arithmetic, driver swap, retry, reset or automatic END on a
// failure. After any uncertain call, close once only: the driver is solely
// responsible for retaining/quarantining uncertain resources. Begin leaves
// the uploaded weights held after SPAN_END; Finish explicitly releases them.
// The caller must retain the same canonical plan until Finish or destruction.
// Its public interface has no inference or warm-load operation; the separate
// candidate-only full session is a friend so it can retain the same FD.
template<class Pages,class IO,bool Tracked=false> class PioneerQ38HybridUploadSession {
#ifdef PIONEER_Q38_HYBRID_FULL_CLIENT
 friend class PioneerQ38HybridFullSession<Pages,IO>;
#endif
#ifndef PIONEER_Q38_HYBRID_TRACKED_UPLOAD_CLIENT
 static_assert(!Tracked,"tracked hybrid upload requires its distinct client selection");
#endif
 using C=PioneerQ38CanonicalModelPlan;
 enum State {Virgin,Open,Owned,SpansClosed,BanksClosed,Done,Failed};
 Pages& pages;IO& io;const C* layout=nullptr;int fd=-1;State state=Virgin;uint64_t generation=0;
 PioneerQ38HybridSpanUpload<Pages,IO> upload;
 bool attempted=false;
#ifdef PIONEER_Q38_HYBRID_CANDIDATE_UPLOAD_CLIENT
#ifdef PIONEER_Q38_HYBRID_FIXED_126
 static_assert(PIONEER_Q38_HYBRID_FIXED_126==1,"exact fixed candidate selection required");
 static constexpr uint64_t TrackedTag=UINT64_C(0x2026110700010126);
#else
 static constexpr uint64_t TrackedTag=UINT64_C(0x2026110700010125);
#endif
#else
 static constexpr uint64_t TrackedTag=UINT64_C(0x51544b4e4f445256); // non-loadable context only
#endif
 bool Stop(std::string& error,const char* reason){
  if(error.empty())error=reason;
  state=Failed;
  if(fd>=0){const int old=fd;fd=-1;io.Close(old);} // CLOSE ONLY, no device control/retry
  return false;
 }
 template<class T>bool Call(unsigned op,T& packet){
  return fd>=0 && state!=Failed && pages.AllVerified() && io.Call(fd,op,&packet,sizeof(packet)) && pages.AllVerified();
 }
 bool Identity(unsigned owner){
  tt_pioneer_driver_reload q={};q.version=1;
  if(!Call(TT_PCI_PROBE_RELOAD_STATUS,q) || !PioneerDriverReloadRequest(q) || q.expected_tag || q.expected_generation
   || q.driver!=55 || q.guard_version!=1 || q.build_tag!=(Tracked?TrackedTag:UINT64_C(0x2026102100010012))
   || !q.generation || (generation && generation!=q.generation) || q.quiesced || q.cold
   || q.active_owner!=owner || q.open_count!=1)return false;
  generation=q.generation;return true;
 }
#ifdef PIONEER_Q38_HYBRID_TRACKED_UPLOAD_CLIENT
 bool Receipt(unsigned operation,unsigned expectedSpans,bool sealed){
  namespace H=PioneerQ38HybridUpload;H::Packet p;p.operation=operation;
  return H::Request(p) && Call(TT_PCI_PROBE_Q38_HYBRID_UPLOAD_CONTROL,p)
   && p.magic==H::Magic && p.version==H::Version && p.operation==operation && !p.reserved
   && p.accepted==1 && p.sealed==unsigned(sealed) && p.spans==expectedSpans && !p.recovery_required;
 }
#endif
public:
 PioneerQ38HybridUploadSession(Pages& prepared,IO& transport):pages(prepared),io(transport){}
 PioneerQ38HybridUploadSession(const PioneerQ38HybridUploadSession&)=delete;
 PioneerQ38HybridUploadSession& operator=(const PioneerQ38HybridUploadSession&)=delete;
 ~PioneerQ38HybridUploadSession(){if(fd>=0)io.Close(fd);} // close only, never implicit END
 bool Complete()const{return state==Done;}
 bool FailedState()const{return state==Failed;}
 bool WeightsHeld()const{return state==SpansClosed && fd>=0 && layout && layout->Ready()
  && pages.AllVerified() && upload.CompleteHeld();}
 enum {RootManifestWords=64*3*7};
 // Address table for the experimental ROOT, derived from this session's
 // actual uploaded parts while the SAME source and bank owner remain held.
 // This is data only: it does not install firmware or authorize inference.
 bool RootManifest(uint32_t* output,unsigned words)const{
  if(!output || words!=RootManifestWords || !WeightsHeld())return false;
  const uintptr_t out=reinterpret_cast<uintptr_t>(output),self=reinterpret_cast<uintptr_t>(this);
  const uintptr_t source=reinterpret_cast<uintptr_t>(&pages),model=reinterpret_cast<uintptr_t>(layout);
  const size_t bytes=sizeof(uint32_t)*RootManifestWords;
  if((out&3) || out>UINTPTR_MAX-bytes || self>UINTPTR_MAX-sizeof(*this)
   || source>UINTPTR_MAX-sizeof(pages) || model>UINTPTR_MAX-sizeof(*layout)
   || (out<self+sizeof(*this) && self<out+bytes) || (out<source+sizeof(pages) && source<out+bytes)
   || (out<model+sizeof(*layout) && model<out+bytes))return false;
  PioneerQ38HybridModelPlan::Proposal expected;
  if(!PioneerQ38HybridModelPlan::Build(*layout,UINT64_C(0x1fff000000000000),expected)
   || expected.parts.size()!=layout->Count())return false;
  uint32_t staged[RootManifestWords]={};unsigned found=0;
  for(unsigned i=0;i<layout->Count();i++){
   const auto* original=layout->Part(i);const auto* actual=pages.Part(i);
   if(!original || !actual)return false;
   const uintptr_t address=reinterpret_cast<uintptr_t>(actual);
   if(address>UINTPTR_MAX-sizeof(*actual) || (out<address+sizeof(*actual) && address<out+bytes))return false;
   const auto& want=expected.parts[i];
   if(actual->original!=want.original || actual->bank!=want.bank || actual->offset!=want.offset
    || actual->bytes!=want.bytes || actual->allocated!=want.allocated || actual->encoding!=want.encoding)return false;
   if(original->kind!=C::WeightKind || original->layer>=64 || original->role<2 || original->role>4)continue;
   const bool eight=original->layer>=48 && original->layer<=60;
   if(original->bank>=7 || actual->original!=i || actual->bank!=original->bank
    || actual->encoding!=(eight?PioneerQ38HybridModelPlan::Bfp8:PioneerQ38HybridModelPlan::Bfp4)
    || !actual->allocated || (actual->offset&4095)
    || actual->offset>UINT32_MAX-C::Base || actual->offset>C::Capacity
    || actual->allocated>C::Capacity-actual->offset)return false;
   const unsigned at=(original->layer*3+original->role-2)*7+original->bank;
   if(staged[at])return false;
   staged[at]=C::Base+actual->offset;found++;
  }
  if(found!=RootManifestWords || !WeightsHeld())return false;
  std::memcpy(output,staged,bytes);return true;
 }
 const PioneerQ38HybridSpanUpload<Pages,IO>& Upload()const{return upload;}
 bool Begin(const C& canonical,std::string& error,const std::function<void(uint64_t,uint64_t)>& progress={}){
  if(attempted){error="Hybrid upload session is one-shot; no retry.";return false;}
  attempted=true;error.clear();
  if(!canonical.Ready() || !pages.AllVerified())return Stop(error,"Preverified model and caches required before device open.");
  fd=io.Open();if(fd<0)return Stop(error,"Cannot open the P100A diagnostic device.");
  state=Open;
  if(!Identity(0))return Stop(error,"Driver identity or exclusive idle state refused.");
#ifdef PIONEER_Q38_HYBRID_FULL_CLIENT
  if constexpr(Tracked){
   PioneerQ38FullOffload::Support support={};
   if(!Call(TT_PCI_PROBE_Q38_FULL_OFFLOAD_SUPPORT,support) || !PioneerQ38FullOffload::Supported(support))
    return Stop(error,"Hybrid full-card capability refused before upload.");
  }
#endif
  tt_q38_bank_support banks={};tt_q38_span_support spans={};
  if(!Call(TT_PCI_PROBE_Q38_BANK_SUPPORT,banks) || banks.driver!=45 || banks.version!=1 || banks.banks!=7
   || banks.page_bytes!=4096 || banks.max_pages!=4 || banks.storage_only!=1 || banks.bytes_per_bank!=C::Capacity
   || !Call(TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT,spans) || spans.driver!=55 || spans.version!=1 || spans.banks!=7
   || spans.page_bytes!=4096 || spans.max_bytes!=TT_Q38_SPAN_PUBLIC_BYTES || spans.before_cohort!=1
   || spans.explicit_end!=1 || spans.reserved)return Stop(error,"Bank/span capabilities refused.");
  tt_q38_bank_session begin={};begin.version=1;
  if(!Call(TT_PCI_PROBE_Q38_BANK_BEGIN,begin) || begin.version!=1 || begin.reserved || begin.result.version!=1
   || begin.result.state!=TT_RESIDENT_SESSION_READY || begin.result.active!=1 || begin.result.host_ready!=1
   || begin.result.cold || begin.result.capabilities || begin.result.bytes!=PioneerResidentBanks::Total)
   return Stop(error,"Bank BEGIN uncertain; close only.");
  state=Owned;
#ifdef PIONEER_Q38_HYBRID_TRACKED_UPLOAD_CLIENT
  if constexpr(Tracked)if(!Receipt(PioneerQ38HybridUpload::Begin,0,false))
   return Stop(error,"Tracked hybrid receipt BEGIN uncertain; close only.");
#endif
  if(!upload.Run(pages,canonical,io,fd,error,progress) || !upload.CompleteHeld()
   || upload.UploadedParts()!=3551 || upload.UploadedSpans()!=22005 || upload.UploadedBytes()!=UINT64_C(21137039360))
   return Stop(error,"Hybrid upload uncertain; close only.");
  tt_q38_span_end spanEnd={1,0,0,0};
  if(!Call(TT_PCI_PROBE_Q38_SPAN_END,spanEnd) || spanEnd.version!=1 || spanEnd.reserved
   || spanEnd.closed!=1 || spanEnd.recovery_required)return Stop(error,"Span END uncertain; close only.");
#ifdef PIONEER_Q38_HYBRID_TRACKED_UPLOAD_CLIENT
  if constexpr(Tracked)if(!Receipt(PioneerQ38HybridUpload::Seal,upload.UploadedSpans(),true))
   return Stop(error,"Tracked hybrid receipt SEAL uncertain; close only.");
#endif
  layout=&canonical;state=SpansClosed;error.clear();return true;
 }
 bool Finish(std::string& error){
  if(state!=SpansClosed){error="No held hybrid upload to finish.";return false;}
  error.clear();
  if(!WeightsHeld())return Stop(error,"Held hybrid source or upload changed; close only.");
  tt_q38_bank_session bankEnd={};bankEnd.version=1;
  if(!Call(TT_PCI_PROBE_Q38_BANK_END,bankEnd) || bankEnd.version!=1 || bankEnd.reserved || bankEnd.result.version!=1
   || bankEnd.result.state!=TT_RESIDENT_SESSION_CLOSED || bankEnd.result.active || bankEnd.result.cold
   || bankEnd.result.pci_restored!=1 || bankEnd.result.host_released!=1)return Stop(error,"Bank END uncertain; close only.");
  state=BanksClosed;if(!Identity(0))return Stop(error,"Post-END driver state uncertain; close only.");
  const int owned=fd;fd=-1;if(io.Close(owned)!=0)return Stop(error,"Device close failed.");
  state=Done;error.clear();return true;
 }
 bool Run(const C& canonical,std::string& error,const std::function<void(uint64_t,uint64_t)>& progress={}){
  return Begin(canonical,error,progress) && Finish(error);
 }
};
#endif
