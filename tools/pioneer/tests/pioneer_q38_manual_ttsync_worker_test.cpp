/* SPDX-License-Identifier: MIT */
// Execute COMPLETE authenticated old/new RV32 workers. The NoC, raw tensors
// and completion latency are independent synthetic endpoints, NOT silicon.
#define PIONEER_Q38_CONSTANT_ISSUE_WORKER_MACHINE_ONLY
#include "pioneer_q38_q4_constant_issue_worker_test.cpp"

struct ManualTTSyncPeer:ConstantIssuePeer {
 using ConstantIssuePeer::ConstantIssuePeer;
 bool manual=false,ordered=false,retiring=false,bypass=false;
 unsigned completionDelay=0,manualStores=0,manualLoads=0,blockedTicks=0,loseAt=0;
 uint32_t undefinedBits=0xdeadbeef;
 uint64_t backendReadyAt=0;
 unsigned instructionDepth=0;
 std::vector<uint32_t> submitted;
 struct EarlyCompletion {};
 struct MissingOrdering {};

 void Instruction(uint32_t op)override {
  // Replay/matrix retirement recursively invokes this semantic observer.
  // Record real submitted commands ONCE, not a second invented issue at
  // their eventual completion. Retain every actual arithmetic command.
  if(matrixScope && !instructionDepth && !retiring)submitted.push_back(op);
  instructionDepth++;
  ConstantIssuePeer::Instruction(op);
  instructionDepth--;
  if(manual && !retiring)backendReadyAt=uint64_t(steps)+completionDelay;
 }
 void RetireBackend() {
  // Same arithmetic completion primitives as the original ACK endpoint,
  // NOT injected STALLWAIT/STOREREG opcodes or fabricated output values.
  assert(manual && ordered && !backendWait && !recordRemaining);
  retiring=true;
  if(rawPending)CompleteRawUnpack();
  if(!phaseQueue.empty()) {
   auto batch=phaseQueue;phaseQueue.clear();assert(!executingPhases);
   executingPhases=true;for(auto op:batch)Instruction(op);executingPhases=false;
  }
  for(const auto& cell:pendingAccumulator)
   StoreFloatDst(cell.first.first,cell.first.second,cell.second);
  committedStores+=unsigned(pendingAccumulator.size());pendingAccumulator.clear();
  retiring=false;drained=true;drains++;
 }
 void ProgressIndependentNoC() {
  // A blocked RISC load does not stop an independently submitted NIU read.
  // Retain the parent's exact disjoint-buffer and between-tile conditions.
  if(matrixScope && asynchronous && pending && uint64_t(steps)>=readyAt && requests!=faultAt) {
   const auto& p=packets.at(requests-1);
   bool math=p.tile>=int(Batch()) && matrixInstructions>unsigned(p.tile-int(Batch()))*TilePhases()
    && matrixInstructions<unsigned(p.tile)*TilePhases();
   if(!betweenTiles || p.tile<int(Batch()) || math){if(math)retiredDuringMath++;CompleteRead();}
  }
 }
 void StepBlock()override {
  ProgressIndependentNoC();
  if(manual && pc==0x7200c) {
   assert(l1[Index(pc)]==0x0007a703 && regs[15]==0xffe80004);
   if(!ordered)throw MissingOrdering{};
   bool ready=manualStores!=loseAt && uint64_t(steps)>=backendReadyAt;
   if(!ready && !bypass) {
    // The actual load has NOT retired: no PC, destination-register,
    // pending operand, Dst, output or completion-state change.
    assert(++steps<StepLimit());blockedTicks++;return;
   }
  }
  ConstantIssuePeer::StepBlock();
 }
 void Store(uint32_t at,uint32_t value)override {
  if(at==0xffe80004) {
   assert(manual && pc==0x72008 && l1[Index(pc)]==0x0007a023 && !value && !ordered);
   ordered=true;manualStores++;return;
  }
  ConstantIssuePeer::Store(at,value);
 }
 uint32_t Load(uint32_t at)override {
  if(at==0xffe80004) {
   if(!ordered)throw MissingOrdering{};
   assert(manual && pc==0x7200c);
   if(manualStores==loseAt || uint64_t(steps)<backendReadyAt)throw EarlyCompletion{};
   RetireBackend();ordered=false;manualLoads++;return undefinedBits;
  }
  return ConstantIssuePeer::Load(at);
 }
 void CheckManualEnd()const {
  assert(manual && !ordered && !rawPending && phaseQueue.empty() && pendingAccumulator.empty()
   && !recordRemaining && !backendWait && manualStores==manualLoads && manualLoads);
 }
};

static std::pair<std::vector<uint32_t>,unsigned> WithoutAckPairs(const std::vector<uint32_t>& input) {
 std::vector<uint32_t> output;unsigned pairs=0;
 for(unsigned i=0;i<input.size();i++) {
  if(input[i]==0xa2101816) {
   assert(i+1<input.size() && input[i+1]==0x4500017e);i++;pairs++;
  }else {assert(input[i]!=0x4500017e);output.push_back(input[i]);}
 }
 return {output,pairs};
}
static void EquivalentManual(const ManualTTSyncPeer& old,const ManualTTSyncPeer& next) {
 const auto a=WithoutAckPairs(old.submitted),b=WithoutAckPairs(next.submitted);
 assert(a.first==b.first && a.second==next.manualLoads && !b.second
  && old.matrixInstructions==next.matrixInstructions && old.rawReads==next.rawReads
  && old.rawUnpacks==next.rawUnpacks && old.drains==next.drains && old.vectorOps==next.vectorOps
  && old.replayRecords==next.replayRecords && old.replayExpansions==next.replayExpansions
  && old.requests==next.requests && old.responses==next.responses
  && old.sumReads==next.sumReads && old.sumWrites==next.sumWrites
  && old.affineRecords==next.affineRecords && old.affineExpansions==next.affineExpansions
  && old.baseWrites==next.baseWrites && old.copyWrites==next.copyWrites
  && old.committedStores==next.committedStores && !old.dotProbeReads && !next.dotProbeReads);
 for(unsigned row=0;row<old.FfnPersistentPeer::groups*16;row++)assert(old.buffers[4096+row]==next.buffers[4096+row]);
 for(unsigned i=0;i<64;i++)assert(old.mailbox[i]==next.mailbox[i]);
 next.CheckManualEnd();
}

int main(int argc,char** argv) {
 assert(argc==4 || (argc==5 && !strcmp(argv[4],"--pilot")));setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Compare;(void)&RunPersistentGdnPair;
 unsigned cases=0,stack=0,losses=0,asyncCases=0,completionLosses=0,bypassRefusals=0;
 for(unsigned type:{8U,12U,14U})for(unsigned k:{5120U,6144U,17408U})for(unsigned groups:{1U,3U,16U}) {
  if(argc==5 && (k!=5120 || groups!=1))continue;
  printf("MANUAL_TTSYNC_CASE type%u K%u groups%u\n",type,k,groups);
  auto old=std::make_unique<ManualTTSyncPeer>(argv[1],argv[2],69,type,k,groups);
  auto next=std::make_unique<ManualTTSyncPeer>(argv[1],argv[3],69,type,k,groups);next->manual=true;
  for(unsigned side=0;side<2;side++) {
   auto& p=side?*next:*old;p.delay=3;p.BootPushed();p.Prepare();p.StagePushed();
   p.AwaitPayload();p.FinishPushed(true);p.CheckEnd();
  }
  EquivalentManual(*old,*next);
  std::array<uint32_t,16> first[2];
  for(unsigned side=0;side<2;side++) {
   auto& p=side?*next:*old;for(unsigned i=0;i<16;i++)first[side][i]=p.buffers[4096+i];
   p.ReceiveQueued();p.AwaitSecond();p.FinishSecond(first[side]);stack=std::max(stack,0x70000-p.minStack);
  }
  EquivalentManual(*old,*next);cases++;
 }
 if(argc==5){printf("PASS_MANUAL_TTSYNC_PARTIAL actualWorkerShapes%u changedInputJobs%u;NOT_COMPLETE_OR_NATIVE_OR_TPS\n",cases,cases*2);return 0;}
 for(unsigned type:{8U,12U,14U})for(unsigned schedule:{0U,1U,2U,3U}) {
  auto p=std::make_unique<ManualTTSyncPeer>(argv[1],argv[3],69,type);
  p->manual=true;p->asynchronous=true;p->latency=schedule==0?1024:schedule==1?4099:schedule==2?65537:0;
  p->completionDelay=schedule==0?1:schedule==1?31:schedule==2?257:7;
  p->betweenTiles=schedule==3;p->undefinedBits=schedule==0?0:schedule==1?1:schedule==2?0xffffffff:0xdeadbeef;
  printf("MANUAL_TTSYNC_ASYNC type%u schedule%u syntheticNoCDelay%u syntheticCompletionDelay%u\n",type,schedule,p->latency,p->completionDelay);
  p->BootPushed();p->Prepare();p->StagePushed();p->AwaitPayload();p->FinishPushed();p->CheckEnd();p->CheckManualEnd();
  if(p->betweenTiles)assert(p->retiredDuringMath);
  if(p->completionDelay>=31)assert(p->blockedTicks);
  p->StopLocal();asyncCases++;
 }
 for(unsigned type:{8U,12U,14U})for(unsigned which:{0U,1U,2U,3U}) {
  auto p=std::make_unique<ManualTTSyncPeer>(argv[1],argv[3],69,type,5120,3);p->manual=true;
  p->delay=3;p->BootPushed();p->Prepare();p->StagePushed();
  p->faultAt=which==0?1:which==1?p->inputReads+1
   :which==2?p->inputReads+unsigned(p->tiles.size()/p->Batch()/2):unsigned(p->packets.size());
  p->ReadFault();losses++;
 }
 for(unsigned type:{8U,12U,14U})for(unsigned which:{0U,1U,2U}) {
  auto p=std::make_unique<ManualTTSyncPeer>(argv[1],argv[3],69,type);p->manual=true;
  p->delay=3;p->BootPushed();p->Prepare();p->StagePushed();
  // Stop at three DISTINCT classes of actual pending backend work. No
  // instruction/result is replaced to manufacture the outstanding state.
  unsigned budget=0;
  for(;;) {
   assert(++budget<1000000 && !p->pushes && p->pc!=p->faultPC);
   if(p->pc==0x72008 && (which==0?p->rawPending:which==1?!p->phaseQueue.empty():!p->pendingAccumulator.empty()))break;
   p->StepBlock();
  }
  p->loseAt=p->manualStores+1;p->StepBlock();assert(p->pc==0x7200c && p->ordered);
  const auto regs=std::vector<uint32_t>(p->regs,p->regs+32);
  const auto data=p->memory;
  const auto accumulator=p->pendingAccumulator;
  const auto phase=p->phaseQueue;
  unsigned requests=p->requests,responses=p->responses,math=p->matrixInstructions,drains=p->drains,loads=p->manualLoads;
  bool raw=p->rawPending;
  for(unsigned n=0;n<4096;n++)p->StepBlock();
  assert(p->pc==0x7200c && p->ordered && p->memory==data && p->pendingAccumulator==accumulator && p->phaseQueue==phase
   && std::equal(regs.begin(),regs.end(),p->regs) && p->rawPending==raw && p->requests==requests && p->responses==responses
   && p->matrixInstructions==math && p->drains==drains && p->manualLoads==loads && !p->pushes && !p->heldPayload
   && !p->mailbox[0] && !p->memory.at(M::Status+4));completionLosses++;
  // A synthetic override bypass MUST expose an unsafe early completion,
  // not be mistaken for a production/native override guard.
  p->bypass=true;bool refused=false;try{p->StepBlock();}catch(const ManualTTSyncPeer::EarlyCompletion&){refused=true;}
  assert(refused && p->pc==0x7200c && p->memory==data && !p->pushes);bypassRefusals++;
 }
 printf("PASS_MANUAL_TTSYNC_ACTUAL_WORKER shapes%u retainedChangedInputJobs%u asyncSchedules%u NoCLossInert%u actualPendingCompletionLosses%u expectedBypassHazards%u maxCombinedStack%u/8192;"
  "same arithmetic stream+FP32/FP64+rawUNPACK+deferredSTORE+replay+source+NoC+ACK+STOP;ONLY exact old ACK pairs replaced;"
  "SYNTHETIC endpoints/latencies,NO native lifetime/override guard,original model,physical timing,Work or TPS grant\n",
  cases,cases*2,asyncCases,losses,completionLosses,bypassRefusals,stack);
}
