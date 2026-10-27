/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_MACHINE_ONLY
#include "pioneer_q38_mixed_live_block_root_test.cpp"
#include "../PioneerQ38MixedLiveBlockPlan.h"
struct CanonicalLiveMachine:LiveBlockMachine {
 using LiveBlockMachine::LiveBlockMachine;
 std::unique_ptr<PioneerQ38MixedLiveBlockPlan> plan;
 void StageCanonical(unsigned tokens,unsigned trial,unsigned layer){
  StageLive(tokens,trial,layer,bool(trial));uint32_t sources[1920];
  for(unsigned i=0;i<16;i++)sources[i]=source[i];
  for(unsigned w=0;w<119;w++)for(unsigned i=0;i<16;i++)sources[(w+1)*16+i]=child[w]->source[i];
  plan=std::make_unique<PioneerQ38MixedLiveBlockPlan>();
  assert(plan->Build(*canonical,layer,tokens-1,outer[52],outer[7],outer[6],sequence,{1,2,3,4},sources,blockOuter[73],blockOuter[74])
   && plan->Matches(*canonical));
  // Compare independently staged canonical matrix/norm records BEFORE
  // replacing any RAW command bytes. These are not arithmetic products.
  for(unsigned i=0;i<128;i++)assert(blockOuter[i]==plan->Outer()[i]);
  for(unsigned w=0;w<120;w++)for(unsigned i=0;i<64;i++){
   unsigned actual=w?child[w-1]->memory.at(M::Boot+i*4):memory.at(T::Boot+i*4);assert(actual==plan->Boot(w)[i]);
  }
  for(unsigned slot=0;slot<7;slot++)for(unsigned i=0;i<1280;i++){
   unsigned actual=slot<4?memory.at(T::Recipes+slot*T::RecipeStride+i*4):WordAt(outer[8],blockOuter[68]+(slot-4)*T::RecipeStride+i*4);
   assert(actual==plan->Recipe(slot)[i]);
  }
  // The old synthetic fabric used arbitrary noncanonical KV addresses.
  // Execute canonical actual-model HEAD-major locations instead. The
  // endpoint supplies the SAME RAW prior cache bytes at these locations;
  // actual ROOT/worker arithmetic and independent observers are unchanged.
  for(unsigned w=0;w<96;w++)for(unsigned i=0;i<64;i++){
   unsigned address=T::Frames+w*256+i*4;
   if(i<8 || i>=14)assert(memory.at(address)==plan->Dense(w)[i]);
   memory[address]=plan->Dense(w)[i];
  }
  puts("STAGED immutable canonical QBL1 plan/ALL7matrix recipes+120boot seals match independently staged bytes/96canonical KV frames;RAW synthetic tensors only");
 }
};
int main(int argc,char** argv){
 assert(argc==5);(void)&BlockInitialRefusals;(void)&BlockLaterRefusals;(void)&JoinedRefusals;(void)&ProjectionRefusals;
 (void)&Refusals;(void)&Group;(void)&Geometry;(void)&Faults;(void)&D::Run;setvbuf(stdout,nullptr,_IOLBF,0);
 puts("START actual QBL1 canonical DATA plan/device instructions/SAME fresh119/2fullblocks+STOP;NOT native or original-GGUF arithmetic");
 auto m=std::make_unique<CanonicalLiveMachine>(argv[1],argv[2],argv[3],argv[4]);assert(m->WaitLive(0));
 for(unsigned trial=0;trial<2;trial++){
  m->StageCanonical(trial?33:1,trial,trial?7:3);assert(m->WaitLive(m->sequence));m->VerifyLive();
  printf("PASS actual canonical command plan frame%u/N%u/layer%u/complete block+device output input/SAME119/stack%u\n",m->sequence,trial?33:1,trial?7:3,0x70000-m->minStack);
 }
 m->StageStop();assert(m->WaitLive(m->sequence,true));m->VerifyStop();
 puts("PASS canonical QBL1 plan actual instructions/2wholeblocks+real119STOP/rootterminal;SYNTHETIC RAW arithmetic endpoints/NO native/full64/tokencommit/silicon-speed grant");
}
