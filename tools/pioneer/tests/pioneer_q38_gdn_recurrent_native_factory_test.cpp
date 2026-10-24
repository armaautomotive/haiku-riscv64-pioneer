/* SPDX-License-Identifier: MIT */
// ACTUAL native init/source/factories/SAME owner -> ONE emitted QRJ1.
// Original-model references sealed BEFORE context. Endpoints simulated only.
#define PIONEER_Q38_RESIDENT_DECODER_NATIVE_FACTORY_ONLY
#include "pioneer_q38_resident_decoder_native_factory_test.cpp"
#include "pioneer_q38_decoder_original_source.h"
int main(int argc,char** argv){
 assert(argc==9 || (argc==10 && !strcmp(argv[9],"--faults")));setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 puts("NATIVE_QRJ_ALL_ORIGINAL_REFERENCES_BEFORE_CONTEXT;NO_DEVICE_ACCESS");
 auto source=OpenGdnRecurrentOriginalSource(argv[7]);assert(source && source->Prepare(0) && source->SealBeforeNativeContext());
 DecoderNativeFixture f(argv[3],argv[4],argv[5]);f.io.originalEmbeddingWeights=source->EmbeddingTile();f.io.originalRmsWeights=source->LearnedNorm();
 assert(BindQ38EmbeddingModel(*f.c,0,0,0));auto* m=f.c->q38EmbeddingModel;PioneerQ38StateZeroResult zero;
 assert(InitializeQ38GdnStateModel(*f.c,zero) && m->MatchesInitializedGdn(f.owner.banks,f.owner.dram));
 assert(InitializeQ38ConvHistoryModel(*f.c,f.historyResult) && f.Initialize() && f.Run() && RunQ38LearnedRmsModel(*f.c,f.norm));
 f.Attach(argv[1],argv[2]);f.transport->DecoderSources(argv[6],m->plan,m->bank,f.io);
 f.transport->GdnInputSource(argv[8]);f.transport->LearnedSource(*source);f.transport->ObserveDecoder(f.decoderResult);
 assert(PrepareQ38ProjectionExecution(*f.c,f.projection));
 for(unsigned role:{10U,11U,5U,6U})assert(RunQ38ProjectionExecution(*f.c,role,f.projection));
 assert(RunQ38ResidentDecoderModel(*f.c,f.decoderResult));auto* snapshot=&m->projectionMemory;auto* cohort=m->projectionCohort;
 unsigned n=f.io.touches,d=f.transport->Touches();PioneerQ38ResidentDecoderResult forged;forged.gdnLayerPublished=1;forged.gdnHeld=forged.gdnCompleted=96;
 assert(!m->decoder.GdnLayerCompleted() && !m->decoder.Close(f.io,forged) && !PrepareQ38ResidentGdnInputs(*f.c,forged)
  && f.io.touches==n && f.transport->Touches()==d && !m->Unsafe());
 puts("NATIVE_QRJ_REAL_CBO_ALL120_HELD;SAME_INITIALIZED_GDN;FORGED_COMPLETION_INERT");
 if(argc==10){
  for(unsigned fault=1;fault<=18;fault++){
   pid_t child=fork();assert(child>=0);
   if(!child){
    if(fault<=14)f.transport->GdnInputFault(fault);
    if(fault==15)currentTeam=8;if(fault==16)m->gdnZero.nonce=671;
    if(fault==17)f.transport->DecoderFault(5);if(fault==18)f.transport->DecoderFault(6);
    bool ran=RunQ38ResidentGdnRecurrent(*f.c,f.decoderResult);
    if(fault==17 || fault==18)assert(ran && !CloseQ38ResidentDecoderModel(*f.c,f.decoderResult));else assert(!ran);
    if(fault<=14)assert(f.transport->GdnInputFaultInjected());
    f.UnknownNative();n=f.io.touches;d=f.transport->Touches();
    assert(!RunQ38ResidentGdnRecurrent(*f.c,forged) && f.io.touches==n && f.transport->Touches()==d);
    if(fault==13 || fault==14)assert(!f.decoderResult.gdnHeld);
    printf("PASS native QRJ fault%u SAME snapshot/ALLowners retained/ZERO laterIO\n",fault);_exit(0);
   }
   int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));
  }
 }
 assert(RunQ38ResidentGdnRecurrent(*f.c,f.decoderResult) && f.transport->GdnInputStarts()==96 && m->decoder.GdnLayerCompleted()
  && f.decoderResult.gdnLayerPublished==1 && f.decoderResult.gdnHeld==96 && m->projectionCohort==cohort && &m->projectionMemory==snapshot);
 for(unsigned half=0;half<96;half++){
  assert(f.io.SelectRuntimeCore(half+1));uint32_t prepared[322],next[8192],output[64];
  for(unsigned i=0;i<322;i++)assert(f.io.ProjectionGraphMemoryReadChecked(half+1,0x8e000+i*4,prepared[i]));
  for(unsigned i=0;i<8192;i++)assert(f.io.ProjectionGraphMemoryReadChecked(half+1,0xa0000+i*4,next[i]));
  for(unsigned i=0;i<64;i++)assert(f.io.ProjectionGraphMemoryReadChecked(half+1,0x8e800+i*4,output[i]));
  assert(source->CheckGdnRecurrent(half,prepared,next,output));
  PioneerQ38GdnBankExtent span;auto* g=m->graph.Recurrent(m->identity);uint32_t remote[8192];
  assert(g && g->InitializeHalf(m->identity,0,1,half,span) && f.io.ReadGdnState(span.bank,span.address,remote,8192)
   && !memcmp(remote,next,sizeof(next)) && g->InitializeHalf(m->identity,0,0,half,span) && f.io.ReadGdnState(span.bank,span.address,remote,8192));
  for(auto word:remote)assert(!word);
 }
 for(unsigned region=0;region<672;region++){
  const auto& r=m->plan.RecurrentRegions()[region];if(!r.layer && r.slot==1)continue;
  unsigned first=f.io.recurrentFirst[region]*1024,words=f.io.recurrentExtents[region].bytes/4;
  for(unsigned i=0;i<words;i++)assert(!f.io.recurrentWords[first+i]);
 }
 assert(source->ReferenceReads()==1672 && source->GdnInputReads()==192);
 n=f.io.touches;d=f.transport->Touches();assert(!RunQ38ResidentGdnRecurrent(*f.c,forged) && f.io.touches==n && f.transport->Touches()==d);
 assert(CloseQ38ResidentDecoderModel(*f.c,f.decoderResult) && !m->decoder.GdnLayerCompleted() && m->Closed() && EndFactory(*f.c) && !sColdRecoveryRequired
  && f.decoderResult.memory.restoredWords==8134072 && f.decoderResult.memory.verifiedOriginalWords==8134072);
 puts("PASS ORIGINAL token0 SAME ACTUAL native init73728/history1680/QPE1/QRN1/4QPS1/QCB1ALL120->ONE_QRJ1_ALL96/786432next+6144outputs+30912prepared values/independentBEFOREcontext references/immutableCURRENT+unrelatedstate/ALL120holds/private layer0NEXT publication/full8134072word restore/actualEND;NO host PREP-to-RECURRENCE boundary/current-slot-flip/nativeCARD_OR_SPEED_CLAIM");
}
