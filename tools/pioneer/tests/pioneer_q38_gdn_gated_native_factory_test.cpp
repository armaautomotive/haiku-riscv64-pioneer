/* SPDX-License-Identifier: MIT */
// SAME actual native init/source/CBO owner -> concurrently emitted QGH1.
// Original references BEFORE context. ONLY device/kernel byte endpoints simulated.
#define PIONEER_Q38_RESIDENT_DECODER_NATIVE_FACTORY_ONLY
#include "pioneer_q38_resident_decoder_native_factory_test.cpp"
#include "pioneer_q38_decoder_original_source.h"
int main(int argc,char** argv){
 assert(argc==9 || (argc==10 && (!strcmp(argv[9],"--delayed") || !strcmp(argv[9],"--late-negative") || !strcmp(argv[9],"--host-deadline") || !strcmp(argv[9],"--clock-receipt-fault") || !strcmp(argv[9],"--controller-faults"))));setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 puts("NATIVE_QGH_ALL_ORIGINAL_REFERENCES_BEFORE_CONTEXT;NO_DEVICE_ACCESS");
 auto source=OpenGdnGatedNativeOriginalSource(argv[7]);assert(source && source->Prepare(0) && source->SealBeforeNativeContext());
 DecoderNativeFixture f(argv[3],argv[4],argv[5]);f.io.originalEmbeddingWeights=source->EmbeddingTile();f.io.originalRmsWeights=source->LearnedNorm();
 {auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());unsigned found=0;
  for(unsigned i=0;i<plan->Count();i++){
   const auto* p=plan->Part(i);if(p->kind!=plan->WeightKind || p->layer || p->role!=9)continue;
   unsigned page=p->bank*unsigned(plan->Capacity/4096)+p->offset/4096;assert(f.learned[page]==1);
   auto epoch=sQ38EmbeddingModelEpoch;unsigned touches=f.io.touches;
   for(unsigned value:{0U,2U}){f.learned[page]=value;assert(!BindQ38EmbeddingModel(*f.c,0,0,0) && !f.c->q38EmbeddingModel
    && f.io.touches==touches && sQ38EmbeddingModelEpoch==epoch);}
   f.learned[page]=1;found++;
  }
  assert(found==1);puts("NATIVE_QGH_missing-corrupt ORIGINAL norm page refused BEFORE epoch/claims/IO");
 }
 assert(BindQ38EmbeddingModel(*f.c,0,0,0));auto* m=f.c->q38EmbeddingModel;PioneerQ38StateZeroResult zero;
 assert(InitializeQ38GdnStateModel(*f.c,zero) && m->MatchesInitializedGdn(f.owner.banks,f.owner.dram));
 assert(InitializeQ38ConvHistoryModel(*f.c,f.historyResult) && f.Initialize() && f.Run() && RunQ38LearnedRmsModel(*f.c,f.norm));
 f.Attach(argv[1],argv[2]);f.transport->DecoderSources(argv[6],m->plan,m->bank,f.io);
 f.transport->GdnInputSource(argv[8]);f.transport->LearnedSource(*source);f.transport->ObserveDecoder(f.decoderResult);
 assert(PrepareQ38ProjectionExecution(*f.c,f.projection));
 for(unsigned role:{10U,11U,5U,6U})assert(RunQ38ProjectionExecution(*f.c,role,f.projection));
 assert(RunQ38ResidentDecoderModel(*f.c,f.decoderResult));auto* snapshot=&m->projectionMemory;auto* cohort=m->projectionCohort;
 assert(f.projection.memory.savedWords==8330680 && f.projection.memory.installedWords==1185912);
 unsigned n=f.io.touches,d=f.transport->Touches();PioneerQ38ResidentDecoderResult forged;
 forged.gdnLayerPublished=forged.gdnGatedPublished=1;forged.gdnHeld=forged.gdnCompleted=96;
 assert(!m->decoder.GdnGatedCompleted() && !m->decoder.GdnLayerCompleted() && !m->decoder.Close(f.io,forged)
  && !PrepareQ38ResidentGdnInputs(*f.c,forged) && f.io.touches==n && f.transport->Touches()==d && !m->Unsafe());
 puts("NATIVE_QGH_REAL_CBO_ALL120_HELD;SAME_INITIALIZED_GDN;forged publications inert;starting ACTUAL cooperative96 streams");
 if(argc==10 && !strcmp(argv[9],"--controller-faults")){
  for(unsigned fault=1;fault<=18;fault++){
   pid_t child=fork();assert(child>=0);
   if(!child){
    if(fault<=14)f.transport->GdnInputFault(fault);
    if(fault==15)currentTeam=8;
    if(fault==16)m->gdnZero.nonce=671;
    if(fault==17)f.transport->DecoderFault(5);
    if(fault==18)f.transport->DecoderFault(6);
    bool ran=RunQ38ResidentGdnRecurrent(*f.c,f.decoderResult);
    if(fault>=17)assert(ran && !CloseQ38ResidentDecoderModel(*f.c,f.decoderResult));
    else assert(!ran && !f.decoderResult.gdnLayerPublished && !f.decoderResult.gdnGatedPublished);
    if(fault<=14)assert(f.transport->GdnInputFaultInjected());
    f.UnknownNative();n=f.io.touches;d=f.transport->Touches();
    assert(!RunQ38ResidentGdnRecurrent(*f.c,forged) && !CloseQ38ResidentDecoderModel(*f.c,forged)
     && f.io.touches==n && f.transport->Touches()==d);
    printf("PASS ACTUAL native QGT1 controller fault%u/SAME snapshots+ALL owners retained/ZERO later IO/reset-restore-retry\n",fault);_exit(0);
   }
   int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));
  }
 }
 bool delay=argc==10 && strcmp(argv[9],"--clock-receipt-fault") && strcmp(argv[9],"--controller-faults");
 if(delay)f.transport->GatedLaunchDelay(2,!strcmp(argv[9],"--host-deadline")?1000:20000,!strcmp(argv[9],"--host-deadline")?30000001:0);
 if(delay && strcmp(argv[9],"--delayed")){
  assert(!RunQ38ResidentGdnRecurrent(*f.c,f.decoderResult) && f.transport->GatedLaunchDelayInjected()
   && !m->decoder.GdnGatedCompleted() && !f.decoderResult.gdnGatedPublished && !f.decoderResult.gdnLayerPublished && !f.decoderResult.memory.restoredWords);
  f.UnknownNative();n=f.io.touches;d=f.transport->Touches();
  assert(!RunQ38ResidentGdnRecurrent(*f.c,forged) && f.io.touches==n && f.transport->Touches()==d);
  printf("PASS ACTUAL SAME native failure%s/real delayed peer release/NO gated orNEXT publication/ALL owners-snapshots retained/ZERO following IO/NO reset-restore-retry;OFFLINE ONLY\n",argv[9]);return 0;
 }
 assert(RunQ38ResidentGdnRecurrent(*f.c,f.decoderResult) && f.transport->GdnInputStarts()==96
  && m->decoder.GdnGatedCompleted() && m->decoder.GdnLayerCompleted() && f.decoderResult.gdnGatedPublished==1
  && f.decoderResult.gdnLayerPublished==1 && f.decoderResult.gdnHeld==96 && m->projectionCohort==cohort && &m->projectionMemory==snapshot);
 for(unsigned half=0;half<96;half++){
  unsigned w=half+1;assert(f.io.SelectRuntimeCore(w));uint32_t prepared[322],next[8192],raw[64],gated[64];
  for(unsigned i=0;i<322;i++)assert(f.io.ProjectionGraphMemoryReadChecked(w,0x8e000+i*4,prepared[i]));
  for(unsigned i=0;i<8192;i++)assert(f.io.ProjectionGraphMemoryReadChecked(w,0xa0000+i*4,next[i]));
  for(unsigned i=0;i<64;i++)assert(f.io.ProjectionGraphMemoryReadChecked(w,0x8e800+i*4,raw[i])
   && f.io.ProjectionGraphMemoryReadChecked(w,0x8ea00+i*4,gated[i]) && gated[i]==f.io.wholeWorkspace[TT_Q38_GDN_GATED_OUTPUT/4+half*64+i]);
  assert(source->CheckGdnRecurrent(half,prepared,next,raw) && source->CheckGdnGatedNorm(half,gated));
  PioneerQ38GdnBankExtent span;auto* g=m->graph.Recurrent(m->identity);uint32_t remote[8192];
  assert(g && g->InitializeHalf(m->identity,0,1,half,span) && f.io.ReadGdnState(span.bank,span.address,remote,8192)
   && !memcmp(remote,next,sizeof(next)) && g->InitializeHalf(m->identity,0,0,half,span) && f.io.ReadGdnState(span.bank,span.address,remote,8192));
  for(auto value:remote)assert(!value);
 }
 for(unsigned region=0;region<672;region++){
  const auto& r=m->plan.RecurrentRegions()[region];if(!r.layer && r.slot==1)continue;
  unsigned first=f.io.recurrentFirst[region]*1024,words=f.io.recurrentExtents[region].bytes/4;
  for(unsigned i=0;i<words;i++)assert(!f.io.recurrentWords[first+i]);
 }
 assert(source->ReferenceReads()==1673 && source->GdnInputReads()==192 && source->GdnNormReads()==96);
#ifdef PIONEER_Q38_GDN_RENDEZVOUS_NATIVE
 puts("PASS QGT1 native timed clock receipts/84Gcycle fixed horizon;HOST deadline starts BEFORE first release;SYNTHETIC CLOCK_NOT_PHYSICAL_TIME");
#endif
 if(delay)assert(f.transport->GatedLaunchDelayInjected());
 n=f.io.touches;d=f.transport->Touches();assert(!RunQ38ResidentGdnRecurrent(*f.c,forged) && f.io.touches==n && f.transport->Touches()==d);
#ifdef PIONEER_Q38_GDN_RENDEZVOUS_NATIVE
 if(argc==10 && !strcmp(argv[9],"--clock-receipt-fault")){
  for(unsigned word=0;word<4;word++){
   pid_t child=fork();assert(child>=0);
   if(!child){
    assert(f.io.SelectRuntimeCore(1));uint32_t clock[16],gd[64];
    for(unsigned i=0;i<16;i++)assert(f.io.ProjectionGraphMemoryReadChecked(1,TT_Q38_GDN_RENDEZVOUS_CLOCK+i*4,clock[i]));
    for(unsigned i=0;i<64;i++)assert(f.io.ProjectionGraphMemoryReadChecked(1,TT_Q38_GDN_GATED_DESCRIPTOR+i*4,gd[i]));
    if(word==1)clock[2]--;else if(word==3)clock[4]++;else clock[1+word]^=1;
    assert(PioneerQ38GdnRendezvousClockReceipt(clock,gd,clock[5])); // still valid DATA;not the accepted immutable bytes
    assert(f.io.ProjectionGraphMemoryWrite(1,TT_Q38_GDN_RENDEZVOUS_CLOCK+(1+word)*4,clock[1+word]));
    bool closed=CloseQ38ResidentDecoderModel(*f.c,f.decoderResult);
    printf("QGT1_ACCEPTED_CLOCK_MUTATION word%u closed%u earlierRestoredWords%u\n",word,unsigned(closed),f.decoderResult.memory.restoredWords);
    assert(!closed); // per-worker restore may finish earlier workers BEFORE discovering this failure
    f.UnknownNative();n=f.io.touches;d=f.transport->Touches();
    assert(!CloseQ38ResidentDecoderModel(*f.c,forged) && !RunQ38ResidentGdnRecurrent(*f.c,forged)
     && f.io.touches==n && f.transport->Touches()==d);
    printf("PASS ACTUAL native QGT1 changed ACCEPTED clock word%u rejected even when semantically valid;ALL owners/snapshots retained/ZERO following IO/reset-restore-retry\n",word);_exit(0);
   }
   int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));
  }
  // Parent still has the SAME genuine accepted bytes; prove complete normal Close/END below.
 }
#endif
 bool closed=CloseQ38ResidentDecoderModel(*f.c,f.decoderResult);
 printf("NATIVE_QGH_CLOSE closed%u phase%u worker%u word%u restored%u verified%u unsafe%u\n",unsigned(closed),f.decoderResult.phase,f.decoderResult.worker,f.decoderResult.word,
  f.decoderResult.memory.restoredWords,f.decoderResult.memory.verifiedOriginalWords,f.decoderResult.unsafe);
 assert(closed && !m->decoder.GdnGatedCompleted() && m->Closed()
  && f.decoderResult.memory.restoredWords==8330680 && f.decoderResult.memory.verifiedOriginalWords==8330680);
 assert(EndFactory(*f.c) && !sColdRecoveryRequired);
 puts("PASS ORIGINAL token0 SAME ACTUAL native init73728/history1680/QPE1/QRN1/4QPS1/QCB1ALL120->COOPERATIVE_QGH1_ALL96/829632original values/real NoC peer stamps+raw+originalnorm+retainedZ/private gatedGDDR;ALL96receipts/ALL120holds/immutableCURRENT+unrelatedstate/private layer0NEXT+gated publication/complete8330680word restore/actualEND;NO CPU numeric inputs/host stage boundary/Work promotion/current-slot-flip/nativeCARD_OR_SPEED_CLAIM");
}
