/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_SMALL_RECURRENT_SPAN_UPLOAD_H
#define PIONEER_Q38_SMALL_RECURRENT_SPAN_UPLOAD_H
#if !defined(PIONEER_Q38_SMALL_RECURRENT_BFP8_PLAN_INSPECTION) || PIONEER_Q38_SMALL_RECURRENT_BFP8_PLAN_INSPECTION != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) \
 || defined(PIONEER_Q38_BF16_FULL_CLIENT) || defined(PIONEER_Q38_BF16_FULL_NATIVE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Small recurrent transport is an isolated inspection, not a loadable or Work client
#endif
#include "PioneerQ38SmallRecurrentUploadPlan.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38SpanProtocol.h"
#include <functional>
#include <memory>

// The caller owns and authenticates the bank, receipt and transport session.
// A failed or uncertain call permanently stops this object and does not grant
// permission to close, retry, end, reset or recover the caller's device.
template<class Pages,class IO> class PioneerQ38SmallRecurrentSpanUpload {
 using C=PioneerQ38CanonicalModelPlan;
 PioneerQ38SmallRecurrentUploadPlan<Pages> plan;
 bool attempted=false,touched=false,complete=false,failed=false;
 uint64_t bytes=0;unsigned spans=0;
 bool Fail(std::string& error,const char* reason){failed=true;if(error.empty())error=reason;return false;}
public:
 bool DeviceTouched()const{return touched;}
 bool CompleteHeld()const{return complete && !failed;}
 bool Failed()const{return failed;}
 uint64_t UploadedBytes()const{return bytes;}
 unsigned UploadedSpans()const{return spans;}
 bool Run(Pages& pages,const C& canonical,IO& io,int fd,std::string& error,
  const std::function<void(uint64_t,uint64_t)>& progress={}){
  if(attempted)return Fail(error,"Small recurrent upload is one-shot.");
  attempted=true;error.clear();
  if(fd<0 || !pages.AllVerified() || !plan.Bind(canonical,pages,error))return Fail(error,"Verified prepared pages and an owned transport are required.");
  auto packet=std::make_unique<tt_q38_span_upload>();
  unsigned fragment=0,bank=0,length=0;uint64_t offset=0;
  while(plan.Next(fragment,bank,offset,length)){
   std::memset(packet.get(),0,sizeof(*packet));unsigned stagedFragment=0,stagedBank=0,stagedLength=0;uint64_t stagedOffset=0;
   if(!plan.Stage(reinterpret_cast<uint8_t*>(packet->data),sizeof(packet->data),stagedFragment,stagedBank,stagedOffset,stagedLength,error)
    || stagedFragment!=fragment || stagedBank!=bank || stagedOffset!=offset || stagedLength!=length)
    return Fail(error,"Prepared span staging failed; retain any owner.");
   packet->version=TT_Q38_SPAN_PUBLIC_VERSION;packet->bank=bank;packet->offset=offset;packet->bytes=length;
   if(!PioneerQ38SpanPublicShape(packet->version,bank,offset,length,packet->reserved) || !pages.AllVerified())
    return Fail(error,"Prepared span changed before upload; retain any owner.");
   touched=true;
   if(!io.Call(fd,TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD,packet.get(),sizeof(*packet))){plan.Acknowledge(false);return Fail(error,"Uncertain span call; retain owner.");}
   const auto& r=packet->result;
   if(packet->version!=TT_Q38_SPAN_PUBLIC_VERSION || packet->bank!=bank || packet->offset!=offset
    || packet->bytes!=length || packet->reserved || r.version!=1 || r.state!=TT_RUNTIME_PASSED
    || r.bytes!=length || r.pages!=length/4096 || r.nonce!=spans+1 || r.started!=1 || r.completed!=1
    || r.held!=1 || r.host_exact || r.matched_words || r.recovery_required || r.reserved || !pages.AllVerified()){
    plan.Acknowledge(false);return Fail(error,"Uncertain span reply or source; retain owner.");
   }
   if(!plan.Acknowledge(true))return Fail(error,"Upload acknowledgement failed; retain owner.");
   bytes+=length;spans++;if(progress)progress(bytes,UINT64_C(20754878464));
  }
  if(!plan.Finished() || spans!=21475 || bytes!=UINT64_C(20754878464))return Fail(error,"Incomplete small recurrent upload; retain owner.");
  complete=true;error.clear();return true;
 }
};
#endif
