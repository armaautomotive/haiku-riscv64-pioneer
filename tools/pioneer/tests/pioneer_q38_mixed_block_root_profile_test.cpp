/* SPDX-License-Identifier: MIT */
// Instruction/packet workload, NOT a cycle model or silicon benchmark.
// Reuse exactly the numerical observers and byte endpoints of QBR1.
#define PIONEER_Q38_MIXED_BLOCK_ROOT_MACHINE_ONLY
#include "pioneer_q38_mixed_block_root_test.cpp"
struct BlockProfileMachine:BlockRootMachine {
 bool profiling=false;unsigned profilePhase=0,rootSteps=0,rootPackets=0;
 std::array<unsigned,119> workerSteps={};
 using BlockRootMachine::BlockRootMachine;
 void Snapshot(){
  rootSteps=steps;rootPackets=packets;for(unsigned w=0;w<119;w++)workerSteps[w]=child[w]->steps;
 }
 void Record(){
  uint64_t sum=0;unsigned maximum=0;
  for(unsigned w=0;w<119;w++){
   unsigned delta=child[w]->steps-workerSteps[w];sum+=delta;maximum=std::max(maximum,delta);
  }
  printf("QBR_WORKLOAD phase%u root_instructions%u root_packets%u worker_max_instructions%u worker_sum_instructions%llu;NOT cycles or native timing\n",
   profilePhase,steps-rootSteps,packets-rootPackets,maximum,static_cast<unsigned long long>(sum));
  Snapshot();
 }
 void Store(uint32_t address,uint32_t value)override{
  if(profiling && address==B::Status+20 && value!=profilePhase){Record();profilePhase=value;}
  BlockRootMachine::Store(address,value);
 }
};
int main(int argc,char** argv){
 assert(argc==5);(void)&BlockInitialRefusals;(void)&BlockLaterRefusals;(void)&JoinedRefusals;(void)&ProjectionRefusals;
 (void)&Refusals;(void)&Group;(void)&Geometry;(void)&Faults;(void)&D::Run;setvbuf(stdout,nullptr,_IOLBF,0);
 puts("START QBR1 actual ISA/packet workload profile;unchanged pinned code+SAME119/numerical observers enabled;NOT native cycles or inference throughput");
 auto m=std::make_unique<BlockProfileMachine>(argv[1],argv[2],argv[3],argv[4]);assert(m->WaitBlock(0));
 m->StageBlock(33,0,3);unsigned before=m->steps,packets=m->packets;m->Snapshot();m->profiling=true;
 assert(m->WaitBlock(m->sequence));m->Record();m->VerifyBlock();
 printf("PASS QBR1 complete numerically checked block WORKLOAD root_instructions%u root_packets%u;qualification audits+scalar ROOT arithmetic are not a fast-Work guarantee;NO cycle/latency/throughput claim\n",m->steps-before,m->packets-packets);
}
