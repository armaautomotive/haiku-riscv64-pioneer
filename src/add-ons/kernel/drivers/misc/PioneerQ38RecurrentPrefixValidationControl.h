/* SPDX-License-Identifier: MIT */
#include "PioneerQ38RecurrentPrefixValidationControlRole.h"
#include "PioneerQ38RecurrentPrefixValidationProtocol.h"
static_assert(unsigned(TT_Q38_RECURRENT_PREFIX_VALIDATION_WORDS)==unsigned(PioneerQ38RecurrentPrefixReadbackPlan::Words)
 && unsigned(TT_Q38_RECURRENT_PREFIX_VALIDATION_PAGES)==unsigned(PioneerQ38RecurrentPrefixReadbackPlan::Pages),"fixed LAST-layer readback only");
struct PioneerQ38RecurrentPrefixValidationResults {
 PioneerQ38ProjectionZeroResult zero;PioneerQ38EmbeddingResult embedding;
 PioneerQ38StateZeroResult recurrent;PioneerQ38ConvHistoryZeroResult history;
 PioneerQ38RecurrentPrefixMemoryResult memory;PioneerQ38RecurrentPrefixResult execution;PioneerQ38RecurrentPrefixReadbackResult readback;
}; // HEAP ONLY;no20KiB request/results on a kernel stack
static void EndResidentSession(PioneerProbeCookie*,tt_resident_session_result&,bool);
// The compute-call seal must be gone BEFORE END may destroy its context.
// Reacquire the actual lock and rebind the exact context+model identity to
// refuse replacement/ABA contexts before any END IO. Never retain a foreign
// replacement just because the caller's old computation finished.
[[maybe_unused]] __attribute__((noinline)) static status_t Q38RecurrentPrefixValidationEnd(PioneerProbeCookie* cookie,
 const PioneerStoreContext* expected,const PioneerQ38GdnIdentity& identity,tt_resident_session_result& result){
 status_t status=AcquireQ38GdnSessionLock(cookie);if(status!=B_OK)return status;
 auto* current=cookie?cookie->context:NULL;
 const bool same=current && current==expected && current->q38EmbeddingModel && current->q38EmbeddingModel->identity==identity;
 struct{tt_resident_session_result result;}request={};
 if(!same)status=B_NOT_ALLOWED;
 else{
#include "PioneerQ38RecurrentPrefixFinalEndCall.inc"
  if(status!=B_OK || cookie->context || sRuntimeOwner || sColdRecoveryRequired || request.result.version!=1
   || request.result.state!=TT_RESIDENT_SESSION_CLOSED || request.result.active || request.result.cold || request.result.capabilities
   || request.result.bytes!=UINT64_C(21)*1024*1024*1024 || request.result.pci_restored!=1 || request.result.host_released!=1){
   if(cookie->context==expected){tt_resident_session_result retained={};RetainQ38DispatchOwner(*cookie->context,retained);}
   if(status==B_OK)status=B_DEV_NOT_READY;
  }
 }
 result=request.result;release_sem(sLock);return status;
} // no dereference of expected after END;all user output still private
[[maybe_unused]] __attribute__((noinline)) static status_t Q38RecurrentPrefixValidationControl(PioneerProbeCookie* cookie,uint32 operation,void* data,size_t length){
 if(!cookie || cookie->magic!=0x50313030 || !data)return B_BAD_VALUE;
 thread_info thread={};status_t status=get_thread_info(find_thread(NULL),&thread);
 if(status!=B_OK || thread.team!=cookie->team)return status!=B_OK?status:B_NOT_ALLOWED;
 if(operation==TT_PCI_PROBE_Q38_RECURRENT_PREFIX_VALIDATION_SUPPORT){
  if(length!=sizeof(tt_q38_embedding_support))return B_BAD_VALUE;
  status=AcquireQ38GdnSessionLock(cookie);if(status!=B_OK)return status;
  const tt_q38_embedding_support support={1,55,TT_Q38_RECURRENT_PREFIX_VALIDATION_NAMESPACE,
   TT_Q38_RECURRENT_PREFIX_VALIDATION_WORDS,248320,512,0,0};
  status=user_memcpy(data,&support,sizeof(support));release_sem(sLock);return status;
 }
 if(operation!=TT_PCI_PROBE_Q38_RECURRENT_PREFIX_VALIDATE || length!=sizeof(tt_q38_recurrent_prefix_validation))return B_BAD_VALUE;
 auto* reply=new(std::nothrow) tt_q38_recurrent_prefix_validation;if(!reply)return B_NO_MEMORY;
 status=user_memcpy(reply,data,sizeof(*reply));
 if(status!=B_OK || !PioneerQ38RecurrentPrefixValidationRequest(*reply)){delete reply;return status!=B_OK?status:B_BAD_VALUE;}
 auto* results=new(std::nothrow) PioneerQ38RecurrentPrefixValidationResults();
 if(!results){delete reply;return B_NO_MEMORY;}
 bool okay=false;PioneerStoreContext* expected=NULL;PioneerQ38GdnIdentity identity={};
 const uint64 began=uint64(system_time());
 {
  PioneerQ38GdnCallScope scope(cookie);auto* c=scope.Context();
  const bool hadModel=c && c->q38EmbeddingModel;
  if(!c || !BindQ38EmbeddingModel(*c,0,0,0)){
   if(c && !hadModel && c->q38EmbeddingModel){tt_resident_session_result retained={};RetainQ38DispatchOwner(*c,retained);}
   delete results;delete reply;return scope.Status()!=B_OK?scope.Status():B_NOT_ALLOWED;
  }
  auto& z=results->zero;auto& e=results->embedding;auto& g=results->recurrent;auto& h=results->history;
  auto& m=results->memory;auto& x=results->execution;auto& b=results->readback;
  okay=InitializeQ38EmbeddingModel(*c,z) && RunQ38EmbeddingModel(*c,e) && BindQ38RecurrentPrefixNative(*c)
   && InitializeQ38RecurrentPrefixState(*c,g) && InitializeQ38RecurrentPrefixHistory(*c,h)
   && PrepareQ38RecurrentPrefixNativeHeld(*c,m) && PrepareQ38RecurrentPrefixNativeExecution(*c,x)
   && RunQ38RecurrentPrefixNativeExecution(*c,x)
   && CloseQ38RecurrentPrefixNativeValidation(*c,x,m,reply->values,TT_Q38_RECURRENT_PREFIX_VALIDATION_WORDS,b);
  auto* input=c->q38EmbeddingModel;auto* native=c->q38RecurrentPrefixNative;
  constexpr unsigned packets=3*((PioneerQ38RecurrentPrefixLayout::Bytes+16383)/16384);
  static_assert(packets==57 && PioneerQ38RecurrentPrefixLayout::Layers==3,"ONLY complete immutable3layer packages");
  okay=okay && input && native && !c->Unsafe() && !input->Unsafe() && input->Closed() && native->Closed()
   && native->InitializationsClosed(c->banks,c->baseline.baseline.baseline.baseline.dram)
   && input->zero.closed && !input->zero.unsafe && z.nonce==2 && z.completed && z.held && !z.recovery_required
   && e.closed && !e.unsafe && e.completed==1 && e.held==1
   && g.state==TT_RUNTIME_PASSED && g.nonce==672 && g.started && g.completed && g.held && !g.recovery_required
   && h.state==TT_RUNTIME_PASSED && h.nonce==336 && h.pages==5 && h.started && h.completed && h.held && !h.recovery_required
   && m.closed && !m.unsafe && m.savedWords==TT_Q38_RECURRENT_PREFIX_SAVED_WORDS
   && m.verifiedOriginalWords==2*TT_Q38_RECURRENT_PREFIX_SAVED_WORDS && m.installedWords==TT_Q38_RECURRENT_PREFIX_CODE_WORDS
   && m.restoredWords==TT_Q38_RECURRENT_PREFIX_SAVED_WORDS && x.closed && !x.unsafe && x.prepared==1 && x.completed==119
   && x.armed==119 && x.released==119 && x.stopped==119 && x.held==120 && x.rootStarts==1 && x.rootHolds==1 && x.packets==packets
   && b.started && b.completed && b.pages==TT_Q38_RECURRENT_PREFIX_VALIDATION_PAGES && b.words==TT_Q38_RECURRENT_PREFIX_VALIDATION_WORDS;
  auto& r=reply->metadata;
  r.session=input?input->identity.session:0;r.driver_generation=input?input->identity.driverGeneration:0;
  if(okay){
   for(unsigned i=0;i<PioneerResidentBanks::Pages;i++)if(c->initialized[i]==1)r.source_pages++;
   okay=r.source_pages==205785;
  }
  if(okay){
   expected=c;identity=input->identity;r.workspace_pages=512;r.saved_words=e.savedWords+m.savedWords;r.restored_words=e.restoredWords+m.restoredWords;
   reply->zero_runs=z.nonce;reply->embedding_roots=e.completed;reply->recurrent_regions=g.nonce;reply->history_regions=h.nonce;
   reply->recurrent_pages=73728;reply->history_pages=1680;
   reply->cohort_prepared=x.prepared;reply->cohort_completed=x.completed;reply->cohort_stopped=x.stopped;reply->cohort_held=x.held;
   reply->root_runs=x.rootStarts;reply->root_holds=x.rootHolds;reply->packets=x.packets;
   reply->saved_words=m.savedWords;reply->verified_words=m.verifiedOriginalWords;reply->installed_words=m.installedWords;reply->restored_words=m.restoredWords;
   reply->readback_pages=b.pages;reply->readback_words=b.words;reply->final_rows=5120;
   reply->layers_completed=PioneerQ38RecurrentPrefixLayout::Layers;reply->matrices_completed=8*PioneerQ38RecurrentPrefixLayout::Layers;reply->root_stop_request=4;
  }else{
   tt_resident_session_result retained={};RetainQ38DispatchOwner(*c,retained); // MEMORY ONLY,no post-failure device IO
  }
 } // actual compute-call scope released;NEVER outlive END's context destruction
 if(okay){
  tt_resident_session_result ended={};status=Q38RecurrentPrefixValidationEnd(cookie,expected,identity,ended);
  okay=status==B_OK;
  if(okay){reply->final_end_state=ended.state;reply->pci_restored=ended.pci_restored;reply->host_released=ended.host_released;reply->capabilities=ended.capabilities;}
 }
 const uint64 ended=uint64(system_time());okay=okay && ended>=began;
 auto& r=reply->metadata;r.state=okay?TT_RUNTIME_PASSED:TT_RUNTIME_COMPLETION_FAILED;
 r.closed=r.input_ready=okay?1:0;r.recovery_required=okay?0:1;r.elapsed_us=okay?ended-began:0;
 if(!okay){
  // Only CPU-held diagnostic metadata survives. Do NOT inspect/retry/END a
  // failed source,or dereference expected (successful END may have freed it).
  memset(reinterpret_cast<uint8*>(reply)+sizeof(reply->metadata),0,sizeof(*reply)-sizeof(reply->metadata));
 }
 status_t copied=user_memcpy(data,reply,sizeof(*reply));delete results;delete reply;
 return copied!=B_OK?copied:status;
} // ACTUAL diagnostic body;physical selection has its OWN exact role;NEVER Work
