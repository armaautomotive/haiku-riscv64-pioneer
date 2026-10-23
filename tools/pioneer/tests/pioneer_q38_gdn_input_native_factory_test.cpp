/* SPDX-License-Identifier: MIT */
// SAME actual native factories/owner/snapshot through QCB1->QGI1. Only
// physical/OS/learned-byte endpoints simulated;not a card/backend launch.
#define PIONEER_Q38_RESIDENT_DECODER_NATIVE_FACTORY_ONLY
#include "pioneer_q38_resident_decoder_native_factory_test.cpp"
#include "pioneer_q38_decoder_original_source.h"
int main(int argc,char** argv){
 assert(argc==9 || (argc==10 && !strcmp(argv[9],"--faults")));setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 puts("NATIVE_GDN_ALL_ORIGINAL_REFERENCES_BEFORE_CONTEXT;NO_DEVICE_ACCESS");
 auto source=OpenGdnInputOriginalSource(argv[7]);assert(source && source->Prepare(0) && source->SealBeforeNativeContext());
 DecoderNativeFixture f(argv[3],argv[4],argv[5]);assert(f.PreBindRefusals()==11);
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());unsigned prebind=0;
 for(unsigned i=0;i<canonical->Count();i++){const auto* p=canonical->Part(i);
  if(p->kind!=canonical->WeightKind || p->layer || (p->role!=7 && p->role!=8))continue;
  unsigned page=p->bank*786432+p->offset/4096,n=f.io.touches;uint64 epoch=sQ38EmbeddingModelEpoch;
  for(unsigned bad:{0U,2U}){
   assert(f.learned[page]==1);f.learned[page]=bad;
   assert(!BindQ38EmbeddingModel(*f.c,0,0,0) && !f.c->q38EmbeddingModel && f.io.touches==n && sQ38EmbeddingModelEpoch==epoch);
   f.learned[page]=1;prebind++;
  }
 }
 assert(prebind==4);f.io.originalEmbeddingWeights=source->EmbeddingTile();f.io.originalRmsWeights=source->LearnedNorm();
 f.ProduceNative();auto* same=f.c->q38EmbeddingModel;
 f.Attach(argv[1],argv[2]);f.transport->DecoderSources(argv[6],same->plan,same->bank,f.io);
 f.transport->GdnInputSource(argv[8]);f.transport->LearnedSource(*source);f.transport->ObserveDecoder(f.decoderResult);
 assert(PrepareQ38ProjectionExecution(*f.c,f.projection));
 for(unsigned role:{10U,11U,5U,6U})assert(RunQ38ProjectionExecution(*f.c,role,f.projection));
 assert(RunQ38ResidentDecoderModel(*f.c,f.decoderResult) && f.transport->DecoderStarts()==120 && !same->decoder.Closed());
 auto* snapshot=&same->projectionMemory;auto* cohort=same->projectionCohort;
 unsigned touched=f.io.touches,device=f.transport->Touches();PioneerQ38ResidentDecoderResult forged;forged.gdnCompleted=forged.gdnHeld=96;
 assert(!same->decoder.Close(f.io,forged) && !same->decoder.GdnInputsPrepared() && !same->decoder.GdnInputsClaimed()
  && f.io.touches==touched && f.transport->Touches()==device && !same->decoder.Unsafe());
 puts("NATIVE_GDN_REAL_CBO_ALL120HELD_SAME_SNAPSHOT;NO_CLOSE_OR_NUMERIC_HOST_INPUT");
 if(argc==10){
  for(unsigned fault=1;fault<=20;fault++){
   pid_t child=fork();assert(child>=0);
   if(!child){
    if(fault<=14)f.transport->GdnInputFault(fault);
    if(fault==15)currentTeam=8;if(fault==16)same->historyZero.nonce=335;
    if(fault==17)f.transport->DecoderFault(5);if(fault==18)f.transport->DecoderFault(6);
    if(fault==19){
     for(unsigned i=0;i<canonical->Count();i++){const auto* p=canonical->Part(i);
      if(p->kind==canonical->WeightKind && !p->layer && p->role==7)f.learned[p->bank*786432+p->offset/4096]=0;
     }
    }
    bool prepared=PrepareQ38ResidentGdnInputs(*f.c,f.decoderResult);
    if(fault==17 || fault==18){assert(prepared && !CloseQ38ResidentDecoderModel(*f.c,f.decoderResult));}
    else if(fault==20){assert(prepared && !EndFactory(*f.c));}
    else assert(!prepared);
    if(fault<=14)assert(f.transport->GdnInputFaultInjected());
    assert(&same->projectionMemory==snapshot && same->projectionCohort==cohort && !same->Closed());
    f.UnknownNative();unsigned n=f.io.touches,d=f.transport->Touches();
    assert(!PrepareQ38ResidentGdnInputs(*f.c,f.decoderResult) && !same->decoder.PrepareGdnInputs(f.io,forged)
     && f.io.touches==n && f.transport->Touches()==d);
    if(fault<=2 || fault==15 || fault==16 || fault==19)assert(!f.transport->GdnInputStarts());
    if(fault==13 || fault==14)assert(!f.decoderResult.gdnHeld); // detect untouched ROOT/last peer BEFORE ANY hold
    printf("PASS native GDN fault%u SAME snapshot/ALLowners retained/ZERO later IO\n",fault);_exit(0);
   }
   int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));
  }
  puts("PASS20 SAME native QCB1->QGI1 partial-code/descriptor/receipt/counter/source/command/status/PC/image/untouched24/team/history/ledger/restore/unfinished-END faults;NOT_CARD");
  // Parent's genuine CBO producer was unchanged by isolated children.
  // Finish its full ORIGINAL positive path too,without rehashing a file
  // or manufacturing a fresh producer/completion capability.
 }
 assert(PrepareQ38ResidentGdnInputs(*f.c,f.decoderResult) && f.transport->GdnInputStarts()==96 && same->decoder.GdnInputsPrepared()
  && &same->projectionMemory==snapshot && same->projectionCohort==cohort && !same->Closed() && !same->projectionMemory.Closed());
 assert(source->CheckEmbeddingRms(f.io.wholeWorkspace));
 for(unsigned role:{10U,11U,5U,6U})assert(source->CheckProjection(role,f.io.wholeWorkspace.data()+PioneerQ38ProjectionResultOffset(role)/4));
 for(unsigned half=0;half<96;half++){
  assert(f.io.SelectRuntimeCore(half+1));uint32_t values[322];
  for(unsigned i=0;i<322;i++)assert(f.io.ProjectionGraphMemoryReadChecked(half+1,0x8e000+i*4,values[i]));
  assert(source->CheckGdnInput(half,values));
 }
 assert(source->ReferenceReads()==1672 && source->ProjectionReads()==164800 && source->ConvolutionReads()==640 && source->GdnInputReads()==192);
 touched=f.io.touches;device=f.transport->Touches();
 assert(!PrepareQ38ResidentGdnInputs(*f.c,forged) && !same->decoder.PrepareGdnInputs(f.io,forged)
  && f.io.touches==touched && f.transport->Touches()==device && !same->Unsafe());
 const auto& r=f.decoderResult;
 assert(r.gdnInstalled==96 && r.gdnArmed==96 && r.gdnReleased==96 && r.gdnCompleted==96 && r.gdnHeld==96
  && CloseQ38ResidentDecoderModel(*f.c,f.decoderResult) && same->Closed() && !f.c->Unsafe()
  && r.memory.restoredWords==8134072 && r.memory.verifiedOriginalWords==8134072 && EndFactory(*f.c)
  && f.c->q38Dispatch->closed && !sColdRecoveryRequired);
 puts("PASS ORIGINAL token0 SAME ACTUAL native source/cookie/team/thread/clock/ALLhosts/history/QPE1/QRN1/QPS1/QCB1ALL120->QGI1ALL96/672reads/192originalA-dtreads/30912outputs/strict-relative-gates/original8134072word snapshot retained THROUGH BOTH stages/full restore/actual END;NO_STATE_INIT_RECURRENCE_CARD_OR_SPEED_CLAIM");
}
