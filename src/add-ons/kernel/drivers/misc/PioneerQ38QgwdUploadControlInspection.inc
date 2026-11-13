/* SPDX-License-Identifier: MIT */
#include "../../../../../tools/pioneer/PioneerQ38QgwdNativeInspectionRole.h"
#if !defined(PIONEER_Q38_QGWD_CONTROL_INSPECTION) || PIONEER_Q38_QGWD_CONTROL_INSPECTION != 1 \
 || !defined(PIONEER_Q38_QGWD_SPAN_INSPECTION) || PIONEER_Q38_QGWD_SPAN_INSPECTION != 1 \
 || !defined(PIONEER_Q38_QGW8_SPAN_INSPECTION) || PIONEER_Q38_QGW8_SPAN_INSPECTION != 1 \
 || (defined(_KERNEL_MODE) && !defined(PIONEER_Q38_QGWD_NATIVE_INSPECTION) && (!defined(PIONEER_Q38_HYBRID_KERNEL_COMPILE_ONLY) || PIONEER_Q38_HYBRID_KERNEL_COMPILE_ONLY != 1)) \
 || ((defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT)) && !defined(PIONEER_Q38_QGWD_NATIVE_INSPECTION)) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE)
#error QGWD upload control is isolated simulated ownership only
#endif
#include "PioneerQ38QgwdUploadProtocol.h"
// A distinct gapped-ledger control. It is intentionally not hooked to a
// real ioctl: a candidate driver must bind this same cookie/team/generation,
// receipt, dispatch owner and cleanup before a physical run is permitted.
static __attribute__((noinline)) status_t Q38QgwdUploadControlInspection(PioneerProbeCookie* cookie,void* data,size_t length){
 namespace H=PioneerQ38QgwdUpload;
 if(geteuid()!=0)return B_NOT_ALLOWED;
 if(!cookie || cookie->magic!=0x50313030 || !data || length!=sizeof(H::Packet))return B_BAD_VALUE;
 thread_info thread={};status_t status=get_thread_info(find_thread(NULL),&thread);
 if(status!=B_OK || thread.team!=cookie->team)return status!=B_OK?status:B_NOT_ALLOWED;
 H::Packet packet;status=user_memcpy(&packet,data,sizeof(packet));
 if(status!=B_OK || !H::Request(packet))return status!=B_OK?status:B_BAD_VALUE;
 status=AcquireProbeLock(cookie,true);if(status!=B_OK)return status;
 auto* c=cookie->context;bool okay=false;
 if(c && sRuntimeOwner==cookie && sBankStorageOnly && !sColdRecoveryRequired && !sReloadQuiesced && sReloadGeneration
  && PioneerQ38SpanOwner(*c) && !c->q38TokenNative && !c->q38HybridReceipt && !c->q38Qgw8Receipt
#ifdef PIONEER_Q38_RECURRENT_KERNEL_COMPILE_ONLY
  && !c->q38SmallRecurrentReceipt
#endif
  && !c->Unsafe() && c->io.SpanBeforeCohort()){
  if(packet.operation==H::Begin && !c->q38QgwdReceipt && !c->q38Span && !c->uploadedPages){
   auto* receipt=new(std::nothrow) PioneerQ38DenseQ8UploadReceiptInspection;
   if(receipt && receipt->Begin(c,cookie,c->hashes,c->initialized,cookie->team,sReloadGeneration,
    c->banks,c->baseline.baseline.baseline.baseline.dram,uint32(c->bytes/4096))){
    c->q38QgwdReceipt=receipt;okay=true;
   }else delete receipt;
  }else if(packet.operation==H::Seal && c->q38QgwdReceipt && c->q38Span){
   if(c->q38QgwdReceipt->Seal(*c->q38Span)){
    okay=c->q38QgwdReceipt->Revalidate(c,cookie,c->hashes,c->initialized,cookie->team,sReloadGeneration);
    if(!okay)c->q38QgwdReceipt->Invalidate();
   }
  }
 }
 if(okay){
  packet.accepted=1;packet.sealed=unsigned(c->q38QgwdReceipt->Sealed());packet.spans=c->q38QgwdReceipt->Spans();
  status=user_memcpy(data,&packet,sizeof(packet));
  if(status!=B_OK){c->q38QgwdReceipt->Invalidate();tt_resident_session_result retained={};RetainQ38DispatchOwner(*c,retained);}
 }else{
  status=c && c->Unsafe()?B_DEV_NOT_READY:B_NOT_ALLOWED;
  if(c && c->Unsafe()){tt_resident_session_result retained={};RetainQ38DispatchOwner(*c,retained);}
 }
 release_sem(sLock);return status;
}
