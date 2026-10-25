/* SPDX-License-Identifier: MIT */
// SAME actual native init/source/CBO owner -> concurrently emitted QGH1.
// Original references BEFORE context. ONLY device/kernel byte endpoints simulated.
#define PIONEER_Q38_RESIDENT_DECODER_NATIVE_FACTORY_ONLY
#include "pioneer_q38_resident_decoder_native_factory_test.cpp"
#include "pioneer_q38_decoder_original_source.h"
#ifdef PIONEER_Q38_NEXT_PROJECTION_NATIVE
#include "pioneer_q38_next_projection_execution_machine.h"
#endif
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
#ifdef PIONEER_Q38_RESIDENT_SILU_PRECISION_NATIVE
 static_assert(TT_Q38_CONV_BATCH_TEXT_BYTES==7560 && TT_Q38_CONV_BATCH_IMAGE_WORDS==1891,
  "native66 BOTH SAME-owner convolutions require the exact physically qualified precision image");
 puts("NATIVE66_RESIDENT_PRECISION_BOTH_CONVOLUTIONS;GENUINE_FULL_PREFIX_REQUIRED;NO_DEBUG64_OR_WORK");
#endif
#ifdef PIONEER_Q38_NEXT_PROJECTION_NATIVE
 assert(NextProjectionExecutionMachine::GrammarSelfTest()); // actual linked QPS2,not a weak QPS1 fixture
#endif
#ifdef PIONEER_Q38_FFN_RESIDUAL_NATIVE
#ifdef PIONEER_Q38_NEXT_RMS_NATIVE
#ifdef PIONEER_Q38_NEXT_PROJECTION_NATIVE
#ifdef PIONEER_Q38_NEXT_FFN_RESIDUAL_NATIVE
 assert(argc==18 || (argc==19 && !strcmp(argv[18],"--next-ffn-residual-faults")));setvbuf(stdout,nullptr,_IOLBF,0);
 const bool nextProjectionFaults=false,nextConvFaults=false,nextGdnFaults=false,nextOutputFaults=false,nextResidualFaults=false;
 const bool nextFfnFaults=argc==19;
#elif defined(PIONEER_Q38_NEXT_RESIDUAL_RMS_NATIVE)
 assert(argc==18 || (argc==19 && !strcmp(argv[18],"--next-residual-rms-faults")));setvbuf(stdout,nullptr,_IOLBF,0);
 const bool nextProjectionFaults=false,nextConvFaults=false,nextGdnFaults=false,nextOutputFaults=false;
 const bool nextResidualFaults=argc==19;
#elif defined(PIONEER_Q38_NEXT_GDN_OUTPUT_NATIVE)
 assert(argc==17 || (argc==18 && !strcmp(argv[17],"--next-gdn-output-faults")));setvbuf(stdout,nullptr,_IOLBF,0);
 const bool nextProjectionFaults=false,nextConvFaults=false,nextGdnFaults=false;
 const bool nextOutputFaults=argc==18;
#elif defined(PIONEER_Q38_NEXT_GDN_NATIVE)
 assert(argc==15 || (argc==16 && !strcmp(argv[15],"--next-gdn-faults")));setvbuf(stdout,nullptr,_IOLBF,0);
 const bool nextProjectionFaults=false,nextConvFaults=false,nextGdnFaults=argc==16;
#elif defined(PIONEER_Q38_NEXT_CONV_NATIVE)
 assert(argc==15 || (argc==16 && !strcmp(argv[15],"--next-conv-faults")));setvbuf(stdout,nullptr,_IOLBF,0);
 const bool nextProjectionFaults=false,nextConvFaults=argc==16;
#else
 assert(argc==15 || (argc==16 && !strcmp(argv[15],"--next-projection-faults")));setvbuf(stdout,nullptr,_IOLBF,0);
 const bool nextProjectionFaults=argc==16;
#endif
 const bool nextRmsFaults=false;
#else
 assert(argc==14 || (argc==15 && !strcmp(argv[14],"--next-rms-faults")));setvbuf(stdout,nullptr,_IOLBF,0);
 const bool nextRmsFaults=argc==15;
#endif
#else
 assert(argc==14 || (argc==15 && !strcmp(argv[14],"--ffn-faults")));setvbuf(stdout,nullptr,_IOLBF,0);
#endif
 const bool ffnFaults=argc==15 && !strcmp(argv[14],"--ffn-faults");
#elif defined(PIONEER_Q38_RESIDUAL_RMS_NATIVE)
 assert(argc==12 || (argc==13 && !strcmp(argv[12],"--residual-faults")));setvbuf(stdout,nullptr,_IOLBF,0);
#elif defined(PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE)
 assert(argc==11 || (argc==12 && !strcmp(argv[11],"--output-faults")));setvbuf(stdout,nullptr,_IOLBF,0);
#else
 assert(argc==9 || (argc==10 && (!strcmp(argv[9],"--delayed") || !strcmp(argv[9],"--late-negative") || !strcmp(argv[9],"--host-deadline") || !strcmp(argv[9],"--clock-receipt-fault") || !strcmp(argv[9],"--controller-faults"))));setvbuf(stdout,nullptr,_IOLBF,0);
#endif
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 puts("NATIVE_QGH_ALL_ORIGINAL_REFERENCES_BEFORE_CONTEXT;NO_DEVICE_ACCESS");
#ifdef PIONEER_Q38_NEXT_RMS_NATIVE
#ifdef PIONEER_Q38_NEXT_PROJECTION_NATIVE
#ifdef PIONEER_Q38_NEXT_FFN_RESIDUAL_NATIVE
 auto source=OpenNextFfnResidualOriginalSource(argv[7]);
#elif defined(PIONEER_Q38_NEXT_RESIDUAL_RMS_NATIVE)
 auto source=OpenNextResidualRmsOriginalSource(argv[7]);
#elif defined(PIONEER_Q38_NEXT_GDN_OUTPUT_NATIVE)
 auto source=OpenNextGdnOutputOriginalSource(argv[7]);
#elif defined(PIONEER_Q38_NEXT_GDN_NATIVE)
 auto source=OpenNextGdnOriginalSource(argv[7]);
#elif defined(PIONEER_Q38_NEXT_CONV_NATIVE)
 auto source=OpenNextConvOriginalSource(argv[7]);
#else
 auto source=OpenNextProjectionOriginalSource(argv[7]);
#endif
#else
 auto source=OpenNextRmsOriginalSource(argv[7]);
#endif
 static_assert(!std::is_default_constructible<PioneerQ38NextRmsProducer>::value && !std::is_copy_constructible<PioneerQ38NextRmsProducer>::value
  && !std::is_move_constructible<PioneerQ38NextRmsProducer>::value,"only actual SAME owner publishes next-layer normalized+fresh raw input");
#elif defined(PIONEER_Q38_FFN_RESIDUAL_NATIVE)
 auto source=OpenFfnResidualOriginalSource(argv[7]);
 static_assert(!std::is_default_constructible<PioneerQ38FfnHiddenProducer>::value
  && !std::is_copy_constructible<PioneerQ38FfnHiddenProducer>::value && !std::is_move_constructible<PioneerQ38FfnHiddenProducer>::value,"actual SAME native owner alone publishes next-layer hidden");
#elif defined(PIONEER_Q38_RESIDUAL_RMS_NATIVE)
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
#ifdef PIONEER_Q38_NEXT_FFN_RESIDUAL_NATIVE
 assert(source->NextFfnWeightsReady() && !source->NextFfnReads());
 {auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());unsigned stripes=0,pages=0;
  for(unsigned role=0;role<3;role++)for(unsigned stripe=0;stripe<7;stripe++){
   const auto* p=plan->FfnStripe(1,role,stripe);assert(p && p->type==12);unsigned first=p->bank*786432+p->offset/4096;
   for(unsigned page=0;page<p->allocated/4096;page++)f.learned[first+page]=1;stripes++;pages+=p->allocated/4096;
  }assert(stripes==21 && pages==36720);}
#endif
#ifdef PIONEER_Q38_NEXT_RESIDUAL_RMS_NATIVE
 assert(source->NextResidualRmsWeightsReady() && !source->NextResidualRmsReads());
 {auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());unsigned found=0;
  for(unsigned i=0;i<plan->Count();i++){const auto* p=plan->Part(i);if(p->kind!=plan->WeightKind || p->layer!=1 || p->role!=1)continue;
   assert(!p->type && p->bytes==20480 && p->allocated==20480);unsigned first=p->bank*786432+p->offset/4096;
   for(unsigned page=0;page<5;page++)f.learned[first+page]=1;found++;
  }assert(found==1);}
#endif
#ifdef PIONEER_Q38_NEXT_GDN_OUTPUT_NATIVE
 assert(source->NextGdnOutputWeightsReady() && !source->NextGdnOutputReads());
 {auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());unsigned stripes=0,pages=0;
  for(unsigned stripe=0;stripe<7;stripe++){
   const auto* p=plan->ProjectionStripe(1,13,stripe);assert(p && p->type==8 && p->rowBytes==6912);
   unsigned first=p->bank*786432+p->offset/4096;for(unsigned page=0;page<p->allocated/4096;page++)f.learned[first+page]=1;
   stripes++;pages+=p->allocated/4096;
  }assert(stripes==7 && pages==8640);}
#endif
#ifdef PIONEER_Q38_NEXT_GDN_NATIVE
 assert(source->NextGdnWeightsReady() && !source->NextGdnReads());
 {auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());unsigned mask=0;
  for(unsigned i=0;i<plan->Count();i++){const auto* p=plan->Part(i);if(p->kind!=plan->WeightKind || p->layer!=1 || p->role<7 || p->role>9)continue;
   assert(!p->type && p->allocated==4096);f.learned[p->bank*786432+p->offset/4096]=1;mask|=1U<<(p->role-7);
  }assert(mask==7);}
#endif
#ifdef PIONEER_Q38_NEXT_CONV_NATIVE
 assert(source->NextConvWeightsReady() && !source->NextConvReads());
 {auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());unsigned stripes=0,pages=0;
  for(unsigned i=0;i<plan->Count();i++){const auto* p=plan->Part(i);if(p->kind!=plan->WeightKind || p->layer!=1 || p->role!=12)continue;
   unsigned first=p->bank*786432+p->offset/4096;for(unsigned page=0;page<6;page++)f.learned[first+page]=1;stripes++;pages+=6;
  }assert(stripes==7 && pages==42);}
#endif
#ifdef PIONEER_Q38_NEXT_PROJECTION_NATIVE
 assert(source->NextProjectionWeightsReady() && !source->NextProjectionReads());
 {auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());unsigned stripes=0,pages=0;
  for(unsigned role:{10U,11U,5U,6U})for(unsigned stripe=0;stripe<(role==10 || role==11?1U:7U);stripe++){
   const auto* p=plan->ProjectionStripe(1,role,stripe);assert(p && p->type==8 && p->rowBytes==5760);
   unsigned first=p->bank*786432+p->offset/4096;for(unsigned page=0;page<p->allocated/4096;page++)f.learned[first+page]=1;
   stripes++;pages+=p->allocated/4096;
  }assert(stripes==16 && pages==23176);}
#endif
#ifdef PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE
 assert(source->GdnOutputWeightsReady() && !source->GdnOutputReads());
 {auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());unsigned count=0;
  for(unsigned i=0;i<canonical->Count();i++){
   const auto* p=canonical->Part(i);if(p->kind!=canonical->WeightKind || p->layer || p->role!=13)continue;
   for(unsigned page=0;page<p->allocated/4096;page++)f.learned[p->bank*786432+p->offset/4096+page]=1;count++;
  }
  assert(count==7);}
#endif
#ifdef PIONEER_Q38_NEXT_RMS_NATIVE
 assert(source->NextRmsWeightsReady() && !source->NextRmsReads());
 {auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());unsigned found=0;
  for(unsigned i=0;i<canonical->Count();i++){const auto* p=canonical->Part(i);if(p->kind!=canonical->WeightKind || p->layer!=1 || p->role)continue;
   assert(!p->type && p->allocated==20480);unsigned first=p->bank*786432+p->offset/4096;
   for(unsigned page=0;page<5;page++)f.learned[first+page]=1;found++;
  }assert(found==1);}
#endif
#ifdef PIONEER_Q38_FFN_RESIDUAL_NATIVE
 assert(source->FfnWeightsReady() && !source->FfnReads());
 {auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());unsigned count=0,pages=0;
  for(unsigned role=0;role<3;role++)for(unsigned stripe=0;stripe<7;stripe++){
   const auto* p=canonical->FfnStripe(0,role,stripe);unsigned first=p->bank*786432+p->offset/4096;
   for(unsigned page=0;page<p->allocated/4096;page++)f.learned[first+page]=1;count++;pages+=p->allocated/4096;
  }
  assert(count==21 && pages==36720);}
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
#ifdef PIONEER_Q38_FFN_RESIDUAL_NATIVE
 {auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());unsigned refused=0;
  for(unsigned role=0;role<3;role++)for(unsigned stripe=0;stripe<7;stripe++){
   const auto* p=canonical->FfnStripe(0,role,stripe);unsigned first=p->bank*786432+p->offset/4096,last=first+p->allocated/4096-1;
   for(unsigned at:{first,last})for(unsigned value:{0U,2U}){
    f.learned[at]=value;auto epoch=sQ38EmbeddingModelEpoch;unsigned touches=f.io.touches;
    assert(!BindQ38EmbeddingModel(*f.c,0,0,0) && !f.c->q38EmbeddingModel && f.io.touches==touches && sQ38EmbeddingModelEpoch==epoch);
    f.learned[at]=1;refused++;
   }
  }
  assert(refused==84);puts("PASS NATIVE57 ALL21 ORIGINAL FFN stripes first+last pages missing/corrupt refuse BEFORE epoch/claim/IO");}
#endif
#ifdef PIONEER_Q38_NEXT_RMS_NATIVE
 // ALL other required FFN/postnorm/GDN pages are present before these
 // refusals;missing earlier-stage pages cannot mask an ignored new gate.
 {auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());unsigned found=0;
  for(unsigned i=0;i<canonical->Count();i++){const auto* p=canonical->Part(i);if(p->kind!=canonical->WeightKind || p->layer!=1 || p->role)continue;
   unsigned first=p->bank*786432+p->offset/4096;
   for(unsigned page=0;page<5;page++)for(unsigned value:{0U,2U}){f.learned[first+page]=value;auto epoch=sQ38EmbeddingModelEpoch;unsigned touches=f.io.touches;
    assert(!BindQ38EmbeddingModel(*f.c,0,0,0) && !f.c->q38EmbeddingModel && f.io.touches==touches && sQ38EmbeddingModelEpoch==epoch);f.learned[first+page]=1;
   }found++;
  }assert(found==1);puts("PASS NATIVE58 ALL5 ORIGINAL layer1 pre-attention norm pages missing/corrupt refuse BEFORE epoch/claim/IO");}
#endif
#ifdef PIONEER_Q38_NEXT_CONV_NATIVE
 {auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());unsigned refused=0;
  for(unsigned i=0;i<plan->Count();i++){const auto* p=plan->Part(i);if(p->kind!=plan->WeightKind || p->layer!=1 || p->role!=12)continue;
   unsigned first=p->bank*786432+p->offset/4096;
   for(unsigned page=0;page<6;page++)for(unsigned value:{0U,2U}){auto epoch=sQ38EmbeddingModelEpoch;unsigned touches=f.io.touches;
    f.learned[first+page]=value;assert(!BindQ38EmbeddingModel(*f.c,0,0,0) && !f.c->q38EmbeddingModel && f.io.touches==touches && sQ38EmbeddingModelEpoch==epoch);
    f.learned[first+page]=1;refused++;
   }
  }assert(refused==84);puts("PASS native60 ALL42 original layer1 F32conv pages missing/corrupt refused BEFORE epoch/claim/IO");}
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
#ifdef PIONEER_Q38_NEXT_PROJECTION_NATIVE
 {auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());unsigned refused=0;
  for(unsigned role:{10U,11U,5U,6U})for(unsigned stripe=0;stripe<(role==10 || role==11?1U:7U);stripe++){
   const auto* p=plan->ProjectionStripe(1,role,stripe);unsigned first=p->bank*786432+p->offset/4096;
   for(unsigned at:{first,first+p->allocated/4096-1})for(unsigned value:{0U,2U}){
    f.learned[at]=value;auto epoch=sQ38EmbeddingModelEpoch;unsigned touches=f.io.touches;
    assert(!BindQ38EmbeddingModel(*f.c,0,0,0) && !f.c->q38EmbeddingModel && f.io.touches==touches && sQ38EmbeddingModelEpoch==epoch);f.learned[at]=1;refused++;
   }
  }assert(refused==64);puts("PASS native59 ALL16 ORIGINAL layer1 projection stripes first+last missing/corrupt refused BEFORE epoch/claim/IO");}
#endif
#ifdef PIONEER_Q38_NEXT_GDN_NATIVE
 {auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());unsigned refused=0;
  for(unsigned i=0;i<plan->Count();i++){const auto* p=plan->Part(i);if(p->kind!=plan->WeightKind || p->layer!=1 || p->role<7 || p->role>9)continue;
   unsigned at=p->bank*786432+p->offset/4096;
   for(unsigned value:{0U,2U}){auto epoch=sQ38EmbeddingModelEpoch;unsigned touches=f.io.touches;f.learned[at]=value;
    assert(!BindQ38EmbeddingModel(*f.c,0,0,0) && !f.c->q38EmbeddingModel && f.io.touches==touches && sQ38EmbeddingModelEpoch==epoch);f.learned[at]=1;refused++;
   }
  }assert(refused==6);puts("PASS native61 ALL3 original layer1 A+dt+norm pages missing/corrupt refused BEFORE epoch/claim/IO");}
#endif
#ifdef PIONEER_Q38_NEXT_GDN_OUTPUT_NATIVE
 {auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());unsigned refused=0;
  for(unsigned stripe=0;stripe<7;stripe++){
   const auto* p=plan->ProjectionStripe(1,13,stripe);unsigned first=p->bank*786432+p->offset/4096;
   for(unsigned at:{first,first+p->allocated/4096-1})for(unsigned value:{0U,2U}){
    auto epoch=sQ38EmbeddingModelEpoch;unsigned touches=f.io.touches;f.learned[at]=value;
    assert(!BindQ38EmbeddingModel(*f.c,0,0,0) && !f.c->q38EmbeddingModel && f.io.touches==touches && sQ38EmbeddingModelEpoch==epoch);
    f.learned[at]=1;refused++;
   }
  }assert(refused==28);puts("PASS native62 ALL7 original layer1 output stripes first+last missing/corrupt refused BEFORE epoch/claim/IO");}
#endif
#ifdef PIONEER_Q38_NEXT_RESIDUAL_RMS_NATIVE
 {auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());unsigned refused=0;
  for(unsigned i=0;i<plan->Count();i++){const auto* p=plan->Part(i);if(p->kind!=plan->WeightKind || p->layer!=1 || p->role!=1)continue;
   unsigned first=p->bank*786432+p->offset/4096;
   for(unsigned page=0;page<5;page++)for(unsigned value:{0U,2U}){
    auto epoch=sQ38EmbeddingModelEpoch;unsigned touches=f.io.touches;f.learned[first+page]=value;
    assert(!BindQ38EmbeddingModel(*f.c,0,0,0) && !f.c->q38EmbeddingModel && f.io.touches==touches && sQ38EmbeddingModelEpoch==epoch);
    f.learned[first+page]=1;refused++;
   }
  }assert(refused==10);puts("PASS native63 ALL5 ORIGINAL layer1 post-attention norm pages missing/corrupt refused BEFORE epoch/claim/IO");}
#endif
#ifdef PIONEER_Q38_NEXT_FFN_RESIDUAL_NATIVE
 {auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());unsigned refused=0;
  for(unsigned role=0;role<3;role++)for(unsigned stripe=0;stripe<7;stripe++){
   const auto* p=plan->FfnStripe(1,role,stripe);unsigned first=p->bank*786432+p->offset/4096;
   for(unsigned at:{first,first+p->allocated/4096-1})for(unsigned bad:{0U,2U}){
    f.learned[at]=bad;auto epoch=sQ38EmbeddingModelEpoch;unsigned touches=f.io.touches;
    assert(!BindQ38EmbeddingModel(*f.c,0,0,0) && !f.c->q38EmbeddingModel && f.io.touches==touches && sQ38EmbeddingModelEpoch==epoch);
    f.learned[at]=1;refused++;
   }
  }assert(refused==84);puts("PASS native65 ALL21 ORIGINAL layer1 FFN stripes first+last missing/corrupt refused BEFORE epoch/claim/IO");}
#endif
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
#ifdef PIONEER_Q38_FFN_RESIDUAL_NATIVE
 f.transport->FfnSources(argv[12],argv[13]);
#endif
#ifdef PIONEER_Q38_NEXT_FFN_RESIDUAL_NATIVE
 f.transport->NextFfnSources(argv[12],argv[13]);
#endif
#ifdef PIONEER_Q38_NEXT_RMS_NATIVE
 f.transport->NextRmsSource(argv[5]);
#endif
#ifdef PIONEER_Q38_NEXT_PROJECTION_NATIVE
 f.transport->NextProjectionSources(argv[14],argv[2]);
#endif
#ifdef PIONEER_Q38_NEXT_GDN_OUTPUT_NATIVE
 f.transport->NextOutputSources(argv[15],argv[16]);
#endif
#ifdef PIONEER_Q38_NEXT_RESIDUAL_RMS_NATIVE
 f.transport->NextResidualSource(argv[17]);
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
#ifdef PIONEER_Q38_FFN_RESIDUAL_NATIVE
 assert(!m->decoder.FfnCompleted() && !m->decoder.FfnHiddenInput());n=f.io.touches;d=f.transport->Touches();currentTeam=8;
 assert(!RunQ38ResidentFfn(*f.c,forged) && post->Current() && f.io.touches==n && f.transport->Touches()==d);currentTeam=7;
 forged.ffnPublished=1;
 assert(!RunQ38ResidentFfn(*f.c,forged) && post->Current() && !m->decoder.FfnHiddenInput() && f.io.touches==n && f.transport->Touches()==d);forged.ffnPublished=0;
 for(unsigned role=0;role<3;role++)for(unsigned stripe=0;stripe<7;stripe++){
  const auto* p=m->plan.FfnStripe(0,role,stripe);unsigned first=p->bank*786432+p->offset/4096,last=first+p->allocated/4096-1;
  for(unsigned at:{first,last})for(unsigned value:{0U,2U}){
   f.learned[at]=value;assert(!RunQ38ResidentFfn(*f.c,forged) && post->Current() && !m->decoder.FfnHiddenInput() && !m->Unsafe()
    && f.io.touches==n && f.transport->Touches()==d);f.learned[at]=1;
  }
 }
 auto checkFfnFault=[&](unsigned fault,bool ran){
  printf("NATIVE57_FAULT_OBS kind%u ran%u injected%u phase%u worker%u word%u installed%u booted%u runs%u stopped%u published%u\n",fault,unsigned(ran),unsigned(f.transport->FfnFaultInjected()),
   f.decoderResult.phase,f.decoderResult.worker,f.decoderResult.word,f.decoderResult.ffnInstalled,f.decoderResult.ffnBooted,f.decoderResult.ffnRootRuns,f.decoderResult.ffnStopped,f.decoderResult.ffnPublished);
  if(fault==6)assert(ran && !CloseQ38ResidentDecoderModel(*f.c,f.decoderResult));else assert(!ran && !f.decoderResult.ffnPublished);
  assert(f.transport->FfnFaultInjected() && !m->decoder.FfnHiddenInput() && !post->Current() && !m->decoder.PostAttentionInput());
  f.UnknownNative();unsigned host=f.io.touches,device=f.transport->Touches();
  assert(!RunQ38ResidentFfn(*f.c,forged) && !RunQ38ResidentResidualRms(*f.c,forged) && !RunQ38ResidentGdnOutput(*f.c,forged)
   && !CloseQ38ResidentDecoderModel(*f.c,forged) && !EndFactory(*f.c) && f.io.touches==host && f.transport->Touches()==device);
  printf("PASS ACTUAL SAME native57 fault%u/real code-descriptor-source-summary-PC-deadline-math-restore failure/ALL owners+snapshots+hosts retained/ZERO following IO/reset-restore-retry\n",fault);
 };
 if(ffnFaults)for(unsigned fault:{1U,2U,3U,7U,9U}){
  pid_t child=fork();assert(child>=0);if(!child){f.transport->FfnFault(fault);checkFfnFault(fault,RunQ38ResidentFfn(*f.c,f.decoderResult));_exit(0);}
  int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));
 }
 unsigned lateFfnFault=0;
 struct LateFfnContext {ProjectionNativeTransport* transport;unsigned* fault;};LateFfnContext lateFfn={f.transport.get(),&lateFfnFault};
 if(ffnFaults)f.transport->FfnRunTerminalCheckpoint([](void* context){
  auto& late=*static_cast<LateFfnContext*>(context);
  // Fork ONLY this actual in-flight native call after its complete emitted
  // FFN RUN,BEFORE receipt acceptance. No owner/sequence/ready fabrication.
  for(unsigned fault:{4U,5U,6U,8U}){
   pid_t child=fork();assert(child>=0);if(!child){*late.fault=fault;late.transport->FfnFault(fault);return;}
   int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));
  }
 },&lateFfn);
 const auto beforeFfn=f.io.wholeWorkspace;
 bool ranFfn=RunQ38ResidentFfn(*f.c,f.decoderResult);
 if(lateFfnFault){checkFfnFault(lateFfnFault,ranFfn);_exit(0);}
 printf("NATIVE57_RUN ran%u phase%u worker%u word%u installed%u booted%u runs%u stopped%u published%u unsafe%u\n",unsigned(ranFfn),f.decoderResult.phase,f.decoderResult.worker,
  f.decoderResult.word,f.decoderResult.ffnInstalled,f.decoderResult.ffnBooted,f.decoderResult.ffnRootRuns,f.decoderResult.ffnStopped,f.decoderResult.ffnPublished,f.decoderResult.unsafe);
 assert(ranFfn && m->decoder.FfnCompleted() && !post->Current() && !m->decoder.PostAttentionInput()
  && f.decoderResult.ffnInstalled==120 && f.decoderResult.ffnBooted==119 && f.decoderResult.ffnRootRuns==2 && f.decoderResult.ffnStopped==119 && f.decoderResult.ffnPublished==1);
 const auto* hidden=m->decoder.FfnHiddenInput();assert(hidden && hidden->Current() && hidden->MatchesCohort(*cohort) && !hidden->MatchesCohort(*foreignCohort));
 f.transport->VerifyFfn();assert(source->FfnReads()==65280 && source->CheckFfnResidual(beforeFfn,f.io.wholeWorkspace));
 n=f.io.touches;d=f.transport->Touches();assert(!RunQ38ResidentFfn(*f.c,forged) && !RunQ38ResidentResidualRms(*f.c,forged)
  && !RunQ38ResidentGdnOutput(*f.c,forged) && f.io.touches==n && f.transport->Touches()==d);
 for(unsigned fault=0;fault<3;fault++){
  pid_t child=fork();assert(child>=0);if(!child){
   if(!fault)m->graph.Fault();if(fault==1)m->graph.Recurrent(m->identity)->Fault();if(fault==2)cohort->worker[119].image.running=true;
   assert(!hidden->Current() && !m->decoder.FfnHiddenInput() && f.io.touches==n && f.transport->Touches()==d);_exit(0);
  }int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));
 }
 puts("PASS SAME ACTUAL native57 consumes genuine post-attention producer BEFORE IO/ALL21originalFFNstripes/357actualmatrices+88actualproducers+119STOP/final5120residual BIT-EXACT/private next-layer hidden/old input EXPIRED;NO CPU products/public selector/card speed claim");
#endif
#ifdef PIONEER_Q38_NEXT_RMS_NATIVE
 assert(!m->decoder.NextLayerInput() && !m->decoder.NextRmsCompleted());n=f.io.touches;d=f.transport->Touches();currentTeam=8;
 assert(!RunQ38NextRms(*f.c,forged) && hidden->Current() && f.io.touches==n && f.transport->Touches()==d);currentTeam=7;
 forged.nextRmsPublished=1;
 assert(!RunQ38NextRms(*f.c,forged) && hidden->Current() && !m->decoder.NextLayerInput() && f.io.touches==n && f.transport->Touches()==d);forged.nextRmsPublished=0;
 for(unsigned i=0;i<m->plan.Count();i++){const auto* p=m->plan.Part(i);if(p->kind!=m->plan.WeightKind || p->layer!=1 || p->role)continue;
  unsigned first=p->bank*786432+p->offset/4096;
  for(unsigned page=0;page<5;page++)for(unsigned value:{0U,2U}){f.learned[first+page]=value;
   assert(!RunQ38NextRms(*f.c,forged) && hidden->Current() && !m->decoder.NextLayerInput() && !m->Unsafe()
    && f.io.touches==n && f.transport->Touches()==d);f.learned[first+page]=1;
  }
 }
 const auto beforeNextRms=f.io.wholeWorkspace;bool staleDiffers=false;
 for(unsigned i=0;i<5120;i++)staleDiffers |= beforeNextRms[i]!=beforeNextRms[TT_Q38_RMS_BACKUP/4+i];assert(staleDiffers);
 if(nextRmsFaults)for(unsigned fault=1;fault<=9;fault++){
  pid_t child=fork();assert(child>=0);if(!child){
   f.transport->NextRmsFault(fault);bool ran=RunQ38NextRms(*f.c,f.decoderResult);
   if(fault==6)assert(ran && !CloseQ38ResidentDecoderModel(*f.c,f.decoderResult));else assert(!ran && !f.decoderResult.nextRmsPublished);
   assert(f.transport->NextRmsFaultInjected() && !hidden->Current() && !m->decoder.NextLayerInput() && !m->decoder.FfnHiddenInput());
   if(fault==9)assert(f.io.wholeWorkspace==beforeNextRms && source->NextRmsReads()==2);
   printf("NATIVE58_FAULT_OBS kind%u ran%u injected1 phase%u worker%u word%u installed%u runs%u held%u published%u\n",fault,unsigned(ran),f.decoderResult.phase,
    f.decoderResult.worker,f.decoderResult.word,f.decoderResult.nextRmsInstalled,f.decoderResult.nextRmsRuns,f.decoderResult.nextRmsHeld,f.decoderResult.nextRmsPublished);
   f.UnknownNative();unsigned host=f.io.touches,device=f.transport->Touches();
   assert(!RunQ38NextRms(*f.c,forged) && !RunQ38ResidentFfn(*f.c,forged) && !CloseQ38ResidentDecoderModel(*f.c,forged)
    && !EndFactory(*f.c) && f.io.touches==host && f.transport->Touches()==device);
   printf("PASS SAME ACTUAL native58 fault%u real code/descriptor/source/summary/PC/restore/deadline/oldmain/NaN failure;ALL resources retained/ZERO following IO-reset-restore-retry\n",fault);_exit(0);
  }int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));
 }
 bool ranNext=RunQ38NextRms(*f.c,f.decoderResult);
 printf("NATIVE58_RUN ran%u phase%u worker%u word%u installed%u runs%u held%u published%u unsafe%u\n",unsigned(ranNext),f.decoderResult.phase,f.decoderResult.worker,
  f.decoderResult.word,f.decoderResult.nextRmsInstalled,f.decoderResult.nextRmsRuns,f.decoderResult.nextRmsHeld,f.decoderResult.nextRmsPublished,f.decoderResult.unsafe);
 assert(ranNext && m->decoder.NextRmsCompleted() && !hidden->Current() && !m->decoder.FfnHiddenInput()
  && f.decoderResult.nextRmsInstalled==10241 && f.decoderResult.nextRmsRuns==1 && f.decoderResult.nextRmsHeld==1 && f.decoderResult.nextRmsPublished==1);
 const auto* nextInput=m->decoder.NextLayerInput();assert(nextInput && nextInput->Current() && nextInput->MatchesCohort(*cohort) && !nextInput->MatchesCohort(*foreignCohort));
 f.transport->VerifyNextRms();assert(source->NextRmsReads()==2 && source->CheckNextRms(beforeNextRms,f.io.wholeWorkspace));
 for(unsigned i=0;i<beforeNextRms.size();i++){unsigned a=i*4;if(a>=20480 && (a<TT_Q38_RMS_BACKUP || a>=TT_Q38_RMS_BACKUP+20480))assert(f.io.wholeWorkspace[i]==beforeNextRms[i]);}
 n=f.io.touches;d=f.transport->Touches();assert(!RunQ38NextRms(*f.c,forged) && !RunQ38ResidentFfn(*f.c,forged) && f.io.touches==n && f.transport->Touches()==d);
 for(unsigned fault=0;fault<3;fault++){pid_t child=fork();assert(child>=0);if(!child){
   if(!fault)m->graph.Fault();if(fault==1)m->graph.Recurrent(m->identity)->Fault();if(fault==2)cohort->worker[119].image.running=true;
   assert(!nextInput->Current() && !m->decoder.NextLayerInput() && f.io.touches==n && f.transport->Touches()==d);_exit(0);
  }int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));}
 puts("PASS SAME ACTUAL native58 claims genuine FFN hidden BEFORE IO/actual FINAL STOP+1 epoch/ORIGINAL layer1 norm5120/8emitted packets/5120normalized+FRESHraw BITEXACT/ALL120held/private next-layer input+expiry/unrelatedGDDR unchanged;NO initialRMS-facade/hostproducts/publicselector/CARDspeed claim");
#endif
#ifdef PIONEER_Q38_NEXT_PROJECTION_NATIVE
 static_assert(!std::is_default_constructible<PioneerQ38NextProjectionProducer>::value && !std::is_copy_constructible<PioneerQ38NextProjectionProducer>::value
  && !std::is_move_constructible<PioneerQ38NextProjectionProducer>::value,"only SAME actual decoder publishes next projections");
 n=f.io.touches;d=f.transport->Touches();currentTeam=8;
 assert(!RunQ38NextProjections(*f.c,forged) && nextInput->Current() && f.io.touches==n && f.transport->Touches()==d);currentTeam=7;
 forged.nextProjectionPublished=1;assert(!RunQ38NextProjections(*f.c,forged) && nextInput->Current() && !m->decoder.NextProjectionInput()
  && f.io.touches==n && f.transport->Touches()==d);forged.nextProjectionPublished=0;
 for(unsigned role:{10U,11U,5U,6U})for(unsigned stripe=0;stripe<(role==10 || role==11?1U:7U);stripe++){
  const auto* p=m->plan.ProjectionStripe(1,role,stripe);unsigned first=p->bank*786432+p->offset/4096;
  for(unsigned at:{first,first+p->allocated/4096-1})for(unsigned value:{0U,2U}){
   f.learned[at]=value;assert(!RunQ38NextProjections(*f.c,forged) && nextInput->Current() && !m->decoder.NextProjectionInput() && !m->Unsafe()
    && f.io.touches==n && f.transport->Touches()==d);f.learned[at]=1;
  }
 }
 auto checkNextProjectionFault=[&](unsigned fault,bool ran){
  bool closed=fault==6 && ran?CloseQ38ResidentDecoderModel(*f.c,f.decoderResult):false;
  printf("NATIVE59_FAULT_OBS kind%u ran%u injected%u phase%u worker%u word%u installed%u booted%u runs%u stopped%u published%u\n",fault,unsigned(ran),unsigned(f.transport->NextProjectionFaultInjected()),
   f.decoderResult.phase,f.decoderResult.worker,f.decoderResult.word,f.decoderResult.nextProjectionInstalled,f.decoderResult.nextProjectionBooted,
   f.decoderResult.nextProjectionRuns,f.decoderResult.nextProjectionStopped,f.decoderResult.nextProjectionPublished);
  if(fault==6)assert(ran && !closed);else assert(!ran && !f.decoderResult.nextProjectionPublished);
  assert(f.transport->NextProjectionFaultInjected() && !nextInput->Current() && !m->decoder.NextLayerInput() && !m->decoder.NextProjectionInput());
  f.UnknownNative();unsigned host=f.io.touches,device=f.transport->Touches();
  assert(!RunQ38NextProjections(*f.c,forged) && !RunQ38NextRms(*f.c,forged) && !RunQ38ResidentFfn(*f.c,forged)
   && !RunQ38ResidentResidualRms(*f.c,forged) && !RunQ38ResidentGdnOutput(*f.c,forged)
   && !CloseQ38ResidentDecoderModel(*f.c,forged) && !EndFactory(*f.c) && f.io.touches==host && f.transport->Touches()==device);
  printf("PASS SAME ACTUAL native59 fault%u real code/descriptor/source/summary/STOP-PC/restore/deadline/table failure;ALL owners+snapshots+hosts retained/ZERO following IO-reset-restore-retry-END\n",fault);
 };
 if(nextProjectionFaults)for(unsigned fault:{1U,2U,3U,4U,7U,8U}){
  pid_t child=fork();assert(child>=0);if(!child){f.transport->NextProjectionFault(fault);checkNextProjectionFault(fault,RunQ38NextProjections(*f.c,f.decoderResult));_exit(0);}
  int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));
 }
 unsigned lateNextProjectionFault=0;
 struct LateNextProjectionContext {ProjectionNativeTransport* transport;unsigned* fault;};
 LateNextProjectionContext lateNextProjection={f.transport.get(),&lateNextProjectionFault};
 if(nextProjectionFaults)f.transport->NextProjectionRunTerminalCheckpoint([](void* context){
  auto& late=*static_cast<LateNextProjectionContext*>(context);
  // Actual last QPS2 RUN has executed,BEFORE receipt acceptance. Fork the
  // real live stack/cohort,never fabricate a producer or completion bit.
  for(unsigned fault:{5U,6U}){
   pid_t child=fork();assert(child>=0);if(!child){*late.fault=fault;late.transport->NextProjectionFault(fault);return;}
   int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));
  }
 },&lateNextProjection);
 const auto beforeNextProjection=f.io.wholeWorkspace;bool ranNextProjection=RunQ38NextProjections(*f.c,f.decoderResult);
 if(lateNextProjectionFault){checkNextProjectionFault(lateNextProjectionFault,ranNextProjection);_exit(0);}
 printf("NATIVE59_RUN ran%u phase%u worker%u word%u installed%u booted%u runs%u stopped%u published%u unsafe%u\n",unsigned(ranNextProjection),f.decoderResult.phase,f.decoderResult.worker,f.decoderResult.word,
  f.decoderResult.nextProjectionInstalled,f.decoderResult.nextProjectionBooted,f.decoderResult.nextProjectionRuns,f.decoderResult.nextProjectionStopped,f.decoderResult.nextProjectionPublished,f.decoderResult.unsafe);
 assert(ranNextProjection && m->decoder.NextProjectionsCompleted() && !nextInput->Current() && !m->decoder.NextLayerInput()
  && f.decoderResult.nextProjectionInstalled==120 && f.decoderResult.nextProjectionBooted==119 && f.decoderResult.nextProjectionRuns==5
  && f.decoderResult.nextProjectionStopped==119 && f.decoderResult.nextProjectionPublished==1);
 const auto* nextProjection=m->decoder.NextProjectionInput();assert(nextProjection && nextProjection->Current()
  && nextProjection->MatchesCohort(*cohort) && !nextProjection->MatchesCohort(*foreignCohort));
 f.transport->VerifyNextProjections();assert(source->NextProjectionReads()==164800 && source->CheckNextProjections(beforeNextProjection,f.io.wholeWorkspace));
 for(unsigned i=0;i<beforeNextProjection.size();i++){
  unsigned a=i*4;bool writable=(a>=TT_Q38_PROJECTION_NATIVE_OFFSET && a<TT_Q38_PROJECTION_NATIVE_OFFSET+10240)
   || (a>=TT_Q38_PROJECTION_LINES_OFFSET && a<TT_Q38_PROJECTION_LINES_OFFSET+2560)
   || (a>=TT_Q38_PROJECTION_COMPACT_OFFSET && a<TT_Q38_PROJECTION_COMPACT_OFFSET+640);
  for(unsigned role:{10U,11U,5U,6U}){unsigned at=PioneerQ38ProjectionResultOffset(role);writable |= a>=at && a<at+PioneerQ38ProjectionResultRows(role)*4;}
  if(!writable)assert(f.io.wholeWorkspace[i]==beforeNextProjection[i]);
 } // Preserve convolution scratch/raw backup/old FFN/untouched result padding outside the exact writable ranges.
 n=f.io.touches;d=f.transport->Touches();assert(!RunQ38NextProjections(*f.c,forged) && !RunQ38NextRms(*f.c,forged) && f.io.touches==n && f.transport->Touches()==d);
 for(unsigned fault=0;fault<3;fault++){pid_t child=fork();assert(child>=0);if(!child){
   if(!fault)m->graph.Fault();if(fault==1)m->graph.Recurrent(m->identity)->Fault();if(fault==2)cohort->worker[119].image.running=true;
   assert(!nextProjection->Current() && !m->decoder.NextProjectionInput() && f.io.touches==n && f.transport->Touches()==d);_exit(0);
  }int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));}
 puts("PASS SAME ACTUAL native59 claims genuine native58 input/ALL16ORIGINAL layer1 stripes/4QPS2+80producers+244matrices/164800original tile reads/ALL16480independent original packed-row references bitexact/119STOP+120holds/expiredoldinput/private next projections+unchangedraw;NO initial RMS facade/CPUproducts/Work/publicselector/CARDspeed claim");
#endif
#ifdef PIONEER_Q38_NEXT_CONV_NATIVE
 static_assert(!std::is_default_constructible<PioneerQ38NextConvProducer>::value && !std::is_copy_constructible<PioneerQ38NextConvProducer>::value
  && !std::is_move_constructible<PioneerQ38NextConvProducer>::value,"only SAME actual decoder publishes next convolution");
 n=f.io.touches;d=f.transport->Touches();currentTeam=8;
 assert(!RunQ38NextConv(*f.c,forged) && nextProjection->Current() && f.io.touches==n && f.transport->Touches()==d);currentTeam=7;
 forged.nextConvPublished=1;assert(!RunQ38NextConv(*f.c,forged) && nextProjection->Current() && !m->decoder.NextConvolutionInput()
  && f.io.touches==n && f.transport->Touches()==d);forged.nextConvPublished=0;
 for(unsigned i=0;i<m->plan.Count();i++){const auto* p=m->plan.Part(i);if(p->kind!=m->plan.WeightKind || p->layer!=1 || p->role!=12)continue;
  unsigned first=p->bank*786432+p->offset/4096;
  for(unsigned page=0;page<6;page++)for(unsigned value:{0U,2U}){f.learned[first+page]=value;
   assert(!RunQ38NextConv(*f.c,forged) && nextProjection->Current() && !m->decoder.NextConvolutionInput() && !m->Unsafe()
    && f.io.touches==n && f.transport->Touches()==d);f.learned[first+page]=1;
  }
 }
 const auto beforeNextConv=f.io.wholeWorkspace;
 if(nextConvFaults)for(unsigned fault=1;fault<=8;fault++){
  pid_t child=fork();assert(child>=0);if(!child){
   // Fork the SAME genuinely completed native59 stack;no fabricated old
   // producer,completion flag,initialization grant,or shortened prefix.
   f.transport->NextConvFault(fault);bool ran=RunQ38NextConv(*f.c,f.decoderResult);
   bool closed=fault==6 && ran?CloseQ38ResidentDecoderModel(*f.c,f.decoderResult):false;
   printf("NATIVE60_FAULT_OBS kind%u ran%u injected%u phase%u worker%u word%u installed%u armed%u released%u completed%u held%u published%u actualStarts%u\n",
    fault,unsigned(ran),unsigned(f.transport->NextConvFaultInjected()),f.decoderResult.phase,f.decoderResult.worker,f.decoderResult.word,
    f.decoderResult.nextConvInstalled,f.decoderResult.nextConvArmed,f.decoderResult.nextConvReleased,f.decoderResult.nextConvCompleted,
    f.decoderResult.nextConvHeld,f.decoderResult.nextConvPublished,f.transport->NextConvStarts());
   if(fault==6)assert(ran && !closed && f.decoderResult.nextConvPublished==1);
   else assert(!ran && !f.decoderResult.nextConvPublished);
   assert(f.transport->NextConvFaultInjected() && !nextProjection->Current() && !m->decoder.NextProjectionInput()
    && !m->decoder.NextConvolutionInput() && !m->decoder.NextConvCompleted());
   if(fault<=2)assert(!f.transport->NextConvStarts() && f.io.wholeWorkspace==beforeNextConv);
   if(fault==3 || fault==7)assert(f.transport->NextConvStarts()==1 && source->NextConvReads()==6
    && !f.decoderResult.nextConvReleased && !f.decoderResult.nextConvCompleted && !f.decoderResult.nextConvHeld);
   if(fault>=4 && fault!=7)assert(f.transport->NextConvStarts()==120);
   if(fault==4 || fault==5 || fault==8)assert(f.decoderResult.nextConvCompleted==119 && !f.decoderResult.nextConvHeld);
   if(fault==7)assert(f.decoderResult.phase==33 && !f.decoderResult.worker && f.decoderResult.nextConvInstalled==120 && f.decoderResult.nextConvArmed==120);
   f.UnknownNative();unsigned host=f.io.touches,device=f.transport->Touches();
   assert(!RunQ38NextConv(*f.c,forged) && !RunQ38NextProjections(*f.c,forged) && !RunQ38NextRms(*f.c,forged)
    && !RunQ38ResidentFfn(*f.c,forged) && !RunQ38ResidentResidualRms(*f.c,forged) && !RunQ38ResidentGdnOutput(*f.c,forged)
    && !CloseQ38ResidentDecoderModel(*f.c,forged) && !EndFactory(*f.c) && f.io.touches==host && f.transport->Touches()==device);
   printf("PASS SAME ACTUAL native60 fault%u real code/table/source/summary/terminal-PC/restore/deadline/retained-tail endpoint;ALL owners+snapshots+hosts retained/ZERO following IO-reset-restore-retry-END\n",fault);
   _exit(0);
  }int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));
 }
 bool ranNextConv=RunQ38NextConv(*f.c,f.decoderResult);
 printf("NATIVE60_RUN ran%u phase%u worker%u word%u installed%u armed%u released%u completed%u held%u published%u unsafe%u\n",unsigned(ranNextConv),f.decoderResult.phase,f.decoderResult.worker,f.decoderResult.word,
  f.decoderResult.nextConvInstalled,f.decoderResult.nextConvArmed,f.decoderResult.nextConvReleased,f.decoderResult.nextConvCompleted,f.decoderResult.nextConvHeld,f.decoderResult.nextConvPublished,f.decoderResult.unsafe);
 assert(ranNextConv && m->decoder.NextConvCompleted() && !nextProjection->Current() && !m->decoder.NextProjectionInput()
  && f.decoderResult.nextConvInstalled==120 && f.decoderResult.nextConvArmed==120 && f.decoderResult.nextConvReleased==120
  && f.decoderResult.nextConvCompleted==120 && f.decoderResult.nextConvHeld==120 && f.decoderResult.nextConvPublished==1);
 const auto* nextConvolution=m->decoder.NextConvolutionInput();assert(nextConvolution && nextConvolution->Current()
  && nextConvolution->MatchesCohort(*cohort) && !nextConvolution->MatchesCohort(*foreignCohort));
 f.transport->VerifyNextConv();assert(source->NextConvReads()==640 && source->CheckNextConv(beforeNextConv,f.io.wholeWorkspace));
 for(unsigned i=0;i<beforeNextConv.size();i++)if(!(i>=TT_Q38_CONV_OUTPUT/4 && i<(TT_Q38_CONV_OUTPUT+40960)/4)
  && !(i>=TT_Q38_CONV_NEXT/4 && i<(TT_Q38_CONV_NEXT+122880)/4))assert(f.io.wholeWorkspace[i]==beforeNextConv[i]);
 for(auto word:f.io.historyWords)assert(!word);
 n=f.io.touches;d=f.transport->Touches();assert(!RunQ38NextConv(*f.c,forged) && !RunQ38NextProjections(*f.c,forged) && f.io.touches==n && f.transport->Touches()==d);
 for(unsigned fault=0;fault<3;fault++){pid_t child=fork();assert(child>=0);if(!child){
   if(!fault)m->graph.Fault();if(fault==1)m->graph.Recurrent(m->identity)->Fault();if(fault==2)cohort->worker[119].image.running=true;
   assert(!nextConvolution->Current() && !m->decoder.NextConvolutionInput() && f.io.touches==n && f.transport->Touches()==d);_exit(0);
  }int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));}
 puts("PASS SAME ACTUAL native60 claims genuine native59 input BEFORE IO/ORIGINAL layer1 F32conv640tiles/120workers640jobs/ALL10240convolution values independent original-weight bound+30720NEXTexact/ALL120receiptsBEFOREholds/private nextconv+expiry/history+unrelatedGDDR unchanged;NO host products/current-history-flip/Work/publicselector/CARDspeed claim");
#endif
#ifdef PIONEER_Q38_NEXT_GDN_NATIVE
 static_assert(!std::is_default_constructible<PioneerQ38NextGdnProducer>::value && !std::is_copy_constructible<PioneerQ38NextGdnProducer>::value
  && !std::is_move_constructible<PioneerQ38NextGdnProducer>::value,"only SAME actual decoder publishes next-layer GDN");
 n=f.io.touches;d=f.transport->Touches();currentTeam=8;
 assert(!RunQ38NextGdn(*f.c,forged) && nextConvolution->Current() && f.io.touches==n && f.transport->Touches()==d);currentTeam=7;
 forged.nextGdnPublished=1;assert(!RunQ38NextGdn(*f.c,forged) && nextConvolution->Current() && !m->decoder.NextAttentionInput()
  && f.io.touches==n && f.transport->Touches()==d);forged.nextGdnPublished=0;
 for(unsigned i=0;i<m->plan.Count();i++){const auto* p=m->plan.Part(i);if(p->kind!=m->plan.WeightKind || p->layer!=1 || p->role<7 || p->role>9)continue;
  unsigned at=p->bank*786432+p->offset/4096;
  for(unsigned value:{0U,2U}){f.learned[at]=value;
   assert(!RunQ38NextGdn(*f.c,forged) && nextConvolution->Current() && !m->decoder.NextAttentionInput() && !m->Unsafe()
    && f.io.touches==n && f.transport->Touches()==d);f.learned[at]=1;
  }
 }
 const auto beforeNextGdn=f.io.wholeWorkspace,beforeNextGdnState=f.io.recurrentWords;
 if(nextGdnFaults)for(unsigned fault=1;fault<=13;fault++){
  pid_t child=fork();assert(child>=0);if(!child){
   // Fork SAME genuinely completed native60 stack;NO fake state/producer/ready flags.
   f.transport->NextGdnFault(fault);bool ran=RunQ38NextGdn(*f.c,f.decoderResult);
   bool failedClose=fault==6 && ran?!CloseQ38ResidentDecoderModel(*f.c,f.decoderResult):false;
   printf("NATIVE61_FAULT_OBS kind%u ran%u injected%u phase%u worker%u word%u installed%u armed%u released%u completed%u held%u published%u actualStarts%u learnedReads%llu\n",
    fault,unsigned(ran),unsigned(f.transport->NextGdnFaultInjected()),f.decoderResult.phase,f.decoderResult.worker,f.decoderResult.word,
    f.decoderResult.nextGdnInstalled,f.decoderResult.nextGdnArmed,f.decoderResult.nextGdnReleased,f.decoderResult.nextGdnCompleted,
    f.decoderResult.nextGdnHeld,f.decoderResult.nextGdnPublished,f.transport->NextGdnStarts(),(unsigned long long)source->NextGdnReads());
   if(fault==6)assert(ran && failedClose && f.decoderResult.nextGdnPublished==1);
   else assert(!ran && !f.decoderResult.nextGdnPublished);
   assert(f.transport->NextGdnFaultInjected() && !nextConvolution->Current() && !m->decoder.NextConvolutionInput()
    && !m->decoder.NextAttentionInput() && !m->decoder.NextGdnCompleted());
   if(fault<=2)assert(!f.transport->NextGdnStarts() && f.io.wholeWorkspace==beforeNextGdn && f.io.recurrentWords==beforeNextGdnState);
   if(fault==3 || fault==7)assert(f.transport->NextGdnStarts()==1 && !source->NextGdnReads()
    && !f.decoderResult.nextGdnReleased && !f.decoderResult.nextGdnCompleted && !f.decoderResult.nextGdnHeld);
   if(fault>=4 && fault!=7)assert(f.transport->NextGdnStarts()==96);
   if(fault==4 || fault==5 || fault==8 || fault==9 || fault==10 || fault==11 || fault==12){
    // Cooperative streams complete out of order:worker96 need NOT be
    // the last accepted receipt. Still NO full completion/hold/publication.
    assert(f.decoderResult.phase==39 && f.decoderResult.worker==96 && f.decoderResult.nextGdnCompleted<96 && !f.decoderResult.nextGdnHeld);
   }
   if(fault==7)assert(f.decoderResult.phase==38 && f.decoderResult.worker==1 && f.decoderResult.nextGdnInstalled==96 && f.decoderResult.nextGdnArmed==96);
   if(fault==13)assert(f.decoderResult.phase==40 && f.decoderResult.worker==96 && f.decoderResult.nextGdnCompleted==96 && f.decoderResult.nextGdnHeld==95);
   f.UnknownNative();unsigned host=f.io.touches,device=f.transport->Touches();
   assert(!RunQ38NextGdn(*f.c,forged) && !RunQ38NextConv(*f.c,forged) && !RunQ38NextProjections(*f.c,forged)
    && !CloseQ38ResidentDecoderModel(*f.c,forged) && !EndFactory(*f.c) && f.io.touches==host && f.transport->Touches()==device);
   printf("PASS SAME ACTUAL native61 fault%u real code/table/source/summary/terminal-PC/restore/deadline/retained-tail/peer-ready/clock/late-status/rollback/hold endpoint;ALL owners+snapshots+hosts retained/ZERO following IO-reset-restore-retry-END\n",fault);
   _exit(0);
  }int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));
 }
 bool ranNextGdn=RunQ38NextGdn(*f.c,f.decoderResult);
 printf("NATIVE61_RUN ran%u phase%u worker%u word%u installed%u armed%u released%u completed%u held%u published%u unsafe%u\n",unsigned(ranNextGdn),f.decoderResult.phase,f.decoderResult.worker,f.decoderResult.word,
  f.decoderResult.nextGdnInstalled,f.decoderResult.nextGdnArmed,f.decoderResult.nextGdnReleased,f.decoderResult.nextGdnCompleted,f.decoderResult.nextGdnHeld,f.decoderResult.nextGdnPublished,f.decoderResult.unsafe);
 assert(ranNextGdn && m->decoder.NextGdnCompleted() && !nextConvolution->Current() && !m->decoder.NextConvolutionInput()
  && f.decoderResult.nextGdnInstalled==96 && f.decoderResult.nextGdnArmed==96 && f.decoderResult.nextGdnReleased==96
  && f.decoderResult.nextGdnCompleted==96 && f.decoderResult.nextGdnHeld==96 && f.decoderResult.nextGdnPublished==1
  && m->decoder.GdnLayerCompleted() && m->decoder.GdnGatedCompleted()); // first-layer proof stays HISTORICAL,not retagged
 const auto* nextAttention=m->decoder.NextAttentionInput();assert(nextAttention && nextAttention->Current()
  && nextAttention->MatchesCohort(*cohort) && !nextAttention->MatchesCohort(*foreignCohort));
 f.transport->VerifyNextGdn();assert(source->NextGdnReads()==288 && source->GdnInputReads()==192 && source->GdnNormReads()==96);
 for(unsigned half=0;half<96;half++){
  unsigned w=half+1;assert(f.io.SelectRuntimeCore(w));uint32_t prepared[322],next[8192],raw[64],gated[64],remote[8192];
  for(unsigned i=0;i<322;i++)assert(f.io.ProjectionGraphMemoryReadChecked(w,0x8e000+i*4,prepared[i]));
  for(unsigned i=0;i<8192;i++)assert(f.io.ProjectionGraphMemoryReadChecked(w,0xa0000+i*4,next[i]));
  for(unsigned i=0;i<64;i++)assert(f.io.ProjectionGraphMemoryReadChecked(w,0x8e800+i*4,raw[i])
   && f.io.ProjectionGraphMemoryReadChecked(w,0x8ea00+i*4,gated[i]) && gated[i]==f.io.wholeWorkspace[TT_Q38_GDN_GATED_OUTPUT/4+half*64+i]);
  assert(source->CheckNextGdn(half,beforeNextGdn,prepared,next,raw,gated));
  PioneerQ38GdnBankExtent span;auto* g=m->graph.Recurrent(m->identity);
  assert(g && g->InitializeHalf(m->identity,1,1,half,span) && f.io.ReadGdnState(span.bank,span.address,remote,8192)
   && !memcmp(remote,next,sizeof(next)) && g->InitializeHalf(m->identity,1,0,half,span) && f.io.ReadGdnState(span.bank,span.address,remote,8192));
  for(auto value:remote)assert(!value);
 }
 for(unsigned region=0;region<672;region++){
  const auto& r=m->plan.RecurrentRegions()[region];if(r.layer==1 && r.slot==1)continue;
  unsigned first=f.io.recurrentFirst[region]*1024,words=f.io.recurrentExtents[region].bytes/4;
  assert(!memcmp(f.io.recurrentWords.data()+first,beforeNextGdnState.data()+first,words*4));
 }
 for(unsigned i=0;i<beforeNextGdn.size();i++)if(i<TT_Q38_GDN_GATED_OUTPUT/4 || i>=(TT_Q38_GDN_GATED_OUTPUT+6144*4)/4)assert(f.io.wholeWorkspace[i]==beforeNextGdn[i]);
 for(auto word:f.io.historyWords)assert(!word);
 n=f.io.touches;d=f.transport->Touches();assert(!RunQ38NextGdn(*f.c,forged) && !RunQ38NextConv(*f.c,forged) && f.io.touches==n && f.transport->Touches()==d);
 for(unsigned fault=0;fault<3;fault++){pid_t child=fork();assert(child>=0);if(!child){
   if(!fault)m->graph.Fault();if(fault==1)m->graph.Recurrent(m->identity)->Fault();if(fault==2)cohort->worker[119].image.running=true;
   assert(!nextAttention->Current() && !m->decoder.NextAttentionInput() && f.io.touches==n && f.transport->Touches()==d);_exit(0);
  }int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));}
 puts("PASS SAME ACTUAL native61 claims genuine native60 BEFORE IO/ORIGINAL layer1 A+dt+norm288reads/96real QGT1 streams/all829632prepared+state+raw+gated values independent libm/96self+peer+clockreceipts BEFORE ALL120holds/private layer1 NEXT;CURRENT+all unrelated states/GDDR/history unchanged;NO CPU products/current-tokenflip/Work/public selector/CARDspeed claim");
#endif
#ifdef PIONEER_Q38_NEXT_GDN_OUTPUT_NATIVE
 static_assert(!std::is_default_constructible<PioneerQ38NextGdnOutputProducer>::value && !std::is_copy_constructible<PioneerQ38NextGdnOutputProducer>::value
  && !std::is_move_constructible<PioneerQ38NextGdnOutputProducer>::value,"only SAME actual decoder publishes next output");
 n=f.io.touches;d=f.transport->Touches();currentTeam=8;
 assert(!RunQ38NextGdnOutput(*f.c,forged) && nextAttention->Current() && f.io.touches==n && f.transport->Touches()==d);currentTeam=7;
 forged.nextOutputPublished=1;assert(!RunQ38NextGdnOutput(*f.c,forged) && nextAttention->Current() && !m->decoder.NextPostAttentionInput()
  && f.io.touches==n && f.transport->Touches()==d);forged.nextOutputPublished=0;
 for(unsigned stripe=0;stripe<7;stripe++){
  const auto* p=m->plan.ProjectionStripe(1,13,stripe);unsigned first=p->bank*786432+p->offset/4096;
  for(unsigned at:{first,first+p->allocated/4096-1})for(unsigned value:{0U,2U}){f.learned[at]=value;
   assert(!RunQ38NextGdnOutput(*f.c,forged) && nextAttention->Current() && !m->decoder.NextPostAttentionInput() && !m->Unsafe()
    && f.io.touches==n && f.transport->Touches()==d);f.learned[at]=1;
  }
 }
 const auto beforeNextOutput=f.io.wholeWorkspace,beforeNextOutputState=f.io.recurrentWords;
 auto nextOutputPlan=std::make_unique<PioneerQ38NextGdnOutputProjectionPlan>();
 auto nextOutputBootstrap=std::make_unique<PioneerQ38NextGdnOutputBootstrapPlan>();
 assert(nextOutputPlan->Build(*nextAttention) && nextOutputPlan->Current() && nextOutputPlan->Producer()==nextAttention
  && nextOutputPlan->Count()==119 && nextOutputBootstrap->Build(*nextOutputPlan,*cohort)
  && nextOutputBootstrap->Matches(*nextOutputPlan,*cohort) && !nextOutputBootstrap->Matches(*nextOutputPlan,*foreignCohort));
 uint32_t nextDescriptor[64],nextGather[64],nextSources[24][16];
 for(unsigned chunk=0;chunk<24;chunk++){
  assert(nextOutputPlan->InputDescriptor(chunk,*cohort,nextDescriptor) && nextDescriptor[42]==1 && nextDescriptor[43]==13
   && nextDescriptor[0]==TT_Q38_NEXT_GDN_OUTPUT_INPUT_MARKER && PioneerQ38NextGdnOutputInputShape(nextDescriptor)
   && !PioneerQ38GdnOutputInputShape(nextDescriptor));
 }
 assert(nextOutputPlan->GatherDescriptor(*cohort,nextGather) && nextGather[42]==1 && nextGather[43]==13
  && nextOutputPlan->GatherSources(*cohort,nextSources));
 for(unsigned w=0;w<24;w++)for(unsigned i=0;i<16;i++)assert(nextSources[w][i]==cohort->authority.source[w+1][i]);
 assert(!nextOutputPlan->InputDescriptor(24,*cohort,nextDescriptor) && !nextOutputPlan->InputDescriptor(0,*foreignCohort,nextDescriptor)
  && !nextOutputPlan->GatherDescriptor(*foreignCohort,nextGather) && !nextOutputPlan->GatherSources(*foreignCohort,nextSources)
  && f.io.touches==n && f.transport->Touches()==d);
 puts("PASS native62 CPU DATA built ONLY from genuine native61/119jobs/24QOI2+QOG2/actual peer sources/foreign refusals/oldQOI1 refused;NO IO or caller-ready grant");
 if(nextOutputFaults)for(unsigned fault=1;fault<=16;fault++){
  pid_t child=fork();assert(child>=0);if(!child){
   // SAME genuine native61 stack,not a fabricated completion/producer or shortened prefix.
   f.transport->NextOutputFault(fault);bool ran=RunQ38NextGdnOutput(*f.c,f.decoderResult);
   bool failedClose=fault==6 && ran?!CloseQ38ResidentDecoderModel(*f.c,f.decoderResult):false;
   printf("NATIVE62_FAULT_OBS kind%u ran%u injected%u phase%u worker%u word%u installed%u booted%u runs%u stopped%u held%u published%u actualStarts%u learnedReads%llu\n",
    fault,unsigned(ran),unsigned(f.transport->NextOutputFaultInjected()),f.decoderResult.phase,f.decoderResult.worker,f.decoderResult.word,
    f.decoderResult.nextOutputInstalled,f.decoderResult.nextOutputBooted,f.decoderResult.nextOutputRootRuns,f.decoderResult.nextOutputStopped,
    f.decoderResult.nextOutputHeld,f.decoderResult.nextOutputPublished,f.transport->NextOutputStarts(),(unsigned long long)source->NextGdnOutputReads());
   assert(f.transport->NextOutputFaultInjected());
   if(fault==6)assert(ran && failedClose && f.decoderResult.nextOutputPublished==1);
   else assert(!ran && !f.decoderResult.nextOutputPublished);
   assert(!nextAttention->Current() && !m->decoder.NextAttentionInput() && !m->decoder.NextPostAttentionInput()
    && !m->decoder.NextGdnOutputCompleted() && !nextOutputPlan->Current() && !nextOutputBootstrap->Current());
   if(fault<=2)assert(!f.transport->NextOutputStarts() && f.io.wholeWorkspace==beforeNextOutput && !source->NextGdnOutputReads());
   if(fault==3 || fault==7)assert(f.transport->NextOutputStarts()==1 && !source->NextGdnOutputReads() && !f.decoderResult.nextOutputBooted);
   if(fault==9)assert(f.transport->NextOutputStarts()==120 && !source->NextGdnOutputReads() && f.decoderResult.nextOutputBooted==119);
   if(fault==14 || fault==15)assert(f.transport->NextOutputStarts()==119 && !source->NextGdnOutputReads()
    && f.decoderResult.nextOutputBooted==118 && !f.decoderResult.nextOutputRootRuns);
   if(fault==4 || fault==8 || fault==10 || fault==11)assert(f.transport->NextOutputStarts()==120 && source->NextGdnOutputReads()==61440
    && !f.decoderResult.nextOutputStopped && !f.decoderResult.nextOutputHeld);
   if(fault==5)assert(f.transport->NextOutputStarts()==121 && f.decoderResult.nextOutputStopped==118 && !f.decoderResult.nextOutputHeld);
   if(fault==12)assert(f.transport->NextOutputStarts()==121 && f.decoderResult.nextOutputStopped==119 && f.decoderResult.nextOutputHeld==118);
   if(fault==13 || fault==16)assert(f.transport->NextOutputStarts()==121 && !f.decoderResult.nextOutputStopped && !f.decoderResult.nextOutputHeld);
   assert(f.io.recurrentWords==beforeNextOutputState);for(auto word:f.io.historyWords)assert(!word);
   f.UnknownNative();unsigned host=f.io.touches,device=f.transport->Touches();
   assert(!RunQ38NextGdnOutput(*f.c,forged) && !RunQ38NextGdn(*f.c,forged) && !RunQ38NextConv(*f.c,forged)
    && !RunQ38NextProjections(*f.c,forged) && !RunQ38ResidentFfn(*f.c,forged) && !RunQ38ResidentGdnOutput(*f.c,forged)
    && !CloseQ38ResidentDecoderModel(*f.c,forged) && !EndFactory(*f.c) && f.io.touches==host && f.transport->Touches()==device);
   printf("PASS SAME ACTUAL native62 fault%u real code/frame/source/stats/PC/restore/boot-RUN-STOP clocks/hold endpoints;ALL owners+snapshots+hosts retained/ZERO following IO-reset-restore-retry-END\n",fault);
   _exit(0);
  }int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));
 }
 bool ranNextOutput=RunQ38NextGdnOutput(*f.c,f.decoderResult);
 printf("NATIVE62_RUN ran%u phase%u worker%u word%u installed%u booted%u runs%u stopped%u held%u published%u unsafe%u starts%u\n",unsigned(ranNextOutput),f.decoderResult.phase,f.decoderResult.worker,f.decoderResult.word,
  f.decoderResult.nextOutputInstalled,f.decoderResult.nextOutputBooted,f.decoderResult.nextOutputRootRuns,f.decoderResult.nextOutputStopped,
  f.decoderResult.nextOutputHeld,f.decoderResult.nextOutputPublished,f.decoderResult.unsafe,f.transport->NextOutputStarts());
 assert(ranNextOutput && m->decoder.NextGdnOutputCompleted() && !nextAttention->Current() && !m->decoder.NextAttentionInput()
  && f.decoderResult.nextOutputInstalled==120 && f.decoderResult.nextOutputBooted==119 && f.decoderResult.nextOutputRootRuns==2
  && f.decoderResult.nextOutputStopped==119 && f.decoderResult.nextOutputHeld==119 && f.decoderResult.nextOutputPublished==1);
 assert(!nextOutputPlan->Current() && !nextOutputPlan->Producer() && !nextOutputPlan->Count() && !nextOutputPlan->Part(0)
  && !nextOutputBootstrap->Current() && !nextOutputBootstrap->Manifest() && !nextOutputBootstrap->Sources());
 const auto* nextPost=m->decoder.NextPostAttentionInput();assert(nextPost && nextPost->Current()
  && nextPost->MatchesCohort(*cohort) && !nextPost->MatchesCohort(*foreignCohort));
 f.transport->VerifyNextOutput();assert(source->NextGdnOutputReads()==61440 && source->CheckNextGdnOutput(beforeNextOutput,f.io.wholeWorkspace));
 assert(f.io.recurrentWords==beforeNextOutputState);for(auto word:f.io.historyWords)assert(!word);
 for(unsigned i=0;i<beforeNextOutput.size();i++){
  unsigned a=i*4;bool writable=(a>=TT_Q38_PROJECTION_NATIVE_OFFSET && a<TT_Q38_PROJECTION_NATIVE_OFFSET+12288)
   || (a>=TT_Q38_PROJECTION_LINES_OFFSET && a<TT_Q38_PROJECTION_LINES_OFFSET+3072)
   || (a>=TT_Q38_PROJECTION_COMPACT_OFFSET && a<TT_Q38_PROJECTION_COMPACT_OFFSET+768)
   || (a>=TT_Q38_NEXT_GDN_OUTPUT_RESULT && a<TT_Q38_NEXT_GDN_OUTPUT_RESULT+20480);
  if(!writable)assert(f.io.wholeWorkspace[i]==beforeNextOutput[i]);
 }
 n=f.io.touches;d=f.transport->Touches();assert(!RunQ38NextGdnOutput(*f.c,forged) && !RunQ38NextGdn(*f.c,forged) && f.io.touches==n && f.transport->Touches()==d);
 for(unsigned fault=0;fault<3;fault++){pid_t child=fork();assert(child>=0);if(!child){
   if(!fault)m->graph.Fault();if(fault==1)m->graph.Recurrent(m->identity)->Fault();if(fault==2)cohort->worker[119].image.running=true;
   assert(!nextPost->Current() && !m->decoder.NextPostAttentionInput() && f.io.touches==n && f.transport->Touches()==d);_exit(0);
  }int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));}
 puts("PASS SAME ACTUAL native62 claims genuine native61/ALL7 original layer1 output stripes/24real QOI2+QOG2/119QOP2 matrices/61440original tiles/ALL5120 independent original packed-row references bitexact/119STOP+120holds/private result+raw residual/unchanged state+history;NO CPU products/Work/public selector/CARDspeed claim");
#endif
#ifdef PIONEER_Q38_NEXT_RESIDUAL_RMS_NATIVE
 static_assert(!std::is_default_constructible<PioneerQ38NextFfnProducer>::value && !std::is_copy_constructible<PioneerQ38NextFfnProducer>::value
  && !std::is_move_constructible<PioneerQ38NextFfnProducer>::value,"only SAME actual decoder publishes next FFN input");
 n=f.io.touches;d=f.transport->Touches();currentTeam=8;
 assert(!RunQ38NextResidualRms(*f.c,forged) && nextPost->Current() && f.io.touches==n && f.transport->Touches()==d);currentTeam=7;
 for(unsigned field=0;field<4;field++){
  auto* value=field==0?&forged.nextResidualInstalled:field==1?&forged.nextResidualRuns:field==2?&forged.nextResidualHeld:&forged.nextResidualPublished;
  *value=1;assert(!RunQ38NextResidualRms(*f.c,forged) && nextPost->Current() && !m->decoder.NextFfnInput()
   && f.io.touches==n && f.transport->Touches()==d);*value=0;
 }
 for(unsigned i=0;i<m->plan.Count();i++){const auto* p=m->plan.Part(i);if(p->kind!=m->plan.WeightKind || p->layer!=1 || p->role!=1)continue;
  unsigned first=p->bank*786432+p->offset/4096;
  for(unsigned page=0;page<5;page++)for(unsigned value:{0U,2U}){
   f.learned[first+page]=value;assert(!RunQ38NextResidualRms(*f.c,forged) && nextPost->Current() && !m->decoder.NextFfnInput()
    && !m->Unsafe() && f.io.touches==n && f.transport->Touches()==d);f.learned[first+page]=1;
  }
 }
 const auto beforeNextResidual=f.io.wholeWorkspace,beforeNextResidualState=f.io.recurrentWords;
 if(nextResidualFaults)for(unsigned fault=1;fault<=16;fault++){
  pid_t child=fork();assert(child>=0);if(!child){
   // SAME genuine native62 stack/121actual starts+119STOP+120holds. No
   // synthesized initialization,metadata-only readiness or CPU products.
   f.transport->NextResidualFault(fault);bool ran=RunQ38NextResidualRms(*f.c,f.decoderResult);
   bool failedClose=fault==6 && ran?!CloseQ38ResidentDecoderModel(*f.c,f.decoderResult):false;
   printf("NATIVE63_FAULT_OBS kind%u ran%u injected%u phase%u worker%u word%u installed%u runs%u held%u published%u actualStarts%u learnedReads%llu\n",
    fault,unsigned(ran),unsigned(f.transport->NextResidualFaultInjected()),f.decoderResult.phase,f.decoderResult.worker,f.decoderResult.word,
    f.decoderResult.nextResidualInstalled,f.decoderResult.nextResidualRuns,f.decoderResult.nextResidualHeld,f.decoderResult.nextResidualPublished,
    f.transport->NextResidualStarts(),(unsigned long long)source->NextResidualRmsReads());
   assert(f.transport->NextResidualFaultInjected());
   if(fault==6)assert(ran && failedClose && f.decoderResult.nextResidualPublished==1);
   else assert(!ran && !f.decoderResult.nextResidualPublished);
   assert(!nextPost->Current() && !m->decoder.NextPostAttentionInput() && !m->decoder.NextFfnInput() && !m->decoder.NextResidualRmsCompleted());
   if(fault<=2 || fault==15)assert(!f.transport->NextResidualStarts() && f.io.wholeWorkspace==beforeNextResidual && !source->NextResidualRmsReads());
   else assert(f.transport->NextResidualStarts()==1 && source->NextResidualRmsReads()==2);
   if(fault==9)assert(f.io.wholeWorkspace==beforeNextResidual); // actual NaN fault BEFORE first GDDR sink write
   if(fault==13 || fault==14 || fault==16)assert(f.decoderResult.nextResidualHeld==1);
   if(fault==7 || (fault>=10 && fault<=16))assert(f.transport->Touches()==f.transport->NextResidualFaultTouch());
   assert(f.io.recurrentWords==beforeNextResidualState);for(auto word:f.io.historyWords)assert(!word);
   f.UnknownNative();unsigned host=f.io.touches,device=f.transport->Touches();
   assert(!RunQ38NextResidualRms(*f.c,forged) && !RunQ38NextGdnOutput(*f.c,forged) && !RunQ38NextGdn(*f.c,forged)
    && !RunQ38NextConv(*f.c,forged) && !RunQ38NextProjections(*f.c,forged) && !RunQ38ResidentFfn(*f.c,forged)
    && !CloseQ38ResidentDecoderModel(*f.c,forged) && !EndFactory(*f.c) && f.io.touches==host && f.transport->Touches()==device);
   printf("PASS SAME ACTUAL native63 fault%u real code/frame/source/summary/PC/restore/one-shot clock/hold endpoints;ALL retained/ZERO following IO-reset-restore-retry-END\n",fault);
   _exit(0);
  }int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));
 }
 bool ranNextResidual=RunQ38NextResidualRms(*f.c,f.decoderResult);
 printf("NATIVE63_RUN ran%u phase%u worker%u word%u installed%u runs%u held%u published%u unsafe%u starts%u\n",unsigned(ranNextResidual),
  f.decoderResult.phase,f.decoderResult.worker,f.decoderResult.word,f.decoderResult.nextResidualInstalled,f.decoderResult.nextResidualRuns,
  f.decoderResult.nextResidualHeld,f.decoderResult.nextResidualPublished,f.decoderResult.unsafe,f.transport->NextResidualStarts());
 assert(ranNextResidual && m->decoder.NextResidualRmsCompleted() && !nextPost->Current() && !m->decoder.NextPostAttentionInput()
  && !m->decoder.NextGdnOutputCompleted() && f.decoderResult.nextResidualInstalled==10241 && f.decoderResult.nextResidualRuns==1
  && f.decoderResult.nextResidualHeld==1 && f.decoderResult.nextResidualPublished==1 && f.transport->NextResidualStarts()==1);
 const auto* nextFfn=m->decoder.NextFfnInput();assert(nextFfn && nextFfn->Current() && nextFfn->MatchesCohort(*cohort) && !nextFfn->MatchesCohort(*foreignCohort));
 assert(source->NextResidualRmsReads()==2 && source->CheckNextResidualRms(beforeNextResidual,f.io.wholeWorkspace));
 assert(f.io.recurrentWords==beforeNextResidualState);for(auto word:f.io.historyWords)assert(!word);
 for(unsigned i=0;i<beforeNextResidual.size();i++)if(i>=5120 && (i<TT_Q38_RMS_BACKUP/4 || i>=TT_Q38_RMS_BACKUP/4+5120))assert(f.io.wholeWorkspace[i]==beforeNextResidual[i]);
 n=f.io.touches;d=f.transport->Touches();assert(!RunQ38NextResidualRms(*f.c,forged) && !RunQ38NextGdnOutput(*f.c,forged)
  && f.io.touches==n && f.transport->Touches()==d);
 for(unsigned fault=0;fault<3;fault++){pid_t child=fork();assert(child>=0);if(!child){
   if(!fault)m->graph.Fault();if(fault==1)m->graph.Recurrent(m->identity)->Fault();if(fault==2)cohort->worker[119].image.running=true;
   assert(!nextFfn->Current() && !m->decoder.NextFfnInput() && f.io.touches==n && f.transport->Touches()==d);_exit(0);
  }int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));}
 puts("PASS SAME ACTUAL native63 claims genuine native62 output+native58 raw BEFORE IO/ORIGINAL layer1 norm5120/10actual QRR2 packets/ALL5120sum+norm BIT-EXACT/ROOT hold+ALL120held/private FFN input+old producer expiry/unrelatedstate+GDDR retained;NO CPU products/Work/public selector/card speed claim");
#endif
#ifdef PIONEER_Q38_NEXT_FFN_RESIDUAL_NATIVE
 static_assert(!std::is_default_constructible<PioneerQ38NextFfnHiddenProducer>::value
  && !std::is_copy_constructible<PioneerQ38NextFfnHiddenProducer>::value
  && !std::is_move_constructible<PioneerQ38NextFfnHiddenProducer>::value,"only SAME actual owner publishes layer2 hidden");
 n=f.io.touches;d=f.transport->Touches();currentTeam=8;
 assert(!RunQ38NextFfnResidual(*f.c,forged) && nextFfn->Current() && f.io.touches==n && f.transport->Touches()==d);currentTeam=7;
 for(unsigned field=0;field<5;field++){
  auto* value=field==0?&forged.nextFfnInstalled:field==1?&forged.nextFfnBooted:field==2?&forged.nextFfnRootRuns:
   field==3?&forged.nextFfnStopped:&forged.nextFfnPublished;
  *value=1;assert(!RunQ38NextFfnResidual(*f.c,forged) && nextFfn->Current() && !m->decoder.NextFfnHiddenInput()
   && f.io.touches==n && f.transport->Touches()==d);*value=0;
 }
 for(unsigned role=0;role<3;role++)for(unsigned stripe=0;stripe<7;stripe++){
  const auto* p=m->plan.FfnStripe(1,role,stripe);unsigned first=p->bank*786432+p->offset/4096;
  for(unsigned at:{first,first+p->allocated/4096-1})for(unsigned bad:{0U,2U}){
   f.learned[at]=bad;assert(!RunQ38NextFfnResidual(*f.c,forged) && nextFfn->Current() && !m->decoder.NextFfnHiddenInput()
    && !m->Unsafe() && f.io.touches==n && f.transport->Touches()==d);f.learned[at]=1;
  }
 }
 const auto beforeNextFfn=f.io.wholeWorkspace,beforeNextFfnState=f.io.recurrentWords;
#include "pioneer_q38_next_ffn_native_faults.inc"
 bool ranNextFfn=RunQ38NextFfnResidual(*f.c,f.decoderResult);
 if(lateNextFfnFault){checkNextFfnFault(lateNextFfnFault,ranNextFfn);_exit(0);}
 printf("NATIVE65_RUN ran%u phase%u worker%u word%u installed%u booted%u runs%u stopped%u published%u unsafe%u actualStarts%u learnedReads%llu\n",
  unsigned(ranNextFfn),f.decoderResult.phase,f.decoderResult.worker,f.decoderResult.word,f.decoderResult.nextFfnInstalled,
  f.decoderResult.nextFfnBooted,f.decoderResult.nextFfnRootRuns,f.decoderResult.nextFfnStopped,f.decoderResult.nextFfnPublished,
  f.decoderResult.unsafe,f.transport->NextFfnStarts(),(unsigned long long)source->NextFfnReads());
 assert(ranNextFfn && m->decoder.NextFfnCompleted() && !nextFfn->Current() && !m->decoder.NextFfnInput()
  && f.decoderResult.nextFfnInstalled==120 && f.decoderResult.nextFfnBooted==119 && f.decoderResult.nextFfnRootRuns==2
  && f.decoderResult.nextFfnStopped==119 && f.decoderResult.nextFfnPublished==1 && f.transport->NextFfnStarts()==121);
 const auto* nextHidden=m->decoder.NextFfnHiddenInput();assert(nextHidden && nextHidden->Current()
  && nextHidden->MatchesCohort(*cohort) && !nextHidden->MatchesCohort(*foreignCohort));
 f.transport->VerifyNextFfn();assert(source->NextFfnReads()==65280 && source->CheckNextFfnResidual(beforeNextFfn,f.io.wholeWorkspace));
 assert(f.io.recurrentWords==beforeNextFfnState);for(auto word:f.io.historyWords)assert(!word);
 n=f.io.touches;d=f.transport->Touches();assert(!RunQ38NextFfnResidual(*f.c,forged) && !RunQ38NextResidualRms(*f.c,forged)
  && f.io.touches==n && f.transport->Touches()==d);
 puts("PASS SAME ACTUAL native65 claims genuine native63/ALL21 original layer1 FFN stripes/actual QFG3+119QFP2/357matrices+88producers/65280original tiles/ALL5120 output rows checked/private layer2 hidden+old input expiry/state retained;NO CPU products/Work/public selector/card speed claim");
#endif
 bool closed=CloseQ38ResidentDecoderModel(*f.c,f.decoderResult);
 printf("NATIVE_QGH_CLOSE closed%u phase%u worker%u word%u restored%u verified%u unsafe%u\n",unsigned(closed),f.decoderResult.phase,f.decoderResult.worker,f.decoderResult.word,
  f.decoderResult.memory.restoredWords,f.decoderResult.memory.verifiedOriginalWords,f.decoderResult.unsafe);
 assert(closed && !m->decoder.GdnGatedCompleted() && m->Closed()
  && f.decoderResult.memory.restoredWords==8330680 && f.decoderResult.memory.verifiedOriginalWords==8330680);
#ifdef PIONEER_Q38_RESIDUAL_RMS_NATIVE
 assert(!post->Current() && !m->decoder.PostAttentionInput() && !m->decoder.PostAttentionCompleted());
#endif
#ifdef PIONEER_Q38_FFN_RESIDUAL_NATIVE
 assert(!hidden->Current() && !m->decoder.FfnHiddenInput() && !m->decoder.FfnCompleted());
#endif
#ifdef PIONEER_Q38_NEXT_RMS_NATIVE
 assert(!nextInput->Current() && !m->decoder.NextLayerInput() && !m->decoder.NextRmsCompleted());
#endif
#ifdef PIONEER_Q38_NEXT_PROJECTION_NATIVE
 assert(!nextProjection->Current() && !m->decoder.NextProjectionInput() && !m->decoder.NextProjectionsCompleted());
#endif
#ifdef PIONEER_Q38_NEXT_CONV_NATIVE
 assert(!nextConvolution->Current() && !m->decoder.NextConvolutionInput() && !m->decoder.NextConvCompleted());
#endif
#ifdef PIONEER_Q38_NEXT_GDN_NATIVE
 assert(!nextAttention->Current() && !m->decoder.NextAttentionInput() && !m->decoder.NextGdnCompleted());
#endif
#ifdef PIONEER_Q38_NEXT_GDN_OUTPUT_NATIVE
 assert(!nextPost->Current() && !m->decoder.NextPostAttentionInput() && !m->decoder.NextGdnOutputCompleted());
#endif
#ifdef PIONEER_Q38_NEXT_RESIDUAL_RMS_NATIVE
 assert(!nextFfn->Current() && !m->decoder.NextFfnInput() && !m->decoder.NextResidualRmsCompleted());
#endif
#ifdef PIONEER_Q38_NEXT_FFN_RESIDUAL_NATIVE
 assert(!nextHidden->Current() && !m->decoder.NextFfnHiddenInput() && !m->decoder.NextFfnCompleted());
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
