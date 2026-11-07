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

// A single upload-only diagnostic session around the existing seven-bank
// owner and span transport. The source must remain authenticated and ALL
// converted cache entries preverified throughout. No token/compute START,
// validation arithmetic, driver swap, retry, reset or automatic END on a
// failure. After any uncertain call, close once only: the driver is solely
// responsible for retaining/quarantining uncertain resources. A clean END
// deliberately releases weights; it is not an inference or warm-load path.
template<class Pages,class IO> class PioneerQ38HybridUploadSession {
 using C=PioneerQ38CanonicalModelPlan;
 enum State {Virgin,Open,Owned,SpansClosed,BanksClosed,Done,Failed};
 Pages& pages;IO& io;int fd=-1;State state=Virgin;uint64_t generation=0;
 PioneerQ38HybridSpanUpload<Pages,IO> upload;
 bool attempted=false;
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
   || q.driver!=55 || q.guard_version!=1 || q.build_tag!=UINT64_C(0x2026102100010012)
   || !q.generation || (generation && generation!=q.generation) || q.quiesced || q.cold
   || q.active_owner!=owner || q.open_count!=1)return false;
  generation=q.generation;return true;
 }
public:
 PioneerQ38HybridUploadSession(Pages& prepared,IO& transport):pages(prepared),io(transport){}
 PioneerQ38HybridUploadSession(const PioneerQ38HybridUploadSession&)=delete;
 PioneerQ38HybridUploadSession& operator=(const PioneerQ38HybridUploadSession&)=delete;
 ~PioneerQ38HybridUploadSession(){if(fd>=0)io.Close(fd);} // close only, never implicit END
 bool Complete()const{return state==Done;}
 bool FailedState()const{return state==Failed;}
 const PioneerQ38HybridSpanUpload<Pages,IO>& Upload()const{return upload;}
 bool Run(const C& canonical,std::string& error,const std::function<void(uint64_t,uint64_t)>& progress={}){
  if(attempted){error="Hybrid upload session is one-shot; no retry.";return false;}
  attempted=true;error.clear();
  if(!canonical.Ready() || !pages.AllVerified())return Stop(error,"Preverified model and caches required before device open.");
  fd=io.Open();if(fd<0)return Stop(error,"Cannot open the P100A diagnostic device.");
  state=Open;
  if(!Identity(0))return Stop(error,"Driver identity or exclusive idle state refused.");
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
  if(!upload.Run(pages,canonical,io,fd,error,progress) || !upload.CompleteHeld()
   || upload.UploadedParts()!=3551 || upload.UploadedBytes()!=UINT64_C(21137039360))
   return Stop(error,"Hybrid upload uncertain; close only.");
  tt_q38_span_end spanEnd={1,0,0,0};
  if(!Call(TT_PCI_PROBE_Q38_SPAN_END,spanEnd) || spanEnd.version!=1 || spanEnd.reserved
   || spanEnd.closed!=1 || spanEnd.recovery_required)return Stop(error,"Span END uncertain; close only.");
  state=SpansClosed;tt_q38_bank_session bankEnd={};bankEnd.version=1;
  if(!Call(TT_PCI_PROBE_Q38_BANK_END,bankEnd) || bankEnd.version!=1 || bankEnd.reserved || bankEnd.result.version!=1
   || bankEnd.result.state!=TT_RESIDENT_SESSION_CLOSED || bankEnd.result.active || bankEnd.result.cold
   || bankEnd.result.pci_restored!=1 || bankEnd.result.host_released!=1)return Stop(error,"Bank END uncertain; close only.");
  state=BanksClosed;if(!Identity(0))return Stop(error,"Post-END driver state uncertain; close only.");
  const int owned=fd;fd=-1;if(io.Close(owned)!=0)return Stop(error,"Device close failed.");
  state=Done;error.clear();return true;
 }
};
#endif
