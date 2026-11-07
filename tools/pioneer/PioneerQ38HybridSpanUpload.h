/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_HYBRID_SPAN_UPLOAD_H
#define PIONEER_Q38_HYBRID_SPAN_UPLOAD_H
#if !defined(PIONEER_Q38_HYBRID_UPLOAD_DIAGNOSTIC) || PIONEER_Q38_HYBRID_UPLOAD_DIAGNOSTIC != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) \
 || defined(PIONEER_Q38_BF16_FULL_CLIENT) || defined(PIONEER_Q38_BF16_FULL_NATIVE) \
 || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_DRIVER_BUILD_TAG) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Hybrid span upload is a separate experimental diagnostic, not native or Work authority
#endif
#include "PioneerQ38HybridModelPlan.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38CanonicalModelPlan.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38SpanProtocol.h"
#include <algorithm>
#include <cstring>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>

// Preserve the existing 13-layer format as the only default. QGW8 uses a
// distinct explicitly selected diagnostic plan and cannot be silently
// accepted by existing owners, manifests or driver identities.
struct PioneerQ38HybridLegacyUploadPlan {
 using Proposal=PioneerQ38HybridModelPlan::Proposal;
 template<class Canonical>static bool Build(const Canonical& source,Proposal& output){
  return PioneerQ38HybridModelPlan::Build(source,UINT64_C(0x1fff000000000000),output);
 }
};
#ifdef PIONEER_Q38_HYBRID_HEAD_UPLOAD_DIAGNOSTIC
#if PIONEER_Q38_HYBRID_HEAD_UPLOAD_DIAGNOSTIC != 1 \
 || !defined(PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION) || PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION != 1
#error QGW8 span upload requires its distinct head-plan inspection role
#endif
#include "PioneerQ38HybridHeadModelPlan.h"
struct PioneerQ38HybridHeadUploadPlan {
 using Proposal=PioneerQ38HybridHeadModelPlan::Proposal;
 template<class Canonical>static bool Build(const Canonical& source,Proposal& output){
  return PioneerQ38HybridHeadModelPlan::Build(source,output);
 }
};
#endif

// One-shot adapter from preverified hybrid pages to the EXISTING Work span
// wire protocol, for a separately qualified experimental userspace owner.
// The caller must already hold the same exclusive, fresh seven-bank session;
// this object cannot establish bank/driver/source authority or end the span.
// It never issues BEGIN, END, START, reset, retry, close or recovery. A failed
// or uncertain Call is NOT safe release: the caller must retain its owner and
// obey the driver's quarantine/recovery rules. Success likewise leaves the
// span and bank owner held; it is not inference or model-quality evidence.
template<class Pages,class IO,class Plan=PioneerQ38HybridLegacyUploadPlan> class PioneerQ38HybridSpanUpload {
#ifdef PIONEER_Q38_HYBRID_HEAD_UPLOAD_DIAGNOSTIC
 static_assert(std::is_same<Plan,PioneerQ38HybridLegacyUploadPlan>::value || std::is_same<Plan,PioneerQ38HybridHeadUploadPlan>::value,
  "Only explicit hybrid upload plans are supported");
#else
 static_assert(std::is_same<Plan,PioneerQ38HybridLegacyUploadPlan>::value,"Only the legacy hybrid upload plan is enabled");
#endif
 using C=PioneerQ38CanonicalModelPlan;
 bool attempted=false,touched=false,complete=false,failed=false;
 uint64_t bytes=0,uploaded=0;unsigned spans=0,parts=0;
 bool Fail(std::string& error,const char* message){failed=true;error=message;return false;}
public:
 PioneerQ38HybridSpanUpload()=default;
 PioneerQ38HybridSpanUpload(const PioneerQ38HybridSpanUpload&)=delete;
 PioneerQ38HybridSpanUpload& operator=(const PioneerQ38HybridSpanUpload&)=delete;
 bool Attempted()const{return attempted;}bool DeviceTouched()const{return touched;}
 bool CompleteHeld()const{return complete && !failed;}bool Failed()const{return failed;}
 uint64_t PlannedBytes()const{return bytes;}uint64_t UploadedBytes()const{return uploaded;}
 unsigned UploadedSpans()const{return spans;}unsigned UploadedParts()const{return parts;}
 bool Run(Pages& pages,const C& canonical,IO& io,int fd,std::string& error,
  const std::function<void(uint64_t,uint64_t)>& progress={}){
  if(attempted)return Fail(error,"Hybrid upload is one-shot; no retry.");
  attempted=true;
  if(fd<0 || !pages.AllVerified() || !canonical.Ready())return Fail(error,"Hybrid upload requires a preverified live source and caller-owned session.");
  typename Plan::Proposal expected;
  if(!Plan::Build(canonical,expected)
   || canonical.Count()!=4678 || expected.parts.size()!=canonical.Count())return Fail(error,"Invalid full-model hybrid geometry.");
  unsigned weights=0;
  for(unsigned i=0;i<canonical.Count();i++){
   const auto* old=canonical.Part(i);const auto* actual=pages.Part(i);const auto& want=expected.parts[i];
   if(!old || !actual || actual->original!=want.original || actual->bank!=want.bank || actual->offset!=want.offset
    || actual->bytes!=want.bytes || actual->allocated!=want.allocated || actual->encoding!=want.encoding)
    return Fail(error,"Prepared source and full hybrid geometry differ.");
   if(old->kind==C::WeightKind){
    if(!want.allocated || (want.allocated&4095) || bytes>UINT64_MAX-want.allocated)
     return Fail(error,"Invalid learned-page count.");
    bytes+=want.allocated;weights++;
   }
  }
  if(weights!=3551 || !bytes || !pages.AllVerified())return Fail(error,"Incomplete or changed full-model preflight.");
  auto packet=std::make_unique<tt_q38_span_upload>();
  for(unsigned i=0;i<canonical.Count();i++){
   const auto* old=canonical.Part(i);if(old->kind!=C::WeightKind)continue;
   const auto& p=expected.parts[i];
   for(unsigned at=0;at<p.allocated/4096;){
    const unsigned count=std::min(256U,p.allocated/4096-at);
    std::memset(packet.get(),0,sizeof(*packet));packet->version=TT_Q38_SPAN_PUBLIC_VERSION;
    packet->bank=p.bank;packet->offset=uint64_t(p.offset)+uint64_t(at)*4096;packet->bytes=count*4096;
    if(!PioneerQ38SpanPublicShape(packet->version,packet->bank,packet->offset,packet->bytes,packet->reserved))
     return Fail(error,"Invalid relocated span.");
    for(unsigned page=0;page<count;){
     const unsigned batch=std::min(16U,count-page);
     if(!pages.ReadPages(i,at+page,batch,reinterpret_cast<uint8_t*>(packet->data)+page*4096,batch*4096,error)){
      failed=true;if(error.empty())error="Prepared page read failed; retain any caller-owned session.";return false;
     }
     page+=batch;
    }
    if(!pages.AllVerified())return Fail(error,"Source changed before upload.");
    const unsigned bank=packet->bank,packetBytes=packet->bytes;const uint64_t offset=packet->offset;
    touched=true; // Call may have side effects even when it returns failure.
    if(!io.Call(fd,TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD,packet.get(),sizeof(*packet)))return Fail(error,"Uncertain span call; retain owner.");
    const auto& r=packet->result;
    if(packet->version!=TT_Q38_SPAN_PUBLIC_VERSION || packet->bank!=bank || packet->offset!=offset
     || packet->bytes!=packetBytes || packet->reserved || r.version!=1 || r.state!=TT_RUNTIME_PASSED
     || r.bytes!=packetBytes || r.pages!=count || r.nonce!=spans+1 || r.started!=1 || r.completed!=1
     || r.held!=1 || r.host_exact || r.matched_words || r.recovery_required || r.reserved)
     return Fail(error,"Uncertain span reply; retain owner.");
    uploaded+=packetBytes;spans++;at+=count;
    if(!pages.AllVerified())return Fail(error,"Source changed after upload; retain owner.");
    if(progress)progress(uploaded,bytes);
   }
   parts++;
  }
  if(parts!=3551 || uploaded!=bytes || !pages.AllVerified())return Fail(error,"Incomplete full-model upload; retain owner.");
  complete=true;error.clear();return true;
 }
};
#endif
