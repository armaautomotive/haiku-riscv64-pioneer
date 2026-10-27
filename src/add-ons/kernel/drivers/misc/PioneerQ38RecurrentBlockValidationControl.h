/* SPDX-License-Identifier: MIT */
#include "PioneerQ38RecurrentBlockValidationRole.h"
#include "PioneerQ38RecurrentBlockValidationProtocol.h"
static_assert(unsigned(TT_Q38_RECURRENT_BLOCK_VALIDATION_WORDS)==unsigned(PioneerQ38RecurrentBlockReadbackPlan::Words)
 && unsigned(TT_Q38_RECURRENT_BLOCK_VALIDATION_PAGES)==unsigned(PioneerQ38RecurrentBlockReadbackPlan::Pages),"fixed private readback ONLY");
struct PioneerQ38RecurrentBlockValidationResults {
 PioneerQ38ProjectionZeroResult zero;PioneerQ38EmbeddingResult embedding;
 PioneerQ38StateZeroResult recurrent;PioneerQ38ConvHistoryZeroResult history;
 PioneerQ38RecurrentBlockMemoryResult memory;PioneerQ38RecurrentBlockResult execution;PioneerQ38RecurrentBlockReadbackResult readback;
}; // HEAP ONLY;no 20KiB kernel request or large result-set stack local
static status_t Q38RecurrentBlockValidationControl(PioneerProbeCookie* cookie,uint32 operation,void* data,size_t length){
 if(!cookie || cookie->magic!=0x50313030 || !data)return B_BAD_VALUE;
 thread_info thread={};status_t status=get_thread_info(find_thread(NULL),&thread);
 if(status!=B_OK || thread.team!=cookie->team)return status!=B_OK?status:B_NOT_ALLOWED;
 if(operation==TT_PCI_PROBE_Q38_RECURRENT_BLOCK_VALIDATION_SUPPORT){
  if(length!=sizeof(tt_q38_embedding_support))return B_BAD_VALUE;
  status=AcquireQ38GdnSessionLock(cookie);if(status!=B_OK)return status;
  const tt_q38_embedding_support support={1,55,TT_Q38_RECURRENT_BLOCK_VALIDATION_NAMESPACE,
   TT_Q38_RECURRENT_BLOCK_VALIDATION_WORDS,248320,512,0,0};
  status=user_memcpy(data,&support,sizeof(support));release_sem(sLock);return status;
 }
 if(operation!=TT_PCI_PROBE_Q38_RECURRENT_BLOCK_VALIDATE || length!=sizeof(tt_q38_recurrent_block_validation))return B_BAD_VALUE;
 auto* reply=new(std::nothrow) tt_q38_recurrent_block_validation;if(!reply)return B_NO_MEMORY;
 status=user_memcpy(reply,data,sizeof(*reply));
 if(status!=B_OK || !PioneerQ38RecurrentBlockValidationRequest(*reply)){delete reply;return status!=B_OK?status:B_BAD_VALUE;}
 auto* results=new(std::nothrow) PioneerQ38RecurrentBlockValidationResults();
 if(!results){delete reply;return B_NO_MEMORY;}
 PioneerQ38GdnCallScope scope(cookie);auto* c=scope.Context();
 const bool hadModel=c && c->q38EmbeddingModel;
 if(!c || !BindQ38EmbeddingModel(*c,0,0,0)){
  if(c && !hadModel && c->q38EmbeddingModel){tt_resident_session_result retained={};RetainQ38DispatchOwner(*c,retained);}
  delete results;delete reply;return scope.Status()!=B_OK?scope.Status():B_NOT_ALLOWED;
 }
 auto& z=results->zero;auto& e=results->embedding;auto& g=results->recurrent;auto& h=results->history;
 auto& m=results->memory;auto& x=results->execution;auto& b=results->readback;
 const uint64 began=uint64(system_time());
 bool okay=InitializeQ38EmbeddingModel(*c,z) && RunQ38EmbeddingModel(*c,e) && BindQ38RecurrentBlockNative(*c)
  && InitializeQ38RecurrentBlockState(*c,g) && InitializeQ38RecurrentBlockHistory(*c,h)
  && PrepareQ38RecurrentBlockNativeHeld(*c,m) && PrepareQ38RecurrentBlockNativeExecution(*c,x)
  && RunQ38RecurrentBlockNativeExecution(*c,x)
  && CloseQ38RecurrentBlockNativeValidation(*c,x,m,reply->values,TT_Q38_RECURRENT_BLOCK_VALIDATION_WORDS,b);
 const uint64 ended=uint64(system_time());auto* input=c->q38EmbeddingModel;auto* native=c->q38RecurrentBlockNative;
 constexpr unsigned packets=8+(PioneerQ38MixedRecurrentCohortLayout::ConvWords+4095)/4096
  +(PioneerQ38MixedRecurrentCohortLayout::GdnWords+4095)/4096;
 static_assert(packets==25,"only8recipes+12Conv+5GDN metadata packets");
 okay=okay && ended>=began && input && native && !c->Unsafe() && !input->Unsafe() && input->Closed() && native->Closed()
  && native->InitializationsClosed(c->banks,c->baseline.baseline.baseline.baseline.dram)
  && input->zero.closed && !input->zero.unsafe && z.nonce==2 && z.completed && z.held && !z.recovery_required
  && e.closed && !e.unsafe && e.completed==1 && e.held==1
  && g.state==TT_RUNTIME_PASSED && g.nonce==672 && g.started && g.completed && g.held && !g.recovery_required
  && h.state==TT_RUNTIME_PASSED && h.nonce==336 && h.pages==5 && h.started && h.completed && h.held && !h.recovery_required
  && m.closed && !m.unsafe && m.savedWords==TT_Q38_RECURRENT_BLOCK_SAVED_WORDS
  && m.verifiedOriginalWords==2*TT_Q38_RECURRENT_BLOCK_SAVED_WORDS && m.installedWords==TT_Q38_RECURRENT_BLOCK_CODE_WORDS
  && m.restoredWords==TT_Q38_RECURRENT_BLOCK_SAVED_WORDS && x.closed && !x.unsafe && x.prepared==1 && x.completed==119
  && x.armed==119 && x.released==119 && x.stopped==119 && x.held==120 && x.rootStarts==1 && x.rootHolds==1 && x.packets==packets
  && b.started && b.completed && b.pages==TT_Q38_RECURRENT_BLOCK_VALIDATION_PAGES && b.words==TT_Q38_RECURRENT_BLOCK_VALIDATION_WORDS;
 auto& r=reply->metadata;r.state=okay?TT_RUNTIME_PASSED:TT_RUNTIME_COMPLETION_FAILED;
 r.closed=r.input_ready=okay?1:0;r.recovery_required=okay?0:1;
 r.session=input?input->identity.session:0;r.driver_generation=input?input->identity.driverGeneration:0;r.elapsed_us=okay?ended-began:0;
 if(okay){
  for(unsigned i=0;i<PioneerResidentBanks::Pages;i++)if(c->initialized[i]==1)r.source_pages++;
  r.workspace_pages=512;r.saved_words=e.savedWords+m.savedWords;r.restored_words=e.restoredWords+m.restoredWords;
  reply->zero_runs=z.nonce;reply->embedding_roots=e.completed;reply->recurrent_regions=g.nonce;reply->history_regions=h.nonce;
  reply->recurrent_pages=73728;reply->history_pages=1680;
  reply->cohort_prepared=x.prepared;reply->cohort_completed=x.completed;reply->cohort_stopped=x.stopped;reply->cohort_held=x.held;
  reply->root_runs=x.rootStarts;reply->root_holds=x.rootHolds;reply->packets=x.packets;
  reply->saved_words=m.savedWords;reply->verified_words=m.verifiedOriginalWords;reply->installed_words=m.installedWords;reply->restored_words=m.restoredWords;
  reply->readback_pages=b.pages;reply->readback_words=b.words;reply->final_rows=5120;
 }else{
  dprintf("Q38-RECURRENT-DEBUG71:FAIL zero%u embedding%u gdn%u/%u history%u/%u memory%u/%u/%u execution%u/%u/%u readback%u/%u;CPU_METADATA_ONLY_NO_POST_FAILURE_DEVICE_IO\n",
   z.phase,e.phase,g.phase,g.nonce,h.phase,h.nonce,m.phase,m.worker,m.word,x.phase,x.worker,x.word,b.pages,b.words);
  memset(reinterpret_cast<uint8*>(reply)+sizeof(reply->metadata),0,sizeof(*reply)-sizeof(reply->metadata));
 }
 status=user_memcpy(data,reply,sizeof(*reply));delete reply;
 if(!okay || status!=B_OK || c->Unsafe()){tt_resident_session_result retained={};RetainQ38DispatchOwner(*c,retained);}
 delete results;return status;
} // Caller MUST compare original output BEFORE explicit BANK_END;failure CLOSES ONLY.
