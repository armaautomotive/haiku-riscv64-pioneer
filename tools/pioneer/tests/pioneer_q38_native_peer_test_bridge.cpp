/* SPDX-License-Identifier: MIT */
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(PIONEER_Q38_TOKEN_PUSHED_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_PRECISE_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION) || defined(PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION)
#error Offline peer implementation may never compile into native/Work code
#endif
#define PIONEER_Q38_NATIVE_PEER_TEST_BRIDGE 1
#define PIONEER_Q38_TEST_SHARED_WORKER_STORAGE 1
#include "PioneerQ38SharedTestStorage.h"
// Include standard/system declarations OUTSIDE the isolated model namespace.
// The same generic ISA classes also exist in the native test's other TU;
// distinct namespace prevents conflicting layouts/vtables/ODR definitions.
#include <algorithm>
#include <array>
#include <cassert>
#include <cerrno>
#include <climits>
#include <cmath>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <limits>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include <sys/wait.h>
#include <unistd.h>
namespace NativeQ38PeerInstructionModel {
#define PIONEER_Q38_GLOBAL_CHAIN_MACHINE_ONLY
#include "pioneer_q38_global_chain_instruction_test.cpp"
#include "../PioneerQ38TokenSelectedPrograms.h"
struct NativePeer:GlobalChainPeer,CohortBytes {
 PioneerQ38SharedTestBinding& binding;unsigned deviceReads=0,deviceWrites=0,statusWrites=0,scalarFpInstructions=0;
 uint32_t publishedCounts[8]={},publishedCommands[10]={};bool publishedPending=false;
 bool endpointInputsObserved=false;
 NativePeer(const char* old,const char* global,unsigned worker,PioneerQ38SharedTestBinding& b)
  :GlobalChainPeer(old,global,worker),binding(b){
  (void)&RunPersistentGdnPair;(void)&Refusals;(void)&LateRefusals;
  assert(!steps && pc==0x6000 && !active && !sequence && !matrixInstructions && !requests && !responses);
  bytes=this;PoisonDst();Poison();
#ifdef PIONEER_Q38_TOKEN_PUSHED_PROGRAMS
  assert(binding.bytes.push);
  pushedContext=&binding;pushedBytes=[](void* context,unsigned worker,const uint32_t* packet,const uint32_t* raw){
   auto& b=*static_cast<PioneerQ38SharedTestBinding*>(context);
   assert(b.executing && worker==b.worker && b.bytes.push);b.bytes.push(b.bytes.context,worker,packet,raw);
  }; // SAME raw byte/CSR fabric,only after the actual sender's NoC instruction.
#endif
  // Never expose generic constructor mailboxes,operands or code as native
  // data. Actual held staging supplies boot/code/status before first ARM.
  memory.ForgetConstruction();buffers.ForgetConstruction();l1.ForgetConstruction();
 }
 uint32_t Get(unsigned route,unsigned address)const override{
  uint8_t raw[4];assert(binding.bytes.read(binding.bytes.context,route,address,raw,4));
  return uint32_t(raw[0])|uint32_t(raw[1])<<8|uint32_t(raw[2])<<16|uint32_t(raw[3])<<24;
 }
 void ReadRaw(unsigned route,unsigned address,uint8_t* out,unsigned count)const override{
  assert(binding.bytes.read(binding.bytes.context,route,address,out,count));
 }
 void Put(unsigned route,unsigned address,uint32_t value)override{assert(binding.bytes.put(binding.bytes.context,route,address,value));}
 uint64_t Clock()const override{return binding.bytes.clock(binding.bytes.context);}
 uint32_t RootWord(unsigned address)const override{return binding.bytes.word(binding.bytes.context,0,address);}
 bool CodeAddress(uint32_t address)const override{uint32_t word;return PioneerQ38TokenPrograms::ProgramWord(logicalWorker,address,word);}
 unsigned Index(uint32_t address)override{assert(!(address&3) && address<0x180000);return address/4;}
 unsigned StepLimit()const override{return 4000000000U;}
 void Publish(unsigned commandWrites=0){
  uint32_t delta[8];bool changed=commandWrites || pending!=publishedPending;
  for(unsigned i=0;i<8;i++){const uint32_t now=Count(i);delta[i]=now-publishedCounts[i];publishedCounts[i]=now;changed|=delta[i]!=0;}
  for(unsigned i=0;i<10;i++){if(command[i]!=publishedCommands[i])commandWrites|=1U<<i;publishedCommands[i]=command[i];}
  if(changed || commandWrites)binding.bytes.events(binding.bytes.context,logicalWorker,delta,command,commandWrites,pending!=publishedPending,pending);
  publishedPending=pending;
 } // actual event DELTAS/explicit writes ONLY;NEVER overwrite an unrelated physical CSR fault
 uint32_t Load(uint32_t address)override{
  deviceReads++;
  if(CodeAddress(address))return binding.Word(address);
  if(Range(address,0xffb20000,0x2000))for(unsigned i=0;i<16;i++)if(address-0xffb20000==BlackholeDramSourceOffset(i))
   return binding.bytes.noc(binding.bytes.context,logicalWorker,address-0xffb20000);
  const uint32_t value=GlobalChainPeer::Load(address);
  if(Range(address,0xffb20000,0x2000))Publish();
  return Range(address,0xffb20000,0x2000)?binding.bytes.noc(binding.bytes.context,logicalWorker,address-0xffb20000):value;
 }
 void Store(uint32_t address,uint32_t value)override{
  assert(!CodeAddress(address));GlobalChainPeer::Store(address,value);deviceWrites++;
  if(Range(address,G::Status,64))statusWrites++;
  if(Range(address,0xffb20000,0x2000))Publish(Range(address,0xffb20000,40)?1U<<((address-0xffb20000)/4):0U);
 }
 void StepBlock()override{
  uint32_t expected;assert(PioneerQ38TokenPrograms::ProgramWord(logicalWorker,pc,expected) && binding.Word(pc)==expected);
  const unsigned opcode=expected&127;if(opcode==7 || opcode==0x27 || opcode==0x53)scalarFpInstructions++;
  // Parent service FP decoders may use their immutable ROM byte vectors.
  // ALL instruction bytes must first equal this SAME installed L1 word.
  GlobalChainPeer::StepBlock();
  // Hardware events originate ONLY at actual NoC Load/Store endpoints
  // above. Re-scanning every completed packet after every ordinary ISA/
  // L1/tensor instruction is quadratic HOST work,not a hardware event.
  // Keep all instructions and their real endpoint events;do not repair a
  // physical CSR fault or skip processor history when no event occurred.
 }
 void Advance(unsigned count){
  if(!endpointInputsObserved){
   assert(!steps && pc==0x6000);endpointInputsObserved=true;
   for(unsigned i=0;i<16;i++)source[i]=binding.bytes.noc(binding.bytes.context,logicalWorker,BlackholeDramSourceOffset(i));
   for(unsigned i=0;i<8;i++)publishedCounts[i]=countBase[i]=binding.bytes.noc(binding.bytes.context,logicalWorker,CounterOffset(i));
   for(unsigned i=0;i<10;i++)publishedCommands[i]=command[i]=binding.bytes.noc(binding.bytes.context,logicalWorker,i*4);
   assert(!binding.bytes.noc(binding.bytes.context,logicalWorker,0x40));publishedPending=false;
  } // initial physical CSR bytes,not copied processor/native-owner state
  for(unsigned i=0;i<count;i++){
   if(pc==faultPC || pc==stopPC)break;
   if(!active && (pc==waitPC || pc==waitPC+4) && memory.at(G::Doorbell)){
    if(memory.at(G::Doorbell)==memory.at(G::Job+4) && memory.at(G::Job+4)==sequence+1)Receive();
    // A staged NEXT job plus an OLD nonzero doorbell is NOT a delivery.
   }
   StepBlock();if(active && (GdnFinished(sequence) || (pc==stopPC && memory.at(G::Status+4)==sequence)))active=false;
  }
 }
};
}
struct Q38NativePeerTestCpu::Impl {
 PioneerQ38SharedTestBinding binding;std::unique_ptr<NativeQ38PeerInstructionModel::NativePeer> cpu;
 uint64_t retiredInstructions=0;unsigned observedLayers=0;
 Impl(const char* old,const char* global,unsigned worker,const Q38NativePeerTestBytes& bytes):binding{bytes,worker,false}{
  assert(bytes.context && bytes.word && bytes.noc && bytes.events && bytes.read && bytes.put && bytes.clock && worker && worker<120);
  PioneerQ38SharedTestConstruction construction(binding);
  cpu=std::make_unique<NativeQ38PeerInstructionModel::NativePeer>(old,global,worker,binding);binding.executing=true;
 }
};
Q38NativePeerTestCpu::Q38NativePeerTestCpu(const char* old,const char* global,unsigned worker,const Q38NativePeerTestBytes& bytes)
 :impl(new Impl(old,global,worker,bytes)){}
Q38NativePeerTestCpu::~Q38NativePeerTestCpu(){delete impl;}
void Q38NativePeerTestCpu::Link(Q38NativePeerTestCpu& other){
 auto& p=*impl->cpu;auto& q=*other.impl->cpu;assert(!p.steps && !q.steps && p.logicalWorker<=96
  && ((p.logicalWorker-1)^1U)==q.logicalWorker-1 && impl->binding.bytes.context==other.impl->binding.bytes.context);p.peer=&q;
}
void Q38NativePeerTestCpu::Advance(unsigned count){assert(count && count<=10000);impl->cpu->Advance(count);}
Q38NativePeerTestStats Q38NativePeerTestCpu::Stats()const{
 const auto& p=*impl->cpu;return {p.pc,p.steps,p.matrixInstructions,p.moveInstructions,p.scalarFpInstructions,p.requests,p.responses,unsigned(p.pending),
  p.deviceReads,p.deviceWrites,p.statusWrites,p.minStack,p.sequence,unsigned(p.active),impl->retiredInstructions+p.steps};
}
bool Q38NativePeerTestCpu::ObserveMatrixOutput()const{
 const auto& p=*impl->cpu;
 return !p.active && !p.pending && (p.pc==p.waitPC || p.pc==p.waitPC+4)
  && p.memory.at(NativeQ38PeerInstructionModel::G::Status+4)==p.sequence && p.sequence
  && p.requests==p.packets.size() && !p.dotProbeReads && p.CorrectRows();
} // actual native L1 outputs vs separate FP32/FP64 arithmetic,NO products supplied
bool Q38NativePeerTestCpu::ObserveStopped()const{
 namespace G=NativeQ38PeerInstructionModel::G;const auto& p=*impl->cpu;
 return p.pc==p.stopPC && !p.active && !p.pending && p.sequence
  && p.memory.at(G::Status+4)==p.sequence && !p.memory.at(G::Status+8) && p.memory.at(G::Status+16)==G::Stop
  && p.memory.at(G::Job+4)==p.sequence && p.memory.at(G::Job+8)==G::Stop;
} // observed actual STOP only;NO native terminal/restore/END permission
void Q38NativePeerTestCpu::AdvanceStopped(unsigned count){
 assert(count && count<=2048 && ObserveStopped());auto& p=*impl->cpu;
 for(unsigned i=0;i<count;i++){p.StepBlock();assert(ObserveStopped());}
} // real STOP instructions,not a host no-op or a supplied STOP receipt
bool Q38NativePeerTestCpu::ObserveFaulted()const{
 namespace G=NativeQ38PeerInstructionModel::G;const auto& p=*impl->cpu;
 return p.pc==p.faultPC && p.memory.at(G::Status+8)==G::Fault;
}
void Q38NativePeerTestCpu::AdvanceFaulted(unsigned count){
 assert(count && count<=2048 && ObserveFaulted());auto& p=*impl->cpu;
 for(unsigned i=0;i<count;i++){p.StepBlock();assert(ObserveFaulted());}
} // read/execute only;NO method changes a fault marker or processor state
bool Q38NativePeerTestCpu::ObserveLayerBoundary(){
 namespace G=NativeQ38PeerInstructionModel::G;auto& p=*impl->cpu;
 if(impl->observedLayers>=192 || p.memory.at(G::Job+8)!=1 || p.memory.at(G::Job+5*4)!=impl->observedLayers%64
  || p.memory.at(G::Job+6*4)!=impl->observedLayers/64 || p.memory.at(G::Job+17*4)!=12
  || p.memory.at(G::Job+21*4)!=17408 || !ObserveMatrixOutput() || p.steps>=4000000000U)return false;
 const unsigned low=p.steps&255U;impl->retiredInstructions+=p.steps-low;p.steps=low;impl->observedLayers++;return true;
} // HOST instruction-accounting window ONLY;low scheduler bits preserved;
  // NO PC/register/NoC/tensor/private model state or native owner is changed
