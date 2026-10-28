/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_RECURRENT_LIFECYCLE_FIXTURE_ONLY
#include "pioneer_q38_mixed_recurrent_block_lifecycle_test.cpp"
#include "../PioneerQ38RecurrentHistoryLayout.h"
namespace H=PioneerQ38RecurrentHistoryLayout;
template<class Change> static void HistoryFault(RecurrentLifecycleMachine& m,const char* name,Change change){
 fflush(nullptr);pid_t child=fork();assert(child>=0);
 if(!child){
  change(m);assert(!m.WaitBlock(m.request) && m.unknown && m.pc==0x4009c
   && m.memory.at(RB::JointSummary)!=RB::Magic && m.memory.at(RB::Status+4)!=m.request && !m.finalWords);
  unsigned packets=m.packets,writes=m.peerWrites;auto physical=m.counts;auto commands=m.command;
  m.memory[RB::Doorbell]=m.request+1;for(unsigned i=0;i<2048;i++)m.StepScalar();
  assert(m.pc==0x4009c && m.packets==packets && m.peerWrites==writes && m.counts==physical && m.command==commands);
  printf("PASS actual history-staging refusal %s/inert2048/no later packet,final hidden,receipt,retry or commit\n",name);fflush(nullptr);_exit(0);
 }
 int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));
}
static void VerifyStaged(RecurrentLifecycleMachine& m){
 assert(m.outer[4]==0 && m.outer[12]==0 && m.outer[13]==0);
 uint32_t address=0;assert(H::Address(m.outer[9],m.outer[3],address));
 const unsigned receipt[]={H::Marker,address,H::Bytes,m.request,m.outer[3],m.outer[4],m.outer[12],1};
 for(unsigned i=0;i<8;i++)assert(m.memory.at(RB::JointSummary+(56+i)*4)==receipt[i]);
 for(unsigned i=0;i<H::Bytes/4;i++)assert(m.Get(m.outer[8],address+i*4)==m.Get(m.outer[8],m.outer[9]+TT_Q38_CONV_NEXT+i*4));
 printf("PASS actual card instructions preserved layer%u history%u bytes/receipt%u;NO host arithmetic or current-history commit\n",m.outer[3],H::Bytes,m.request);
}
static void VerifyZeroCurrent(const RecurrentLifecycleMachine& m){
 assert(m.outer[3]<3 && m.outer[4]==0 && m.outer[12]==0 && m.outer[13]==0);
 for(unsigned w=0;w<120;w++)for(unsigned job=0;job<PioneerQ38ConvBatchCount(w);job++){
  const auto* d=m.block->Conv()+w*384+job*64;
  for(unsigned i=0;i<16*3;i++)assert(m.Get(d[41],d[15]+i*4)==0);
 }
 for(unsigned w=0;w<96;w++){
  const auto* d=m.block->Gdn()+w*192+64;
  for(unsigned i=0;i<8192;i++)assert(m.Get(d[12],d[5]+i*4)==0);
 }
 printf("PASS token0 prefix layer%u/position0/token0/slot0/raw current Conv+GDN zero before RUN;NOT native initialization authority\n",m.outer[3]);
}
struct SavedHistory {
 unsigned route,address;std::vector<uint32_t> words;
 explicit SavedHistory(const RecurrentLifecycleMachine& m):route(m.outer[8]),address(0),words(H::Bytes/4){
  assert(H::Address(m.outer[9],m.outer[3],address));
  for(unsigned i=0;i<words.size();i++)words[i]=m.Get(route,address+i*4);
 }
 void Verify(const RecurrentLifecycleMachine& m)const{
  for(unsigned i=0;i<words.size();i++)assert(m.Get(route,address+i*4)==words[i]);
 }
};
int main(int argc,char** argv){
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;(void)&Refusals;(void)&LateRefusals;
 auto m=std::make_unique<RecurrentLifecycleMachine>(argv[1],argv[2],true,true);assert(m->WaitBlock(0));
 VerifyZeroCurrent(*m);
 m->memory[RB::Doorbell]=1;uint32_t first=0;assert(H::Address(m->outer[9],0,first));
 m->BeforeSubmit(14,m->outer[9]+TT_Q38_CONV_NEXT,false);
 HistoryFault(*m,"unknown first history read",[](RecurrentLifecycleMachine& q){q.failPacket=q.packets+1;});
 HistoryFault(*m,"counter discontinuity at first read",[](RecurrentLifecycleMachine& q){q.counterFaultPacket=q.packets+1;});
 HistoryFault(*m,"source discontinuity at first read",[](RecurrentLifecycleMachine& q){q.sourceFaultPacket=q.packets+1;});
 m->BeforeSubmit(14,first,true);
 HistoryFault(*m,"unknown first staging write",[](RecurrentLifecycleMachine& q){q.failPacket=q.packets+1;});
 const unsigned tail=(H::Bytes-1)/16384*16384;m->BeforeSubmit(14,first+tail,true);
 HistoryFault(*m,"unknown final staging write",[](RecurrentLifecycleMachine& q){q.failPacket=q.packets+1;});
 HistoryFault(*m,"counter discontinuity at final write",[](RecurrentLifecycleMachine& q){q.counterFaultPacket=q.packets+1;});
 HistoryFault(*m,"source discontinuity at final write",[](RecurrentLifecycleMachine& q){q.sourceFaultPacket=q.packets+1;});
 HistoryFault(*m,"changed original hidden before staging closes",[](RecurrentLifecycleMachine& q){q.memory[RB::Original]^=1;});
 assert(m->WaitBlock(1));m->VerifyBlock();VerifyStaged(*m);
 std::vector<SavedHistory> saved;saved.emplace_back(*m);
 for(unsigned layer=1;layer<3;layer++){
  m->StageNext(layer,0,0,0,0,true);VerifyZeroCurrent(*m);
  assert(m->WaitBlock(m->request));m->VerifyBlock();VerifyStaged(*m);
  for(const auto& previous:saved)previous.Verify(*m);
  saved.emplace_back(*m);
 }
 m->StageStop();assert(m->WaitStop());m->VerifyStop();
 puts("PASS history-staging ROOT/3actual complete blocks0->1->2/SAME token0+position0+slot0/SAME119/zero raw current states/8actual reached-phase faults/earlier histories survive scratch reuse/realSTOP;SYNTHETIC learned+initial hidden,NOT original/native/physical/current-slot/token commit/fullmodel/TPS");
}
