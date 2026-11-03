/* SPDX-License-Identifier: MIT */
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(PIONEER_Q38_TOKEN_PUSHED_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_PRECISE_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION) || defined(PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION)
#error Offline peer implementation may never compile into native/Work code
#endif
#define PIONEER_Q38_NATIVE_PEER_TEST_BRIDGE 1
#define PIONEER_Q38_TEST_SHARED_WORKER_STORAGE 1
#ifdef PIONEER_Q38_NATIVE_PEER_WIDE_GDN_TEST
#if PIONEER_Q38_NATIVE_PEER_WIDE_GDN_TEST != 1 \
 || !defined(PIONEER_Q38_NATIVE_PEER_PRECISE_BETA_TEST) || PIONEER_Q38_NATIVE_PEER_PRECISE_BETA_TEST != 1 \
 || !defined(PIONEER_Q38_NATIVE_PEER_TWIN_BATCH_PREFETCH_TEST) || PIONEER_Q38_NATIVE_PEER_TWIN_BATCH_PREFETCH_TEST != 1 \
 || !defined(PIONEER_Q38_WIDE_GDN_RETURNING_WORKER_TEST) || PIONEER_Q38_WIDE_GDN_RETURNING_WORKER_TEST != 1
#error Own wide GDN HOST peer requires shared-array math and complete twin-batch bridge
#endif
#elif defined(PIONEER_Q38_WIDE_GDN_RETURNING_WORKER_TEST)
#error Wide GDN shared-array math cannot silently retag an old native peer bridge
#endif
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
#define PIONEER_Q38_INSTRUCTION_STACK_OBSERVER_TEST 1
#include "PioneerQ38InstructionStackObserver.h"
#ifdef PIONEER_Q38_NATIVE_PEER_PRECISE_BETA_TEST
#if PIONEER_Q38_NATIVE_PEER_PRECISE_BETA_TEST != 1
#error Exact separate HOST native-peer beta TEST required
#endif
#include "pioneer_q38_precise_beta_program_data_bridge.h"
#endif
namespace NativeQ38PeerInstructionModel {
#define PIONEER_Q38_GLOBAL_CHAIN_MACHINE_ONLY
#include "pioneer_q38_global_chain_instruction_test.cpp"
#ifndef PIONEER_Q38_NATIVE_PEER_PRECISE_BETA_TEST
#include "../PioneerQ38TokenSelectedPrograms.h"
#endif
static bool ExpectedProgramWord(unsigned worker,uint32_t address,uint32_t& output){
#ifdef PIONEER_Q38_NATIVE_PEER_PRECISE_BETA_TEST
 return Q38PreciseBetaTestProgramWord(worker,address,output);
#else
 return PioneerQ38TokenPrograms::ProgramWord(worker,address,output);
#endif
} // expected CONST ROM DATA,never a guest memory or arithmetic product source
#ifdef PIONEER_Q38_NATIVE_PEER_TWIN_BATCH_PREFETCH_TEST
#include "PioneerQ38NativeTwinBatchPrefetchObserver.h"
using NativePeerBase=NativeTwinBatchPrefetchPeer<GlobalChainPeer>;
#elif defined(PIONEER_Q38_NATIVE_PEER_BATCHED_PREFETCH_TEST)
#include "PioneerQ38NativeBatchedPrefetchObserver.h"
using NativePeerBase=NativeBatchedPrefetchPeer<GlobalChainPeer>;
#elif defined(PIONEER_Q38_NATIVE_PEER_PREFETCH_TEST)
#include "PioneerQ38NativePrefetchObserver.h"
using NativePeerBase=NativePrefetchPeer<GlobalChainPeer>;
#else
using NativePeerBase=GlobalChainPeer;
#endif
struct NativePeer:NativePeerBase,CohortBytes {
#ifdef PIONEER_Q38_NATIVE_PEER_MANUAL_TTSYNC_TEST
#if PIONEER_Q38_NATIVE_PEER_MANUAL_TTSYNC_TEST != 1 || !defined(PIONEER_Q38_NATIVE_PEER_WIDE_GDN_TEST) \
 || !defined(PIONEER_Q38_NATIVE_PEER_TWIN_BATCH_PREFETCH_TEST) || defined(PIONEER_Q38_NATIVE_PEER_MATRIX_PHASE_TEST)
#error Manual completion HOST endpoint requires its complete distinct returning worker
#endif
 bool manualOrdered=false;
 unsigned manualStores=0,manualLoads=0;
#ifdef PIONEER_Q38_NATIVE_PEER_MANUAL_WATCHDOG_TEST
 unsigned manualLostStore=0;uint64_t manualBlocked=0;
#endif
 void CompleteManualBackend(){
  assert(manualOrdered && !backendWait && !recordRemaining);
  if(rawPending)CompleteRawUnpack();
  if(!phaseQueue.empty()){
   auto batch=phaseQueue;phaseQueue.clear();assert(!executingPhases);executingPhases=true;
   for(auto word:batch)Instruction(word);executingPhases=false;
  }
  for(const auto& cell:pendingAccumulator)StoreFloatDst(cell.first.first,cell.first.second,cell.second);
  committedStores+=unsigned(pendingAccumulator.size());pendingAccumulator.clear();
  drained=true;drains++;manualOrdered=false;manualLoads++;
 } // Existing independent arithmetic retirement,NOT fake ACK words or supplied output.
#endif
#ifdef PIONEER_Q38_NATIVE_PEER_MATRIX_PHASE_TEST
#if PIONEER_Q38_NATIVE_PEER_MATRIX_PHASE_TEST != 1 || !defined(PIONEER_Q38_NATIVE_PEER_WIDE_GDN_TEST)
#error Separate current sampler HOST clock requires complete returning wide-GDN peer
#endif
 uint64_t matrixClockReads=0;unsigned matrixMetricNext=0;
 uint32_t matrixMetricObserved[16]={};bool matrixMetricAtEntry=false;
 bool PrivateObservationRow(unsigned row)const override{return row>=320 && row<336;}
 bool UnusedRowValue(unsigned row,uint32_t value)const override{
  if(!PrivateObservationRow(row))return NativePeerBase::UnusedRowValue(row,value);
  // These are observed RAW private words,not numerical expected products.
  // They may change ONLY at the authenticated actual publisher below.
  return matrixMetricAtEntry && !matrixMetricNext && value==matrixMetricObserved[row-320];
 }
 bool CorrectRows()const override{
  const bool okay=NativePeerBase::CorrectRows();
  if(!okay)fprintf(stderr,"NATIVE_MATRIX_ROW_OBSERVER_FAILURE worker%u type%u K%u groups%u matrix%u pending%u drained%u req%u resp%u dotReads%u metricNext%u;original numerical+FP64 comparisons UNCHANGED\n",
   logicalWorker,type,inputs,FfnPersistentPeer::groups,matrixInstructions,unsigned(pending),unsigned(drained),requests,responses,dotProbeReads,matrixMetricNext);
  return okay;
 }
 uint32_t ReadCycleCsr(unsigned csr)override{
  if(gdnActive)return NativePeerBase::ReadCycleCsr(csr);
  // Three exact read-only CSR sites from authenticated d5ba complete ELF.
  // HOST clock endpoint only;not measured card cycles or a frequency.
  assert(pc==0x68010+unsigned(matrixClockReads%3)*4
   && csr==(matrixClockReads%3==1?0xb00U:0xb80U));
  matrixClockReads++;
  const uint64_t now=(UINT64_C(1)<<40)+uint64_t(steps)*19;
  return csr==0xb00?uint32_t(now):uint32_t(now>>32);
 }
#endif
 PioneerQ38SharedTestBinding& binding;unsigned deviceReads=0,deviceWrites=0,statusWrites=0,scalarFpInstructions=0;
 uint32_t publishedCounts[8]={},publishedCommands[10]={};bool publishedPending=false;
 bool endpointInputsObserved=false;
 Q38InstructionStackObserver instructionStack;
 NativePeer(const char* old,const char* global,unsigned worker,PioneerQ38SharedTestBinding& b)
  :NativePeerBase(old,global,worker),binding(b){
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
 bool CodeAddress(uint32_t address)const override{uint32_t word;return ExpectedProgramWord(logicalWorker,address,word);}
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
#ifdef PIONEER_Q38_NATIVE_PEER_MANUAL_TTSYNC_TEST
  if(address==0xffe80004){
   assert(pc==0x7200c && manualOrdered && manualStores==manualLoads+1);
   CompleteManualBackend();return 0xdeadbeef; // documented undefined bits,not an ACK
  }
#endif
  if(CodeAddress(address))return binding.Word(address);
  if(Range(address,0xffb20000,0x2000))for(unsigned i=0;i<16;i++)if(address-0xffb20000==BlackholeDramSourceOffset(i))
   return binding.bytes.noc(binding.bytes.context,logicalWorker,address-0xffb20000);
  const uint32_t value=NativePeerBase::Load(address);
  if(Range(address,0xffb20000,0x2000))Publish();
  return Range(address,0xffb20000,0x2000)?binding.bytes.noc(binding.bytes.context,logicalWorker,address-0xffb20000):value;
 }
 void Store(uint32_t address,uint32_t value)override{
#ifdef PIONEER_Q38_NATIVE_PEER_MANUAL_TTSYNC_TEST
  if(address==0xffe80004){
   assert(pc==0x72008 && !value && !manualOrdered && manualStores==manualLoads);
   manualOrdered=true;manualStores++;deviceWrites++;return;
  }
#endif
#ifdef PIONEER_Q38_NATIVE_PEER_MATRIX_PHASE_TEST
  if(Range(address,0x84500,64)){
   // Raw writes ONLY from the exact frozen sampler publisher. Preserve
   // all old RowsMachine bounds;do not widen its output/operand window.
   const unsigned word=(address-0x84500)/4;
   assert(pc==0x68178 && !(address&3) && word==matrixMetricNext && active && type==12 && prefetchMatrix
    && !gdnActive && !convActive && !denseActive && !preciseActive && drained && !pending && !rawPending
    && phaseQueue.empty() && pendingAccumulator.empty() && !prefetchReturn
    && requests==packets.size() && responses==requests
    && matrixInstructions==MatrixPhasesPerPanel()*FfnPersistentPeer::groups*(inputs/16)
    && matrixClockReads && matrixClockReads%3==0 && memory.at(0x74038)==1);
   const uint32_t expected[]={0x324d5051,2,memory.at(0x74030),logicalWorker,12,inputs,FfnPersistentPeer::groups};
   if(word<7)assert(value==expected[word]);
   if(word==7)assert(value<=1);
   if(word>=14)assert(!value);
   assert(matrixMetricAtEntry);binding.Word(address)=value;matrixMetricObserved[word]=value;deviceWrites++;
   if(++matrixMetricNext==16)matrixMetricNext=0;
   return;
  }
#endif
  assert(!CodeAddress(address));NativePeerBase::Store(address,value);deviceWrites++;
  if(Range(address,G::Status,64))statusWrites++;
  if(Range(address,0xffb20000,0x2000))Publish(Range(address,0xffb20000,40)?1U<<((address-0xffb20000)/4):0U);
 }
 void StepBlock()override{
  uint32_t expected;assert(ExpectedProgramWord(logicalWorker,pc,expected) && binding.Word(pc)==expected);
#ifdef PIONEER_Q38_NATIVE_PEER_MANUAL_WATCHDOG_TEST
#ifndef PIONEER_Q38_NATIVE_PEER_MANUAL_TTSYNC_TEST
#error Completion-loss endpoint requires the actual manual-wait worker
#endif
  if(manualLostStore && manualStores==manualLostStore && pc==0x7200c){
   assert(expected==0x0007a703 && regs[15]==0xffe80004 && manualOrdered
    && manualStores==manualLoads+1 && !backendWait && !recordRemaining);
   manualBlocked++;return;
  } // actual emitted LOAD cannot retire:PC/registers/operand/Dst/output unchanged.
    // Scheduler observations have their OWN count,not retired instructions.
#endif
  const unsigned opcode=expected&127;if(opcode==7 || opcode==0x27 || opcode==0x53)scalarFpInstructions++;
  // Parent service FP decoders may use their immutable ROM byte vectors.
  // ALL instruction bytes must first equal this SAME installed L1 word.
  NativePeerBase::StepBlock();
  assert(instructionStack.Retired(regs[2])); // actual completed instruction;NO guest-state setter
  // Hardware events originate ONLY at actual NoC Load/Store endpoints
  // above. Re-scanning every completed packet after every ordinary ISA/
  // L1/tensor instruction is quadratic HOST work,not a hardware event.
  // Keep all instructions and their real endpoint events;do not repair a
  // physical CSR fault or skip processor history when no event occurred.
 }
 void Advance(unsigned count){
  if(!endpointInputsObserved){
   assert(!steps && pc==0x6000);endpointInputsObserved=true;
#ifdef PIONEER_Q38_NATIVE_PEER_MATRIX_PHASE_TEST
   // Read-only initial private-byte observation BEFORE the virgin boot.
   // Never supply these bytes back to a processor or native owner.
   assert(!matrixMetricAtEntry);matrixMetricAtEntry=true;
   for(unsigned i=0;i<16;i++)matrixMetricObserved[i]=binding.Word(0x84500+i*4);
#endif
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
#ifdef PIONEER_Q38_NATIVE_PEER_MANUAL_WATCHDOG_TEST
void Q38NativePeerTestCpu::LoseNextManualCompletion(){
 auto& p=*impl->cpu;assert(!p.manualLostStore && !p.manualOrdered && p.manualStores==p.manualLoads);
 p.manualLostStore=p.manualStores+1;
} // change only the simulated hardware completion endpoint,not processor/native state
Q38NativeManualWaitStats Q38NativePeerTestCpu::ManualWaitStats()const{
 const auto& p=*impl->cpu;return {unsigned(p.manualOrdered),p.manualStores,p.manualLoads,
  unsigned(p.manualLostStore!=0),p.manualBlocked};
}
#endif
Q38NativePeerTestStats Q38NativePeerTestCpu::Stats()const{
 const auto& p=*impl->cpu;return {p.pc,p.steps,p.matrixInstructions,p.moveInstructions,p.scalarFpInstructions,p.requests,p.responses,unsigned(p.pending),
  p.deviceReads,p.deviceWrites,p.statusWrites,p.minStack,p.sequence,unsigned(p.active),impl->retiredInstructions+p.steps};
}
Q38NativePeerTestStack Q38NativePeerTestCpu::Stack()const{
 const auto& p=*impl->cpu;const auto& s=p.instructionStack;
 assert(s.Valid() && s.Instructions()==impl->retiredInstructions+p.steps
  && p.minStack>=0x6e000 && p.minStack<=0x70000);
 return {s.MinimumSp(),p.minStack,s.Instructions()};
} // inspect only at an instruction boundary;NO memory,CSR or native-owner IO
bool Q38NativePeerTestCpu::ObserveMatrixOutput()const{
 const auto& p=*impl->cpu;
 return !p.active && !p.pending && (p.pc==p.waitPC || p.pc==p.waitPC+4)
  && p.memory.at(NativeQ38PeerInstructionModel::G::Status+4)==p.sequence && p.sequence
  && p.requests==p.packets.size() && !p.dotProbeReads && p.CorrectRows();
} // actual native L1 outputs vs separate FP32/FP64 arithmetic,NO products supplied
bool Q38NativePeerTestCpu::ObserveStopped()const{
 namespace G=NativeQ38PeerInstructionModel::G;const auto& p=*impl->cpu;
#if defined(PIONEER_Q38_NATIVE_PEER_PREFETCH_TEST) || defined(PIONEER_Q38_NATIVE_PEER_BATCHED_PREFETCH_TEST) \
 || defined(PIONEER_Q38_NATIVE_PEER_TWIN_BATCH_PREFETCH_TEST)
 if(!p.prefetchJobsVerified || !p.prefetchPendingOps)return false;
#endif
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
