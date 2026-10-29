/* SPDX-License-Identifier: MIT */
// ACTUAL pinned QGW1 instructions. Synthetic NoC/byte endpoints ONLY;
// no ROOT/native owner,completion receipt,model product or card authority.
#define PIONEER_Q38_GLOBAL_WORKER_INITIAL_MACHINE_ONLY
#include "pioneer_q38_global_worker_initial_test.cpp"
struct LocalGuardPeer:GlobalInitialPeer {
 using GlobalInitialPeer::GlobalInitialPeer;
 void Stage(){
  const auto j=Command(false);assert(G::JobShape(j.data(),boot.data(),0,0));
  for(unsigned i=0;i<64;i++)memory[M::Job+i*4]=j[i];memory[M::Doorbell]=j[1];
 }
 void FirstRequest(){
  while(!requests && pc!=faultPC && pc!=stopPC)StepBlock();
  assert(requests==1 && pc!=faultPC && pc!=stopPC && !matrixInstructions);
 } // positive admission control,NOT completion/producer/first-layer proof
 void RetainedFault(unsigned issued){
  while(pc!=faultPC && pc!=stopPC)StepBlock();
  assert(pc==faultPC && memory.at(M::Status+8)==G::Fault && !memory.at(M::Status+4)
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
 printf("PASS ACTUAL pinned QGW1 valid first-matrix admission controls%u/all16 local physical-source faults x3 workers:%u BEFORE any NoC,%u AFTER first genuine NoC request/NO following request,matrix product,completion or retry/actual2048 inert steps each;synthetic endpoints ONLY,NOT ROOT/native/full-model/physical/Work/TPS qualification\n",controls,early,after);
}
