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
void ExecuteMixedOriginalBlock(const char* root,const char* worker,const char* qdp,const char* qda,
 ProjectionExecutionLearnedBytes& bytes,const uint32_t* embedding,unsigned words,uint32_t* output){
 (void)&BlockInitialRefusals;(void)&BlockLaterRefusals;(void)&JoinedRefusals;(void)&ProjectionRefusals;
 (void)&Refusals;(void)&Group;(void)&Geometry;(void)&Faults;(void)&D::Run;
 assert(bytes.Ready() && embedding && output && words==5120);
 auto m=std::make_unique<CanonicalLiveMachine>(root,worker,qdp,qda);assert(m->WaitLive(0));
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
 for(unsigned i=0;i<words;i++)output[i]=m->WordAt(finalRoute,finalBase+i*4);
 printf("PASS original standalone layer3/position0/ALL7 real-weight matrices+norms/actual QBL1+SAME119 instructions/real119STOP+ROOT terminal/stack%u;NOT GDN0..2/fullmodel/native hardware/Work/TPS\n",0x70000-m->minStack);
}
