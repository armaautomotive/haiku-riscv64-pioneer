/* SPDX-License-Identifier: MIT */
// SAME actual native init/source/CBO owner -> concurrently emitted QGH1.
// Original references BEFORE context. ONLY device/kernel byte endpoints simulated.
#define PIONEER_Q38_RESIDENT_DECODER_NATIVE_FACTORY_ONLY
#include "pioneer_q38_resident_decoder_native_factory_test.cpp"
#include "pioneer_q38_decoder_original_source.h"
#ifdef PIONEER_Q38_GDN_OUTPUT_PRODUCER_NATIVE
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnOutputProjectionPlan.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnOutputBootstrapPlan.h"
#include <type_traits>
static_assert(!std::is_default_constructible<PioneerQ38GdnOutputProducer>::value
 && !std::is_copy_constructible<PioneerQ38GdnOutputProducer>::value
 && !std::is_move_constructible<PioneerQ38GdnOutputProducer>::value,"only genuine owner can construct/bind producer");
static_assert(!std::is_copy_constructible<PioneerQ38GdnOutputBootstrapPlan>::value
 && !std::is_move_constructible<PioneerQ38GdnOutputBootstrapPlan>::value,"same borrowed producer/cohort heap relation cannot be copied");
#endif
int main(int argc,char** argv){
#ifdef PIONEER_Q38_RESIDUAL_RMS_NATIVE
 assert(argc==12 || (argc==13 && !strcmp(argv[12],"--residual-faults")));setvbuf(stdout,nullptr,_IOLBF,0);
#elif defined(PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE)
 assert(argc==11 || (argc==12 && !strcmp(argv[11],"--output-faults")));setvbuf(stdout,nullptr,_IOLBF,0);
#else
 assert(argc==9 || (argc==10 && (!strcmp(argv[9],"--delayed") || !strcmp(argv[9],"--late-negative") || !strcmp(argv[9],"--host-deadline") || !strcmp(argv[9],"--clock-receipt-fault") || !strcmp(argv[9],"--controller-faults"))));setvbuf(stdout,nullptr,_IOLBF,0);
#endif
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 puts("NATIVE_QGH_ALL_ORIGINAL_REFERENCES_BEFORE_CONTEXT;NO_DEVICE_ACCESS");
#ifdef PIONEER_Q38_RESIDUAL_RMS_NATIVE
 auto source=OpenResidualRmsOriginalSource(argv[7]);
 static_assert(!std::is_default_constructible<PioneerQ38PostAttentionProducer>::value
  && !std::is_copy_constructible<PioneerQ38PostAttentionProducer>::value && !std::is_move_constructible<PioneerQ38PostAttentionProducer>::value,"actual native owner alone publishes next FFN input");
#elif defined(PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE)
 auto source=OpenGdnOutputOriginalSource(argv[7]);
#else
 auto source=OpenGdnGatedNativeOriginalSource(argv[7]);
#endif
 assert(source && source->Prepare(0) && source->SealBeforeNativeContext());
 DecoderNativeFixture f(argv[3],argv[4],argv[5]);f.io.originalEmbeddingWeights=source->EmbeddingTile();f.io.originalRmsWeights=source->LearnedNorm();
#ifdef PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE
 assert(source->GdnOutputWeightsReady() && !source->GdnOutputReads());
 {auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());unsigned count=0;
  for(unsigned i=0;i<canonical->Count();i++){
   const auto* p=canonical->Part(i);if(p->kind!=canonical->WeightKind || p->layer || p->role!=13)continue;
   for(unsigned page=0;page<p->allocated/4096;page++)f.learned[p->bank*786432+p->offset/4096+page]=1;count++;
  }
  assert(count==7);}
#endif
#ifdef PIONEER_Q38_RESIDUAL_RMS_NATIVE
 assert(source->ResidualRmsWeightsReady() && !source->ResidualRmsReads());
 {auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());unsigned found=0;
  for(unsigned i=0;i<canonical->Count();i++){
   const auto* p=canonical->Part(i);if(p->kind!=canonical->WeightKind || p->layer || p->role!=1)continue;
   assert(p->allocated==20480);unsigned first=p->bank*786432+p->offset/4096;
   for(unsigned page=0;page<5;page++)f.learned[first+page]=1;
   auto epoch=sQ38EmbeddingModelEpoch;unsigned touches=f.io.touches;
   for(unsigned page=0;page<5;page++)for(unsigned value:{0U,2U}){
    f.learned[first+page]=value;assert(!BindQ38EmbeddingModel(*f.c,0,0,0) && !f.c->q38EmbeddingModel
     && f.io.touches==touches && sQ38EmbeddingModelEpoch==epoch);f.learned[first+page]=1;
   }found++;
  }
  assert(found==1);puts("PASS NATIVE56 ALL5 ORIGINAL post-attention norm pages missing/corrupt refused BEFORE epoch/claim/IO");}
#endif
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
#ifdef PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE
 f.transport->OutputSources(argv[9],argv[10]);
#endif
#ifdef PIONEER_Q38_RESIDUAL_RMS_NATIVE
 f.transport->ResidualRmsSource(argv[11]);
#endif
 assert(PrepareQ38ProjectionExecution(*f.c,f.projection));
 for(unsigned role:{10U,11U,5U,6U})assert(RunQ38ProjectionExecution(*f.c,role,f.projection));
 assert(RunQ38ResidentDecoderModel(*f.c,f.decoderResult));auto* snapshot=&m->projectionMemory;auto* cohort=m->projectionCohort;
 assert(f.projection.memory.savedWords==8330680 && f.projection.memory.installedWords==1185912);
 unsigned n=f.io.touches,d=f.transport->Touches();PioneerQ38ResidentDecoderResult forged;
 forged.gdnLayerPublished=forged.gdnGatedPublished=1;forged.gdnHeld=forged.gdnCompleted=96;
#ifdef PIONEER_Q38_GDN_OUTPUT_PRODUCER_NATIVE
 assert(!m->decoder.GatedOutput()); // public result flags do not mint a typed producer
#endif
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
#ifdef PIONEER_Q38_GDN_OUTPUT_PRODUCER_NATIVE
 const auto* producer=m->decoder.GatedOutput();assert(producer && producer->Current());
 auto outputPlan=std::make_unique<PioneerQ38GdnOutputProjectionPlan>();n=f.io.touches;d=f.transport->Touches();
#ifndef PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE
 assert(!outputPlan->Build(*producer) && !outputPlan->Current() && !outputPlan->Build(*producer)); // ssm_out weights not uploaded in this fixture yet
#endif
 // Test learned-upload ledger BYTE endpoint ONLY. No output matrix code,
 // weights read,device writes or ssm_out numeric computation occur here.
 unsigned stripes=0,refused=0;
 for(unsigned i=0;i<m->plan.Count();i++){
  const auto* p=m->plan.Part(i);if(p->kind!=m->plan.WeightKind || p->layer || p->role!=13)continue;
  for(unsigned page=0;page<p->allocated/4096;page++)f.learned[p->bank*786432+p->offset/4096+page]=1;
  stripes++;
 }
 assert(stripes==7);outputPlan=std::make_unique<PioneerQ38GdnOutputProjectionPlan>();
 assert(outputPlan->Build(*producer) && outputPlan->Producer()==producer && outputPlan->Current() && outputPlan->Count()==119);
 auto bootstrap=std::make_unique<PioneerQ38GdnOutputBootstrapPlan>();
 auto deniedBootstrap=std::make_unique<PioneerQ38GdnOutputBootstrapPlan>();
 auto wrongBootstrapCohort=std::make_unique<PioneerQ38DispatchLease>();
 assert(!deniedBootstrap->Build(*outputPlan,*wrongBootstrapCohort) && !deniedBootstrap->Current()
  && !deniedBootstrap->Manifest() && !deniedBootstrap->Sources() && !deniedBootstrap->Build(*outputPlan,*cohort));
 assert(bootstrap->Build(*outputPlan,*cohort) && bootstrap->Current() && bootstrap->Matches(*outputPlan,*cohort)
  && !bootstrap->Build(*outputPlan,*cohort) && !bootstrap->Matches(*outputPlan,*wrongBootstrapCohort));
 auto* nativeManifest=const_cast<uint32_t*>(bootstrap->Manifest());auto* nativeSources=const_cast<uint32_t*>(bootstrap->Sources());
 assert(nativeManifest && nativeSources && PioneerQ38GdnOutputGraphManifest(nativeManifest,nativeSources)
  && nativeManifest[46]==TT_Q38_GDN_RENDEZVOUS_MARKER && nativeManifest[47]==m->token);
 unsigned bootstrapMutations=0;
 for(unsigned i=64;i<192;i++)assert(!nativeManifest[i]); // fresh new image boot REQUIRED;not stale peer history
 for(unsigned i=0;i<64;i++){
  nativeManifest[i]^=1;assert(!bootstrap->Current() && !bootstrap->Manifest() && !bootstrap->Sources());nativeManifest[i]^=1;bootstrapMutations++;
 }
 for(unsigned j=0;j<119;j++){
  const auto& p=*outputPlan->Part(j);const auto* record=nativeManifest+192+j*8;
  assert(record[0]==m->plan.Base+p.offset && record[1]==p.bank && record[2]==p.groups && record[3]==p.firstRow);
  for(unsigned i=0;i<8;i++){nativeManifest[192+j*8+i]^=1;assert(!bootstrap->Current());nativeManifest[192+j*8+i]^=1;bootstrapMutations++;}
 }
 for(unsigned i=0;i<119*16;i++){
  assert(nativeSources[i]==cohort->authority.source[1+i/16][i%16]);
  nativeSources[i]^=1;assert(!bootstrap->Current());nativeSources[i]^=1;bootstrapMutations++;
  assert(PioneerQ38ProjectionGraphSnapshotContains(0,TT_Q38_GDN_OUTPUT_GRAPH_SOURCES+i*4)
   && PioneerQ38ProjectionGraphSnapshotContains(0,TT_Q38_GDN_OUTPUT_GRAPH_SOURCE_SHADOW+i*4));
 }
 for(unsigned i:{64U,182U,183U,191U,192U+119*8,unsigned(TT_Q38_PROJECTION_GRAPH_WORDS-1)}){
  nativeManifest[i]^=1;assert(!bootstrap->Current());nativeManifest[i]^=1;bootstrapMutations++;
 }
 for(unsigned w:{1U,24U,119U})for(unsigned i:{0U,15U}){
  cohort->authority.source[w][i]^=1;assert(!bootstrap->Current());cohort->authority.source[w][i]^=1;bootstrapMutations++;
 }
 {uint32_t old[64];memcpy(old,nativeManifest,sizeof(old));old[0]=TT_Q38_PROJECTION_GRAPH_MARKER;
  assert(!PioneerQ38ProjectionGraphFrame(old));} // old RMS-only parser stays strict
 assert(bootstrap->Current() && f.io.touches==n && f.transport->Touches()==d);
 printf("PASS SAME native held producer canonical QOP1 bootstrap/ALL119jobs+1904actual source words/mutations%u/fresh-boot sequences/old RMS role13 refused/ALL bytes inside original snapshot;DATA_ONLY_NO boot or IO grant\n",bootstrapMutations);
 const auto* workspace=m->plan.Workspace(m->bank);assert(workspace);
 assert(outputPlan->Bank()==m->bank && outputPlan->InputAddress()==m->plan.Base+workspace->offset+TT_Q38_GDN_GATED_OUTPUT
  && outputPlan->OutputAddress()==outputPlan->InputAddress()+6144*4 && outputPlan->Phase());
 unsigned rows=0;for(unsigned i=0;i<119;i++){const auto* p=outputPlan->Part(i);assert(p && p->groups);rows+=p->groups*16;}
 assert(rows==5120 && !outputPlan->Part(119));
 uint32_t inputDescriptor[64];for(auto& word:inputDescriptor)word=0xdeadbeef;
 const auto descriptorPoison=std::vector<uint32_t>(inputDescriptor,inputDescriptor+64);
 auto foreignCohort=std::make_unique<PioneerQ38DispatchLease>();
 assert(!producer->MatchesCohort(*foreignCohort) && !outputPlan->InputDescriptor(0,*foreignCohort,inputDescriptor)
  && !memcmp(inputDescriptor,descriptorPoison.data(),sizeof(inputDescriptor)));
 for(unsigned chunk:{24U,UINT32_MAX})assert(!outputPlan->InputDescriptor(chunk,*cohort,inputDescriptor)
  && !memcmp(inputDescriptor,descriptorPoison.data(),sizeof(inputDescriptor)));
 for(unsigned chunk=0;chunk<24;chunk++){
  assert(producer->MatchesCohort(*cohort) && outputPlan->InputDescriptor(chunk,*cohort,inputDescriptor)
   && PioneerQ38GdnOutputInputShape(inputDescriptor) && !PioneerQ38ProjectionInputShape(inputDescriptor)
   && inputDescriptor[4]+inputDescriptor[10]==outputPlan->InputAddress()+chunk*1024
   && inputDescriptor[6]==outputPlan->Phase() && inputDescriptor[7]==chunk+1 && inputDescriptor[45]==m->token);
  for(unsigned word=0;word<16;word++)assert(inputDescriptor[16+word]==cohort->authority.source[chunk+1][word]);
 }
 printf("PASS genuine owner/EXACT retained cohort ALL24 QOI1 descriptors/gated input offsets/all peer sources;foreign cohort/out-of-range atomic refusal/NO IO\n");
 uint32_t gatherDescriptor[64],gatherSources[24][16];
 for(auto& word:gatherDescriptor)word=0xdeadbeef;for(auto& peer:gatherSources)for(auto& word:peer)word=0xdeadbeef;
 const auto gatherPoison=std::vector<uint32_t>(gatherDescriptor,gatherDescriptor+64);
 const auto sourcesPoison=std::vector<uint32_t>(384,0xdeadbeef);
 assert(!outputPlan->GatherDescriptor(*foreignCohort,gatherDescriptor) && !outputPlan->GatherSources(*foreignCohort,gatherSources)
  && !memcmp(gatherDescriptor,gatherPoison.data(),sizeof(gatherDescriptor)) && !memcmp(gatherSources,sourcesPoison.data(),sizeof(gatherSources)));
 assert(outputPlan->GatherDescriptor(*cohort,gatherDescriptor) && outputPlan->GatherSources(*cohort,gatherSources)
  && PioneerQ38GdnOutputGatherShape(gatherDescriptor) && !PioneerQ38ProjectionGatherShape(gatherDescriptor)
  && gatherDescriptor[4]+TT_Q38_GDN_GATED_OUTPUT==outputPlan->InputAddress() && gatherDescriptor[6]==outputPlan->Phase());
 for(unsigned word=0;word<16;word++)assert(gatherDescriptor[16+word]==cohort->authority.source[0][word]);
 for(unsigned w=0;w<24;w++)for(unsigned word=0;word<16;word++)assert(gatherSources[w][word]==cohort->authority.source[w+1][word]);
 for(unsigned word=0;word<384;word++)assert(PioneerQ38ProjectionGraphSnapshotContains(0,TT_Q38_GDN_OUTPUT_GATHER_SOURCES+word*4)
  && PioneerQ38ProjectionGraphSnapshotContains(0,TT_Q38_GDN_OUTPUT_GATHER_SHADOW+word*4));
 puts("PASS genuine typed QOG1 descriptor/ALL384actual peer source words/foreign-cohort atomic refusal/ALL table+shadow bytes in SAME original ROOT snapshot;NO IO");
 const auto* rootSource=m->projectionCohort->authority.source[0];
 assert(outputPlan->MatchesSource(m->plan,m->identity,f.learned.data(),PioneerResidentBanks::Pages,rootSource));
 auto other=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(other->Build());auto wrongId=m->identity;wrongId.modelEpoch++;
 assert(!outputPlan->MatchesSource(*other,m->identity,f.learned.data(),PioneerResidentBanks::Pages,rootSource)
  && !outputPlan->MatchesSource(m->plan,wrongId,f.learned.data(),PioneerResidentBanks::Pages,rootSource)
  && !outputPlan->MatchesSource(m->plan,m->identity,nullptr,PioneerResidentBanks::Pages,rootSource)
  && !outputPlan->MatchesSource(m->plan,m->identity,f.learned.data(),PioneerResidentBanks::Pages-1,rootSource));refused+=4;
 for(unsigned word=0;word<16;word++){
  uint32_t changed[16];memcpy(changed,rootSource,sizeof(changed));changed[word]^=1;
  assert(!outputPlan->MatchesSource(m->plan,m->identity,f.learned.data(),PioneerResidentBanks::Pages,changed));refused++;
 }
 for(unsigned i=0;i<m->plan.Count();i++){
  const auto* p=m->plan.Part(i);if(p->kind!=m->plan.WeightKind || p->layer || p->role!=13)continue;
  for(unsigned page:{0U,p->allocated/4096-1}){
   unsigned at=p->bank*786432+p->offset/4096+page;
   for(unsigned value:{0U,2U}){
    f.learned[at]=value;auto denied=std::make_unique<PioneerQ38GdnOutputProjectionPlan>();
    assert(!denied->Build(*producer) && !outputPlan->Current() && !outputPlan->Count() && !outputPlan->Part(0) && !bootstrap->Current());f.learned[at]=1;refused++;
   }
  }
 }
 assert(outputPlan->Current() && producer->Current() && !RunQ38ProjectionExecution(*f.c,13,f.projection)
  && f.io.touches==n && f.transport->Touches()==d && !m->Unsafe());refused++;
 for(unsigned fault=0;fault<3;fault++){
  pid_t child=fork();assert(child>=0);
  if(!child){
   if(fault==0)m->graph.Fault();if(fault==1)m->graph.Recurrent(m->identity)->Fault();if(fault==2)m->projectionCohort->worker[119].image.running=true;
   assert(!producer->Current() && !m->decoder.GatedOutput() && !outputPlan->Current() && !outputPlan->Count() && !bootstrap->Current()
    && !outputPlan->InputAddress() && !outputPlan->OutputAddress() && f.io.touches==n && f.transport->Touches()==d);_exit(0);
  }
  int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));refused++;
 }
 printf("PASS SAME genuine gated producer/6144GDDR->canonical119jobs/5120ssm_out_DATA_ONLY/refusals%u/no host activation or epoch hints/no endpointIO/NO ssm_out execution\n",refused);
#endif
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
#ifdef PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE
#ifndef PIONEER_Q38_RESIDUAL_RMS_NATIVE
 if(argc==12){
  n=f.io.touches;d=f.transport->Touches();currentTeam=8;
  assert(!RunQ38ResidentGdnOutput(*f.c,forged) && f.io.touches==n && f.transport->Touches()==d && !m->Unsafe());currentTeam=7;
  forged.outputPublished=1;
  assert(!RunQ38ResidentGdnOutput(*f.c,forged) && f.io.touches==n && f.transport->Touches()==d && !m->decoder.GdnOutputCompleted());forged.outputPublished=0;
  for(unsigned kind=1;kind<=6;kind++){
   pid_t child=fork();assert(child>=0);
   if(!child){
    f.transport->OutputFault(kind);bool ran=RunQ38ResidentGdnOutput(*f.c,f.decoderResult);
    printf("NATIVE54_FAULT_OBS kind%u ran%u injected%u phase%u worker%u word%u\n",kind,unsigned(ran),unsigned(f.transport->OutputFaultInjected()),f.decoderResult.phase,f.decoderResult.worker,f.decoderResult.word);
    if(kind==6)assert(ran && !CloseQ38ResidentDecoderModel(*f.c,f.decoderResult));
    else assert(!ran && !f.decoderResult.outputPublished);
    assert(f.transport->OutputFaultInjected() && !producer->Current() && !outputPlan->Current() && !bootstrap->Current());
    f.UnknownNative();n=f.io.touches;d=f.transport->Touches();
    assert(!RunQ38ResidentGdnOutput(*f.c,forged) && !CloseQ38ResidentDecoderModel(*f.c,forged)
     && !RunQ38ResidentGdnRecurrent(*f.c,forged) && f.io.touches==n && f.transport->Touches()==d);
    printf("PASS SAME ACTUAL native54 fault%u phase%u worker%u restored%u/real failed write-source-receipt-PC-restore/ALL owners-snapshot-hosts retained/ZERO following IO/reset-restore-retry\n",kind,f.decoderResult.phase,f.decoderResult.worker,f.decoderResult.memory.restoredWords);_exit(0);
   }
   int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));
  }
 }
#endif
 const auto beforeOutput=f.io.wholeWorkspace;
 assert(RunQ38ResidentGdnOutput(*f.c,f.decoderResult) && m->decoder.GdnOutputCompleted()
  && f.decoderResult.outputInstalled==120 && f.decoderResult.outputBooted==119 && f.decoderResult.outputRootRuns==2
  && f.decoderResult.outputStopped==119 && f.decoderResult.outputPublished==1);
 assert(!producer->Current() && !outputPlan->Current() && !bootstrap->Current() && !m->decoder.GatedOutput());
 f.transport->VerifyGdnOutput();assert(source->GdnOutputReads()==61440);
 for(unsigned i=0;i<beforeOutput.size();i++){
  unsigned at=i*4;bool written=(at>=131072 && at<131072+12288) || (at>=196608 && at<196608+3072)
   || (at>=212992 && at<212992+768) || (at>=TT_Q38_GDN_OUTPUT_RESULT && at<TT_Q38_GDN_OUTPUT_RESULT+20480);
  if(!written)assert(f.io.wholeWorkspace[i]==beforeOutput[i]);
 }
 n=f.io.touches;d=f.transport->Touches();
 assert(!RunQ38ResidentGdnOutput(*f.c,forged) && f.io.touches==n && f.transport->Touches()==d);
 puts("PASS SAME ACTUAL native54 claim/new pinned images/fresh119boot/QOP1RUN+STOP/61440original ssm_out tile reads/ALL5120FP32+FP64 rows/119physicalSTOP+ALL120holds/old producer EXPIRED/privateNEXT+CURRENT+unrelatedworkspace retained;NO native speed claim");
#endif
#ifdef PIONEER_Q38_RESIDUAL_RMS_NATIVE
 assert(!m->decoder.PostAttentionInput() && !m->decoder.PostAttentionCompleted());
 n=f.io.touches;d=f.transport->Touches();currentTeam=8;
 assert(!RunQ38ResidentResidualRms(*f.c,forged) && f.io.touches==n && f.transport->Touches()==d);currentTeam=7;
 forged.residualPublished=1;
 assert(!RunQ38ResidentResidualRms(*f.c,forged) && !m->decoder.PostAttentionInput() && f.io.touches==n && f.transport->Touches()==d);forged.residualPublished=0;
 for(unsigned i=0;i<m->plan.Count();i++){
  const auto* p=m->plan.Part(i);if(p->kind!=m->plan.WeightKind || p->layer || p->role!=1)continue;
  unsigned first=p->bank*786432+p->offset/4096;
  for(unsigned page=0;page<5;page++)for(unsigned value:{0U,2U}){
   f.learned[first+page]=value;assert(!RunQ38ResidentResidualRms(*f.c,forged) && m->decoder.GdnOutputCompleted()
    && !m->decoder.PostAttentionInput() && !m->Unsafe() && f.io.touches==n && f.transport->Touches()==d);f.learned[first+page]=1;
  }
 }
 if(argc==13)for(unsigned fault=1;fault<=9;fault++){
  pid_t child=fork();assert(child>=0);
  if(!child){
   f.transport->ResidualRmsFault(fault);bool ran=RunQ38ResidentResidualRms(*f.c,f.decoderResult);
   printf("NATIVE56_FAULT_OBS kind%u ran%u injected%u phase%u worker%u word%u\n",fault,unsigned(ran),unsigned(f.transport->ResidualRmsFaultInjected()),f.decoderResult.phase,f.decoderResult.worker,f.decoderResult.word);
   if(fault==6)assert(ran && !CloseQ38ResidentDecoderModel(*f.c,f.decoderResult));else assert(!ran && !f.decoderResult.residualPublished);
   assert(f.transport->ResidualRmsFaultInjected() && !m->decoder.PostAttentionInput() && !m->decoder.GdnOutputCompleted());
   f.UnknownNative();n=f.io.touches;d=f.transport->Touches();
   assert(!RunQ38ResidentResidualRms(*f.c,forged) && !RunQ38ResidentGdnOutput(*f.c,forged)
    && !CloseQ38ResidentDecoderModel(*f.c,forged) && !EndFactory(*f.c) && f.io.touches==n && f.transport->Touches()==d);
   printf("PASS ACTUAL SAME native56 fault%u/real code-descriptor-source-summary-PC-deadline-oldmain-math-restore failure/ALL owners+snapshots+hosts retained/ZERO following IO/reset-restore-retry\n",fault);_exit(0);
  }
  int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));
 }
 const auto beforeResidual=f.io.wholeWorkspace;
 assert(RunQ38ResidentResidualRms(*f.c,f.decoderResult) && m->decoder.PostAttentionCompleted() && !m->decoder.GdnOutputCompleted()
  && f.decoderResult.residualInstalled==10241 && f.decoderResult.residualRuns==1 && f.decoderResult.residualHeld==1 && f.decoderResult.residualPublished==1
  && source->ResidualRmsReads()==2 && source->CheckResidualRms(beforeResidual,f.io.wholeWorkspace));
 const auto* post=m->decoder.PostAttentionInput();assert(post && post->Current() && post->MatchesCohort(*cohort) && !post->MatchesCohort(*foreignCohort));
 for(unsigned i=0;i<beforeResidual.size();i++){unsigned a=i*4;bool written=a<20480 || (a>=TT_Q38_RMS_BACKUP && a<TT_Q38_RMS_BACKUP+20480);if(!written)assert(f.io.wholeWorkspace[i]==beforeResidual[i]);}
 n=f.io.touches;d=f.transport->Touches();
 assert(!RunQ38ResidentResidualRms(*f.c,forged) && !RunQ38ResidentGdnOutput(*f.c,forged) && f.io.touches==n && f.transport->Touches()==d);
 for(unsigned fault=0;fault<3;fault++){
  pid_t child=fork();assert(child>=0);if(!child){
   if(!fault)m->graph.Fault();if(fault==1)m->graph.Recurrent(m->identity)->Fault();if(fault==2)cohort->worker[119].image.running=true;
   assert(!post->Current() && !m->decoder.PostAttentionInput() && f.io.touches==n && f.transport->Touches()==d);_exit(0);
  }int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));
 }
 puts("PASS SAME ACTUAL native56 BOTH genuine lineages claimed BEFORE IO/QRR1 ORIGINAL postnorm5120/ALL5120 residual+normalized BIT-EXACT/10actualpackets/ROOT physical hold+ALL119STOP/typed FFN input publication/old output EXPIRED/unrelated GDDR unchanged;NO CPU products or new public selector");
#endif
 bool closed=CloseQ38ResidentDecoderModel(*f.c,f.decoderResult);
 printf("NATIVE_QGH_CLOSE closed%u phase%u worker%u word%u restored%u verified%u unsafe%u\n",unsigned(closed),f.decoderResult.phase,f.decoderResult.worker,f.decoderResult.word,
  f.decoderResult.memory.restoredWords,f.decoderResult.memory.verifiedOriginalWords,f.decoderResult.unsafe);
 assert(closed && !m->decoder.GdnGatedCompleted() && m->Closed()
  && f.decoderResult.memory.restoredWords==8330680 && f.decoderResult.memory.verifiedOriginalWords==8330680);
#ifdef PIONEER_Q38_RESIDUAL_RMS_NATIVE
 assert(!post->Current() && !m->decoder.PostAttentionInput() && !m->decoder.PostAttentionCompleted());
#endif
#ifdef PIONEER_Q38_GDN_OUTPUT_PRODUCER_NATIVE
 assert(!producer->Current() && !m->decoder.GatedOutput() && !outputPlan->Current() && !outputPlan->Producer()
  && !outputPlan->Part(0) && !outputPlan->Count() && !outputPlan->InputAddress() && !outputPlan->OutputAddress());
 assert(!outputPlan->InputDescriptor(0,*cohort,inputDescriptor));
 assert(!outputPlan->GatherDescriptor(*cohort,gatherDescriptor) && !outputPlan->GatherSources(*cohort,gatherSources));
 assert(!bootstrap->Current() && !bootstrap->Manifest() && !bootstrap->Sources());
 puts("PASS genuine gated producer expires on SAME owner's physical restore Close;NO stale input for later projection");
#endif
 assert(EndFactory(*f.c) && !sColdRecoveryRequired);
 puts("PASS ORIGINAL token0 SAME ACTUAL native init73728/history1680/QPE1/QRN1/4QPS1/QCB1ALL120->COOPERATIVE_QGH1_ALL96/829632original values/real NoC peer stamps+raw+originalnorm+retainedZ/private gatedGDDR;ALL96receipts/ALL120holds/immutableCURRENT+unrelatedstate/private layer0NEXT+gated publication/complete8330680word restore/actualEND;NO CPU numeric inputs/host stage boundary/Work promotion/current-slot-flip/nativeCARD_OR_SPEED_CLAIM");
}
