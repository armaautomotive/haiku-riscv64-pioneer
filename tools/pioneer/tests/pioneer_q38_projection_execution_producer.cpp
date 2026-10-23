/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_RMS_FACTORY_FIXTURE_ONLY
#include "pioneer_q38_learned_rms_native_factory_test.cpp"
#include "pioneer_q38_projection_execution_producer.h"
struct ProjectionExecutionProducer::Impl:NativeFactoryFixture {
 PioneerQ38LearnedRmsResult rms;
 Impl(const char* zero,const char* embedding,const char* norm):NativeFactoryFixture(zero,embedding){
  (void)&::Input;(void)&::Prepare;(void)&::Upload;(void)&::Close;(void)&::PreparePanel;(void)&::Run;(void)&::Job;(void)&::ClosePanel;(void)&::Finish;
  io.rmsImage=norm;
  auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());
  for(unsigned i=0;i<plan->Count();i++){const auto* p=plan->Part(i);
   bool selected=p->kind==plan->WeightKind && !p->layer && !p->role;
   if(p->kind==plan->WeightKind && !p->layer && (p->role==5 || p->role==6 || p->role==10 || p->role==11))selected=true;
   if(selected){unsigned first=p->bank*unsigned(plan->Capacity/4096)+p->offset/4096;
    for(unsigned n=0;n<p->allocated/4096;n++)learned[first+n]=1;}
  }
  assert(Bind() && Initialize() && Run() && RunQ38LearnedRmsModel(*c,rms));
  assert(c->q38EmbeddingModel->Closed() && c->q38EmbeddingModel->rms.Completed());
 }
};
ProjectionExecutionProducer::ProjectionExecutionProducer(const char* zero,const char* embedding,const char* rms):implementation(new Impl(zero,embedding,rms)){}
ProjectionExecutionProducer::~ProjectionExecutionProducer(){delete implementation;}
PioneerQ38EmbeddingNativeState& ProjectionExecutionProducer::Model(){return *implementation->c->q38EmbeddingModel;}
const tt_dram_samples_result& ProjectionExecutionProducer::Proof(){return implementation->owner.dram;}
PioneerResidentBanks& ProjectionExecutionProducer::Banks(){return implementation->owner.banks;}
const uint8_t* ProjectionExecutionProducer::Ledger(){return implementation->learned.data();}
std::vector<uint32_t>& ProjectionExecutionProducer::DeviceWords(){return implementation->io.wholeWorkspace;}
bool ProjectionExecutionProducer::Valid()const{
 const auto& f=*implementation;auto& c=*f.c;const auto& clock=c.q38Clock.Snapshot();
 return sRuntimeOwner==&f.cookie && f.cookie.context==&c && f.cookie.magic==0x50313030 && !sColdRecoveryRequired && !sReloadQuiesced
  && !c.dispatchOwnerRetained && c.q38EmbeddingModel && c.q38EmbeddingModel->ledger==c.initialized
  && c.q38EmbeddingModel->Closed() && !c.Unsafe() && c.q38Clock.RequiresExplicitEnd() && clock.state==TT_ARC_CLOCK_BUSY_VERIFIED
  && clock.busy_requested && clock.busy_acknowledged && !clock.restored && !f.io.retained;
}
void ProjectionExecutionProducer::Retain(){tt_resident_session_result r={};RetainQ38DispatchOwner(*implementation->c,r);}
void ProjectionExecutionProducer::RootControls(uint32_t* source,uint32_t* command,uint32_t* counts){
 auto& f=*implementation;
 for(unsigned i=0;i<16;i++)source[i]=i?f.io.source[i]:f.io.config;
 for(unsigned i=0;i<10;i++)command[i]=f.io.command[i];
 for(unsigned i=0;i<8;i++)counts[i]=f.io.PanelIO::ResidentNocRead(PioneerQ38SpanCounter(i));
}
