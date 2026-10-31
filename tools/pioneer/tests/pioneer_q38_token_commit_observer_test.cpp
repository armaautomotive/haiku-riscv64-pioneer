/* SPDX-License-Identifier: MIT */
// Actual extracted HOST observer methods over SYNTHETIC metadata ONLY.
// No processor,model byte fabric,native owner or hardware is instantiated.
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <map>
#include <sys/wait.h>
#include <unistd.h>
namespace RB {enum {Scratch=0x80000,Doorbell=0x75400,Status=0x75440};}
namespace CL {static bool Dense(unsigned){assert(false);return false;}}
struct GlobalChainInstructionMachine {
 std::map<uint32_t,uint32_t> memory;unsigned writes=0;
 virtual ~GlobalChainInstructionMachine()=default;
 virtual void Store(uint32_t a,uint32_t v){memory[a]=v;writes++;}
};
struct Observer:GlobalChainInstructionMachine {
 unsigned parent=0x6f000,position=0,flips=0,completed=64,globalJobs=973,globalPartials=973;
 unsigned copyLayer=64,copyPhase=0,stopJobs=0,layer=0;bool pending=false;
 std::array<uint32_t,128> outer={};
 unsigned Private(unsigned i)const{return parent+(192+i)*4;}
 unsigned ActualPrivate(unsigned i)const{return memory.at(Private(i));}
 explicit Observer(unsigned p):position(p),flips(p){memory[Private(18)]=p;memory[Private(19)]=p&1;}
 void SubmitToken(){assert(false);}void StartLayer(unsigned){assert(false);}void VerifyTokenLayer(){assert(false);}
 void ObservePrepared(){assert(false);}void ObserveAttention(){assert(false);}
#include "actual_token_observer_store.inc"
 bool CanPublish()const{
  const unsigned first=0x100000,remote=first+position*256,local=RB::Scratch;
#include "actual_token_observer_publication.inc"
 }
};
int main(){
 unsigned positives=0,refusals=0;
 for(unsigned pos=0;pos<3;pos++)for(bool slotFirst:{false,true}){
  Observer o(pos);assert(!o.CanPublish());
  o.Store(o.Private(slotFirst?19:18),slotFirst?((pos+1)&1):pos+1);assert(!o.CanPublish());
  o.Store(o.Private(slotFirst?18:19),slotFirst?pos+1:((pos+1)&1));
  assert(o.CanPublish() && o.flips==pos+1 && o.writes==2 && o.ActualPrivate(18)==pos+1 && o.ActualPrivate(19)==((pos+1)&1));positives++;
 }
 for(unsigned pos=0;pos<3;pos++)for(bool slot:{false,true})for(unsigned fault=0;fault<8;fault++){
  fflush(nullptr);const pid_t child=fork();assert(child>=0);
  if(!child){
   Observer o(pos);uint32_t value=slot?((pos+1)&1):pos+1;
   if(fault==0)o.completed=63;if(fault==1)o.globalJobs=972;if(fault==2)o.globalPartials=972;
   if(fault==3)o.copyLayer=63;if(fault==4)o.copyPhase=1;if(fault==5)o.stopJobs=1;
   if(fault==6)o.flips=pos+2;if(fault==7)value^=4;
   o.Store(o.Private(slot?19:18),value);_exit(0);
  }
  int status=0;assert(waitpid(child,&status,0)==child && WIFSIGNALED(status) && WTERMSIG(status)==SIGABRT);refusals++;
 }
 printf("PASS actual extracted HOST commit observer/%u both-order positives/%u early+wrong-value+duplicate-frontier+unfinished-copy refusals/actual publication predicate requires BOTH private words and one slot flip;SYNTHETIC metadata only,NO model/native/physical/Work/TPS proof\n",positives,refusals);
}
