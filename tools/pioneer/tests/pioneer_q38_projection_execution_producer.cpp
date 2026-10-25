/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_RMS_FACTORY_FIXTURE_ONLY
#include "pioneer_q38_learned_rms_native_factory_test.cpp"
#include "pioneer_q38_projection_execution_producer.h"
#ifdef PIONEER_Q38_RESIDENT_DECODER_OWNER
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ConvHistoryZeroController.h"
// TEST adapter to the SAME physical-endpoint fixture. This is NOT an
// implementation of the still-owed actual native history factory/scope.
struct DecoderHistoryTestIO {
 PioneerSessionIO& io;ProjectionExecutionProducer& producer;
 bool ConvHistoryZeroBeforeCohort(){return producer.Valid() && io.q38MutableContext
  && io.q38MutableContext->q38GdnCallThread==currentThread && !io.fleetEnabled && !io.q38SpanAllowed;}
 bool ConvHistoryZeroOtherCoresHeld(){return io.StateZeroOtherCoresHeld();}
 void RuntimeResetWrite(uint32_t value){io.ZeroControlIO::RuntimeResetWrite(value);}
#define DECODER_TEST_FORWARD(name) template<class... A> auto name(A&&... a){return io.name(static_cast<A&&>(a)...);}
 DECODER_TEST_FORWARD(SelectRuntimeCore) DECODER_TEST_FORWARD(Q38PanelMappingStable) DECODER_TEST_FORWARD(BulkMappingStable)
 DECODER_TEST_FORWARD(SpanViewCapacity) DECODER_TEST_FORWARD(ResetRead) DECODER_TEST_FORWARD(SourceRead)
 DECODER_TEST_FORWARD(ResidentNocRead) DECODER_TEST_FORWARD(HostPciRead) DECODER_TEST_FORWARD(HostPageMap)
 DECODER_TEST_FORWARD(OutputBufferMap) DECODER_TEST_FORWARD(OutputBufferPageMap) DECODER_TEST_FORWARD(WideOutputMap)
 DECODER_TEST_FORWARD(WideOutputPageMap) DECODER_TEST_FORWARD(QuarantineHostPage) DECODER_TEST_FORWARD(QuarantineOutputBuffer)
 DECODER_TEST_FORWARD(QuarantineWideOutput) DECODER_TEST_FORWARD(Q38PanelSavedWords) DECODER_TEST_FORWARD(BulkSavedWords)
 DECODER_TEST_FORWARD(RuntimeCodeRead) DECODER_TEST_FORWARD(RuntimeCodeWrite) DECODER_TEST_FORWARD(Q38PanelRead)
 DECODER_TEST_FORWARD(Q38PanelWrite) DECODER_TEST_FORWARD(BulkL1Read) DECODER_TEST_FORWARD(BulkL1Write)
 DECODER_TEST_FORWARD(RuntimeStatusRead) DECODER_TEST_FORWARD(RuntimeStatusWrite) DECODER_TEST_FORWARD(Pause)
 DECODER_TEST_FORWARD(Q38CohortNow) DECODER_TEST_FORWARD(RuntimePcRead)
#undef DECODER_TEST_FORWARD
};
#endif
struct ProjectionExecutionProducer::Impl:NativeFactoryFixture {
 PioneerQ38LearnedRmsResult rms;
#ifdef PIONEER_Q38_RESIDENT_DECODER_OWNER
 std::vector<uint32_t> history=std::vector<uint32_t>(1680*1024,0xa55aa55a);
 PioneerQ38ConvHistoryExtent historyExtents[336]={};PioneerQ38ConvHistoryZeroLease historyZero;
#endif
 Impl(const char* zero,const char* embedding,const char* norm):NativeFactoryFixture(zero,embedding){
  (void)&::Input;(void)&::Prepare;(void)&::Upload;(void)&::Close;(void)&::PreparePanel;(void)&::Run;(void)&::Job;(void)&::ClosePanel;(void)&::Finish;
  io.rmsImage=norm;
  auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());
  for(unsigned i=0;i<plan->Count();i++){const auto* p=plan->Part(i);
   bool selected=p->kind==plan->WeightKind && !p->layer && !p->role;
   if(p->kind==plan->WeightKind && !p->layer && (p->role==5 || p->role==6 || p->role==10 || p->role==11))selected=true;
#ifdef PIONEER_Q38_RESIDENT_DECODER_OWNER
   if(p->kind==plan->WeightKind && !p->layer && p->role==12)selected=true;
#endif
   if(selected){unsigned first=p->bank*unsigned(plan->Capacity/4096)+p->offset/4096;
    for(unsigned n=0;n<p->allocated/4096;n++)learned[first+n]=1;}
  }
  bool bound=
#if defined(PIONEER_Q38_RESIDENT_DECODER_OWNER) || defined(PIONEER_Q38_TEST_CLOSED_INPUT_POSITION_ZERO)
   BindQ38EmbeddingModel(*c,token,0,bank);
#else
   Bind();
#endif
  assert(bound && Initialize() && Run() && RunQ38LearnedRmsModel(*c,rms));
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
#ifdef PIONEER_Q38_RESIDENT_DECODER_OWNER
void ProjectionExecutionProducer::InitializeHistory(){
 auto& f=*implementation;auto& c=*f.c;auto& m=*c.q38EmbeddingModel;auto* h=m.graph.ConvHistory(m.identity);assert(h && Valid());
 DecoderHistoryTestIO io{f.io,*this};PioneerQ38ConvHistoryZeroResult result;
 assert(PioneerQ38ConvHistoryZeroPrepare(io,f.owner.dram,f.owner.noc,*h,f.owner.banks,m.identity,f.owner.host,f.owner.store,f.bulk,f.transfer,f.wide,f.historyZero));
 for(unsigned region=0;region<336;region++){
  assert(h->InitializeSpan(m.identity,region,0,5,f.historyExtents[region]));
  assert(PioneerQ38ConvHistoryZeroRun(io,f.owner.dram,f.owner.noc,*h,f.owner.banks,f.owner.host,f.owner.store,f.bulk,f.transfer,f.wide,
   f.historyZero,m.identity,region,0,5,result) && result.pages==5 && result.completed && result.held);
  assert(f.io.lastDeviceWords.size()==5120);for(unsigned w=0;w<5120;w++)f.history[region*5120+w]=f.io.lastDeviceWords[w];
 }
 assert(PioneerQ38ConvHistoryZeroClose(io,f.owner.dram,f.owner.noc,*h,f.owner.banks,f.owner.host,f.owner.store,f.bulk,f.transfer,f.wide,f.historyZero,true)
  && h->InitializationPublished(m.identity) && Valid());
}
bool ProjectionExecutionProducer::ReadHistory(unsigned bank,uint32_t address,uint32_t* out,unsigned words){
 auto& f=*implementation;if(!out || !words || (address&3))return false;
 for(unsigned i=0;i<336;i++){const auto& e=f.historyExtents[i];
  if(e.bank==bank && address>=e.address && address-e.address<e.bytes && words<=(e.bytes-(address-e.address))/4){
   for(unsigned w=0;w<words;w++)out[w]=f.history[i*5120+(address-e.address)/4+w];return true;
  }
 }
 return false;
}
#endif
void ProjectionExecutionProducer::Retain(){tt_resident_session_result r={};RetainQ38DispatchOwner(*implementation->c,r);}
void ProjectionExecutionProducer::RootControls(uint32_t* source,uint32_t* command,uint32_t* counts){
 auto& f=*implementation;
 for(unsigned i=0;i<16;i++)source[i]=i?f.io.source[i]:f.io.config;
 for(unsigned i=0;i<10;i++)command[i]=f.io.command[i];
 for(unsigned i=0;i<8;i++)counts[i]=f.io.PanelIO::ResidentNocRead(PioneerQ38SpanCounter(i));
}
