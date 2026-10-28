/* SPDX-License-Identifier: MIT */
// Different class names avoid weak ODR collisions with the unflagged QPE1 TU.
#define Machine PrefixOriginalMachineBase
#define RuntimeMachine PrefixOriginalRuntimeMachine
#define BlockMachine PrefixOriginalBlockMachine
#define StreamMachine PrefixOriginalStreamMachine
#define ScalarMachine PrefixOriginalScalarMachine
#define PIONEER_Q38_RECURRENT_PREFIX_FIXTURE_ONLY
#include "pioneer_q38_recurrent_prefix_root_test.cpp"
#include "pioneer_q38_recurrent_prefix_original_instructions.h"
struct OriginalAutonomousPrefixMachine:AutonomousPrefixMachine {
 RecurrentBlockRawLearnedBytes& original;PrefixOriginalInstructionTrace captured;
 OriginalAutonomousPrefixMachine(const char* image,const char* worker,RecurrentBlockRawLearnedBytes& bytes,const uint32_t* embedding)
  :AutonomousPrefixMachine(image,worker),original(bytes){
  assert(original.Ready() && embedding);unsigned count=0;uint64_t length=0;
  for(unsigned i=0;i<canonical->Count();i++){
   const auto* p=canonical->Part(i);if(p->kind!=canonical->WeightKind || p->layer>=3)continue;
   assert(original.Contains(PioneerQ38ProjectionRoute(p->bank),canonical->Base+p->offset,p->bytes));count++;length+=p->bytes;
  }
  assert(count==168 && length==842775168);
  // Remove EVERY synthetic learned word across ALL3 layers. Get below has
  // no synthetic matrix fallback, even if an original extent is missing.
  for(auto i=dram.begin();i!=dram.end();){
   if(original.Contains(unsigned(i->first>>32),unsigned(i->first),4))i=dram.erase(i);else ++i;
  }
  // Input is produced by separate actual QPE1 instructions, NOT an oracle.
  // This is an offline instruction boundary, NOT a native producer grant.
  for(unsigned i=0;i<5120;i++)Put(PioneerQ38ProjectionRoute(outer[68]),outer[69]+i*4,embedding[i]);
 }
 uint32_t Get(unsigned route,unsigned address)const override{
  if(original.Contains(route,address,4)){uint32_t value;assert(original.Word(route,address,value));return value;}
  auto raw=dram.find(Key(route,address));assert(raw!=dram.end());return raw->second;
 }
 void Submit()override{
  assert(original.BeginRawPacket());AutonomousPrefixMachine::Submit();assert(original.EndRawPacket());
 }
 void LocalCapture(PioneerRecurrentOriginalStage stage,unsigned address,unsigned words){
  auto& out=captured.actual.at(outer[3])[stage];assert(out.empty());out.resize(words);
  for(unsigned i=0;i<words;i++)out[i]=memory.at(address+i*4);
 }
 void RemoteCapture(PioneerRecurrentOriginalStage stage,unsigned role,unsigned words){
  auto& out=captured.actual.at(outer[3])[stage];assert(out.empty());out.resize(words);
  for(unsigned i=0;i<words;i++)out[i]=Get(outer[8],outer[9]+PioneerQ38MixedRecurrentOffset(role)+i*4);
 }
 void ObserveMatrix()override{
  bool fresh=Phase()!=observed;RecurrentBlockMachine::ObserveMatrix();if(!fresh)return;
  switch(Phase()){
   case 2:LocalCapture(PioneerRecurrentEmbedding,RB::OriginalCopy,5120);LocalCapture(PioneerRecurrentNormalized,RB::Input,5120);break;
   case 8:{
    RemoteCapture(PioneerRecurrentAlpha,10,48);RemoteCapture(PioneerRecurrentBeta,11,48);RemoteCapture(PioneerRecurrentZ,5,6144);RemoteCapture(PioneerRecurrentQkv,6,10240);
    auto& stages=captured.actual.at(outer[3]);auto& conv=stages[PioneerRecurrentConv];conv.resize(10240,0x7fc00000);
    for(unsigned w=0;w<120;w++)for(unsigned batch=0;batch<PioneerQ38ConvBatchCount(w);batch++){
     const auto* d=block->Conv()+w*384+batch*64;for(unsigned i=0;i<16;i++)conv.at(d[7]+i)=Get(d[8],d[4]+d[43]+i*4);
    }
    auto& state=stages[PioneerRecurrentState];state.resize(786432,0x7fc00000);auto& raw=stages[PioneerRecurrentRaw];raw.resize(6144,0x7fc00000);
    for(unsigned w=0;w<96;w++){
     const auto* d=block->Gdn()+w*192+64;unsigned head=w/2,half=w%2;
     for(unsigned key=0;key<128;key++)for(unsigned value=0;value<64;value++)state[head*16384+key*128+half*64+value]=Get(d[13],d[7]+(key*64+value)*4);
     for(unsigned i=0;i<64;i++)raw[head*128+half*64+i]=child[w]->memory.at(0x8e800+i*4);
    }
    auto& gated=stages[PioneerRecurrentGated];gated.resize(6144);for(unsigned i=0;i<6144;i++)gated[i]=Get(outer[8],outer[9]+TT_Q38_GDN_GATED_OUTPUT+i*4);
    break;
   }
   case 10:RemoteCapture(PioneerRecurrentProjection,13,5120);LocalCapture(PioneerRecurrentResidual,RB::PostResidual,5120);LocalCapture(PioneerRecurrentPostNormalized,RB::PostNormalized,5120);break;
   case 13:RemoteCapture(PioneerRecurrentFfnGate,2,17408);RemoteCapture(PioneerRecurrentFfnUp,3,17408);LocalCapture(PioneerRecurrentActivated,RB::Input,17408);break;
  }
 } // snapshots observe only; never change instructions, metadata or products
 void Store(uint32_t address,uint32_t value)override{
  unsigned before=completedBlocks;AutonomousPrefixMachine::Store(address,value);
  if(completedBlocks==before)return;
  assert(completedBlocks==before+1 && outer[3]==before);RemoteCapture(PioneerRecurrentDown,4,5120);
  auto& out=captured.actual.at(before)[PioneerRecurrentFinal];assert(out.empty());out.resize(5120);
  for(unsigned i=0;i<5120;i++)out[i]=Get(outer[8],outer[9]+RB::FinalOffset+i*4);
 }
};
void ExecutePrefixOriginalInstructions(const char* image,const char* worker,RecurrentBlockRawLearnedBytes& bytes,const uint32_t* embedding,unsigned words,PrefixOriginalInstructionTrace& trace){
 assert(image && worker && embedding && words==5120 && bytes.Ready());(void)&RunPersistentGdnPair;(void)&Refusals;(void)&LateRefusals;
 for(const auto& layer:trace.actual)for(const auto& stage:layer)assert(stage.empty());
 auto m=std::make_unique<OriginalAutonomousPrefixMachine>(image,worker,bytes,embedding);assert(m->WaitBlock(0));
 m->memory[RB::Doorbell]=1; // ONE host start; no later host frame or release
 assert(m->WaitStop());m->VerifyStop();
 assert(m->completedBlocks==3 && m->selfReleases==3 && m->selfOuterWrites==384
  && m->metadataReadBytes==3*(XP::Bytes-512) && m->metadataWriteBytes==m->metadataReadBytes && m->headerReadBytes==5*512);
 for(const auto& previous:m->saved)previous.Verify(*m);
 for(unsigned layer=0;layer<3;layer++)for(const auto& stage:m->captured.actual[layer])assert(!stage.empty());
 for(unsigned layer=1;layer<3;layer++)assert(m->captured.actual[layer][PioneerRecurrentEmbedding]==m->captured.actual[layer-1][PioneerRecurrentFinal]);
 assert(bytes.Ready());trace=std::move(m->captured);
 puts("PASS ORIGINAL autonomous instructions/actual QPE1 input/ONE host start/layers0->1->2/SAME119/ROOT-only metadata+releases/Conv+GDN NEXT retained/54actual snapshots/realSTOP;SIMULATED endpoints,NOT native,physical,persistent-token,full64,Work or TPS");
}
