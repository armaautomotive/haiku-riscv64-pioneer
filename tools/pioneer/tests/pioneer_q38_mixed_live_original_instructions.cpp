/* SPDX-License-Identifier: MIT */
// Keep operand-dependent machine layouts distinct across the QPE1 TU.
#define Machine MixedOriginalMachineBase
#define RuntimeMachine MixedOriginalRuntimeMachine
#define BlockMachine MixedOriginalBlockMachine
#define StreamMachine MixedOriginalStreamMachine
#define ScalarMachine MixedOriginalScalarMachine
#define PIONEER_Q38_MIXED_LIVE_CANONICAL_MACHINE_ONLY
#include "pioneer_q38_mixed_live_canonical_joined_test.cpp"
#include "pioneer_q38_mixed_live_original_instructions.h"
struct OriginalTraceMachine final:CanonicalLiveMachine {
 MixedOriginalInstructionTrace& trace;
 OriginalTraceMachine(const char* r,const char* w,const char* p,const char* a,MixedOriginalInstructionTrace& t)
  :CanonicalLiveMachine(r,w,p,a),trace(t){}
 void Local(PioneerQ38MixedLiveReferenceStage stage,unsigned at,unsigned words){
  auto& out=trace.actual[stage];assert(out.empty());out.resize(words);
  for(unsigned i=0;i<words;i++)out[i]=memory.at(at+i*4);
 }
 void Remote(PioneerQ38MixedLiveReferenceStage stage,unsigned route,unsigned at,unsigned words){
  auto& out=trace.actual[stage];assert(out.empty());out.resize(words);
  for(unsigned i=0;i<words;i++)out[i]=WordAt(route,at+i*4);
 }
 void ObserveInitial()override{
  CanonicalLiveMachine::ObserveInitial();Local(PioneerQ38MixedEmbedding,B::Original,5120);Local(PioneerQ38MixedNormalized,B::Raw,5120);
 }
 void ObserveProjections()override{
  CanonicalLiveMachine::ObserveProjections();PioneerQ38MixedLiveReferenceStage stage;
  switch(descriptor[43]){
   case 5:stage=PioneerQ38MixedQg;break;case 6:stage=PioneerQ38MixedKey;break;case 7:stage=PioneerQ38MixedValue;break;
   case 10:stage=PioneerQ38MixedProjection;break;case 2:stage=PioneerQ38MixedFfnGate;break;
   case 3:stage=PioneerQ38MixedFfnUp;break;case 4:stage=PioneerQ38MixedDown;break;default:assert(false);return;
  }
  Remote(stage,descriptor[8],descriptor[4]+descriptor[10],descriptor[11]);
 }
 void ObservePrepared()override{
  CanonicalLiveMachine::ObservePrepared();Local(PioneerQ38MixedQuery,J::Input,6144);Local(PioneerQ38MixedGate,J::Gate,6144);
  for(unsigned kind=0;kind<2;kind++){
   auto& out=trace.actual[kind?PioneerQ38MixedHalfValue:PioneerQ38MixedHalfKey];assert(out.empty());out.resize(1024);
   for(unsigned g=0;g<4;g++)for(unsigned i=0;i<256;i++){
    uint32_t packed=memory.at((kind?J::NewV:J::NewK)+g*512+(i/2)*4);uint16_t bits=uint16_t(packed>>((i%2)*16));
    _Float16 half;memcpy(&half,&bits,2);out[g*256+i]=Bits(float(half));
   }
  }
 }
 void ObserveAttention()override{CanonicalLiveMachine::ObserveAttention();Local(PioneerQ38MixedAttention,J::Output,6144);}
 void ObservePost()override{
  CanonicalLiveMachine::ObservePost();Local(PioneerQ38MixedResidual,B::PostResidual,5120);Local(PioneerQ38MixedPostNormalized,B::PostNormalized,5120);
 }
 void ObserveActivation()override{CanonicalLiveMachine::ObserveActivation();Local(PioneerQ38MixedActivated,B::Input,17408);}
};
void ExecuteMixedOriginalBlock(const char* root,const char* worker,const char* qdp,const char* qda,
 ProjectionExecutionLearnedBytes& bytes,const uint32_t* embedding,unsigned words,uint32_t* output,MixedOriginalInstructionTrace& trace){
 (void)&BlockInitialRefusals;(void)&BlockLaterRefusals;(void)&JoinedRefusals;(void)&ProjectionRefusals;
 (void)&Refusals;(void)&Group;(void)&Geometry;(void)&Faults;(void)&D::Run;
 assert(bytes.Ready() && embedding && output && words==5120);
 for(const auto& v:trace.actual)assert(v.empty());
 auto m=std::make_unique<OriginalTraceMachine>(root,worker,qdp,qda,trace);assert(m->WaitLive(0));
 m->StageCanonical(1,0,3);m->originalLearned=&bytes;
 // Replace ALL synthetic learned norm endpoints with the original F32
 // bytes. Command metadata and position0 RoPE constants are not learned
 // products. All seven matrix endpoints use the raw reader in Geometry.
 for(unsigned i=0;i<m->canonical->Count();i++){
  const auto* p=m->canonical->Part(i);
  if(p->kind!=m->canonical->WeightKind || p->layer!=3 || p->type)continue;
  assert(p->rows==1 && (p->bytes==20480 || p->bytes==1024));std::vector<uint8_t> raw(p->bytes);
  unsigned route=PioneerQ38ProjectionRoute(p->bank),base=m->canonical->Base+p->offset;
  assert(bytes.Read(route,base,raw.data(),unsigned(raw.size())));
  for(unsigned at=0;at<raw.size();at+=4)m->dram[CanonicalLiveMachine::Key(route,base+at)]=Word(raw,at);
 }
 unsigned route=PioneerQ38ProjectionRoute(m->blockOuter[73]),base=m->blockOuter[74];
 for(unsigned i=0;i<words;i++){
  m->wantedInput[i]=embedding[i];m->dram[CanonicalLiveMachine::Key(route,base+i*4)]=embedding[i];
 }
 assert(m->WaitLive(m->sequence));m->VerifyLive();
 unsigned finalRoute=m->outer[8],finalBase=m->outer[9]+T::FinalOffset;
 m->StageStop();assert(m->WaitLive(m->sequence,true));m->VerifyStop();assert(bytes.Ready());
 m->Remote(PioneerQ38MixedFinal,finalRoute,finalBase,5120);
 for(const auto& v:trace.actual)assert(!v.empty());
 for(unsigned i=0;i<words;i++)output[i]=m->WordAt(finalRoute,finalBase+i*4);
 printf("PASS original standalone layer3/position0/ALL7 real-weight matrices+norms/actual QBL1+SAME119 instructions/real119STOP+ROOT terminal/stack%u;NOT GDN0..2/fullmodel/native hardware/Work/TPS\n",0x70000-m->minStack);
}
