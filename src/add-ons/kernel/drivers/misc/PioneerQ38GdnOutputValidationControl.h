/* SPDX-License-Identifier: MIT */
// Included in real ProbeControl translation unit,after actual factories
// and global call scope. No CPU references,parallel owners or new IO facade.
#if !defined(PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE) || PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE != 1
#error Exact separate DEBUG55 control role required
#endif
struct PioneerQ38GdnOutputValidationResults {
 PioneerQ38StateZeroResult state;PioneerQ38ConvHistoryZeroResult history;PioneerQ38ProjectionZeroResult zero;
 PioneerQ38EmbeddingResult embedding;PioneerQ38LearnedRmsResult norm;
 PioneerQ38ProjectionGraphResult projection;PioneerQ38ResidentDecoderResult decoder;
}; // HEAP ONLY,not a kernel stack frame
static status_t Q38GdnOutputValidationControl(PioneerProbeCookie* cookie,uint32 operation,void* data,size_t length)
{
 if(!cookie || cookie->magic!=0x50313030 || !data)return B_BAD_VALUE;
 thread_info thread={};status_t status=get_thread_info(find_thread(NULL),&thread);
 if(status!=B_OK || thread.team!=cookie->team)return status!=B_OK?status:B_NOT_ALLOWED;
 if(operation==TT_PCI_PROBE_Q38_GDN_OUTPUT_VALIDATION_SUPPORT){
  if(length!=sizeof(tt_q38_embedding_support))return B_BAD_VALUE;
  status=AcquireQ38GdnSessionLock(cookie);if(status!=B_OK)return status;
  const tt_q38_embedding_support support={1,55,18,TT_Q38_GDN_OUTPUT_VALIDATION_WORDS,248320,512,0,0};
  status=user_memcpy(data,&support,sizeof(support));release_sem(sLock);return status;
 }
 if(operation!=TT_PCI_PROBE_Q38_GDN_OUTPUT_VALIDATE || length!=sizeof(tt_q38_gdn_output_validation))return B_BAD_VALUE;
 auto* reply=new(std::nothrow) tt_q38_gdn_output_validation;if(!reply)return B_NO_MEMORY;
 status=user_memcpy(reply,data,sizeof(*reply));
 bool shape=status==B_OK && PioneerQ38EmbeddingRequest(reply->metadata) && !reply->metadata.position;
 if(shape){const auto* bytes=reinterpret_cast<const uint8*>(reply);
  for(unsigned i=sizeof(reply->metadata);i<sizeof(*reply);i++)if(bytes[i]){shape=false;break;}
 }
 if(!shape){delete reply;return status!=B_OK?status:B_BAD_VALUE;}
 auto* results=new(std::nothrow) PioneerQ38GdnOutputValidationResults();
 if(!results){delete reply;return B_NO_MEMORY;}
 PioneerQ38GdnCallScope scope(cookie);auto* c=scope.Context();
 if(!c || !BindQ38EmbeddingModel(*c,reply->metadata.token,0,0)){
  delete results;delete reply;return scope.Status()!=B_OK?scope.Status():B_NOT_ALLOWED;
 }
 auto& state=results->state;auto& history=results->history;auto& zero=results->zero;
 auto& embedding=results->embedding;auto& norm=results->norm;auto& projection=results->projection;auto& decoder=results->decoder;
 uint64 began=uint64(system_time());
 bool okay=InitializeQ38GdnStateModel(*c,state) && InitializeQ38ConvHistoryModel(*c,history)
  && InitializeQ38EmbeddingModel(*c,zero) && RunQ38EmbeddingModel(*c,embedding)
  && RunQ38LearnedRmsModel(*c,norm) && PrepareQ38ProjectionExecution(*c,projection);
 const unsigned roles[]={10,11,5,6};for(unsigned role:roles)if(okay)okay=RunQ38ProjectionExecution(*c,role,projection);
 if(okay)okay=RunQ38ResidentDecoderModel(*c,decoder) && RunQ38ResidentGdnRecurrent(*c,decoder)
  && ReadQ38ResidentGdnOutputValidationInput(*c,decoder,reply->values)
  && RunQ38ResidentGdnOutput(*c,decoder) && CloseQ38ResidentDecoderModel(*c,decoder,reply->values);
 uint64 ended=uint64(system_time());auto* m=c->q38EmbeddingModel;
 okay=okay && ended>=began && m && m->Closed() && m->embedding.Completed() && m->rms.Completed() && !c->Unsafe()
  && m->MatchesInitializedGdn(c->banks,c->baseline.baseline.baseline.baseline.dram)
  && m->gdnZero.closed && m->gdnZero.nonce==672 && state.nonce==672
  && m->historyZero.closed && m->historyZero.nonce==336 && history.nonce==336 && history.pages==5
  && projection.runs==4 && projection.armed==119 && projection.released==119 && projection.rootStarts==4 && projection.rootHolds==4
  && projection.memory.savedWords==8330680 && projection.memory.verifiedOriginalWords==8330680 && projection.memory.installedWords==1185912
  && decoder.closed && !decoder.unsafe && decoder.installed==120 && decoder.armed==120 && decoder.released==120
  && decoder.completed==120 && decoder.held==120 && decoder.stop.stopped==119 && decoder.stop.held==119
  && decoder.stop.rootStarts==1 && decoder.stop.rootHolds==1 && decoder.gdnInstalled==96 && decoder.gdnArmed==96
  && decoder.gdnReleased==96 && decoder.gdnCompleted==96 && decoder.gdnHeld==96 && decoder.gdnLayerPublished==1 && decoder.gdnGatedPublished==1
  && decoder.gdnValidationWorkers==96 && decoder.gdnValidationWords==829632
  && decoder.outputInstalled==120 && decoder.outputBooted==119 && decoder.outputRootRuns==2 && decoder.outputStopped==119 && decoder.outputPublished==1
  && decoder.outputReadbackPages==5 && decoder.outputReadbackWords==5120 && decoder.outputOperandWords==3072 && decoder.outputScaleWords==192
  && decoder.memory.restoredWords==8330680 && decoder.memory.verifiedOriginalWords==8330680;
 auto& r=reply->metadata;r.state=okay?TT_RUNTIME_PASSED:TT_RUNTIME_COMPLETION_FAILED;
 r.closed=r.input_ready=okay?1:0;r.recovery_required=okay?0:1;
 r.session=m?m->identity.session:0;r.driver_generation=m?m->identity.driverGeneration:0;
 r.elapsed_us=okay?ended-began:0; // aggregate init/compute/readback/restore,NEVER token timing
 r.source_pages=okay?31878:0;r.workspace_pages=okay?512:0;
 r.saved_words=okay?embedding.savedWords+norm.savedWords+projection.memory.savedWords:0;
 r.restored_words=okay?embedding.restoredWords+norm.restoredWords+decoder.memory.restoredWords:0;
 if(okay){
  reply->gated_published=decoder.gdnGatedPublished;reply->readback_half_words=8642;
  reply->state_runs=state.nonce;reply->state_pages=73728;reply->history_runs=history.nonce;reply->history_pages=1680;
  reply->projections=projection.runs;reply->peer_boots=projection.released;reply->peer_stopped=decoder.stop.stopped;reply->peer_held=decoder.stop.held;
  reply->root_starts=projection.rootStarts+decoder.stop.rootStarts;reply->root_holds=projection.rootHolds+decoder.stop.rootHolds;
  reply->convolution_workers=decoder.completed;reply->convolution_jobs=640;reply->convolution_channels=10240;reply->convolution_held=decoder.held;
  reply->input_workers=96;reply->input_installed=decoder.gdnInstalled;reply->input_armed=decoder.gdnArmed;
  reply->input_released=decoder.gdnReleased;reply->input_completed=decoder.gdnCompleted;reply->input_held=decoder.gdnHeld;
  reply->layer_published=decoder.gdnLayerPublished;reply->readback_workers=decoder.gdnValidationWorkers;reply->readback_words=decoder.gdnValidationWords;
  reply->saved_words=projection.memory.savedWords;reply->verified_words=projection.memory.verifiedOriginalWords+decoder.memory.verifiedOriginalWords;
  reply->installed_words=projection.memory.installedWords;reply->restored_words=decoder.memory.restoredWords;
  reply->output_installed=decoder.outputInstalled;reply->output_booted=decoder.outputBooted;reply->output_root_runs=decoder.outputRootRuns;
  reply->output_stopped=decoder.outputStopped;reply->output_published=decoder.outputPublished;
  reply->output_pages=decoder.outputReadbackPages;reply->output_words=decoder.outputReadbackWords;
  reply->operand_words=decoder.outputOperandWords;reply->scale_words=decoder.outputScaleWords;
 }else{
  dprintf("Q38-GDN-OUTPUT-DEBUG: FAIL state%u history%u zero%u embedding%u norm%u projection%u decoder%u worker%u word%u gdn%u/%u output%u/%u/%u/%u/%u numeric%u/%u/%u/%u restored%u;NO_POST_FAILURE_DEVICE_IO\n",
   state.phase,history.phase,zero.phase,embedding.phase,norm.phase,projection.phase,decoder.phase,decoder.worker,decoder.word,
   decoder.gdnValidationWorkers,decoder.gdnValidationWords,decoder.outputInstalled,decoder.outputBooted,decoder.outputRootRuns,decoder.outputStopped,decoder.outputPublished,
   decoder.outputReadbackPages,decoder.outputReadbackWords,decoder.outputOperandWords,decoder.outputScaleWords,decoder.memory.restoredWords);
  memset(reinterpret_cast<uint8*>(reply)+sizeof(reply->metadata),0,sizeof(*reply)-sizeof(reply->metadata));
 }
 status=user_memcpy(data,reply,sizeof(*reply));delete reply;
 if(!okay || status!=B_OK || c->Unsafe()){tt_resident_session_result retained={};RetainQ38DispatchOwner(*c,retained);}
 delete results;return status; // independent user DEBUG checks ALL values BEFORE explicit END
}
