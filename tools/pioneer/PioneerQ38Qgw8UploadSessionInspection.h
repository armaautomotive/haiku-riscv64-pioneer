/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_QGW8_UPLOAD_SESSION_INSPECTION_H
#define PIONEER_Q38_QGW8_UPLOAD_SESSION_INSPECTION_H
#if !defined(PIONEER_Q38_QGW8_USER_SESSION_INSPECTION) || PIONEER_Q38_QGW8_USER_SESSION_INSPECTION != 1 \
 || !defined(PIONEER_Q38_HYBRID_UPLOAD_DIAGNOSTIC) || PIONEER_Q38_HYBRID_UPLOAD_DIAGNOSTIC != 1 \
 || !defined(PIONEER_Q38_HYBRID_HEAD_UPLOAD_DIAGNOSTIC) || PIONEER_Q38_HYBRID_HEAD_UPLOAD_DIAGNOSTIC != 1 \
 || !defined(PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION) || PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION != 1 \
 || !defined(PIONEER_Q38_QGW8_LAYOUT_INSPECTION) || PIONEER_Q38_QGW8_LAYOUT_INSPECTION != 1 \
 || !defined(PIONEER_Q38_QGW8_SPAN_INSPECTION) || PIONEER_Q38_QGW8_SPAN_INSPECTION != 1 \
 || !defined(PIONEER_Q38_QGW8_CONTROL_INSPECTION) || PIONEER_Q38_QGW8_CONTROL_INSPECTION != 1 \
 || !defined(PIONEER_Q38_HYBRID_TRACKED_UPLOAD) || PIONEER_Q38_HYBRID_TRACKED_UPLOAD != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) \
 || defined(PIONEER_Q38_BF16_FULL_NATIVE) || defined(PIONEER_Q38_BF16_FULL_CLIENT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error QGW8 upload session is an isolated non-loadable inspection only
#endif
#include "PioneerQ38HybridSpanUpload.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38Qgw8UploadProtocol.h"

// One retained diagnostic session from verified QGW8 pages through the
// existing span builder to an explicitly distinct QGW8 receipt. IO.Control is
// an inspection-only interface: no real ioctl number is assigned. The fixed
// tag below is a synthetic test identity, NEVER a physical driver tag.
// After an uncertain operation, close once and never END/reset/retry here.
template<class Pages,class IO>class PioneerQ38Qgw8UploadSessionInspection {
 using C=PioneerQ38CanonicalModelPlan;using Plan=PioneerQ38HybridHeadModelPlan;
 enum State {Virgin,Open,Owned,SpansClosed,BanksClosed,Done,Failed};
 Pages& pages;IO& io;const C* canonical=nullptr;int fd=-1;State state=Virgin;uint64_t generation=0;
 PioneerQ38HybridSpanUpload<Pages,IO,PioneerQ38HybridHeadUploadPlan> upload;
 bool attempted=false;
 bool Stop(std::string& error,const char* reason){
  if(error.empty())error=reason;state=Failed;
  if(fd>=0){const int owned=fd;fd=-1;io.Close(owned);}return false;
 }
 template<class T>bool Call(unsigned op,T& packet){
  return fd>=0 && state!=Failed && pages.AllVerified() && io.Call(fd,op,&packet,sizeof(packet)) && pages.AllVerified();
 }
 bool Identity(unsigned owner){
  tt_pioneer_driver_reload q={};q.version=1;
  if(!Call(TT_PCI_PROBE_RELOAD_STATUS,q) || !PioneerDriverReloadRequest(q) || q.expected_tag || q.expected_generation
   || q.driver!=55 || q.guard_version!=1 || q.build_tag!=InspectionTag || !q.generation
   || (generation && generation!=q.generation) || q.quiesced || q.cold || q.active_owner!=owner || q.open_count!=1)return false;
  generation=q.generation;return true;
 }
 bool Receipt(unsigned operation,unsigned expectedSpans,bool sealed){
  namespace Q=PioneerQ38Qgw8Upload;Q::Packet p;p.operation=operation;
  return fd>=0 && state!=Failed && pages.AllVerified() && Q::Request(p) && io.Control(fd,p) && pages.AllVerified()
   && p.magic==Q::Magic && p.version==Q::Version && p.operation==operation && !p.reserved
   && p.accepted==1 && p.sealed==unsigned(sealed) && p.spans==expectedSpans && !p.recovery_required;
 }
public:
 static constexpr uint64_t InspectionTag=UINT64_C(0x385747514f4e4452);
 enum {RootManifestWords=64*3*7};
 PioneerQ38Qgw8UploadSessionInspection(Pages& p,IO& transport):pages(p),io(transport){}
 PioneerQ38Qgw8UploadSessionInspection(const PioneerQ38Qgw8UploadSessionInspection&)=delete;
 PioneerQ38Qgw8UploadSessionInspection& operator=(const PioneerQ38Qgw8UploadSessionInspection&)=delete;
 ~PioneerQ38Qgw8UploadSessionInspection(){if(fd>=0)io.Close(fd);}
 bool Complete()const{return state==Done;}
 bool FailedState()const{return state==Failed;}
 bool WeightsHeld()const{return state==SpansClosed && fd>=0 && canonical && canonical->Ready()
  && pages.AllVerified() && upload.CompleteHeld();}
 const PioneerQ38HybridSpanUpload<Pages,IO,PioneerQ38HybridHeadUploadPlan>& Upload()const{return upload;}
 // Only metadata derived from this held synthetic-owner session. The
 // manifest is not itself a device pointer grant, ROOT launch or inference.
 bool RootManifest(uint32_t* output,unsigned words)const{
  if(!output || words!=RootManifestWords || !WeightsHeld())return false;
  const uintptr_t out=reinterpret_cast<uintptr_t>(output),self=reinterpret_cast<uintptr_t>(this);
  const uintptr_t source=reinterpret_cast<uintptr_t>(&pages),model=reinterpret_cast<uintptr_t>(canonical);
  const size_t bytes=sizeof(uint32_t)*RootManifestWords;
  if((out&3) || out>UINTPTR_MAX-bytes || self>UINTPTR_MAX-sizeof(*this) || source>UINTPTR_MAX-sizeof(pages)
   || model>UINTPTR_MAX-sizeof(*canonical) || (out<self+sizeof(*this) && self<out+bytes)
   || (out<source+sizeof(pages) && source<out+bytes) || (out<model+sizeof(*canonical) && model<out+bytes))return false;
  Plan::Proposal expected;if(!Plan::Build(*canonical,expected) || expected.parts.size()!=canonical->Count())return false;
  for(unsigned i=0;i<canonical->Count();i++){
   const auto* p=pages.Part(i);if(!p)return false;const auto& q=expected.parts[i];
   const uintptr_t address=reinterpret_cast<uintptr_t>(p);
   if(address>UINTPTR_MAX-sizeof(*p) || (out<address+sizeof(*p) && address<out+bytes)
    || p->original!=q.original || p->bank!=q.bank || p->offset!=q.offset || p->bytes!=q.bytes
    || p->allocated!=q.allocated || p->encoding!=q.encoding)return false;
  }
  uint32_t staged[RootManifestWords]={};
  if(!Plan::Manifest(*canonical,expected,staged,RootManifestWords) || !WeightsHeld())return false;
  std::memcpy(output,staged,bytes);return true;
 }
 bool Begin(const C& source,std::string& error,const std::function<void(uint64_t,uint64_t)>& progress={}){
  if(attempted){error="QGW8 inspection session is one-shot; no retry.";return false;}
  attempted=true;error.clear();
  if(!source.OriginalFormatLayout() || !pages.AllVerified())return Stop(error,"Preverified QGW8 source required before open.");
  fd=io.Open();if(fd<0)return Stop(error,"Cannot open QGW8 inspection transport.");state=Open;
  if(!Identity(0))return Stop(error,"QGW8 inspection identity or exclusive idle state refused.");
  tt_q38_bank_support banks={};tt_q38_span_support spans={};
  if(!Call(TT_PCI_PROBE_Q38_BANK_SUPPORT,banks) || banks.driver!=45 || banks.version!=1 || banks.banks!=7
   || banks.page_bytes!=4096 || banks.max_pages!=4 || banks.storage_only!=1 || banks.bytes_per_bank!=C::Capacity
   || !Call(TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT,spans) || spans.driver!=55 || spans.version!=1 || spans.banks!=7
   || spans.page_bytes!=4096 || spans.max_bytes!=TT_Q38_SPAN_PUBLIC_BYTES || spans.before_cohort!=1
   || spans.explicit_end!=1 || spans.reserved)return Stop(error,"QGW8 inspection bank/span capability refused.");
  tt_q38_bank_session begin={};begin.version=1;
  if(!Call(TT_PCI_PROBE_Q38_BANK_BEGIN,begin) || begin.version!=1 || begin.reserved || begin.result.version!=1
   || begin.result.state!=TT_RESIDENT_SESSION_READY || begin.result.active!=1 || begin.result.host_ready!=1
   || begin.result.cold || begin.result.capabilities || begin.result.bytes!=PioneerResidentBanks::Total)
   return Stop(error,"QGW8 inspection bank BEGIN uncertain; close only.");
  state=Owned;
  if(!Receipt(PioneerQ38Qgw8Upload::Begin,0,false))return Stop(error,"QGW8 receipt BEGIN uncertain; close only.");
  if(!upload.Run(pages,source,io,fd,error,progress) || !upload.CompleteHeld() || upload.UploadedParts()!=3551
   || upload.UploadedSpans()!=22035 || upload.UploadedBytes()!=UINT64_C(21167624192))
   return Stop(error,"QGW8 inspection upload uncertain; close only.");
  tt_q38_span_end spanEnd={1,0,0,0};
  if(!Call(TT_PCI_PROBE_Q38_SPAN_END,spanEnd) || spanEnd.version!=1 || spanEnd.reserved
   || spanEnd.closed!=1 || spanEnd.recovery_required)return Stop(error,"QGW8 span END uncertain; close only.");
  if(!Receipt(PioneerQ38Qgw8Upload::Seal,upload.UploadedSpans(),true))return Stop(error,"QGW8 receipt SEAL uncertain; close only.");
  canonical=&source;state=SpansClosed;error.clear();return true;
 }
 bool Finish(std::string& error){
  if(state!=SpansClosed){error="No held QGW8 inspection upload to finish.";return false;}
  error.clear();if(!WeightsHeld())return Stop(error,"Held QGW8 source or upload changed; close only.");
  tt_q38_bank_session end={};end.version=1;
  if(!Call(TT_PCI_PROBE_Q38_BANK_END,end) || end.version!=1 || end.reserved || end.result.version!=1
   || end.result.state!=TT_RESIDENT_SESSION_CLOSED || end.result.active || end.result.cold
   || end.result.pci_restored!=1 || end.result.host_released!=1)return Stop(error,"QGW8 bank END uncertain; close only.");
  state=BanksClosed;if(!Identity(0))return Stop(error,"QGW8 post-END driver state uncertain; close only.");
  const int owned=fd;fd=-1;if(io.Close(owned)!=0)return Stop(error,"QGW8 inspection close failed.");
  state=Done;error.clear();return true;
 }
 bool Run(const C& source,std::string& error,const std::function<void(uint64_t,uint64_t)>& progress={}){
  return Begin(source,error,progress) && Finish(error);
 }
};
#endif
