/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_DIRECT_SRCB_IMAGE_MODEL_ONLY
#include "pioneer_q38_direct_srcb_image_test.cpp"
// Only the NIU fabric is simulated. Next-buffer bytes stay POISONED until
// its actual response becomes visible;arithmetic may use ONLY prior data.
struct TilePrefetchImage:DirectSrcBImage{
 std::vector<uint8_t> fetchCode;std::vector<Packet> originalPackets;
 bool restoring=false;unsigned overlappedOps=0,metadataReads=0,restoreCommands=0;
 mutable unsigned validatedTile=UINT32_MAX;
 unsigned skipWaitAt=0,waitCalls=0,wrongBufferAt=0,stageCalls=0;
 uint32_t stageReturn=0;
 bool asynchronous=false,replyBetweenPanels=false;unsigned latency=4099,retiredDuringMath=0;uint64_t readyAt=0;
 struct PrefetchHazard{};
 TilePrefetchImage(const char* path,unsigned t,unsigned k,unsigned g,unsigned worker=69):DirectSrcBImage(path,t,k,g,worker){
  fetchCode=Read((std::string(path)+"/fetch.bin").c_str());assert(!fetchCode.empty()&&fetchCode.size()<=0x700&&!(fetchCode.size()&3));
  for(unsigned w=0;w<fetchCode.size()/4;w++)l1[Index(0x70000+w*4)]=Word(fetchCode,w*4);
  originalPackets=packets;
 }
 bool CodeAddress(uint32_t at)const override{return Range(at,0x70000,unsigned(fetchCode.size()))||DirectSrcBImage::CodeAddress(at);}
 void StepBlock()override{
  bool mathWindow=false;
  if(asynchronous&&pending&&packets.at(requests-1).tile>0){unsigned t=unsigned(packets.at(requests-1).tile),phases=MatrixPhasesPerPanel()*(type==8?2:16);
   mathWindow=matrixInstructions>(t-1)*phases&&matrixInstructions<t*phases;
  }
  if(asynchronous&&pending&&uint64_t(steps)>=readyAt&&requests!=faultAt
   &&(!replyBetweenPanels||packets.at(requests-1).tile<=0||mathWindow)){
   const auto& p=packets.at(requests-1);
   if(p.tile>0){unsigned phases=MatrixPhasesPerPanel()*(type==8?2:16);
    if(matrixInstructions>unsigned(p.tile-1)*phases&&matrixInstructions<unsigned(p.tile)*phases)retiredDuringMath++;
   }
   for(unsigned at=0;at<p.bytes;at+=4){uint32_t value=p.tile<0?root.at(p.address+at):Word(tiles.at(unsigned(p.tile)),at);memory[p.destination+at]=value;}
   pending=false;responses++; // actual data becomes visible ONLY now
  }
  // Negative controls on actual RV32 call/return,NOT fabricated replies.
  // These entry PCs are pinned by the exact fetch image SHA in runner.
  if(pc==0x70164&&++waitCalls==skipWaitAt){pc=regs[1];return;}
  if(pc==0x701c8){assert(!stageReturn);stageReturn=regs[1];stageCalls++;}
  DirectSrcBImage::StepBlock();
  if(stageReturn&&pc==stageReturn){stageReturn=0;if(stageCalls==wrongBufferAt)regs[10]^=0x8000;}
 }
 void Prepare(bool work){
  assert(!pending);memory[0x74038]=work;packets=originalPackets;restoring=false;validatedTile=UINT32_MAX;
  for(auto& p:packets)if(p.tile>=0)p.destination=work&&((unsigned(p.tile)%(inputs/(type==8?32:256)))&1)?0x98000:0x90000;
  Poison();PoisonTail();for(unsigned w=0;w<tile.size()/4;w++)memory[0x98000+w*4]=0x7fc00000;
 }
 bool RawRange(uint32_t at)const{return Range(at,0x90000,unsigned(tile.size()))||Range(at,0x98000,unsigned(tile.size()));}
 unsigned CurrentTile()const{return matrixInstructions/(MatrixPhasesPerPanel()*(type==8?2:16));}
 uint32_t RawInputBase()const override{
  if(!Active())return 0x90000;
  unsigned t=CurrentTile(),blocks=inputs/(type==8?32:256);assert(t<tiles.size());
  uint32_t raw=((t%blocks)&1)?0x98000:0x90000;
  if(responses<inputReads+t+1 || (pending&&packets.at(requests-1).destination==raw))throw PrefetchHazard{};
  if(validatedTile!=t){for(unsigned w=0;w<tile.size()/4;w++)assert(memory.at(raw+w*4)==Word(tiles.at(t),w*4));validatedTile=t;}
  return raw;
 }
 bool DisjointPending()const{
  return Active()&&pending&&requests>inputReads&&packets.at(requests-1).tile>=0
   &&(packets.at(requests-1).destination==0x90000||packets.at(requests-1).destination==0x98000);
 }
 bool LocalComputeRange(uint32_t at)const{
  return Range(at,0xffef0000,224*4)||Range(at,0x97000,0x400)||Range(at,0x95000,0x1100);
 }
 void Instruction(uint32_t op)override{
  if(!pending){DirectSrcBImage::Instruction(op);return;}
  if(!DisjointPending())throw PrefetchHazard{};
  // Replace ONLY the old conservative ALL-NIU-idle arithmetic premise.
  // Actual NIU state/counters are NEVER hidden from NIU Load/Store below.
  // Data operands remain independently bounded/defined;raw UNPACK checks
  // its completed opposite buffer through virtual RawInputBase.
  unsigned issued=requests,completed=responses;pending=false;
  try{DirectSrcBImage::Instruction(op);}catch(...){pending=true;throw;}
  pending=true;assert(requests==issued&&responses==completed);overlappedOps++;
 }
 uint32_t Load(uint32_t at)override{
  if(Range(at,0x70000,unsigned(fetchCode.size())))return l1[Index(at)];
  if(Active()&&RawRange(at)){
   uint32_t raw=RawInputBase();if(at<raw||at-raw+4>tile.size()||(at&3))throw PrefetchHazard{};
   metadataReads++;return memory.at(at);
  }
  if(pending&&LocalComputeRange(at)){
   if(!DisjointPending())throw PrefetchHazard{};
   pending=false;uint32_t result;
   try{result=DirectSrcBImage::Load(at);}catch(...){pending=true;throw;}
   pending=true;return result; // exact CSR/affine/sum view,NOT NIU destination
  }
  return DirectSrcBImage::Load(at); // NIU/status/response sees REAL pending
 }
 void Store(uint32_t at,uint32_t value)override{
  if(!Active()){DirectSrcBImage::Store(at,value);return;}
  if(RawRange(at))throw PrefetchHazard{}; // NO CPU raw copy/early write
  if(Range(at,0xffb20000,40)){
   assert(!pending&&!rawPending&&phaseQueue.empty());unsigned w=(at-0xffb20000)/4;
   assert(!(at&3)&&(w==0||w==2||w==3||w==8));
   unsigned blocks=inputs/(type==8?32:256);
   if(!w){
    restoring=requests>inputReads&&!((requests-inputReads)%blocks)&&value==packets.at(requests-1).address;
    if(restoring)restoreCommands++;
   }
   assert(restoring||requests<packets.size());const auto& p=packets.at(restoring?requests-1:requests);
   unsigned expected=w==0?p.address:w==2?p.route:w==3?(restoring?0x90000:p.destination):p.bytes;
   if(value!=expected)printf("PREFETCH_TEST_COMMAND_MISMATCH word%u value%x expected%x issued%u complete%u restoring%u active%u tile%d base%x\n",w,value,expected,requests,responses,unsigned(restoring),unsigned(Active()),p.tile,base);
   assert(value==expected);command[w]=value;readback[w]=false;return;
  }
  if(at==0xffb20040){assert(!restoring&&!rawPending&&phaseQueue.empty());}
  if(pending&&LocalComputeRange(at)){
   if(!DisjointPending())throw PrefetchHazard{};
   pending=false;
   try{DirectSrcBImage::Store(at,value);}catch(...){pending=true;throw;}
   pending=true;return; // retain EVERY original CSR/affine/drain/value observer
  }
  DirectSrcBImage::Store(at,value);
  if(asynchronous&&at==0xffb20040){assert(pending);readyAt=uint64_t(steps)+latency;}
 }
};
#ifndef PIONEER_Q38_TILE_PREFETCH_IMAGE_MODEL_ONLY
int main(int argc,char** argv){
 assert(argc==3||(argc==4&&!strcmp(argv[3],"--work-pilot")));setvbuf(stdout,nullptr,_IOLBF,0);
 for(unsigned type:{8U,12U,14U})for(unsigned k:{5120U,6144U,17408U})for(unsigned groups:{1U,3U,16U})for(bool work:{false,true}){
  if(argc==4&&(k!=5120||groups!=1||!work))continue;
  printf("TILE_PREFETCH_CASE type%u K%u groups%u work%u\n",type,k,groups,unsigned(work));
  DirectSrcBImage old(argv[1],type,k,groups);TilePrefetchImage next(argv[2],type,k,groups);
  old.delay=next.delay=3; // reply MUST remain pending beyond issue readback
  assert(old.Boot()&&next.Boot());old.memory[0x74038]=work;
  for(unsigned job=1;job<=2;job++){
   if(job==2){old.NextPackets();next.NextPackets();FreshInputs(old);FreshInputs(next);}
   old.Poison();old.PoisonTail();next.Prepare(work);assert(old.Job(job)&&next.Job(job));
   assert(old.requests==next.requests&&old.responses==next.responses&&old.matrixInstructions==next.matrixInstructions
    &&old.sumWrites==next.sumWrites&&old.sumReads==next.sumReads&&old.vectorOps==next.vectorOps&&old.drains==next.drains&&next.pendingAccumulator.empty());
   assert(next.restoreCommands==(work?job*groups:0));
   for(unsigned w=0;w<10;w++)assert(old.command[w]==next.command[w]);
   for(unsigned w=0;w<64;w++)assert(old.memory.at(0x83f00+w*4)==next.memory.at(0x83f00+w*4));
   for(unsigned row=0;row<groups*16;row++)assert(old.buffers[4096+row]==next.buffers[4096+row]);
  }
  if(work)assert(next.overlappedOps&&next.metadataReads);
  old.memory[0x74038]=next.memory[0x74038]=0;assert(old.Job(3,2)&&next.Job(3,2));
 }
 for(unsigned fault:{1U,5U,40U,208U}){
  TilePrefetchImage bad(argv[2],12,17408,3);assert(bad.Boot());bad.Prepare(true);bad.faultAt=fault;
  assert(!bad.Job(1)&&bad.pending&&bad.requests==fault&&bad.responses==fault-1&&!bad.memory.at(0x74144));
  unsigned requests=bad.requests;for(unsigned step=0;step<1000;step++)bad.StepBlock();assert(bad.requests==requests);
 }
 for(unsigned type:{8U,12U,14U})for(unsigned which:{1U,2U,3U}){
  TilePrefetchImage bad(argv[2],type,5120,1);bad.delay=3;assert(bad.Boot());bad.Prepare(true);
  bad.skipWaitAt=which<3?which:5120/(type==8?32:256);
  assert(!bad.Job(1)&&bad.pending&&!bad.outputReads&&!bad.memory.at(0x74144)&&bad.waitCalls==bad.skipWaitAt);
  unsigned requests=bad.requests;for(unsigned step=0;step<1000;step++)bad.StepBlock();assert(bad.requests==requests&&bad.pending);
 }
 for(unsigned type:{8U,12U,14U}){
  TilePrefetchImage bad(argv[2],type,5120,1);bad.delay=3;assert(bad.Boot());bad.Prepare(true);bad.wrongBufferAt=1;
  bool refused=false;try{(void)bad.Job(1);}catch(const TilePrefetchImage::PrefetchHazard&){refused=true;}
  assert(refused&&bad.pending&&!bad.outputReads&&!bad.memory.at(0x74144));
 }
 for(unsigned type:{8U,12U,14U}){
  for(unsigned latency:{1024U,4099U,65537U}){
   TilePrefetchImage m(argv[2],type,5120,1);m.asynchronous=true;m.latency=latency;m.delay=UINT32_MAX;
   assert(m.Boot());m.Prepare(true);assert(m.Job(1)&&m.CorrectRows()&&!m.pending&&m.pendingAccumulator.empty());
  }
  // Adversarial event schedule,not a fixed NoC-latency claim:hold the
  // NEXT reply until the first OLD-panel matrix retires,then deliver it
  // independently between later real RISC instructions. All3 fixed
  // delays above remain required even when they complete outside math.
  TilePrefetchImage m(argv[2],type,5120,1);m.asynchronous=true;m.replyBetweenPanels=true;m.latency=0;m.delay=UINT32_MAX;
  assert(m.Boot());m.Prepare(true);assert(m.Job(1)&&m.CorrectRows()&&m.retiredDuringMath&&!m.pending&&m.pendingAccumulator.empty());
 }
 puts(argc==4?"PASS PARTIAL tile prefetch Work pilots;FULL SUITE SKIPPED":"PASS OFFLINE ACTUAL one-tile prefetch:ALL54 shapes/2inputs+STOP/exact FP32+FP64/SAME64proofs/NoC bytes+counts/matrix/SFPU/drains/final command;raw buffer consumed ONLY after response while compute overlaps disjoint next NIU read;lost first/middle/last response parks inert,NO copy/extra packet/reset/retry;NO native/speed qualification");
}
#endif
