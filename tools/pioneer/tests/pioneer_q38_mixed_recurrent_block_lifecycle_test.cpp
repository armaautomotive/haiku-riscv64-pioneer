/* SPDX-License-Identifier: MIT */
// Retained REAL instruction state across changed complete recurrent blocks
// and terminal STOP. Never replace worker PCs/registers/nonces/Tensix state.
#define PIONEER_Q38_MIXED_RECURRENT_BLOCK_MACHINE_ONLY
#include "pioneer_q38_mixed_recurrent_block_root_test.cpp"
struct RecurrentLifecycleMachine:RecurrentBlockMachine {
 unsigned request=1,runs=0;
 using RecurrentBlockMachine::RecurrentBlockMachine;
 void StageNext(unsigned layer,unsigned position,unsigned bank,unsigned slot){
  VerifyBlock();runs++;unsigned fromBank=outer[7],from=outer[9]+RB::FinalOffset;
  std::array<uint32_t,5120> previous;
  for(unsigned i=0;i<5120;i++)previous[i]=Get(PioneerQ38ProjectionRoute(fromBank),from+i*4);
  std::array<uint32_t,1920> sources;
  for(unsigned i=0;i<16;i++)sources[i]=source[i];
  for(unsigned w=0;w<119;w++){
   assert(!child[w]->active && child[w]->GdnFinished(child[w]->sequence));initial[w]=child[w]->sequence;
   for(unsigned i=0;i<16;i++)sources[(w+1)*16+i]=child[w]->source[i];
   for(unsigned i=0;i<8;i++)peerCounterBaseline[w][i]=child[w]->Count(i);
  }
  // These are OBSERVER counters,not processor state. Only caller-owned
  // command/metadata and primitive learned/prior-state endpoints change.
  matrixStarts=matrixChecks=observed=finalWords=initialNorm=postNorm=activationChecks=gatedChecks=numericCohortChecks=0;
  convStarts=gdnStarts=rootConvPackets=0;cleared.fill(false);verified.fill(false);weights.clear();
  request++;block=std::make_unique<PioneerQ38MixedRecurrentBlockPlan>();
  assert(block->Build(*canonical,layer,position,248319,bank,slot,1001+request*16,request,{1,2,3,4},sources.data(),fromBank,from)
   && block->Matches(*canonical));
  for(unsigned i=0;i<128;i++)memory[RB::Outer+i*4]=outer[i]=block->Outer()[i];
  for(unsigned n=0;n<8;n++){
   const auto* r=block->Recipe(n);for(unsigned i=0;i<1280;i++)Put(outer[8],outer[57]+n*5120+i*4,r[i]);
   for(unsigned w=0;w<r[7];w++){
    const auto* p=r+80+w*8;unsigned bytes=p[2]*(r[14]/(r[15]==8?32:256))*(r[15]==8?576:2304);
    Weight span={p[4],p[0],bytes,r[15],r[14],r[1032+w]};assert(weights.emplace(Key(span.route,span.base),span).second);
   }
   for(unsigned i=0;i<r[11];i++)Put(outer[8],outer[9]+r[10]+i*4,0x7fc00000);
  }
  for(unsigned i=0;i<RC::ConvWords;i++)Put(outer[8],outer[58]+i*4,block->Conv()[i]);
  for(unsigned i=0;i<RC::GdnWords;i++)Put(outer[8],outer[59]+i*4,block->Gdn()[i]);
  SeedRaw(false);
  for(unsigned i=0;i<5120;i++)assert(Get(PioneerQ38ProjectionRoute(fromBank),from+i*4)==previous[i]);
  memory[RB::Doorbell]=request;
 }
 void StageStop(){
  std::array<uint32_t,128> stop={};stop[0]=RB::Magic;stop[1]=1;stop[2]=128;stop[5]=129;
  stop[48]=++request;stop[49]=119;stop[72]=RB::Stop;
  for(unsigned i=0;i<16;i++)stop[16+i]=boot[16+i];for(unsigned i=0;i<8;i++)stop[32+i]=boot[8+i];
  assert(RB::StopShape(stop.data()));for(unsigned i=0;i<128;i++)memory[RB::Outer+i*4]=stop[i];
  memory[RB::Doorbell]=request;
 }
 bool WaitStop(){
  while(pc!=0x4009c && pc!=0x40060)Step();return pc==0x40060;
 }
 void VerifyStop(){
  assert(pc==0x40060 && !unknown && !pending && stopJobs==119 && stopStarts==119 && Phase()==15
   && memory.at(RB::StopSummary)==RB::StopMagic && memory.at(RB::StopSummary+4)==request
   && memory.at(RB::StopSummary+8)==request-1 && memory.at(RB::Status+4)==request && memory.at(RB::Status+16)==2);
  for(unsigned i=0;i<8;i++)assert(memory.at(RB::StopSummary+(8+i)*4)==counts[i] && memory.at(RB::StopSummary+(26+i)*4)==boot[8+i]);
  for(unsigned i=0;i<10;i++)assert(memory.at(RB::StopSummary+(16+i)*4)==command[i]);
  for(unsigned i=0;i<16;i++)assert(memory.at(RB::StopSummary+(34+i)*4)==source[i]);
  for(unsigned w=0;w<119;w++){
   auto& p=*child[w];assert(p.pc==p.stopPC && !p.pending && p.drained && !p.memory.at(M::Status+8)
    && p.memory.at(M::Status+16)==M::Stop && p.sequence==initial[w]+PioneerQ38MixedRecurrentBlockProjectionLayout::RunDelta(w)+1
    && p.memory.at(M::Status+4)==p.sequence && memory.at(RB::Nonces+w*4)==p.sequence);
  }
  unsigned before=packets,writes=peerWrites;auto physical=counts;auto commands=command;
  memory[RB::Doorbell]=request+1;
  for(unsigned i=0;i<2048;i++){StepScalar();for(auto& p:child)p->StepBlock();}
  assert(pc==0x40060 && packets==before && peerWrites==writes && counts==physical && command==commands);
  for(const auto& p:child)assert(p->pc==p->stopPC);
 }
};
template<class Change> static void StopFault(RecurrentLifecycleMachine& base,const char* name,Change change,bool noIO){
 fflush(nullptr);pid_t pid=fork();assert(pid>=0);
 if(!pid){
  unsigned start=base.packets;change(base);assert(!base.WaitStop() && base.unknown && base.pc==0x4009c
   && base.memory.at(RB::StopSummary)!=RB::StopMagic && base.memory.at(RB::Status+4)!=base.request);
  if(noIO)assert(base.packets==start);
  unsigned packets=base.packets,writes=base.peerWrites,stops=base.stopStarts;auto counts=base.counts;auto command=base.command;
  base.memory[RB::Doorbell]=base.request+1;for(unsigned i=0;i<2048;i++)base.StepScalar();
  assert(base.pc==0x4009c && base.packets==packets && base.peerWrites==writes && base.stopStarts==stops
   && base.counts==counts && base.command==command && base.memory.at(RB::StopSummary)!=RB::StopMagic);
  printf("PASS QRB1 STOP refusal %s/inert2048/no later packet,reset,terminal receipt or disposal grant\n",name);_exit(0);
 }
 int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));
}
int main(int argc,char** argv){
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;(void)&Refusals;(void)&LateRefusals;
 puts("START QRB1 virgin119/3actual complete recurrent blocks/changed layer+position+bank+slot/previous ACTUAL hidden/realSTOP;synthetic primitive bytes,NOT fullmodel/native/hardware");
 auto m=std::make_unique<RecurrentLifecycleMachine>(argv[1],argv[2],true);assert(m->WaitBlock(0));
 for(const auto& p:m->child)assert(!p->sequence && p->GdnFinished(0));
 StopFault(*m,"STOP before any completed layer",[](RecurrentLifecycleMachine& q){q.StageStop();},true);
 unsigned refused=1;
 m->memory[RB::Doorbell]=1;assert(m->WaitBlock(1));m->VerifyBlock();
 puts("PASS QRB1 FIRST actual full block/virgin small alpha+beta THEN Z/all119;NO fabricated receipt or initial matrix");
 for(auto next:{std::array<unsigned,4>{1,0,0,1},std::array<unsigned,4>{62,8191,6,1}}){
  m->StageNext(next[0],next[1],next[2],next[3]);assert(m->WaitBlock(m->request));m->VerifyBlock();
  printf("PASS QRB1 retained119 block%u/layer%u/position%u/bank%u/slot%u/actual preceding final hidden;NO processor reset or product seeding\n",m->request,next[0],next[1],next[2],next[3]);
 }
 m->StageStop();
 for(unsigned i=0;i<128;i++){
  StopFault(*m,"changed STOP command",[=](RecurrentLifecycleMachine& q){q.memory[RB::Outer+i*4]^=0x80000000U;},true);refused++;
 }
 m->BeforeSubmit(15,0xffb121b0,false);
 StopFault(*m,"unknown pre-STOP audit",[](RecurrentLifecycleMachine& q){q.failPacket=q.packets+1;},false);refused++;
 StopFault(*m,"held peer before first STOP write",[](RecurrentLifecycleMachine& q){q.badResetWorker=119;},false);refused++;
 StopFault(*m,"counter changed in audit",[](RecurrentLifecycleMachine& q){q.counterFaultPacket=q.packets+1;},false);refused++;
 m->BeforeSubmit(15,M::Job,true);assert(!m->stopJobs && !m->stopStarts);
 StopFault(*m,"unknown first STOP metadata write",[](RecurrentLifecycleMachine& q){q.failPacket=q.packets+1;},false);refused++;
 StopFault(*m,"source changed at first STOP write",[](RecurrentLifecycleMachine& q){q.sourceFaultPacket=q.packets+1;},false);refused++;
 m->BeforeSubmit(15,M::Doorbell,true);assert(m->stopJobs==119 && !m->stopStarts);
 StopFault(*m,"unknown first STOP release",[](RecurrentLifecycleMachine& q){q.failPacket=q.packets+1;},false);refused++;
 m->BeforeSubmit(15,M::Status,false);assert(m->stopStarts==119);
 StopFault(*m,"unknown terminal-status read",[](RecurrentLifecycleMachine& q){q.failPacket=q.packets+1;},false);refused++;
 StopFault(*m,"late peer boot change",[](RecurrentLifecycleMachine& q){q.child[118]->memory[M::Boot+80]^=1;},false);refused++;
 StopFault(*m,"late original-hidden change",[](RecurrentLifecycleMachine& q){q.memory[RB::Original]^=1;},false);refused++;
 assert(m->WaitStop());m->VerifyStop();
 printf("PASS QRB1 3complete blocks/2160matrix jobs+1920Conv+288GDN/real119STOP+ROOT/inert2048/refusals%u/ROOTstack%u;NO state-token commit/native/physical/TPS\n",refused,0x70000-m->minStack);
}
