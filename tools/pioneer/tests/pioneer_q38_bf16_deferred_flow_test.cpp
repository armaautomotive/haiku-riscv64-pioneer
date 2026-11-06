/* SPDX-License-Identifier: MIT */
// Actual extracted Run/BfPrepare control flow. ALL protocol/IO/closures and
// model inputs below are MOCK. Not compiled firmware/native/numerical proof.
#include "../PioneerQ38Bf16DeferredAudit.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include <stdexcept>
#include <vector>
namespace L {enum {Magic=0x314b434d,Summary=0x10000,Scratch=0x12000,Counters=0x20000,Nonces=0x22000,
 PeerJobs=0x25000,Projected=0x40000,Native=0xa0000,NativeShadow=0xc0000,InputShadow=0xe0000};}
namespace BFP {enum {ProjectionInput=0xe0000,Descriptor=0x74200,NativeOffset=0x20000};}
namespace M {enum {Job=0x74000,Doorbell=0x74100};}
namespace PioneerQ38Bf16ProfileData {enum {MatrixPrepare=10,MatrixDispatch=11,MatrixGatherWait=12,MatrixFinish=13};}
static std::array<uint32_t,0x180000/4> memory;
static std::array<uint32_t,256> mmio;
static std::array<bool,119> closed;
static std::array<unsigned,119> idleBits,idleReads;
static unsigned phase,events,failAt,scatters,preWaits,gathers,finalWaits,used;
static bool faulted;
static unsigned latentWorker=UINT32_MAX;static bool latentRaised;
struct Fault{};
static void Step(){assert(!faulted);events++;if(events==failAt){faulted=true;throw Fault{};}}
[[noreturn]] static void scalar_fault_park(){faulted=true;throw Fault{};}
struct PioneerQ38Bf16ProfileScope {
 explicit PioneerQ38Bf16ProfileScope(unsigned p){phase=p;Step();}
 void Next(unsigned p){phase=p;Step();}
};
static volatile uint32_t* Words(unsigned at){
 assert(!(at&3));
 if(at>=0xffb20000U){assert(at-0xffb20000U<mmio.size()*4);return mmio.data()+(at-0xffb20000U)/4;}
 assert(at<memory.size()*4);return memory.data()+at/4;
}
static void Copy(uintptr_t to,uintptr_t from,unsigned words){
 auto* a=to>UINT32_MAX?reinterpret_cast<volatile uint32_t*>(to):Words(unsigned(to));
 auto* b=from>UINT32_MAX?reinterpret_cast<volatile uint32_t*>(from):Words(unsigned(from));
 for(unsigned i=0;i<words;i++)a[i]=b[i];
}
static unsigned Counter(unsigned i){assert(i<8);return i==0?0x208:i==1?0x20c:(i==2 || i==6)?0x210:i==3?0x214:i==4?0x204:i==5?0x220:0x228;}
static bool BfQ4(const uint32_t* d){return d[15]==12 && d[43]>=2 && d[43]<=4;}
static void CaptureRecipe(const uint32_t*){Step();}
static void Seal(const uint32_t*){Step();}
static void BfBuildPairs(const uint32_t*){Step();}
static void Admit(const uint32_t*,uint32_t*,unsigned w){assert(w<119);Step();}
static void BfDescriptor(const uint32_t*,unsigned w,uint32_t* d){assert(w<68);Step();for(unsigned i=0;i<64;i++)d[i]=0;}
static void BfProducerJob(const uint32_t*,unsigned w,unsigned n,uint32_t* j){assert(w<68 && n);Step();for(unsigned i=0;i<64;i++)j[i]=0;}
static void Transfer(const uint32_t*,uint32_t*,unsigned w,unsigned,unsigned bytes,bool){assert(w<119 && (bytes==256 || bytes==4));Step();}
static void BfGatherProducer(const uint32_t*,uint32_t*,unsigned w){assert(w<68);Step();if(!w && latentWorker<119)latentRaised=true;}
static void Wait(const uint32_t*,uint32_t*,unsigned w,unsigned p){
 assert(w<119 && !p);Step();if(latentRaised && w==latentWorker)scalar_fault_park();
 if(phase==10)preWaits++;else{assert(phase==13);finalWaits++;idleBits[w]|=1;}
}
static void BootSeal(const uint32_t*,uint32_t*,unsigned w){assert(w<119);Step();if(phase==13)idleBits[w]|=2;}
static void ClosedJob(const uint32_t*,uint32_t*,unsigned w,bool p){assert(w<119 && !p);Step();if(phase==13)idleBits[w]|=4;}
static unsigned Register(const uint32_t*,uint32_t*,unsigned w,unsigned at){
 assert(w<119 && at>=0xffb20200 && at<=0xffb20228);Step();
 if(phase==13 && ++idleReads[w]==8){assert(idleBits[w]==7);closed[w]=true;}
 return 0;
}
static void BfBankRead(const uint32_t*,uint32_t*,unsigned,unsigned bytes){Step();for(unsigned i=0;i<bytes/4;i++)Words(L::Scratch)[i]=0;}
static void Quantize(const uint32_t*){assert(false);}
static void MatrixJob(const uint32_t*,unsigned w,unsigned n){assert(w<used && n);Step();}
static void Gather(const uint32_t*,uint32_t*,unsigned w){assert(phase==12 && w<used);Step();
 if(latentRaised && w==latentWorker)scalar_fault_park();closed[w]=true;gathers++;}
static void Scatter(const uint32_t*,uint32_t*){Step();for(bool done:closed)assert(done);scatters++;}
static void InputsSeal(const uint32_t*){Step();}
static void FrameSeal(unsigned w){assert(w<119);Step();}
static void Fence(){Step();}
#define BF_ROOT_FN
#include "actual_bf_prepare.inc"
#include "actual_projection_run.inc"
static void Reset(unsigned count,unsigned fail){
 memory.fill(0);mmio.fill(0);closed.fill(false);idleBits.fill(0);idleReads.fill(0);
 phase=events=scatters=preWaits=gathers=finalWaits=0;used=count;failAt=fail;faulted=latentRaised=false;
}
static bool Trial(unsigned width,unsigned count,unsigned fail){
 Reset(count,fail);uint32_t d[64]={};d[7]=count;d[11]=count*32;d[14]=width;d[15]=12;d[43]=width==17408?4:2;d[53]=0x100000;
 try{
  assert(Run(d)==d[11]);assert(!faulted && scatters==1 && gathers==used && finalWaits==119-used);
  for(bool done:closed)assert(done);
#ifdef PIONEER_Q38_BF16_DEFERRED_PREPARE_AUDIT_IMAGE
  assert(!preWaits);
#else
  assert(preWaits==119-width/256);
#endif
  return true;
 }catch(const Fault&){
  assert(faulted);return false;
 }
}
int main(){
 unsigned cases=0,faults=0;
 for(unsigned width:{5120U,17408U})for(unsigned count:{1U,19U,20U,21U,67U,68U,69U,118U,119U}){
#ifdef PIONEER_TEST_MISSING_MARK
  assert(!Trial(width,count,0) && !scatters);cases++;
#else
  assert(Trial(width,count,0));const unsigned total=events;
  std::printf("FLOW width%u consumers%u callbacks%u prepIdleAudits%u finalIdleAudits%u finalMatrixClosures%u\n",width,count,total,preWaits,finalWaits,gathers);
  for(unsigned event=1;event<=total;event++){
   assert(!Trial(width,count,event) && events==event);faults++;
  }
  cases++;
#endif
 }
#ifndef PIONEER_TEST_MISSING_MARK
 unsigned latent=0;
 for(unsigned width:{5120U,17408U})for(unsigned count:{19U,119U}){
  latentWorker=width/256;assert(!Trial(width,count,0) && !scatters);
#ifdef PIONEER_Q38_BF16_DEFERRED_PREPARE_AUDIT_IMAGE
  assert(phase==(latentWorker<count?12U:13U));
#else
  assert(phase==10);
#endif
  latent++;
 }
 std::printf("LATENT_MOCK_IDLE_FAULTS%u:old detection prepare,new detection Gather/finish,both before Scatter;NOT actual hardware fault proof\n",latent);
#endif
 std::printf("PASS extracted controller flow cases%u every-callback-faults%u;all119 final closures before Scatter,missing ledger marks blocked;ALL IO/protocol/closures MOCK,NOT native,numerics,hardware or TPS\n",cases,faults);
}
