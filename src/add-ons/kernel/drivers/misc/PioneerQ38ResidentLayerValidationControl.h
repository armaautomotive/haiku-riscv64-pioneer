/* SPDX-License-Identifier: MIT */
// Included ONLY after the actual native context/factories/call scope.
// One fixed diagnostic lifetime,NOT an inference/Work/CPU-reference path.
#include "PioneerQ38ResidentLayerValidationRole.h"
#include "PioneerQ38ResidentLayerValidationProtocol.h"
static_assert(unsigned(TT_Q38_RESIDENT_LAYER_VALIDATION_WORDS)==unsigned(PioneerQ38ResidentLayerReadbackPlan::Words)
 && unsigned(TT_Q38_RESIDENT_LAYER_VALIDATION_PAGES)==unsigned(PioneerQ38ResidentLayerReadbackPlan::Pages),"public reply EXACTLY matches private fixed observer");
struct PioneerQ38ResidentLayerValidationResults {
 PioneerQ38StateZeroResult state;PioneerQ38ConvHistoryZeroResult history;PioneerQ38ProjectionZeroResult zero;
 PioneerQ38EmbeddingResult embedding;PioneerQ38LearnedRmsResult norm;PioneerQ38ProjectionGraphResult projection;
 PioneerQ38ResidentLayerResult output;PioneerQ38ResidentLayerReadbackResult readback;
}; // HEAP ONLY,including ALL result records;NEVER a large kernel frame
static status_t Q38ResidentLayerValidationControl(PioneerProbeCookie* cookie,uint32 operation,void* data,size_t length)
{
 if(!cookie || cookie->magic!=0x50313030 || !data)return B_BAD_VALUE;
 thread_info thread={};status_t status=get_thread_info(find_thread(NULL),&thread);
 if(status!=B_OK || thread.team!=cookie->team)return status!=B_OK?status:B_NOT_ALLOWED;
 if(operation==TT_PCI_PROBE_Q38_RESIDENT_LAYER_VALIDATION_SUPPORT){
  if(length!=sizeof(tt_q38_embedding_support))return B_BAD_VALUE;
  status=AcquireQ38GdnSessionLock(cookie);if(status!=B_OK)return status;
  const tt_q38_embedding_support support={1,55,TT_Q38_RESIDENT_LAYER_VALIDATION_NAMESPACE,
   TT_Q38_RESIDENT_LAYER_VALIDATION_WORDS,248320,512,0,0};
  status=user_memcpy(data,&support,sizeof(support));release_sem(sLock);return status;
 }
 if(operation!=TT_PCI_PROBE_Q38_RESIDENT_LAYER_VALIDATE || length!=sizeof(tt_q38_resident_layer_validation))return B_BAD_VALUE;
 auto* reply=new(std::nothrow) tt_q38_resident_layer_validation;if(!reply)return B_NO_MEMORY;
 status=user_memcpy(reply,data,sizeof(*reply));
 if(status!=B_OK || !PioneerQ38ResidentLayerValidationRequest(*reply)){
  delete reply;return status!=B_OK?status:B_BAD_VALUE;
 }
 auto* results=new(std::nothrow) PioneerQ38ResidentLayerValidationResults();
 if(!results){delete reply;return B_NO_MEMORY;}
 PioneerQ38GdnCallScope scope(cookie);auto* c=scope.Context();
 const bool hadModel=c && c->q38EmbeddingModel;
 if(!c || !BindQ38EmbeddingModel(*c,reply->metadata.token,0,0) || !BindQ38ResidentLayerNativeState(*c)){
  // A pre-existing clean closed lifetime makes this one-shot call inert.
  // Retain only a NEW partial bind from THIS attempt;never poison the
  // known-clean earlier execution merely because a duplicate was refused.
  if(c && !hadModel && c->q38EmbeddingModel){tt_resident_session_result retained={};RetainQ38DispatchOwner(*c,retained);}
  delete results;delete reply;return scope.Status()!=B_OK?scope.Status():B_NOT_ALLOWED;
 }
 auto& state=results->state;auto& history=results->history;auto& zero=results->zero;
 auto& embedding=results->embedding;auto& norm=results->norm;auto& projection=results->projection;
 auto& output=results->output;auto& readback=results->readback;
 const uint64 began=uint64(system_time());
 bool okay=InitializeQ38ResidentLayerRecurrent(*c,state) && InitializeQ38ResidentLayerHistory(*c,history)
  && InitializeQ38EmbeddingModel(*c,zero) && RunQ38EmbeddingModel(*c,embedding) && RunQ38LearnedRmsModel(*c,norm)
  && PrepareQ38ProjectionExecution(*c,projection);
 const unsigned roles[]={10,11,5,6};for(unsigned role:roles)if(okay)okay=RunQ38ProjectionExecution(*c,role,projection);
 if(okay)okay=CloseQ38ProjectionExecution(*c,projection) && PrepareQ38ResidentLayerNative(*c,output)
  && RunQ38ResidentLayerNative(*c,output)
  && CloseQ38ResidentLayerNativeForValidation(*c,output,reply->values,TT_Q38_RESIDENT_LAYER_VALIDATION_WORDS,readback);
 const uint64 ended=uint64(system_time());auto* m=c->q38EmbeddingModel;auto* native=c->q38ResidentLayerNative;
 okay=okay && ended>=began && m && native && !c->Unsafe() && !m->Unsafe()
  && native->Closed(c->banks,c->baseline.baseline.baseline.baseline.dram) && native->ClosedLayerBinding()
  && native->InitializationsClosed(c->banks,c->baseline.baseline.baseline.baseline.dram)
  && state.nonce==672 && history.nonce==336 && history.pages==5
  && embedding.closed && embedding.completed && embedding.held && !embedding.unsafe
  && norm.closed && norm.completed && norm.held && !norm.unsafe
  && projection.closed && !projection.unsafe && projection.runs==4 && projection.armed==119 && projection.released==119
  && projection.stopped==119 && projection.held==119 && projection.rootStarts==5 && projection.rootHolds==5
  && projection.memory.closed && projection.memory.savedWords==TT_Q38_PROJECTION_GRAPH_SAVED_WORDS
  && projection.memory.verifiedOriginalWords==2*TT_Q38_PROJECTION_GRAPH_SAVED_WORDS
  && projection.memory.installedWords==TT_Q38_PROJECTION_GRAPH_CODE_WORDS
  && projection.memory.restoredWords==TT_Q38_PROJECTION_GRAPH_SAVED_WORDS
  && output.closed && !output.unsafe && output.prepared && output.completed==119 && output.armed==119 && output.released==119
  && output.waiting==119 && output.stopped==119 && output.held==119 && output.rootStarts==1 && output.rootHolds==1
  && output.memory.closed && output.memory.savedWords==TT_Q38_RESIDENT_LAYER_SAVED_WORDS
  && output.memory.verifiedOriginalWords==2*TT_Q38_RESIDENT_LAYER_SAVED_WORDS
  && output.memory.installedWords==TT_Q38_RESIDENT_LAYER_CODE_WORDS && output.memory.restoredWords==TT_Q38_RESIDENT_LAYER_SAVED_WORDS
  && readback.started && readback.completed && readback.pages==TT_Q38_RESIDENT_LAYER_VALIDATION_PAGES
  && readback.words==TT_Q38_RESIDENT_LAYER_VALIDATION_WORDS;
 auto& r=reply->metadata;r.state=okay?TT_RUNTIME_PASSED:TT_RUNTIME_COMPLETION_FAILED;
 r.closed=r.input_ready=okay?1:0;r.recovery_required=okay?0:1;
 r.session=m?m->identity.session:0;r.driver_generation=m?m->identity.driverGeneration:0;r.elapsed_us=okay?ended-began:0;
 if(okay){
  // Actual retained upload-ledger count,NOT a geometry/authentication grant.
  for(unsigned i=0;i<PioneerResidentBanks::Pages;i++)if(c->initialized[i]==1)r.source_pages++;
  r.workspace_pages=512;
  // Compute journals only;initialization runs/pages are separately reported.
  r.saved_words=embedding.savedWords+norm.savedWords+projection.memory.savedWords+output.memory.savedWords;
  r.restored_words=embedding.restoredWords+norm.restoredWords+projection.memory.restoredWords+output.memory.restoredWords;
  reply->state_runs=state.nonce;reply->state_pages=73728;reply->history_runs=history.nonce;reply->history_pages=1680;
  reply->projection_runs=projection.runs;reply->projection_boots=projection.released;
  reply->projection_root_runs=projection.rootStarts;reply->projection_root_holds=projection.rootHolds;
  reply->cohort_boots=output.released;reply->cohort_root_runs=output.rootStarts;reply->cohort_root_holds=output.rootHolds;
  reply->cohort_stopped=output.stopped;reply->cohort_held=output.held;
  reply->convolution_jobs=640;reply->gdn_halves=96;reply->output_chunks=24;reply->output_rows=5120;
  reply->readback_pages=readback.pages;reply->readback_words=readback.words;reply->current_slot=0;reply->next_slot=1;
  reply->saved_words=output.memory.savedWords;reply->verified_words=output.memory.verifiedOriginalWords;
  reply->installed_words=output.memory.installedWords;reply->restored_words=output.memory.restoredWords;
  reply->prefix_saved_words=projection.memory.savedWords;reply->prefix_verified_words=projection.memory.verifiedOriginalWords;
  reply->prefix_installed_words=projection.memory.installedWords;reply->prefix_restored_words=projection.memory.restoredWords;
  // Exact job geometry is already required by the actual root+119 peer proofs.
  reply->ffn_matrices=357;reply->ffn_producers=88;reply->final_rows=5120;
 }else{
  dprintf("Q38-LAYER-DEBUG:FAIL state%u history%u zero%u embedding%u norm%u projection%u output%u worker%u word%u readback%u/%u restored%u;NO_POST_FAILURE_DEVICE_IO\n",
   state.phase,history.phase,zero.phase,embedding.phase,norm.phase,projection.phase,output.phase,output.worker,output.word,
   readback.pages,readback.words,output.memory.restoredWords);
  dprintf("Q38-LAYER-DEBUG:PROJECTION worker%u word%u runs%u stopped%u held%u roots%u/%u closed%u memory%u/%u/%u saved%u verified%u restored%u;CPU_METADATA_ONLY\n",
   projection.worker,projection.word,projection.runs,projection.stopped,projection.held,projection.rootStarts,projection.rootHolds,
   projection.closed,projection.memory.phase,projection.memory.worker,projection.memory.word,projection.memory.savedWords,
   projection.memory.verifiedOriginalWords,projection.memory.restoredWords);
  memset(reinterpret_cast<uint8*>(reply)+sizeof(reply->metadata),0,sizeof(*reply)-sizeof(reply->metadata));
 }
 status=user_memcpy(data,reply,sizeof(*reply));delete reply;
 if(!okay || status!=B_OK || c->Unsafe()){tt_resident_session_result retained={};RetainQ38DispatchOwner(*c,retained);}
 delete results;return status; // independent original-GGUF DEBUG observer must qualify BEFORE explicit END
}
