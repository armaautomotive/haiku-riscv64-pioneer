/* SPDX-License-Identifier: MIT */
// ACTUAL pinned QGW1 or explicitly selected QGW2 instructions. Synthetic endpoints ONLY;
// no ROOT/native owner,completion receipt,model product or card authority.
#define PIONEER_Q38_GLOBAL_WORKER_INITIAL_MACHINE_ONLY
#include "pioneer_q38_global_worker_initial_test.cpp"
#ifdef PIONEER_Q38_LOCAL_GUARD_PRECISE_TEST
#include "../PioneerQ38PreciseWorkerLayout.h"
namespace GuardLayout=PioneerQ38PreciseWorkerLayout;
static constexpr const char* GuardFamily="QGW2";
#else
namespace GuardLayout=G;
static constexpr const char* GuardFamily="QGW1";
#endif
struct LocalGuardPeer:GlobalInitialPeer {
#ifdef PIONEER_Q38_LOCAL_GUARD_PRECISE_TEST
 std::vector<uint8_t> precise;
 LocalGuardPeer(const char* old,const char* image,unsigned worker):GlobalInitialPeer(old,image,worker,8,5120,3),
  precise(Read((std::string(image)+"/precise.bin").c_str())){
  assert(!steps && pc==0x6000 && !precise.empty() && precise.size()<=0x10000);
  boot[0]=GuardLayout::Magic;for(unsigned i=40;i<49;i++)boot[i]=GuardLayout::BootWord(i);
  assert(GuardLayout::BootShape(boot.data()) && !G::BootShape(boot.data()));
  for(unsigned i=0;i<64;i++)memory[M::Boot+i*4]=boot[i];
 }
 const std::vector<uint8_t>* Rom(unsigned address,unsigned& base)const override{
  if(Range(address,0x30000,unsigned(precise.size()))){base=0x30000;return &precise;}
  return GlobalInitialPeer::Rom(address,base);
 }
 void BootGlobal(){
  PoisonDst();Poison();while(pc!=waitPC && pc!=faultPC)StepBlock();
  assert(pc==waitPC && !sequence && !requests && !responses && !pending
   && memory.at(M::Status+12)==GuardLayout::Magic && !memory.at(M::Status+8));
 }
#else
 using GlobalInitialPeer::GlobalInitialPeer;
#endif
 void Stage(){
  std::array<uint32_t,64> j={};const unsigned head[]={GuardLayout::Magic,1,1,boot[3],selfRoute,0,0,2025};
  for(unsigned i=0;i<8;i++){j[i]=head[i];j[8+i]=boot[8+i];}
  for(unsigned i=0;i<10;i++)j[16+i]=matrixJob[i];j[26]=1;
  assert(GuardLayout::JobShape(j.data(),boot.data(),0,0));
  for(unsigned i=0;i<64;i++)memory[M::Job+i*4]=j[i];memory[M::Doorbell]=j[1];
 }
 void FirstRequest(){
  while(!requests && pc!=faultPC && pc!=stopPC)StepBlock();
  assert(requests==1 && pc!=faultPC && pc!=stopPC && !matrixInstructions);
 } // positive admission control,NOT completion/producer/first-layer proof
 void RetainedFault(unsigned issued){
  while(pc!=faultPC && pc!=stopPC)StepBlock();
  assert(pc==faultPC && memory.at(M::Status+8)==GuardLayout::Fault && !memory.at(M::Status+4)
   && requests==issued && !matrixInstructions && !densePackets && !pending);
  const auto beforeResponses=responses;uint32_t beforeCommand[10];
  for(unsigned i=0;i<10;i++)beforeCommand[i]=command[i];
  uint32_t counts[8];for(unsigned i=0;i<8;i++)counts[i]=Count(i);
  const auto frozen=memory;
  for(unsigned i=0;i<2048;i++)StepBlock();
  assert(pc==faultPC && requests==issued && responses==beforeResponses
   && !matrixInstructions && !densePackets && !pending && memory==frozen);
  for(unsigned i=0;i<8;i++)assert(Count(i)==counts[i]);
  for(unsigned i=0;i<10;i++)assert(command[i]==beforeCommand[i]);
 }
};
int main(int argc,char** argv){
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;
 unsigned early=0,after=0,controls=0;
 for(unsigned worker:{1U,96U,119U}){
  auto control=std::make_unique<LocalGuardPeer>(argv[1],argv[2],worker);
  control->BootGlobal();control->Stage();control->FirstRequest();controls++;
  for(unsigned word=0;word<16;word++)for(unsigned timing=0;timing<2;timing++){
   auto peer=std::make_unique<LocalGuardPeer>(argv[1],argv[2],worker);
   peer->BootGlobal();peer->Stage();if(timing)peer->FirstRequest();
   peer->source[word]^=1U<<((word*7+5)%32);
   peer->RetainedFault(timing);if(timing)after++;else early++;
  }
 }
 printf("PASS ACTUAL pinned %s valid first-matrix admission controls%u/all16 local physical-source faults x3 workers:%u BEFORE any NoC,%u AFTER first genuine NoC request/NO following request,matrix product,completion or retry/actual2048 inert steps each;synthetic endpoints ONLY,NOT ROOT/native/full-model/physical/Work/TPS qualification\n",GuardFamily,controls,early,after);
}
