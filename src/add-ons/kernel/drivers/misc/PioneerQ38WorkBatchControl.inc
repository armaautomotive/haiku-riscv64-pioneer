/* SPDX-License-Identifier: MIT */
// Exact candidate driver role OR isolated host inspection; never both.
#if !defined(TT_PIONEER_Q38_WORK_MODE) || TT_PIONEER_Q38_WORK_MODE != 1 \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Work batch control requires Work and excludes CPU validation
#endif
#if defined(PIONEER_Q38_WORK_BATCH_NATIVE_152)
#if PIONEER_Q38_WORK_BATCH_NATIVE_152 != 1 || !defined(_KERNEL_MODE) || _KERNEL_MODE != 1 \
 || !defined(TT_PIONEER_DRIVER_RELOAD) || TT_PIONEER_DRIVER_RELOAD != 1 \
 || !defined(PIONEER_DRIVER_BUILD_TAG) || PIONEER_DRIVER_BUILD_TAG != 0x2026111300010152ULL \
 || !defined(TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR) || defined(PIONEER_Q38_WORK_BATCH_CONTROL_INSPECTION)
#error Work batch candidate requires its exact tag152 kernel role
#endif
#elif !defined(PIONEER_Q38_WORK_BATCH_CONTROL_INSPECTION) || PIONEER_Q38_WORK_BATCH_CONTROL_INSPECTION != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG)
#error Work batch control requires isolated host inspection or exact tag152
#endif
#include "PioneerQ38WorkBatch.h"
#include "PioneerQ38WorkBatchProtocol.h"
static_assert(unsigned(PioneerQ38WorkBatch::Count)==unsigned(PIONEER_Q38_WORK_BATCH_COUNT),"same bounded batch size");
#ifdef PIONEER_Q38_WORK_OUTPUT_DMA
enum { PIONEER_Q38_WORK_BATCH_OUTPUT_SLOT_BYTES=0x20000 };
static_assert(119*1024<=PIONEER_Q38_WORK_BATCH_OUTPUT_SLOT_BYTES
	&& PIONEER_Q38_WORK_BATCH_OUTPUT_SLOT_BYTES+119*1024<=TT_WIDE_OUTPUT_BYTES,
 "two maximum dense Qwen projection outputs fit in the private wide-output owner");
#endif
#ifdef PIONEER_Q38_WORK_BATCH_NATIVE_152
static status_t
Q38WorkBatchSupport(PioneerProbeCookie* cookie,void* data,size_t length)
{
 if(!cookie || !data || length!=sizeof(PioneerQ38WorkBatchSupport))return B_BAD_VALUE;
 thread_info thread={};status_t status=get_thread_info(find_thread(NULL),&thread);
 if(status!=B_OK || thread.team!=cookie->team)return status!=B_OK?status:B_NOT_ALLOWED;
 status=AcquireProbeLock(cookie,true
#ifdef PIONEER_Q38_REUSE_ACTIVATIONS
  ,true
#endif
 );
 if(status!=B_OK)return status;
 const PioneerQ38WorkBatchSupport support={PIONEER_Q38_WORK_BATCH_VERSION,PIONEER_Q38_WORK_BATCH_COUNT,
  TT_Q38_DISPATCH_DRIVER,0,PIONEER_DRIVER_BUILD_TAG};
 status=user_memcpy(data,&support,sizeof(support));release_sem(sLock);return status;
}
#endif
static status_t
Q38WorkBatchControlInspection(PioneerProbeCookie* cookie,void* data,size_t length)
{
 if(!cookie || !data || length!=sizeof(PioneerQ38WorkBatchPacket))return B_BAD_VALUE;
 thread_info thread={};status_t status=get_thread_info(find_thread(NULL),&thread);
 if(status!=B_OK || thread.team!=cookie->team)return status!=B_OK?status:B_NOT_ALLOWED;
 auto* packet=(PioneerQ38WorkBatchPacket*)malloc(sizeof(PioneerQ38WorkBatchPacket));
 if(!packet)return B_NO_MEMORY;
 status=user_memcpy(packet,data,sizeof(*packet));
 if(status!=B_OK || packet->version!=1 || packet->count!=PioneerQ38WorkBatch::Count
  || packet->reserved[0] || packet->reserved[1]){free(packet);return status!=B_OK?status:B_BAD_VALUE;}
 status=AcquireProbeLock(cookie,true
#ifdef PIONEER_Q38_REUSE_ACTIVATIONS
  ,true
#endif
 );
 if(status!=B_OK){free(packet);return status;}
 auto* c=cookie->context;
 if(!c || sRuntimeOwner!=cookie || !c->bankStorageOnly || c->bytes!=PioneerResidentBanks::Total
  || c->q38Cohort || c->panelProbe.saved || c->streamProbe.saved
#ifdef PIONEER_Q38_SPAN_PRELOAD
  || (c->q38Span && !c->q38Span->closed)
#endif
 ){release_sem(sLock);free(packet);return B_NOT_ALLOWED;}
 auto& dram=c->baseline.baseline.baseline.baseline.dram;
 const uint32_t prior=c->q38Dispatch?c->q38Dispatch->rootNonce:0;
 bool prepared=false;unsigned outputSlot=0;
 auto authorize=[&](const tt_q38_dispatch_probe& request){
  for(unsigned w=0;w<request.peers;w++){
   PioneerQ38CohortExtent extent;
   if(!PioneerQ38CohortAuthorizePart(c->banks,dram,c->initialized,uint32(c->bytes/4096),
    request.type,request.inputs,request.part[w],w+1,extent))return false;
  }
  return true;
 };
 auto run=[&](tt_q38_dispatch_probe& request){
  if(!prepared){
   prepared=PrepareQ38Dispatch(*c);
#ifdef PIONEER_Q38_REUSE_ACTIVATIONS
   if(prepared)c->q38Dispatch->inputEpoch=sQ38InputEpoch;
#endif
   if(!prepared)return false;
  }
  const bool okay=PioneerQ38DispatchRun<decltype(c->io),true>(c->io,dram,c->banks,*c->q38Dispatch,
   c->initialized,uint32(c->bytes/4096),request
#ifdef PIONEER_Q38_WORK_OUTPUT_DMA
   // Keep each sequential job's host result in its own bounded slot. This
   // is storage groundwork for a future single-root fused run; jobs remain
   // separate root launches and this does not claim a speedup.
   ,&c->host,&c->bulkHost,&c->wideHost,outputSlot?PIONEER_Q38_WORK_BATCH_OUTPUT_SLOT_BYTES:0U
#endif
  );
  if(okay)++outputSlot;
  return okay && !c->Unsafe() && !request.result.recovery_required;
 };
 auto quarantine=[&](){
  if(c->q38Dispatch)PioneerQ38DispatchQuarantine(*c->q38Dispatch,c->banks);
  c->banks.Quarantine();c->lease.recovery_required=true;c->io.QuarantineHostPage();sColdRecoveryRequired=true;
 };
 auto outcome=PioneerQ38WorkBatch::Execute(packet->jobs,packet->count,prior,authorize,run,quarantine);
 if(outcome==PioneerQ38WorkBatch::Complete && !c->io.SelectRuntimeCore(0)){
  quarantine();outcome=PioneerQ38WorkBatch::Unknown;
 }
 if(outcome==PioneerQ38WorkBatch::Complete){
  status=user_memcpy(data,packet,sizeof(*packet));
  // A failed copy may have partially modified user memory. The caller MUST
  // discard the entire reply on error; retain/quarantine the device owner.
  if(status!=B_OK)quarantine();
 }else status=outcome==PioneerQ38WorkBatch::Refused?B_NOT_ALLOWED:B_DEV_NOT_READY;
 release_sem(sLock);free(packet);return status;
}
