/* SPDX-License-Identifier: MIT */
// Actual QGW1 frontend guards, not a fabricated all64 completion/producer.
#define PIONEER_Q38_MIXED_WORKER_MACHINE_ONLY
#include "pioneer_q38_mixed_worker_test.cpp"
#include "../PioneerQ38GlobalWorkerLayout.h"
#include <sys/wait.h>
#include <unistd.h>
namespace G=PioneerQ38GlobalWorkerLayout;
struct GlobalInitialPeer:MixedPeer {
 GlobalInitialPeer(const char* old,const char* image):MixedPeer(old,1,12,17408,1){
  assert(!steps && pc==0x6000); // replace immutable initial ROM BEFORE first boot ONLY
  code=Read((std::string(image)+"/text.bin").c_str());front=Read((std::string(image)+"/globalworker.bin").c_str());
  dense=Read((std::string(image)+"/dense.bin").c_str());helper=Read((std::string(image)+"/ffn.bin").c_str());
  affineCode=Read((std::string(image)+"/affine.bin").c_str());service=Read((std::string(image)+"/service.bin").c_str());gdnCode=Read((std::string(image)+"/gdn.bin").c_str());
  for(unsigned i=0;i<2048;i++)l1[2048+i]=i*4<code.size()?Word(code,i*4):0;
  l1[0]=Word(Read((std::string(image)+"/boot.bin").c_str()),0);
  std::ifstream symbols(std::string(image)+"/program.symbols");std::string address,kind,name;waitPC=stopPC=faultPC=0;
  while(symbols>>address>>kind>>name){unsigned a=unsigned(std::stoul(address,nullptr,16));if(name=="block_peer_wait_load")waitPC=a;if(name=="block_peer_stopped")stopPC=a;if(name=="block_fault_stopped")faultPC=a;}
  assert(waitPC && stopPC && faultPC);boot[0]=G::Magic;for(unsigned i=40;i<46;i++)boot[i]=G::BootWord(i);
  assert(G::BootShape(boot.data()) && !M::BootShape(boot.data()));for(unsigned i=0;i<64;i++)memory[M::Boot+i*4]=boot[i];
 }
 void BootGlobal(){
  PoisonDst();Poison();while(pc!=waitPC && pc!=faultPC)StepBlock();
  assert(pc==waitPC && !sequence && !requests && !responses && !pending && memory.at(M::Status+12)==G::Magic && !memory.at(M::Status+8));
 }
 std::array<uint32_t,64> Command(bool global=true)const{
  std::array<uint32_t,64> j={};const unsigned h[]={G::Magic,1,global?unsigned(G::GlobalMatrix):1U,1,selfRoute,global?64U:0U,0,2025};
  for(unsigned i=0;i<8;i++){j[i]=h[i];j[8+i]=boot[8+i];}
  if(global){
   const unsigned m[]={0x31483851,14,3392,0,0,5120,0x04000000,1,PioneerQ38ProjectionRoute(0),1};
   for(unsigned i=0;i<10;i++)j[16+i]=m[i];j[27]=0;j[28]=0;j[29]=248320;j[30]=1;j[31]=G::JobWitness;
  }else for(unsigned i=0;i<10;i++)j[16+i]=matrixJob[i];
  j[26]=1;assert(G::JobShape(j.data(),boot.data(),0,0));return j;
 }
 void Fault(const std::array<uint32_t,64>& j){
  for(unsigned i=0;i<64;i++)memory[M::Job+i*4]=j[i];memory[M::Doorbell]=j[1];
  while(pc!=faultPC && pc!=stopPC)StepBlock();
  assert(pc==faultPC && memory.at(M::Status+8)==G::Fault && !memory.at(M::Status+4) && !requests && !responses && !matrixInstructions && !densePackets && !pending);
  for(unsigned i=0;i<2048;i++)StepBlock();assert(pc==faultPC && !requests && !responses && !matrixInstructions);
 }
};
int main(int argc,char** argv){
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;
 auto m=std::make_unique<GlobalInitialPeer>(argv[1],argv[2]);m->BootGlobal();
 for(unsigned bad=0;bad<15;bad++){
  fflush(nullptr);pid_t pid=fork();assert(pid>=0);
  if(!pid){
   auto j=m->Command();
   if(bad==1){j=m->Command(false);j[5]=63;}
   if(bad==2)j[32]=64;if(bad==3)j[27]=1;if(bad==4)j[1]=2;if(bad==5)m->source[11]^=1;
   if(bad==6)m->memory[M::Boot+40*4]^=1;if(bad==7)j[0]=M::Magic;
   if(bad==8){for(unsigned i=16;i<64;i++)j[i]=0;j[2]=G::Stop;}
   if(bad==9)m->memory[M::Boot]=M::Magic;if(bad==10){j=m->Command(false);j[6]=1;}
   if(bad==11)j[23]=17;if(bad==12)j[17]=12;if(bad==13)j[63]=1;if(bad==14)j[4]^=1;
   m->Fault(j);_exit(0);
  }
  int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));
 }
 puts("PASS ACTUAL QGW1 virgin boot/private frontier rejects15 premature global+globalSTOP/fakecompleted/layer63 shortcut/position jump/oldABI/source+boot+job faults/ZERO NoC or matrix products/inert2048;NO positive arithmetic,all64 producer,output ROOT,native,physical,Work,token or TPS proof");
}
