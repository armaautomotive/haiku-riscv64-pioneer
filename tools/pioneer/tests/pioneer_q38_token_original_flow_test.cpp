/* SPDX-License-Identifier: MIT */
#include "pioneer_q38_token_original_fabric.h"
#ifdef PIONEER_Q38_TEST_OBSERVED_TRACE
#include "pioneer_q38_eos_state_capture.h"
#include "PioneerQ38EosStateDataBridge.h"
#endif
#include <csignal>
#if defined(PIONEER_Q38_TEST_CANCELLABLE_ROOT) && PIONEER_Q38_TEST_CANCELLABLE_ROOT != 1
#error Exact HOST cancellable original-root test selection required
#endif
int main(int argc,char** argv){
 assert(argc>=5 && argc<=10);const char* mode=nullptr;const char* traceDirectory=nullptr;bool eos=false,snapshot=false;
 for(int i=5;i<argc;i++){
  if(!strcmp(argv[i],"--prepare-only") || !strcmp(argv[i],"--bootstrap-only") || !strcmp(argv[i],"--first-layer-only")){assert(!mode);mode=argv[i];}
  else if(!strcmp(argv[i],"--eos-after-prompt")){assert(!eos);eos=true;}
  else if(!strcmp(argv[i],"--snapshot-first-layer")){assert(!snapshot);snapshot=true;}
  else{assert(!strcmp(argv[i],"--observed-trace") && !traceDirectory && i+1<argc);traceDirectory=argv[++i];}
 }
 assert(!mode || !traceDirectory || strcmp(mode,"--prepare-only"));
 assert(!mode || strcmp(mode,"--first-layer-only") || traceDirectory);
 assert(!eos || (mode && !strcmp(mode,"--prepare-only")) || traceDirectory);
 assert(!eos || !mode || strcmp(mode,"--first-layer-only"));
 assert(!snapshot || (traceDirectory && !mode && !eos));
 setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;(void)&Refusals;(void)&LateRefusals;
 puts("START authenticate entire SAME-open original27B GGUF BEFORE ROOT construction;no CPU product/native owner/physical IO");
 WholeModelOriginalBytes original;assert(original.Open(argv[4]) && original.Ready());
#ifdef PIONEER_Q38_TEST_OBSERVED_TRACE
 std::unique_ptr<PioneerQ38ObservedTrace> observed;
#else
 assert(!traceDirectory);
#endif
 auto m=std::make_unique<OriginalTokenFlowMachine>(argv[1],argv[2],argv[3],original,eos);
#ifdef PIONEER_Q38_TEST_CANCELLABLE_ROOT
 assert(m->cancelBoundary && !m->memory.at(TL::CancelLocal));
 puts("PASS distinct pinned cancellable FP32 ROOT/raw cancel control ZERO before first instruction;no request or completion invented");
#else
 assert(!m->cancelBoundary);
#endif
 m->VerifyRawEndpoints();
 const auto first=*m->learned.begin();unsigned route=unsigned(first.first>>32),address=unsigned(first.first);
 // A synthetic raw word is deliberately present only in each disposable
 // child. Correct reads must IGNORE it, and source refusal must ABORT rather
 // than fall back to it. The unchanged parent never gains such a byte.
 for(unsigned bad=0;bad<4;bad++){
  assert(original.PrepareFork());fflush(nullptr);pid_t pid=fork();assert(pid>=0);
  if(!pid){
   uint32_t wanted=m->Get(route,address);m->dram[first.first]=wanted^1;assert(m->Get(route,address)==wanted);
   if(bad==0)m->Put(route,address,wanted);
   if(bad==1){original.Revoke();(void)m->Get(route,address);}
   std::array<uint8_t,64> output;output.fill(0x5a);
   if(bad==2){original.Revoke();m->ReadRaw(route,address,output.data(),64);}
   if(bad==3)m->ReadRaw(route,address+first.second->allocated-4,output.data(),8);
   _exit(73);
  }
  int status=0;assert(waitpid(pid,&status,0)==pid && WIFSIGNALED(status) && WTERMSIG(status)==SIGABRT && original.Ready() && original.PrepareFork());
 }
 puts("PASS 4 genuine original-fabric refusals/learned write+revoked word+revoked batch+cross-part batch/no synthetic fallback/unchanged virgin parent;TEST transport safety only");
#ifdef PIONEER_Q38_TEST_OBSERVED_TRACE
 if(traceDirectory){observed=std::make_unique<PioneerQ38ObservedTrace>(eos);m->observed=observed.get();
  m->snapshotFirstLayerDirectory=snapshot?traceDirectory:nullptr;}
#endif
 if(mode && !strcmp(mode,"--prepare-only"))return 0;
 if(mode && !strcmp(mode,"--first-layer-only")){
#ifdef PIONEER_Q38_TEST_OBSERVED_TRACE
  assert(observed);
  // SAME scheduler and real instructions as RunTokens. Stop this HOST-only
  // diagnostic at the actual verified layer boundary, not by supplying PC,
  // registers, numeric products or fabricated worker/STOP state.
  while(!m->allLayers && m->pc!=0x60000 && m->pc!=0x60004){
   m->cycles+=19;m->StepToken();if(!(m->steps&255))for(unsigned w=0;w<119;w++)m->Advance(w,64);
  }
  assert(m->pc!=0x60000 && m->pc!=0x60004 && m->allLayers==1 && m->completed==1 && m->releases==1
   && !m->position && !m->layer && !m->committed && !m->flips && !m->tokenPublications && !m->stopJobs
   && observed->Embeddings()==1 && observed->Layers(0)==1 && !observed->Complete() && original.Ready());
  assert(observed->SaveFirstLayerDiagnostic(traceDirectory));
  puts("SAVED FIRST_LAYER_DIAGNOSTIC actual original embedding+layer0 hidden words ONLY/real verified layer+SAME119 closed/no token committed/NO STOP or terminal capability/NOT full-model/native/physical/Work/TPS qualification");
  return 0;
#else
  assert(false && "first-layer observation requires exact HOST recorder role");
#endif
 }
 if(mode){
  unsigned entry=0;std::ifstream symbols(std::string(argv[1])+"/program.symbols");std::string addressText,kind,name;
  while(symbols>>addressText>>kind>>name)if(name=="q38_return_recurrent_layer")entry=unsigned(std::stoul(addressText,nullptr,16));
  assert(entry);
  while(m->pc!=entry && m->pc!=0x60000){assert(m->steps<200000000);m->StepToken();}
  assert(m->pc==entry && m->releases==1 && m->embedded==1 && m->embeddingReads==20 && m->embeddingWrites==2 && m->embeddingBack==2
   && !m->allLayers && !m->matrixJobs && !m->peerWrites && !m->committed && original.Ready());
  for(const auto& p:m->child)assert(!p->active && !p->sequence && p->GdnFinished(0));
#ifdef PIONEER_Q38_TEST_OBSERVED_TRACE
  if(observed){assert(observed->Embeddings()==1 && !observed->Layers(0) && !observed->Complete());puts("PASS actual observer captured genuine5120 embedding ONLY/no model layer or token invented/no complete trace saved");}
#endif
  printf("PASS actual original27B embedding5120/all metadata/first returning entry/ROOT from RESET+SAME119 still virgin/stack reserved%u written%u/ROOTinstructions%u;NOT first layer/full-model numerics/native/physical/Work/TPS\n",0x70000-m->minSp,0x70000-m->minStack,m->steps);return 0;
 }
 puts(eos?"START actual original27B EOS flow/configured TEST stop IDs70253+17/first match MUST NOT stop prefill/real predictions decide final stop/NO supplied hidden or CPU model products":"START actual original27B FULL3-token flow/from RESET/SAME119/ALL learned bytes from SAME authenticated GGUF/NO supplied hidden or CPU model products");
 bool okay=m->RunTokens();if(m->kvFault){assert(!okay);m->VerifyKvFault();assert(original.Ready());_exit(0);}
 assert(okay && original.Ready());if(eos)m->VerifyEosTokens();else m->VerifyTokens();assert(original.Ready());
#ifdef PIONEER_Q38_TEST_OBSERVED_TRACE
 if(observed){
  if(eos){std::array<uint32_t,64> terminal;for(unsigned i=0;i<64;i++)terminal[i]=m->memory.at(TL::TerminalLocal+i*4);
   // Only ACTUAL stored mutable words. Never Get's initial-zero/learned
   // fallback,produced expected vectors,CPU references or generated state.
   const std::string stateDirectory=std::string(traceDirectory)+"-state";
   const auto commands=m->command;const auto counters=m->counts;const unsigned packets=m->packets,steps=m->steps;unsigned records=0;
   assert(CaptureQ38EosStateData(*m->canonical,[&](unsigned route,unsigned address,uint32_t& word){
    const auto found=m->dram.find(m->Key(route,address));if(found==m->dram.end())return false;word=found->second;return true;
   },[&](unsigned layer,unsigned kind,const uint32_t* words,unsigned count){
    if(!SaveQ38ObservedEosStateData(stateDirectory.c_str(),layer,kind,words,count))return false;records++;return true;
   }));
   assert(records==128 && original.Ready() && m->pc==0x60004 && m->packets==packets && m->steps==steps && m->command==commands && m->counts==counters
    && FinishQ38ObservedEosStateData(stateDirectory.c_str(),records));
   assert(observed->EosComplete() && !observed->Complete() && observed->SaveEos(traceDirectory,terminal.data(),64) && observed->SavedEosComplete());
   puts("SAVED distinct EOS128 state DATA/ACTUAL stored GDN slot0+Conv+2position packed K/V/half-key-value normalization/AFTER real128layers+119STOP/NO device writes,CPU products or fallback zeros;NOT CPU match,native END,Work or TPS proof");
   puts("SAVED distinct EOS DATA family/2 real positions+actual terminal/AFTER genuine early EOS+119STOP/no third token/NOT native END,CPU match,physical,Work or TPS proof");
  }else{assert(observed->Complete() && observed->Save(traceDirectory));puts("SAVED actual emitted output DATA only/3positions/all65hidden+all248320vocab+committed records/AFTER genuine full verification+119STOP;NOT CPU numerical match/native/physical/Work/TPS");}
 }
#endif
}
