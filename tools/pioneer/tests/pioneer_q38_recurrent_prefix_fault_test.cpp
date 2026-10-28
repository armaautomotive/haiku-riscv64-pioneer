/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_RECURRENT_PREFIX_FIXTURE_ONLY
#include "pioneer_q38_recurrent_prefix_root_test.cpp"
template<class Change>static void PrefixFault(AutonomousPrefixMachine& base,const char* name,Change change,bool noIO=false){
 fflush(nullptr);pid_t child=fork();assert(child>=0);
 if(!child){
  const unsigned completed=base.completedBlocks,releases=base.selfReleases,outerWrites=base.selfOuterWrites,packetsBefore=base.packets;
  assert(completed==1 && releases<=1 && !base.stopStarts);
  change(base);assert(!base.WaitStop() && base.unknown && base.pc==0x4009c
   && base.completedBlocks==completed && base.selfReleases==releases && base.selfOuterWrites==outerWrites
   && base.memory.at(RB::Status+4)==1 && !base.stopStarts);
  if(noIO)assert(base.packets==packetsBefore);
  else assert(base.packets==packetsBefore+1); // genuinely reached one uncertain/read/header packet
  const unsigned packets=base.packets,writes=base.peerWrites;auto counts=base.counts;auto command=base.command;
  base.memory[RB::Doorbell]=base.request+1;
  for(unsigned i=0;i<2048;i++)base.StepScalar();
  assert(base.pc==0x4009c && base.packets==packets && base.peerWrites==writes && base.counts==counts && base.command==command
   && base.completedBlocks==completed && base.selfReleases==releases && base.selfOuterWrites==outerWrites && !base.stopStarts);
  for(const auto& saved:base.saved)saved.Verify(base);
  printf("PASS reached autonomous prefix refusal %s/completed%u/releases%u/inert2048/earlier state retained/NO later IO,release,STOP,retry or commit\n",name,completed,releases);
  fflush(nullptr);_exit(0);
 }
 int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));
}
static unsigned TransferFaults(AutonomousPrefixMachine& m,const char* phase,bool includeOuter=false){
 unsigned cases=0;
 PrefixFault(m,phase,[](AutonomousPrefixMachine& q){q.failPacket=q.packets+1;});cases++;
 PrefixFault(m,"counter discontinuity",[](AutonomousPrefixMachine& q){q.counterFaultPacket=q.packets+1;});cases++;
 PrefixFault(m,"source discontinuity",[](AutonomousPrefixMachine& q){q.sourceFaultPacket=q.packets+1;});cases++;
 PrefixFault(m,"command readback discontinuity",[](AutonomousPrefixMachine& q){q.commandFaultPacket=q.packets+1;});cases++;
 if(includeOuter){
  PrefixFault(m,"outer discontinuity",[](AutonomousPrefixMachine& q){q.outerFaultPacket=q.packets+1;});cases++;
  PrefixFault(m,"boot discontinuity",[](AutonomousPrefixMachine& q){q.rootBootFaultPacket=q.packets+1;});cases++;
 }
 return cases;
}
int main(int argc,char** argv){
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;(void)&Refusals;(void)&LateRefusals;
 auto m=std::make_unique<AutonomousPrefixMachine>(argv[1],argv[2]);assert(m->WaitBlock(0));
 m->memory[RB::Doorbell]=1; // no host writes in the positive continuation after this release
 while(m->Phase()!=16){assert(m->pc!=0x4009c);m->Step();}
 assert(m->completedBlocks==1 && !m->selfReleases && !m->selfOuterWrites);
 unsigned cases=0;
 // This is before Phase's actual source seal, not a forged PC/readiness facade.
 PrefixFault(*m,"changed prefix boot before transition IO",[](AutonomousPrefixMachine& q){q.memory[RB::Boot+40*4]^=1;},true);cases++;
 PrefixFault(*m,"changed old outer before transition IO",[](AutonomousPrefixMachine& q){q.memory[RB::Outer+16*4]^=1;},true);cases++;
 uint32_t next=0;assert(XP::Address(m->outer[9],1,next));
 m->BeforeSubmit(16,next,false);
 cases+=TransferFaults(*m,"unknown next header read",true);
 for(unsigned word:{0U,3U,4U,6U,7U,9U,12U,13U,16U,32U,48U,57U,68U,69U,72U}){
  PrefixFault(*m,"malformed next-layer header",[=](AutonomousPrefixMachine& q){q.Put(q.outer[8],next+word*4,q.Get(q.outer[8],next+word*4)^1U);});cases++;
 }
 // Advance only through genuine emitted instructions to the next layer's
 // first metadata read/write and final metadata write. No host restaging.
 m->BeforeSubmit(1,next+XP::RecipesOffset,false);
 assert(m->completedBlocks==1 && m->selfReleases==1 && m->selfOuterWrites==128 && !m->matrixStarts);
 cases+=TransferFaults(*m,"unknown first metadata read",true);
 m->BeforeSubmit(1,m->outer[57],true);
 cases+=TransferFaults(*m,"unknown first metadata write");
 const unsigned tail=(XP::GdnBytes-1)/16384*16384;
 m->BeforeSubmit(1,m->outer[59]+tail,true);
 cases+=TransferFaults(*m,"unknown final metadata write",true);
 assert(cases==39 && m->WaitStop());m->VerifyStop();
 assert(m->completedBlocks==3 && m->selfReleases==3 && m->selfOuterWrites==384
  && m->metadataReadBytes==3*(XP::Bytes-512) && m->metadataWriteBytes==m->metadataReadBytes && m->headerReadBytes==5*512);
 for(const auto& saved:m->saved)saved.Verify(*m);
 printf("PASS autonomous prefix/actual3blocks+realSTOP/reachedfaults%u/ONE host start/SAME119/ROOT releases3/earlier Conv+GDN retained;SYNTHETIC raw bytes,NOT original/native/physical/token/full64/Work/TPS\n",cases);
}
