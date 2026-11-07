/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_HYBRID_TRACKED_UPLOAD
// One actual retained FD and global lock. This source assertion is supplied
// by the trusted loader; it is not a device readback or model authentication.
static __attribute__((noinline)) status_t Q38HybridUploadControl(PioneerProbeCookie* cookie,void* data,size_t length){
 namespace H=PioneerQ38HybridUpload;
 if(geteuid()!=0)return B_NOT_ALLOWED;
 if(!cookie || cookie->magic!=0x50313030 || !data || length!=sizeof(H::Packet))return B_BAD_VALUE;
 thread_info thread={};status_t status=get_thread_info(find_thread(NULL),&thread);
 if(status!=B_OK || thread.team!=cookie->team)return status!=B_OK?status:B_NOT_ALLOWED;
 H::Packet packet;status=user_memcpy(&packet,data,sizeof(packet));
 if(status!=B_OK || !H::Request(packet))return status!=B_OK?status:B_BAD_VALUE;
 status=AcquireProbeLock(cookie,true);if(status!=B_OK)return status;
 auto* c=cookie->context;bool okay=false;
 if(c && sRuntimeOwner==cookie && sBankStorageOnly && !sColdRecoveryRequired && !sReloadQuiesced && sReloadGeneration
  && PioneerQ38SpanOwner(*c) && !c->q38TokenNative && !c->Unsafe() && c->io.SpanBeforeCohort()){
  if(packet.operation==H::Begin && !c->q38HybridReceipt && !c->q38Span && !c->uploadedPages){
   auto* receipt=new(std::nothrow) PioneerQ38HybridUploadReceipt;
   if(receipt && receipt->Begin(c,cookie,c->hashes,c->initialized,cookie->team,sReloadGeneration,
    c->banks,c->baseline.baseline.baseline.baseline.dram,uint32(c->bytes/4096))){
    c->q38HybridReceipt=receipt;okay=true;
   }else delete receipt;
  }else if(packet.operation==H::Seal && c->q38HybridReceipt && c->q38Span){
   okay=c->q38HybridReceipt->Seal(*c->q38Span)
    && c->q38HybridReceipt->Matches(c,cookie,c->hashes,c->initialized,cookie->team,sReloadGeneration);
  }
 }
 if(okay){
  packet.accepted=1;packet.sealed=unsigned(c->q38HybridReceipt->Sealed());packet.spans=c->q38HybridReceipt->Spans();
  status=user_memcpy(data,&packet,sizeof(packet));
  if(status!=B_OK){c->q38HybridReceipt->Invalidate();tt_resident_session_result retained={};RetainQ38DispatchOwner(*c,retained);}
 }else{
  status=c && c->Unsafe()?B_DEV_NOT_READY:B_NOT_ALLOWED;
  if(c && c->Unsafe()){tt_resident_session_result retained={};RetainQ38DispatchOwner(*c,retained);}
 }
 release_sem(sLock);return status;
}
#endif
