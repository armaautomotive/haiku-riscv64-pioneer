/* SPDX-License-Identifier: MIT */
// Actual frozen math+sender instructions,synthetic NoC endpoints ONLY.
// Expectations below are independent of the production batch-plan header.
#define PIONEER_Q38_PRECISE_BETA_WORKER_MACHINE_ONLY
#include "pioneer_q38_precise_beta_worker_test.cpp"
struct BatchedPrefetchPeer:PreciseBetaPeer {
 std::vector<uint8_t> fetch;
 bool matrixScope=false,asynchronous=false,restoring=false,betweenTiles=false;
 unsigned stageCalls=0,waitCalls=0,restoreCommands=0,copyReads=0,copyWrites=0;
 unsigned overlapOps=0,metadataReads=0,latency=4099,skipWaitAt=0,wrongBufferAt=0,retiredDuringMath=0;
 uint32_t stageReturn=0;uint64_t readyAt=0;
 mutable unsigned validatedTile=UINT32_MAX;
 static constexpr unsigned FetchWait=0x70000,FetchStage=0x70194;
 struct Hazard {};
 BatchedPrefetchPeer(const char* old,const char* image,unsigned worker,unsigned type=8,unsigned k=5120,unsigned groups=1)
  :PreciseBetaPeer(old,image,worker,type,k,groups),fetch(Read((std::string(image)+"/fetch.bin").c_str())) {
  assert(!steps && pc==0x6000 && fetch.size()==1260);
  auto original=packets;packets.resize(inputReads);
  for(unsigned t=0;t<tiles.size();t+=Batch()){
   const auto& p=original.at(inputReads+t);
   packets.push_back({p.address,p.route,0x98000,Batch()*unsigned(tile.size()),int(t)});
  }
 }
 unsigned Batch()const{return type==8?4:2;}
 unsigned Blocks()const{return inputs/(type==8?32:256);}
 unsigned TilePhases()const{return MatrixPhasesPerPanel()*(type==8?2:16);}
 unsigned ExpectedReadPackets()const override{return inputReads+unsigned(tiles.size())/Batch();}
 const std::vector<uint8_t>* Rom(unsigned at,unsigned& base)const override {
  if(Range(at,0x70000,unsigned(fetch.size()))){base=0x70000;return &fetch;}
  return PreciseBetaPeer::Rom(at,base);
 }
 unsigned CurrentTile()const {
  if(!matrixScope || !stageCalls || stageCalls>tiles.size())throw Hazard{};
  return stageCalls-1;
 }
 uint32_t CurrentRaw(unsigned t)const{return t%Batch()==Batch()-1?0x90000:0x98000+(t%Batch())*unsigned(tile.size());}
 bool RawRange(unsigned at)const{return Range(at,0x90000,unsigned(tile.size())) || Range(at,0x98000,Batch()*unsigned(tile.size()));}
 uint32_t RawInputBase()const override {
  if(!matrixScope)return PreciseBetaPeer::RawInputBase();
  unsigned t=CurrentTile();uint32_t raw=CurrentRaw(t);
  if(responses<inputReads+t/Batch()+1)throw Hazard{};
  if(pending && raw!=0x90000)throw Hazard{};
  if(validatedTile!=t){
   for(unsigned w=0;w<tile.size()/4;w++)if(memory.at(raw+w*4)!=Word(tiles.at(t),w*4))throw Hazard{};
   validatedTile=t;
  }
  return raw;
 }
 bool DisjointPrefetchCompute()const override {
  if(!matrixScope || !pending || !stageCalls || stageReturn || requests<=inputReads)return false;
  unsigned t=CurrentTile();const auto& p=packets.at(requests-1);
  return t%Batch()==Batch()-1 && t+1<tiles.size() && (t+1)%Blocks()!=0
   && p.tile==int(t+1) && p.destination==0x98000 && p.bytes==Batch()*tile.size()
   && requests==inputReads+(t+1)/Batch()+1 && responses==requests-1
   && matrixInstructions>=t*TilePhases() && matrixInstructions<=(t+1)*TilePhases()
   && RawInputBase()==0x90000;
 }
 void Prepare(){
  assert(pc==waitPC && !pending && !stageCalls);matrixScope=true;
  Poison();PoisonTail();
  for(unsigned at=0;at<Batch()*tile.size();at+=4)memory[0x98000+at]=0x7fc17fc1;
  for(unsigned at=0;at<tile.size();at+=4)memory[0x90000+at]=0x7fc17fc1;
  memory[0x97ffc]=memory[0x99a80]=0xdec0adfe;
 }
 void CompleteRead(){
  assert(pending && requests && requests!=faultAt);const auto& p=packets.at(requests-1);
  for(unsigned at=0;at<p.bytes;at+=4)memory[p.destination+at]=p.tile<0?root.at(p.address+at)
   :Word(tiles.at(unsigned(p.tile)+at/tile.size()),at%tile.size());
  pending=false;responses++;
 }
 void StepBlock()override {
  if(matrixScope && asynchronous && pending && uint64_t(steps)>=readyAt && requests!=faultAt){
   const auto& p=packets.at(requests-1);
   bool math=p.tile>0 && matrixInstructions>unsigned(p.tile-1)*TilePhases() && matrixInstructions<unsigned(p.tile)*TilePhases();
   if(!betweenTiles || p.tile<=0 || math){if(math)retiredDuringMath++;CompleteRead();}
  }
  // Negative controls ONLY;positive runs never alter PC/registers/results.
  if(matrixScope && pc==FetchWait && ++waitCalls==skipWaitAt){pc=regs[1];return;}
  if(matrixScope && pc==FetchStage){assert(!stageReturn);stageReturn=regs[1];stageCalls++;}
  PreciseBetaPeer::StepBlock();
  if(stageReturn && pc==stageReturn){
   stageReturn=0;if(stageCalls==wrongBufferAt)regs[10]^=0x8000;
   if(regs[10]!=RawInputBase())throw Hazard{};
  }
 }
 void Instruction(uint32_t op)override {
  if(matrixScope && pending){if(!DisjointPrefetchCompute())throw Hazard{};overlapOps++;}
  PreciseBetaPeer::Instruction(op);
 }
 uint32_t Load(uint32_t at)override {
  if(matrixScope && RawRange(at)){
   if(stageReturn && Range(pc,0x70000,unsigned(fetch.size())) && Range(at,0x98000,Batch()*unsigned(tile.size()))){
    unsigned t=CurrentTile(),w=copyReads%(tile.size()/4);
    if(pending || rawPending || !phaseQueue.empty() || !pendingAccumulator.empty() || t%Batch()!=Batch()-1
     || at!=0x98000+(t%Batch())*tile.size()+w*4 || memory.at(at)!=Word(tiles.at(t),w*4))throw Hazard{};
    copyReads++;return memory.at(at);
   }
   uint32_t raw=RawInputBase();if(at<raw || at-raw+4>tile.size() || (at&3))throw Hazard{};
   metadataReads++;return memory.at(at);
  }
  if(matrixScope && at==0xffb20040){
   doorbellRead=true;
   if(!asynchronous && pending && ++waits>delay && requests!=faultAt)CompleteRead();
   return unsigned(pending);
  }
  return PreciseBetaPeer::Load(at);
 }
 void Store(uint32_t at,uint32_t value)override {
  if(matrixScope && RawRange(at)){
   unsigned t=CurrentTile(),w=copyWrites%(tile.size()/4);
   if(!stageReturn || !Range(pc,0x70000,unsigned(fetch.size())) || t%Batch()!=Batch()-1 || pending
    || rawPending || !phaseQueue.empty() || !pendingAccumulator.empty() || copyReads!=copyWrites+1
    || at!=0x90000+w*4 || value!=Word(tiles.at(t),w*4))throw Hazard{};
   memory[at]=value;copyWrites++;return;
  }
  if(matrixScope && (Range(pc,0x72000,unsigned(code.size())) || Range(pc,0x70000,unsigned(fetch.size()))) && Range(at,0xffb20000,40)){
   assert(!pending && !rawPending && phaseQueue.empty() && !(at&3));unsigned w=(at-0xffb20000)/4;
   assert(w==0 || w==2 || w==3 || w==8);
   if(!w){restoring=stageCalls && !(stageCalls%Blocks()) && requests>inputReads && !((requests-inputReads)%(Blocks()/Batch()))
     && value==base+stageCalls*unsigned(tile.size())-tile.size();if(restoring)restoreCommands++;}
   assert(restoring || requests<packets.size());const auto& p=packets.at(restoring?requests-1:requests);
   // The unchanged initial single-tile frame is only staged,NEVER issued.
   bool initial=requests==inputReads && !stageCalls;
   unsigned wanted=w==0?(restoring?base+stageCalls*unsigned(tile.size())-tile.size():p.address)
    :w==2?p.route:w==3?(restoring || initial?0x90000:p.destination):(restoring || initial?unsigned(tile.size()):p.bytes);
   if(value!=wanted)fprintf(stderr,"BATCHED_COMMAND_REFUSAL type%u K%u groups%u stage%u read%u word%u actual%x expected%x restoring%u initial%u\n",type,inputs,FfnPersistentPeer::groups,stageCalls,requests,w,value,wanted,unsigned(restoring),unsigned(initial));
   assert(value==wanted);command[w]=value;readback[w]=false;return;
  }
  if(matrixScope && at==0xffb20040 && !command[7])assert(!restoring && !rawPending && phaseQueue.empty());
  PreciseBetaPeer::Store(at,value);
  if(matrixScope && at==0xffb20040 && !command[7] && asynchronous)readyAt=uint64_t(steps)+latency;
 }
 void CheckEnd(){
  assert(CorrectRows() && !pending && !rawPending && phaseQueue.empty() && pendingAccumulator.empty()
   && restoreCommands==FfnPersistentPeer::groups && stageCalls==tiles.size() && metadataReads && !stageReturn
   && requests==ExpectedReadPackets() && responses==requests && !dotProbeReads
   && copyReads==copyWrites && copyWrites==tiles.size()/Batch()*(tile.size()/4)
   && memory.at(0x97ffc)==0xdec0adfe && memory.at(0x99a80)==0xdec0adfe);
  if(!asynchronous)assert(overlapOps);
  matrixScope=false;
 }
 void ReadFault(){
  while(pc!=faultPC && pc!=stopPC)StepBlock();
  assert(pc==faultPC && pending && requests==faultAt && responses==faultAt-1 && !pushes && !heldPayload
   && !mailbox[0] && !memory.at(M::Status+4) && memory.at(M::Status+8)==Push::Fault);
  auto data=memory;unsigned issued=requests,completed=responses,math=matrixInstructions,copies=copyWrites;
  for(unsigned i=0;i<2048;i++)StepBlock();
  assert(pc==faultPC && memory==data && requests==issued && responses==completed && matrixInstructions==math
   && copyWrites==copies && pending && !pushes);
 }
};
#ifndef PIONEER_Q38_BATCHED_PREFETCH_WORKER_MACHINE_ONLY
int main(int argc,char** argv){
 assert(argc==4 || (argc==5 && !strcmp(argv[4],"--matrix-pilot")));setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&RunPersistentGdnPair;unsigned cases=0,stack=0;
 for(unsigned type:{8U,12U,14U})for(unsigned k:{5120U,6144U,17408U})for(unsigned groups:{1U,3U,16U})for(unsigned trial=0;trial<2;trial++){
  if(argc==5 && (k!=5120 || groups!=1 || trial))continue;
  printf("BATCHED_PREFETCH_CASE type%u K%u groups%u trial%u\n",type,k,groups,trial);
  auto old=std::make_unique<PreciseBetaPeer>(argv[1],argv[2],trial?119:69,type,k,groups);
  auto p=std::make_unique<BatchedPrefetchPeer>(argv[1],argv[3],trial?119:69,type,k,groups);
  if(trial){FreshInputs(*old);FreshInputs(*p);}
  old->delay=p->delay=3;old->BootPushed();p->BootPushed();old->PoisonTail();p->Prepare();
  old->StagePushed();p->StagePushed();old->AwaitPayload();p->AwaitPayload();old->FinishPushed();p->FinishPushed();p->CheckEnd();
  for(unsigned row=0;row<groups*16;row++)assert(old->buffers[4096+row]==p->buffers[4096+row]);
  assert(old->matrixInstructions==p->matrixInstructions && old->rawReads==p->rawReads && old->vectorOps==p->vectorOps
   && old->sumWrites==p->sumWrites && old->sumReads==p->sumReads);
  assert(p->drains==old->drains+p->tiles.size()/p->Batch());
  for(unsigned w=0;w<10;w++)assert(old->command[w]==p->command[w]);
  assert((old->requests-old->inputReads)==(p->requests-p->inputReads)*p->Batch());
  for(unsigned i=0;i<8;i++)if(i==1 || i==4 || i==5 || i==7)assert(old->Count(i)==p->Count(i));
  old->StopLocal();p->StopLocal();stack=std::max(stack,0x70000-p->minStack);cases++;
 }
 if(argc==5){printf("PASS_PARTIAL_BATCHED_PREFETCH_EXECUTION shapes%u maxCombinedStack%u/8192;NO_NATIVE_MODEL_WORK_OR_TPS\n",cases,stack);return 0;}
 unsigned faults=0,mutants=0,async=0,sourceFaults=0,pairs=0;
 for(unsigned type:{8U,12U,14U})for(unsigned which:{0U,1U,2U,3U}){
  auto p=std::make_unique<BatchedPrefetchPeer>(argv[1],argv[3],69,type,5120,3);p->delay=3;p->BootPushed();p->Prepare();p->StagePushed();
  p->faultAt=which==0?1:which==1?p->inputReads+1:which==2?p->inputReads+unsigned(p->tiles.size()/p->Batch()/2):unsigned(p->packets.size());
  p->ReadFault();faults++;
 }
 for(unsigned type:{8U,12U,14U})for(unsigned which:{1U,2U,3U,4U}){
  auto p=std::make_unique<BatchedPrefetchPeer>(argv[1],argv[3],69,type);p->delay=3;p->BootPushed();p->Prepare();p->StagePushed();
  if(which==4)p->wrongBufferAt=1;else p->skipWaitAt=which<3?which:p->Blocks()/p->Batch();
  bool refused=false;try{while(!p->heldPayload && p->pc!=p->faultPC)p->StepBlock();}catch(const BatchedPrefetchPeer::Hazard&){refused=true;}
  assert((refused || p->pc==p->faultPC) && !p->pushes && !p->mailbox[0] && !p->memory.at(M::Status+4));
  if(which==4)assert(refused && !p->pending && p->requests==p->responses && p->stageCalls==1
   && !p->matrixInstructions && !p->copyWrites); // first batch already completed,NO next read until last tile
  else assert(p->pending && p->responses+1==p->requests);
  mutants++;
 }
 for(unsigned type:{8U,12U,14U})for(unsigned schedule:{0U,1U,2U,3U}){
  auto p=std::make_unique<BatchedPrefetchPeer>(argv[1],argv[3],69,type);
  p->asynchronous=true;p->latency=schedule==0?1024:schedule==1?4099:schedule==2?65537:0;p->betweenTiles=schedule==3;
  printf("BATCHED_PREFETCH_ASYNC type%u schedule%u latency%u\n",type,schedule,p->latency);
  p->BootPushed();p->Prepare();p->StagePushed();p->AwaitPayload();p->FinishPushed();p->CheckEnd();
  if(p->betweenTiles)assert(p->retiredDuringMath);p->StopLocal();async++;
 }
 for(unsigned worker:{1U,96U,119U}){
  auto p=std::make_unique<BatchedPrefetchPeer>(argv[1],argv[3],worker);p->delay=3;p->BootPushed();p->Prepare();p->StagePushed();p->AwaitPayload();
  for(unsigned branch=0;branch<18;branch++){
   fflush(nullptr);const pid_t pid=fork();assert(pid>=0);
   if(!pid){if(branch<16)p->FaultAfterPayload(branch);else if(branch==16)p->TimeoutPayload();else p->FinishPushed(true);_exit(0);}
   int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));if(branch<16)sourceFaults++;
  }
  p->FinishPushed();p->CheckEnd();p->StopLocal();
 }
 for(unsigned head:{0U,31U,47U}){
  auto a=std::make_unique<BatchedPrefetchPeer>(argv[1],argv[3],head*2+1);
  auto b=std::make_unique<BatchedPrefetchPeer>(argv[1],argv[3],head*2+2);a->peer=b.get();b->peer=a.get();
  for(auto* p:{a.get(),b.get()}){p->delay=3;p->BootPushed();p->Prepare();p->StagePushed();p->AwaitPayload();p->FinishPushed();p->CheckEnd();p->StageInput(1002,1,0xc09dde93);}
  assert(RunPersistentGdnPair(*a,*b,2));a->CheckPreciseBeta();b->CheckPreciseBeta();
  for(auto* p:{a.get(),b.get()}){p->prior=p->next;p->StageInput(1003,2,0x3f81fec2);}
  assert(RunPersistentGdnPair(*a,*b,3));a->CheckPreciseBeta();b->CheckPreciseBeta();
  for(auto* p:{a.get(),b.get()}){p->StopLocal();stack=std::max(stack,0x70000-p->minStack);}pairs++;
 }
 printf("PASS_BATCHED_PREFETCH_EXECUTION shapes%u readFaults%u waitBufferMutants%u asyncSchedules%u sourceFaults%u GdnPairs%u maxCombinedStack%u/8192;EXACT_FP32+FP64/same weight bytes/fewer read commands/last-tile-only copy/actual drain+sender+ACK+commit+STOP;SYNTHETIC_ENDPOINTS_NOT_DENSE_GLOBAL_OUTPUT_ALL119_NATIVE_MODEL_WORK_OR_TPS\n",cases,faults,mutants,async,sourceFaults,pairs,stack);
}
#endif
