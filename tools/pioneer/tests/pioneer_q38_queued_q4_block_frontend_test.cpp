/* SPDX-License-Identifier: MIT */
// Actual compiled queued-E worker; conservative asynchronous MOCK schedule.
// NOT a silicon-cycle model,full frontend implementation,or native grant.
#define PIONEER_Q38_PREPARED_WORKER_MACHINE_ONLY
#include "pioneer_q38_prepared_exact_q4_worker_test.cpp"
#include <deque>

struct QueuedBlockFrontend:QueuedQ4BlockPeer {
 using QueuedQ4BlockPeer::QueuedQ4BlockPeer;
 std::deque<unsigned> fifo;
 bool active=false,gate=false,servicing=false,producerBlocked=false;
 unsigned capacity=13,maxQueued=0,backpressure=0,blockedPolls=0;
 unsigned delayTicks=31,gateSerial=0,missingAt=0,submitted=0,consumed=0;
 unsigned privateSumWritesWhileMathPending=0;
 uint64_t schedule=0,readyAt=0;
 struct ProducerStalled{};
 struct EarlyAccess{};

 bool Service(){
  if(gate){
   if(gateSerial==missingAt || schedule<readyAt){blockedPolls++;return false;}
   // ALL-class STALLWAIT latches immediately,then blocks FOLLOWING ops.
   // Its completion effects in the functional backend are applied only
   // when the independently delayed condition is released here.
   QueuedQ4BlockPeer::Instruction(0xa2ff9816U);gate=false;return true;
  }
  if(fifo.empty())return false;
  const unsigned op=fifo.front();fifo.pop_front();consumed++;
  if(op==0xa2ff9816U){gate=true;gateSerial++;readyAt=schedule+delayTicks+1;return true;}
  servicing=true;
  try{QueuedQ4BlockPeer::Instruction(op);}catch(...){servicing=false;throw;}
  servicing=false;
  if(op==0x4500017e){assert(fifo.empty() && !gate && !queued && MathIdle());active=false;}
  return true;
 }
 void Instruction(unsigned op)override{
  if(servicing){QueuedQ4BlockPeer::Instruction(op);return;}
  if(!active && exact && (op>>24)>=0xb3 && (op>>24)<=0xb6){
   assert(!gate && fifo.empty() && !queued && MathIdle());active=true;
  }
  if(!active){QueuedQ4BlockPeer::Instruction(op);return;}
  assert(exact && type==12 && capacity && capacity<=13);
  // This image has no MOP/RESOURCEDECL/load-only REPLAY in its queued
  // span. Those tracking exceptions need a DIFFERENT model,not guesses.
  assert((op>>24)!=0x01 && (op>>24)!=0x03 && (op>>24)!=0x05);
  assert((op>>24)!=0x04 || (op&3)!=1);
  while(fifo.size()>=capacity){
   schedule++;backpressure++;
   if(!Service() && gate && gateSerial==missingAt){
    // EXTERNAL TEST OBSERVATION,not a guest timeout or successful park.
    // The issuing store has not returned;the original decoder has not
    // advanced PC. Keep it stalled and retain all pending work.
    producerBlocked=true;throw ProducerStalled{};
   }
  }
  fifo.push_back(op);submitted++;maxQueued=std::max(maxQueued,unsigned(fifo.size()));
  assert(fifo.size()<=13 && fifo.size()<=32);
 }
 void StepBlock()override{
  schedule++;Service();
  if(producerBlocked){assert(gate && gateSerial==missingAt && fifo.size()==capacity);return;}
  QueuedQ4BlockPeer::StepBlock();
 }
 unsigned Load(unsigned at)override{
  if(active && (Range(at,0xffef0000,224*4) || Range(at,0xffbd8000,0x4000)))throw EarlyAccess{};
  return QueuedQ4BlockPeer::Load(at);
 }
 void Store(unsigned at,unsigned value)override{
  if(active && (Range(at,0xffef0000,224*4) || Range(at,0xffbd8000,0x4000)
    || Plane(at) || InputRange(at) || SourceRange(at) || RawRange(at)
    || Range(at,L::DenseMask,512) || Range(at,L::Summary,64)))throw EarlyAccess{};
  if(active && at==0xffe000fc)for(unsigned op:fifo)if(op==0x4500017e)throw EarlyAccess{};
  if(active && Range(at,0x95000,4352)){
   // CPU-only integer sum cache is NOT a GPU operand. Preserve every
   // original per-job/order/value/canary constraint;only the old model's
   // unrelated global `drained` requirement does not apply to this range.
   assert(exact && type==12 && memory.at(0x74038)==1 && FfnPersistentPeer::groups>1 && !(at&3)
    && (!pending || DisjointPrefetchCompute()));
   const unsigned slot=(at-0x95000)/4;
   assert(slot==sumWrites && slot<inputs/16 && int(value)==Expected(slot));
   for(const auto& t:tasks)if(t.active)assert(t.address+t.count*4<=0x95000 || t.address>=0x96100);
   if(!MathIdle())privateSumWritesWhileMathPending++;
   memory[at]=value;sumWrites++;return;
  }
  QueuedQ4BlockPeer::Store(at,value);
 }
 void CheckEnd()override{
  QueuedQ4BlockPeer::CheckEnd();
  assert(!active && !gate && fifo.empty() && !producerBlocked && submitted==consumed);
 }
};

#ifndef PIONEER_Q38_QUEUED_Q4_FRONTEND_MACHINE_ONLY
template<class Peer>static void TwoQueuedJobs(Peer& p){
 p.delay=3;p.BootPushed();p.Prepare();p.StagePushed();p.AwaitPayload();p.FinishPushed(true);p.CheckEnd();
 std::array<unsigned,16> first{};for(unsigned i=0;i<16;i++)first[i]=p.buffers[4096+i];
 p.ReceivePreparedQueued();p.AwaitSecond();p.FinishSecond(first);p.CheckEnd();
 assert(p.exactJobs==2 && p.pc==p.stopPC && !p.pending && !p.heldPayload);
}

static bool MissingCompletion(const char* fixture,const char* image,unsigned depth,unsigned event){
 auto p=std::make_unique<QueuedBlockFrontend>(fixture,image,12,5120,1);
 p->capacity=depth;p->missingAt=event;p->delayTicks=17;p->delay=3;
 bool caught=false,faulted=false;p->throwOnFault=true;
 try{p->BootPushed();p->Prepare();p->StagePushed();p->AwaitPayload();}
 catch(const QueuedBlockFrontend::ProducerStalled&){caught=true;}
 catch(const InstructionPrefetchPeer::ReachedFault&){faulted=true;}
 assert(caught!=faulted && p->gate && p->gateSerial==event
  && (caught?(p->producerBlocked && p->fifo.size()==depth && p->pc!=p->faultPC && p->backpressure)
             :(!p->producerBlocked && p->pc==p->faultPC && !p->fifo.empty()))
  && p->pc!=p->stopPC && !p->heldPayload
  && !p->pushes && !p->mailbox[0] && !p->exactJobs && p->prefetchEvents==10);
 const auto memory=p->memory;const auto fifo=p->fifo;
 const unsigned pc=p->pc,steps=p->steps,requests=p->requests,responses=p->responses;
 const unsigned writes=p->configWrites,ops=p->queueWrites,waits=p->queueWaits,sub=p->submitted,con=p->consumed;
 std::array<unsigned,224> config;std::copy(std::begin(p->config),std::end(p->config),config.begin());
 std::array<unsigned,32> regs;std::copy(std::begin(p->regs),std::end(p->regs),regs.begin());
 p->throwOnFault=false;for(unsigned i=0;i<2048;i++)p->StepBlock();
 assert(p->pc==pc && p->steps==steps+(faulted?2048:0) && p->memory==memory && p->fifo==fifo && p->requests==requests && p->responses==responses
  && p->configWrites==writes && p->queueWrites==ops && p->queueWaits==waits && p->submitted==sub && p->consumed==con
  && std::equal(config.begin(),config.end(),std::begin(p->config)) && std::equal(regs.begin(),regs.end(),std::begin(p->regs))
  && !p->heldPayload && !p->pushes && !p->mailbox[0]);
 printf("PASS_QUEUED_MISSING_CONDITION depth%u phaseWait%u %sPC%08x retainedInstructions%zu inertObservations2048 NO_CLEANUP_CLAIM\n",depth,event,caught?"producerStall":"boundedPollFault",pc,p->fifo.size());
 return faulted;
}

int main(int argc,char** argv){
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;(void)&Compare;(void)&Round;
 unsigned positive=0,missing=0,pollFaults=0;
 for(unsigned k:{5120U,6144U,17408U})for(unsigned depth:{1U,2U,8U,13U}){
  auto control=std::make_unique<QueuedQ4BlockPeer>(argv[1],argv[2],12,k,3);
  auto next=std::make_unique<QueuedBlockFrontend>(argv[1],argv[2],12,k,3);
  next->capacity=depth;next->delayTicks=depth==1?0:depth==2?31:depth==8?257:4099;
  TwoQueuedJobs(*control);TwoQueuedJobs(*next);
  assert(control->requests==next->requests && control->responses==next->responses && control->mailbox==next->mailbox
   && control->affineReplayWords==next->affineReplayWords && control->affineReplayRuns==next->affineReplayRuns);
  for(unsigned r=0;r<48;r++)assert(control->buffers[4096+r]==next->buffers[4096+r]);
  assert(next->maxQueued<=depth && next->gateSerial==next->queueWaits && next->maxQueued
   && (depth==1 || (next->backpressure && next->blockedPolls)));
  assert(control->sumReads==next->sumReads && control->sumWrites==next->sumWrites);
  printf("PASS_QUEUED_ASYNC_COMPLETE K%u depth%u delay%u jobs2 groups3 highwater%u backpressure%u delayedObservations%u privateSumWritesDuringMath%u exactFP32_and_real_ACK_payload_commit_STOP\n",
   k,depth,next->delayTicks,next->maxQueued,next->backpressure,next->blockedPolls,next->privateSumWritesWhileMathPending);positive++;
 }
 for(unsigned depth:{2U,13U})for(unsigned event=1;event<=64;event++){pollFaults+=MissingCompletion(argv[1],argv[2],depth,event);missing++;}
 printf("PASS_QUEUED_FINITE_FRONTEND positives%u missingConditions%u producerStalls%u boundedPollFaults%u originalActualROM_and_RV_NoPCSkip exactImmutableOperands_finalACK STOP_or_retainedUnknown NOT_SILICON_TIMING_FULL_FRONTEND_NATIVE_OR_TPS\n",positive,missing,missing-pollFaults,pollFaults);
}
#endif
