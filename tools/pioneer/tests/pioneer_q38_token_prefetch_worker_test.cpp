/* SPDX-License-Identifier: MIT */
// Frozen current QGW3 instructions. Synthetic NoC endpoints, NOT native,
// physical inference, model quality, Work readiness or throughput evidence.
#define PIONEER_Q38_PRECISE_BETA_WORKER_MACHINE_ONLY
#include "pioneer_q38_precise_beta_worker_test.cpp"
struct TokenPrefetchPeer:PreciseBetaPeer {
 std::vector<uint8_t> fetch;
 bool matrixScope=false,restoring=false,asynchronous=false,betweenPanels=false;
 unsigned restoreCommands=0,overlappedOps=0,metadataReads=0,overlappedMetadataReads=0,retiredDuringMath=0;
 unsigned latency=4099,stageCalls=0,waitCalls=0,skipWaitAt=0,wrongBufferAt=0;
 uint64_t readyAt=0;uint32_t stageReturn=0;
 mutable unsigned validatedTile=UINT32_MAX;
 static constexpr unsigned FetchWait=0x70000,FetchStage=0x7018c;
 struct PrefetchHazard {};
 TokenPrefetchPeer(const char* old,const char* image,unsigned worker,unsigned t=8,unsigned k=5120,unsigned g=1)
  :PreciseBetaPeer(old,image,worker,t,k,g),fetch(Read((std::string(image)+"/fetch.bin").c_str())) {
  assert(!steps && pc==0x6000 && fetch.size()==856);
 }
 const std::vector<uint8_t>* Rom(unsigned at,unsigned& base)const override {
  if(Range(at,0x70000,unsigned(fetch.size()))){base=0x70000;return &fetch;}
  return PreciseBetaPeer::Rom(at,base);
 }
 unsigned TilePhases()const{return MatrixPhasesPerPanel()*(type==8?2:16);}
 unsigned Blocks()const{return inputs/(type==8?32:256);}
 unsigned CurrentTile()const {
  if(!matrixScope || !stageCalls || stageReturn || stageCalls>tiles.size())throw PrefetchHazard{};
  return stageCalls-1;
 }
 uint32_t CurrentRaw(unsigned t)const{return ((t%Blocks())&1)?0x98000:0x90000;}
 bool RawRange(unsigned at)const{return Range(at,0x90000,unsigned(tile.size())) || Range(at,0x98000,unsigned(tile.size()));}
 uint32_t RawInputBase()const override {
  if(!matrixScope)return PreciseBetaPeer::RawInputBase();
  unsigned t=CurrentTile();uint32_t raw=CurrentRaw(t);
  if(responses<inputReads+t+1 || (pending && packets.at(requests-1).destination==raw))throw PrefetchHazard{};
  if(validatedTile!=t){
   for(unsigned w=0;w<tile.size()/4;w++)if(memory.at(raw+w*4)!=Word(tiles.at(t),w*4))throw PrefetchHazard{};
   validatedTile=t;
  }
  return raw;
 }
 bool DisjointPrefetchCompute()const override {
  if(!matrixScope || !pending || !stageCalls || stageReturn || requests<=inputReads)return false;
  unsigned t=CurrentTile();const auto& p=packets.at(requests-1);
  if(t+1>=tiles.size() || (t+1)%Blocks()==0 || p.tile!=int(t+1) || p.destination!=CurrentRaw(t+1)
   || p.bytes!=tile.size() || requests!=inputReads+t+2 || responses!=inputReads+t+1
   || matrixInstructions<t*TilePhases() || matrixInstructions>(t+1)*TilePhases())return false;
  return RawInputBase()!=p.destination;
 }
 void Prepare() {
  assert(pc==waitPC && !pending && !stageCalls);matrixScope=true;restoring=false;
  for(auto& p:packets)if(p.tile>=0)p.destination=CurrentRaw(unsigned(p.tile));
  Poison();PoisonTail();
  for(unsigned raw:{0x90000U,0x98000U})for(unsigned w=0;w<tile.size()/4;w++)memory[raw+w*4]=0x7fc17fc1;
 }
 void CompleteRead() {
  assert(pending && requests && requests!=faultAt);const auto& p=packets.at(requests-1);
  for(unsigned at=0;at<p.bytes;at+=4)memory[p.destination+at]=p.tile<0?root.at(p.address+at):Word(tiles.at(unsigned(p.tile)),at);
  pending=false;responses++;
 }
 void StepBlock()override {
  if(matrixScope && asynchronous && pending && uint64_t(steps)>=readyAt && requests!=faultAt){
   const auto& p=packets.at(requests-1);
   bool math=p.tile>0 && matrixInstructions>unsigned(p.tile-1)*TilePhases() && matrixInstructions<unsigned(p.tile)*TilePhases();
   if(!betweenPanels || p.tile<=0 || !(unsigned(p.tile)%Blocks()) || math){if(math)retiredDuringMath++;CompleteRead();}
  }
  // Deliberate mutant controls ONLY: actual call returns without its wait,
  // or actual stage returns the wrong buffer. Positive paths never set PC,
  // guest registers, completion markers, products or NIU pending/counters.
  if(matrixScope && pc==FetchWait && ++waitCalls==skipWaitAt){pc=regs[1];return;}
  if(matrixScope && pc==FetchStage){assert(!stageReturn);stageReturn=regs[1];stageCalls++;}
  PreciseBetaPeer::StepBlock();
  if(stageReturn && pc==stageReturn){
   stageReturn=0;if(stageCalls==wrongBufferAt)regs[10]^=0x8000;
   if(regs[10]!=RawInputBase())throw PrefetchHazard{};
  }
 }
 void Instruction(uint32_t op)override {
  if(matrixScope && pending){if(!DisjointPrefetchCompute())throw PrefetchHazard{};overlappedOps++;}
  PreciseBetaPeer::Instruction(op); // NEVER hide pending from guest or parent.
 }
 uint32_t Load(uint32_t at)override {
  if(matrixScope && RawRange(at)){
   uint32_t raw=RawInputBase();if(at<raw || at-raw+4>tile.size() || (at&3))throw PrefetchHazard{};
   if(pending){assert(DisjointPrefetchCompute());overlappedMetadataReads++;}
   metadataReads++;return memory.at(at);
  }
  if(matrixScope && asynchronous && at==0xffb20040){doorbellRead=true;return unsigned(pending);}
  return PreciseBetaPeer::Load(at);
 }
 void Store(uint32_t at,uint32_t value)override {
  if(matrixScope && RawRange(at))throw PrefetchHazard{}; // No host/raw staging writes.
  // Only matrix/fetch read-command stores; preserve the distinct real QGW3
  // payload+commit endpoint and normalizations owned by unchanged helpers.
  if(matrixScope && (Range(pc,0x72000,unsigned(code.size())) || Range(pc,0x70000,unsigned(fetch.size()))) && Range(at,0xffb20000,40)){
   assert(!pending && !rawPending && phaseQueue.empty() && !(at&3));unsigned w=(at-0xffb20000)/4;
   assert(w==0 || w==2 || w==3 || w==8);
   if(!w){restoring=requests>inputReads && !((requests-inputReads)%Blocks()) && value==packets.at(requests-1).address;if(restoring)restoreCommands++;}
   assert(restoring || requests<packets.size());const auto& p=packets.at(restoring?requests-1:requests);
   assert(value==(w==0?p.address:w==2?p.route:w==3?(restoring?0x90000:p.destination):p.bytes));
   command[w]=value;readback[w]=false;return;
  }
  if(matrixScope && at==0xffb20040 && !command[7])assert(!restoring && !rawPending && phaseQueue.empty());
  PreciseBetaPeer::Store(at,value);
  if(matrixScope && asynchronous && at==0xffb20040 && !command[7]){assert(pending);readyAt=uint64_t(steps)+latency;}
 }
 void CheckMatrixEnd() {
  if(!(CorrectRows() && !pending && !rawPending && phaseQueue.empty() && pendingAccumulator.empty()
   && restoreCommands==FfnPersistentPeer::groups && stageCalls==tiles.size() && metadataReads
   && requests==ExpectedReadPackets() && responses==requests && !dotProbeReads && !stageReturn))
   fprintf(stderr,"TOKEN_PREFETCH_END_DIAGNOSTIC type%u async%u latency%u between%u correct%u pending%u rawPending%u queued%zu accumulator%zu restore%u/%u stages%u/%zu metadata%u overlap%u reads%u/%u responses%u probes%u return%x\n",
    type,unsigned(asynchronous),latency,unsigned(betweenPanels),unsigned(CorrectRows()),unsigned(pending),unsigned(rawPending),phaseQueue.size(),pendingAccumulator.size(),
    restoreCommands,FfnPersistentPeer::groups,stageCalls,tiles.size(),metadataReads,overlappedOps,requests,ExpectedReadPackets(),responses,dotProbeReads,stageReturn);
  assert(CorrectRows() && !pending && !rawPending && phaseQueue.empty() && pendingAccumulator.empty()
   && restoreCommands==FfnPersistentPeer::groups && stageCalls==tiles.size() && metadataReads
   && requests==ExpectedReadPackets() && responses==requests && !dotProbeReads && !stageReturn);
  // A fast asynchronous reply can legitimately finish before the first
  // Tensix operation (Q4/1024). Correctness must still hold. Explicitly
  // held and between-panel schedules MUST witness real pending compute.
  if(!asynchronous || betweenPanels)assert(overlappedOps);
  matrixScope=false;
 }
 void ReadFault() {
  while(pc!=faultPC && pc!=stopPC)StepBlock();
  assert(pc==faultPC && pending && requests==faultAt && responses==faultAt-1 && !pushes && !heldPayload
   && !mailbox[0] && !memory.at(M::Status+4) && memory.at(M::Status+8)==Push::Fault);
  auto data=memory;unsigned issued=requests,completed=responses,math=matrixInstructions;
  for(unsigned i=0;i<2048;i++)StepBlock();
  assert(pc==faultPC && memory==data && requests==issued && responses==completed && matrixInstructions==math && pending && !pushes);
 }
};
#ifndef PIONEER_Q38_TOKEN_PREFETCH_WORKER_MACHINE_ONLY
int main(int argc,char** argv){
 assert(argc==4 || (argc==5 && (!strcmp(argv[4],"--matrix-pilot") || !strcmp(argv[4],"--async-pilot"))));setvbuf(stdout,nullptr,_IOLBF,0);
 bool asyncPilot=argc==5 && !strcmp(argv[4],"--async-pilot");
 (void)&RunPersistentGdnPair;unsigned cases=0,stack=0;
 for(unsigned type:{8U,12U,14U})for(unsigned k:{5120U,6144U,17408U})for(unsigned groups:{1U,3U,16U})for(unsigned trial=0;trial<2;trial++){
  if(asyncPilot || (argc==5 && (k!=5120 || groups!=1 || trial)))continue;
  unsigned worker=trial?119:69;
  printf("TOKEN_PREFETCH_ACTUAL_CASE type%u K%u groups%u trial%u worker%u\n",type,k,groups,trial,worker);
  auto old=std::make_unique<PreciseBetaPeer>(argv[1],argv[2],worker,type,k,groups);
  auto p=std::make_unique<TokenPrefetchPeer>(argv[1],argv[3],worker,type,k,groups);
  if(trial){FreshInputs(*old);FreshInputs(*p);}
  old->delay=p->delay=3;old->BootPushed();p->BootPushed();old->PoisonTail();p->Prepare();
  old->StagePushed();p->StagePushed();old->AwaitPayload();p->AwaitPayload();
  assert(old->payload==p->payload && old->mailbox==p->mailbox && old->root==p->root);
  old->FinishPushed();p->FinishPushed();p->CheckMatrixEnd();
  for(unsigned w=0;w<10;w++)assert(old->command[w]==p->command[w]);
  for(unsigned i=0;i<8;i++)assert(old->Count(i)==p->Count(i));
  for(unsigned row=0;row<groups*16;row++)assert(old->buffers[4096+row]==p->buffers[4096+row]);
  assert(old->matrixInstructions==p->matrixInstructions && old->rawReads==p->rawReads && old->vectorOps==p->vectorOps
   && old->sumWrites==p->sumWrites && old->sumReads==p->sumReads && old->drains==p->drains);
  old->StopLocal();p->StopLocal();stack=std::max(stack,0x70000-p->minStack);cases++;
 }
 if(argc==5 && !asyncPilot){printf("PASS_PARTIAL_TOKEN_PREFETCH_MATRIX_PILOT cases%u maxCombinedStack%u/8192;FULL_EXECUTION_SUITE_SKIPPED_NO_NATIVE_OR_TPS\n",cases,stack);return 0;}
 if(!asyncPilot)printf("PASS_TOKEN_PREFETCH_MATRIX_SHAPES cases%u maxCombinedStack%u/8192;FRESH_VIRGIN_WORKERS_NOT_RETAINED_SECOND_MATRIX_JOB_NO_NATIVE_OR_TPS\n",cases,stack);
 unsigned readFaults=0,mutants=0,async=0,sourceFaults=0,pairs=0;
 for(unsigned type:{8U,12U,14U})for(unsigned which:{0U,1U,2U,3U}){
  auto p=std::make_unique<TokenPrefetchPeer>(argv[1],argv[3],69,type,5120,3);p->delay=3;p->BootPushed();p->Prepare();p->StagePushed();
  p->faultAt=which==0?1:which==1?p->inputReads+1:which==2?p->inputReads+unsigned(p->tiles.size()/2):unsigned(p->packets.size());
  p->ReadFault();readFaults++;
 }
 for(unsigned type:{8U,12U,14U})for(unsigned which:{1U,2U,3U}){
  auto p=std::make_unique<TokenPrefetchPeer>(argv[1],argv[3],69,type);p->delay=3;p->BootPushed();p->Prepare();p->StagePushed();
  p->skipWaitAt=which<3?which:p->Blocks();bool refused=false;
  try{while(!p->heldPayload && p->pc!=p->faultPC)p->StepBlock();}catch(const TokenPrefetchPeer::PrefetchHazard&){refused=true;}
  assert((refused || p->pc==p->faultPC) && p->pending && !p->pushes && !p->mailbox[0] && !p->memory.at(M::Status+4));mutants++;
 }
 for(unsigned type:{8U,12U,14U}){
  auto p=std::make_unique<TokenPrefetchPeer>(argv[1],argv[3],69,type);p->delay=3;p->BootPushed();p->Prepare();p->StagePushed();
  p->wrongBufferAt=1;bool refused=false;
  try{while(!p->heldPayload && p->pc!=p->faultPC)p->StepBlock();}catch(const TokenPrefetchPeer::PrefetchHazard&){refused=true;}
  assert(refused && p->pending && !p->pushes && !p->mailbox[0] && !p->memory.at(M::Status+4));mutants++;
 }
 for(unsigned type:{8U,12U,14U})for(unsigned schedule:{0U,1U,2U,3U}){
  auto p=std::make_unique<TokenPrefetchPeer>(argv[1],argv[3],69,type);
  p->asynchronous=true;p->latency=schedule==0?1024:schedule==1?4099:schedule==2?65537:0;
  printf("TOKEN_PREFETCH_ASYNC_CASE type%u schedule%u latency%u\n",type,schedule,p->latency);
  p->betweenPanels=schedule==3;p->BootPushed();p->Prepare();p->StagePushed();p->AwaitPayload();p->FinishPushed();p->CheckMatrixEnd();
  if(p->betweenPanels)assert(p->retiredDuringMath);p->StopLocal();async++;
  printf("PASS_TOKEN_PREFETCH_ASYNC type%u schedule%u pendingTensorOps%u pendingMetadataReads%u retiredBetweenMatrices%u;NOT_SILICON_LATENCY_OR_TPS\n",type,schedule,p->overlappedOps,p->overlappedMetadataReads,p->retiredDuringMath);
 }
 if(asyncPilot){printf("PASS_PARTIAL_TOKEN_PREFETCH_ASYNC_PILOT readFaults%u waitBufferMutants%u asyncSchedules%u;FULL_SHAPES_GDN_SUITE_SKIPPED_NO_NATIVE_OR_TPS\n",readFaults,mutants,async);return 0;}
 for(unsigned worker:{1U,96U,119U}){
  auto p=std::make_unique<TokenPrefetchPeer>(argv[1],argv[3],worker);p->delay=3;p->BootPushed();p->Prepare();p->StagePushed();p->AwaitPayload();
  for(unsigned branch=0;branch<18;branch++){
   fflush(nullptr);const pid_t pid=fork();assert(pid>=0);
   if(!pid){if(branch<16)p->FaultAfterPayload(branch);else if(branch==16)p->TimeoutPayload();else p->FinishPushed(true);_exit(0);}
   int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));
   if(branch<16)sourceFaults++;
  }
  p->FinishPushed();p->CheckMatrixEnd();p->StopLocal();
 }
 for(unsigned head:{0U,31U,47U}){
  auto a=std::make_unique<TokenPrefetchPeer>(argv[1],argv[3],head*2+1);
  auto b=std::make_unique<TokenPrefetchPeer>(argv[1],argv[3],head*2+2);a->peer=b.get();b->peer=a.get();
  for(auto* p:{a.get(),b.get()}){p->delay=3;p->BootPushed();p->Prepare();p->StagePushed();p->AwaitPayload();p->FinishPushed();p->CheckMatrixEnd();p->StageInput(1002,1,0xc09dde93);}
  assert(RunPersistentGdnPair(*a,*b,2));a->CheckPreciseBeta();b->CheckPreciseBeta();
  for(auto* p:{a.get(),b.get()}){p->prior=p->next;p->StageInput(1003,2,0x3f81fec2);}
  assert(RunPersistentGdnPair(*a,*b,3));a->CheckPreciseBeta();b->CheckPreciseBeta();
  for(auto* p:{a.get(),b.get()}){p->StopLocal();stack=std::max(stack,0x70000-p->minStack);}pairs++;
 }
 printf("PASS_TOKEN_PREFETCH_EXECUTION shapes%u readFaults%u waitBufferMutants%u asyncSchedules%u sourceFaults%u GdnPairs%u maxCombinedStack%u/8192;SYNTHETIC_ENDPOINTS_NOT_DENSE_GLOBAL_OUTPUT_ALL119_NATIVE_MODEL_QUALITY_WORK_OR_TPS\n",cases,readFaults,mutants,async,sourceFaults,pairs,stack);
}
#endif
