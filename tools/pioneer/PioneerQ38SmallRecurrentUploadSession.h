/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_SMALL_RECURRENT_UPLOAD_SESSION_H
#define PIONEER_Q38_SMALL_RECURRENT_UPLOAD_SESSION_H
#if !defined(PIONEER_Q38_SMALL_RECURRENT_SESSION_INSPECTION) || PIONEER_Q38_SMALL_RECURRENT_SESSION_INSPECTION != 1 \
 || !defined(PIONEER_Q38_SMALL_RECURRENT_SPAN_INSPECTION) || PIONEER_Q38_SMALL_RECURRENT_SPAN_INSPECTION != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) \
 || defined(PIONEER_Q38_BF16_FULL_NATIVE) || defined(PIONEER_Q38_BF16_FULL_CLIENT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Small recurrent session is simulated inspection only
#endif
#include "PioneerQ38SmallRecurrentSpanUpload.h"
#include "PioneerQ38SmallRecurrentUploadProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
#include <functional>

// Exact fake compile-profile identity; this tag does not qualify hardware.
// A live descriptor holds the session after successful upload. Uncertainty
// closes once and never retries, ends or resets the potentially owned device.
template<class Pages,class IO> class PioneerQ38HybridFullSession;
template<class Pages,class IO> class PioneerQ38SmallRecurrentUploadSession {
 friend class PioneerQ38HybridFullSession<Pages,IO>;
 using C=PioneerQ38CanonicalModelPlan;
 enum State{Virgin,Open,Owned,Held,Done,Failed};
 Pages& pages;IO& io;PioneerQ38SmallRecurrentSpanUpload<Pages,IO> upload;
 const C* canonical=nullptr;int fd=-1;State state=Virgin;uint64_t generation=0;bool attempted=false;
 bool Stop(std::string& error,const char* reason){if(error.empty())error=reason;state=Failed;
  if(fd>=0){const int old=fd;fd=-1;io.Close(old);}return false;}
 template<class T>bool Call(unsigned op,T& packet){return fd>=0 && state!=Failed && pages.AllVerified()
  && io.Call(fd,op,&packet,sizeof(packet)) && pages.AllVerified();}
 bool Identity(unsigned owner=0){tt_pioneer_driver_reload q={};q.version=1;
  if(!Call(TT_PCI_PROBE_RELOAD_STATUS,q) || !PioneerDriverReloadRequest(q) || q.expected_tag || q.expected_generation
   || q.driver!=55 || q.guard_version!=1 || q.build_tag!=UINT64_C(0x51544b4e4f445256) || !q.generation
   || (generation && generation!=q.generation) || q.quiesced || q.cold || q.active_owner!=owner || q.open_count!=1)return false;
  generation=q.generation;return true;}
 bool Receipt(unsigned op,unsigned expected,bool sealed){namespace Q=PioneerQ38SmallRecurrentUpload;Q::Packet p;p.operation=op;
  return Q::Request(p) && Call(TT_PCI_PROBE_Q38_SMALL_RECURRENT_UPLOAD_CONTROL,p)
   && p.magic==Q::Magic && p.version==Q::Version && p.operation==op && !p.reserved && p.accepted==1
   && p.sealed==unsigned(sealed) && p.spans==expected && !p.recoveryRequired;}
public:
 static constexpr uint64_t InspectionTag=UINT64_C(0x51544b4e4f445256);
 PioneerQ38SmallRecurrentUploadSession(Pages& p,IO& i):pages(p),io(i){}
 PioneerQ38SmallRecurrentUploadSession(const PioneerQ38SmallRecurrentUploadSession&)=delete;
 PioneerQ38SmallRecurrentUploadSession& operator=(const PioneerQ38SmallRecurrentUploadSession&)=delete;
 ~PioneerQ38SmallRecurrentUploadSession(){if(fd>=0)io.Close(fd);}
 bool WeightsHeld()const{return state==Held && fd>=0 && canonical && canonical->OriginalFormatLayout()
  && pages.AllVerified() && upload.CompleteHeld();}
 bool FailedState()const{return state==Failed;}
 bool Begin(const C& source,std::string& error,const std::function<void(uint64_t,uint64_t)>& progress={}){
  if(attempted){error="Small recurrent session is one-shot.";return false;}attempted=true;error.clear();
  if(!source.OriginalFormatLayout() || !pages.AllVerified())return Stop(error,"Preverified source required before opening.");
  fd=io.Open();if(fd<0)return Stop(error,"Cannot open inspection transport.");state=Open;
  if(!Identity())return Stop(error,"Driver identity or exclusive idle state refused.");
  tt_q38_bank_support banks={};tt_q38_span_support spans={};
  if(!Call(TT_PCI_PROBE_Q38_BANK_SUPPORT,banks) || banks.driver!=45 || banks.version!=1 || banks.banks!=7
   || banks.page_bytes!=4096 || banks.max_pages!=4 || banks.storage_only!=1 || banks.bytes_per_bank!=C::Capacity
   || !Call(TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT,spans) || spans.driver!=55 || spans.version!=1 || spans.banks!=7
   || spans.page_bytes!=4096 || spans.max_bytes!=TT_Q38_SPAN_PUBLIC_BYTES || spans.before_cohort!=1
   || spans.explicit_end!=1 || spans.reserved)return Stop(error,"Bank/span capability refused.");
  tt_q38_bank_session begin={};begin.version=1;
  if(!Call(TT_PCI_PROBE_Q38_BANK_BEGIN,begin) || begin.version!=1 || begin.reserved || begin.result.version!=1
   || begin.result.state!=TT_RESIDENT_SESSION_READY || begin.result.active!=1 || begin.result.host_ready!=1
   || begin.result.cold || begin.result.capabilities || begin.result.bytes!=PioneerResidentBanks::Total)
   return Stop(error,"Bank BEGIN uncertain; close only.");
  state=Owned;namespace Q=PioneerQ38SmallRecurrentUpload;
  if(!Receipt(Q::Begin,0,false))return Stop(error,"Receipt BEGIN uncertain; close only.");
  if(!upload.Run(pages,source,io,fd,error,progress) || !upload.CompleteHeld())return Stop(error,"Upload uncertain; close only.");
  tt_q38_span_end end={1,0,0,0};
  if(!Call(TT_PCI_PROBE_Q38_SPAN_END,end) || end.version!=1 || end.reserved || end.closed!=1 || end.recovery_required)
   return Stop(error,"Span END uncertain; close only.");
  if(!Receipt(Q::Seal,upload.UploadedSpans(),true))return Stop(error,"Receipt SEAL uncertain; close only.");
  canonical=&source;state=Held;return true;
 }
 bool Finish(std::string& error){
  error.clear();if(!WeightsHeld())return Stop(error,"No verified held model; close only.");
  tt_q38_bank_session end={};end.version=1;
  if(!Call(TT_PCI_PROBE_Q38_BANK_END,end) || end.version!=1 || end.reserved || end.result.version!=1
   || end.result.state!=TT_RESIDENT_SESSION_CLOSED || end.result.active || end.result.cold
   || end.result.pci_restored!=1 || end.result.host_released!=1)return Stop(error,"Bank END uncertain; close only.");
  if(!Identity())return Stop(error,"Post-END identity uncertain; close only.");
  const int old=fd;fd=-1;if(io.Close(old)!=0)return Stop(error,"Close failed.");state=Done;return true;
 }
};
#endif
