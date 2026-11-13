/* SPDX-License-Identifier: MIT */
#include "../../../../../tools/pioneer/PioneerQ38RecurrentKernelCompileRole.h"
#if !defined(PIONEER_Q38_SMALL_RECURRENT_SPAN_INSPECTION) || PIONEER_Q38_SMALL_RECURRENT_SPAN_INSPECTION != 1 \
 || ((defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT)) && !defined(PIONEER_Q38_RECURRENT_KERNEL_COMPILE_ONLY)) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Small recurrent upload control is non-loadable inspection only
#endif
#include "../../../../../tools/pioneer/PioneerQ38SmallRecurrentUploadProtocol.h"
// No ioctl registration or device authority: caller and context must be
// supplied by an isolated compile/fixture profile with the global lock.
static __attribute__((noinline,used)) status_t Q38SmallRecurrentUploadControl(PioneerProbeCookie* cookie,void* data,size_t length){
 namespace H=PioneerQ38SmallRecurrentUpload;
 if(geteuid()!=0)return B_NOT_ALLOWED;
 if(!cookie || cookie->magic!=0x50313030 || !data || length!=sizeof(H::Packet))return B_BAD_VALUE;
 thread_info thread={};status_t status=get_thread_info(find_thread(NULL),&thread);
 if(status!=B_OK || thread.team!=cookie->team)return status!=B_OK?status:B_NOT_ALLOWED;
 H::Packet packet;status=user_memcpy(&packet,data,sizeof(packet));
 if(status!=B_OK || !H::Request(packet))return status!=B_OK?status:B_BAD_VALUE;
 status=AcquireProbeLock(cookie,true);if(status!=B_OK)return status;
 auto* c=cookie->context;bool okay=false;
 if(c && sRuntimeOwner==cookie && sBankStorageOnly && !sColdRecoveryRequired && !sReloadQuiesced && sReloadGeneration
  && PioneerQ38SpanOwner(*c) && !c->q38TokenNative && !c->q38HybridReceipt && !c->q38Qgw8Receipt && !c->q38QgwdReceipt
#ifdef PIONEER_Q38_RECURRENT_Q8_CONTROL_INSPECTION
  && !c->q38RecurrentReceipt
#endif
  && !c->Unsafe() && c->io.SpanBeforeCohort()){
  if(packet.operation==H::Begin && !c->q38SmallRecurrentReceipt && !c->q38Span && !c->uploadedPages){
   auto* receipt=new(std::nothrow) PioneerQ38SmallRecurrentUploadReceiptInspection;
   if(receipt && receipt->Begin(c,cookie,c->hashes,c->initialized,cookie->team,sReloadGeneration,
    c->banks,c->baseline.baseline.baseline.baseline.dram,uint32(c->bytes/4096))){
    c->q38SmallRecurrentReceipt=receipt;okay=true;
   }else delete receipt;
  }else if(packet.operation==H::Seal && c->q38SmallRecurrentReceipt && c->q38Span){
   if(c->q38SmallRecurrentReceipt->Seal(*c->q38Span)){
    okay=c->q38SmallRecurrentReceipt->Revalidate(c,cookie,c->hashes,c->initialized,cookie->team,sReloadGeneration);
    if(!okay)c->q38SmallRecurrentReceipt->Invalidate();
   }
  }
 }
 if(okay){
  packet.accepted=1;packet.sealed=unsigned(c->q38SmallRecurrentReceipt->Sealed());packet.spans=c->q38SmallRecurrentReceipt->Spans();
  status=user_memcpy(data,&packet,sizeof(packet));
  if(status!=B_OK){c->q38SmallRecurrentReceipt->Invalidate();tt_resident_session_result retained={};RetainQ38DispatchOwner(*c,retained);}
 }else{
  status=c && c->Unsafe()?B_DEV_NOT_READY:B_NOT_ALLOWED;
  if(c && c->Unsafe()){tt_resident_session_result retained={};RetainQ38DispatchOwner(*c,retained);}
 }
 release_sem(sLock);return status;
}
