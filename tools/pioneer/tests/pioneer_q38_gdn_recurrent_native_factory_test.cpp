/* SPDX-License-Identifier: MIT */
// ACTUAL native init/source/factories/SAME owner -> ONE emitted QRJ1.
// Original-model references sealed BEFORE context. Endpoints simulated only.
#define PIONEER_Q38_RESIDENT_DECODER_NATIVE_FACTORY_ONLY
#include "pioneer_q38_resident_decoder_native_factory_test.cpp"
#include "pioneer_q38_decoder_original_source.h"
#ifdef PIONEER_Q38_TEST_GDN_GATED_NORM_LOCAL
#include "pioneer_q38_gdn_gated_norm_bridge.h"
#endif
#ifdef PIONEER_Q38_TEST_GDN_GATED_RESIDENT
#include "pioneer_q38_gdn_gated_resident_bridge.h"
#endif
int main(int argc,char** argv){
#if defined(PIONEER_Q38_TEST_GDN_GATED_NORM_LOCAL) || defined(PIONEER_Q38_TEST_GDN_GATED_RESIDENT)
 assert(argc==10);
#else
 assert(argc==9 || (argc==10 && !strcmp(argv[9],"--faults")));
#endif
 setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 puts("NATIVE_QRJ_ALL_ORIGINAL_REFERENCES_BEFORE_CONTEXT;NO_DEVICE_ACCESS");
 auto source=
#if defined(PIONEER_Q38_TEST_GDN_GATED_NORM_LOCAL) || defined(PIONEER_Q38_TEST_GDN_GATED_RESIDENT)
  OpenGdnGatedNormOriginalSource(argv[7]);
#else
  OpenGdnRecurrentOriginalSource(argv[7]);
#endif
 assert(source && source->Prepare(0) && source->SealBeforeNativeContext());
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
#if !defined(PIONEER_Q38_TEST_GDN_GATED_NORM_LOCAL) && !defined(PIONEER_Q38_TEST_GDN_GATED_RESIDENT)
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
#endif
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
#ifdef PIONEER_Q38_TEST_GDN_GATED_RESIDENT
 // Separate fused TEST execution from genuine immutable CURRENT/workspace
 // and original learned BYTES. NOT a new native producer or Work selector.
 for(unsigned head=0;head<48;head++){
  uint32_t input[2][64],state[2][64];GatedResidentObserved values;
  for(unsigned side=0;side<2;side++){
   unsigned worker=head*2+side+1;assert(f.io.SelectRuntimeCore(worker));
   for(unsigned i=0;i<64;i++)assert(f.io.ProjectionGraphMemoryReadChecked(worker,TT_Q38_GDN_INPUT_DESCRIPTOR+i*4,input[side][i])
    && f.io.ProjectionGraphMemoryReadChecked(worker,TT_Q38_RGDN_DESCRIPTOR+i*4,state[side][i]));
  }
  assert(source->GdnLearnedNorm());
  ExecuteGdnGatedResidentOriginalBytes(argv[9],m->plan,m->bank,input,state,source->GdnLearnedNorm(),f.io.wholeWorkspace,*source,f.io,values);
  for(unsigned side=0;side<2;side++)assert(source->CheckGdnRecurrent(head*2+side,values.prepared[side],values.next[side],values.raw[side])
   && source->CheckGdnGatedNorm(head*2+side,values.gated[side]));
 }
 assert(source->ReferenceReads()==1673 && source->GdnInputReads()==384);
 puts("PASS ORIGINAL token0 ACTUAL fused QGH1 all96workers/823488prepared-next-raw+6144gated values;fresh peer NoC stamp+paired ACTUAL QRJ raw+retainedQPS1Z+originalnorm;independent ORIGINAL references BEFORE context;separate TEST fabric NOT_NATIVE_NOC_OWNER_OR_CARD");
#elif defined(PIONEER_Q38_TEST_GDN_GATED_NORM_LOCAL)
 // Byte bridge ONLY:actual emitted QRJ1 LOCAL results and actual emitted
 // QPS1 resident Z,plus original learned norm bytes. No oracle products.
 for(unsigned half=0;half<96;half++){
  uint32_t raw[128],z[128],values[64];unsigned head=half/2;
  for(unsigned i=0;i<128;i++){
   unsigned worker=head*2+i/64+1;assert(f.io.SelectRuntimeCore(worker)
    && f.io.ProjectionGraphMemoryReadChecked(worker,0x8e800+(i%64)*4,raw[i]));
   z[i]=f.io.wholeWorkspace[PioneerQ38ProjectionResultOffset(5)/4+head*128+i];
  }
  assert(source->GdnLearnedNorm());ExecuteGdnGatedNormLocal(argv[9],half,raw,source->GdnLearnedNorm(),z,values);
  assert(source->CheckGdnGatedNorm(half,values));
 }
 assert(source->ReferenceReads()==1673 && source->GdnInputReads()==192);
 puts("PASS ORIGINAL token0 ACTUAL emitted QRJ1 raw128+QPS1 residentZ+originalnorm128->LOCAL QGN1 all96halves/6144outputs/independent ORIGINAL references BEFORE context;BYTE_BRIDGE_ONLY_NOT_NATIVE_NOC_OWNER_OR_CARD");
#else
 assert(source->ReferenceReads()==1672 && source->GdnInputReads()==192);
#endif
 n=f.io.touches;d=f.transport->Touches();assert(!RunQ38ResidentGdnRecurrent(*f.c,forged) && f.io.touches==n && f.transport->Touches()==d);
 assert(CloseQ38ResidentDecoderModel(*f.c,f.decoderResult) && !m->decoder.GdnLayerCompleted() && m->Closed() && EndFactory(*f.c) && !sColdRecoveryRequired
  && f.decoderResult.memory.restoredWords==8134072 && f.decoderResult.memory.verifiedOriginalWords==8134072);
 puts("PASS ORIGINAL token0 SAME ACTUAL native init73728/history1680/QPE1/QRN1/4QPS1/QCB1ALL120->ONE_QRJ1_ALL96/786432next+6144outputs+30912prepared values/independentBEFOREcontext references/immutableCURRENT+unrelatedstate/ALL120holds/private layer0NEXT publication/full8134072word restore/actualEND;NO host PREP-to-RECURRENCE boundary/current-slot-flip/nativeCARD_OR_SPEED_CLAIM");
}
