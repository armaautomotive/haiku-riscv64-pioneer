/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_AFFINE_REPLAY_IMAGE_MODEL_ONLY
#include "pioneer_q38_affine_replay_image_test.cpp"
struct DirectSrcBImage:ReplayImage {
 using ReplayImage::ReplayImage;
 struct InvalidTail {};
 unsigned paddingWrites=0,directReads=0,baseWrites=0,copyWrites=0;
 bool omitPadding=false;
 bool Active()const{return memory.at(0x74038)==1;}
 uint32_t SourceBPayloadBase()const override {
  return Active()?0xa0000+(matrixInstructions/MatrixPhasesPerPanel()%(inputs/16))*32:0x70920;
 }
 uint32_t SourceBOperandWord(uint32_t address)override {
  if(!Active())return ReplayImage::SourceBOperandWord(address);
  assert(!(address&3) && Range(address,SourceBPayloadBase(),256) && Range(address,0xa0000,inputs*2+224));
  if(paddingWrites!=56 || !memory.count(address))throw InvalidTail{};
  uint32_t word=memory.at(address);
  if(address>=0xa0000+inputs*2){if(word)throw InvalidTail{};}
  else assert(word==root.at(address) && PioneerInt8OperandValid(uint16_t(word)) && PioneerInt8OperandValid(uint16_t(word>>16)));
  directReads++;return word;
 }
 void PoisonTail() {
  assert(!pending && phaseQueue.empty() && pendingAccumulator.empty() && !rawPending);
  paddingWrites=directReads=baseWrites=copyWrites=0;
  for(unsigned word=0;word<56;word++)memory[0xa0000+inputs*2+word*4]=0x7fc17fc1;
  memory[0xa0000+inputs*2+224]=0xdec0adfe;
 }
 void Store(uint32_t address,uint32_t value)override {
  if(Range(address,0xa0000+inputs*2,224)) {
   assert(Active() && !pending && drained && pendingAccumulator.empty() && !rawPending
    && phaseQueue.empty() && !recordRemaining && !value && address==0xa0000+inputs*2+paddingWrites*4);
   paddingWrites++;if(!omitPadding)memory[address]=value;return;
  }
  if(Active() && address==0xffef0000+124*4 && setupConfiguration==25) {
   assert(drained && pendingAccumulator.empty() && !pending && !rawPending && phaseQueue.empty()
    && !recordRemaining && value==(SourceBPayloadBase()-16)/16);baseWrites++;
  }
  if(Range(address,0x70920,32))copyWrites++;
  ReplayImage::Store(address,value);
 }
 uint32_t Load(uint32_t address)override {
  if(Range(address,0xa0000+inputs*2,224)) {
   assert(Active() && paddingWrites==56 && memory.count(address));return memory.at(address);
  }
  return ReplayImage::Load(address);
 }
};
#ifndef PIONEER_Q38_DIRECT_SRCB_IMAGE_MODEL_ONLY
int main(int argc,char** argv) {
 assert(argc==3 || (argc==4 && !strcmp(argv[3],"--work-pilot")));setvbuf(stdout,nullptr,_IOLBF,0);
 for(unsigned type:{8U,12U,14U})for(unsigned k:{5120U,6144U,17408U})
 for(unsigned groups:{1U,3U,16U})for(bool work:{false,true}) {
  if(argc==4 && (k!=5120 || groups!=1 || !work))continue;
  printf("DIRECT_SRCB_CASE type%u K%u groups%u work%u\n",type,k,groups,unsigned(work));
  ReplayImage old(argv[1],type,k,groups);DirectSrcBImage next(argv[2],type,k,groups);
  assert(old.Boot() && next.Boot());old.memory[0x74038]=next.memory[0x74038]=unsigned(work);
  for(unsigned job=1;job<=2;job++) {
   if(job==2){old.NextPackets();next.NextPackets();FreshInputs(old);FreshInputs(next);}
   old.Poison();next.Poison();next.PoisonTail();unsigned before=old.steps,after=next.steps;
   unsigned vectorBefore=old.vectorOps,vectorAfter=next.vectorOps,drainsBefore=old.drains,drainsAfter=next.drains;
   assert(old.Job(job) && next.Job(job));
   assert(old.requests==next.requests && old.responses==next.responses && old.matrixInstructions==next.matrixInstructions
    && old.sumWrites==next.sumWrites && old.sumReads==next.sumReads && old.dotProbeReads==next.dotProbeReads
    && old.rawReads==next.rawReads && old.drains-drainsBefore==next.drains-drainsAfter
    && old.vectorOps-vectorBefore==next.vectorOps-vectorAfter);
   assert(next.paddingWrites==(work?56U:0U) && next.baseWrites==(work?groups*(k/16):0U));
   assert(next.directReads==(work?groups*(k/16)*128:0U));
   // Original Setup zeroing remains;NONE of the hot per-panel copies do.
   assert(next.copyWrites==(work?8U:8U+groups*(k/16)*8));
   assert(next.memory.at(0xa0000+k*2+224)==0xdec0adfe);
   for(unsigned row=0;row<groups*16;row++)assert(old.buffers[4096+row]==next.buffers[4096+row]);
   if(work && groups==16 && job==2)printf("OFFLINE_DIRECT_SRCB type%u K%u RV%u/%u;EXACT_ORIGINAL_OUTPUT_NOT_NATIVE_OR_TOKEN_RATE\n",type,k,old.steps-before,next.steps-after);
  }
  old.memory[0x74038]=next.memory[0x74038]=0;assert(old.Job(3,2) && next.Job(3,2));
 }
 for(unsigned type:{8U,12U,14U}) {
  DirectSrcBImage bad(argv[2],type,5120,1);assert(bad.Boot());bad.memory[0x74038]=1;bad.Poison();bad.PoisonTail();bad.omitPadding=true;
  bool caught=false;try{(void)bad.Job(1);}catch(const DirectSrcBImage::InvalidTail&){caught=true;}
  assert(caught && !bad.outputReads && !bad.memory.at(0x74144));
 }
 for(unsigned fault:{1U,5U,40U,208U}) {
  DirectSrcBImage bad(argv[2],12,17408,3);assert(bad.Boot());bad.memory[0x74038]=1;bad.Poison();bad.PoisonTail();bad.faultAt=fault;
  assert(!bad.Job(1) && bad.pending && bad.requests==fault && bad.responses==fault-1 && !bad.memory.at(0x74144));
  unsigned requests=bad.requests;for(unsigned step=0;step<1000;step++)bad.StepBlock();assert(bad.requests==requests);
 }
 puts(argc==4?"PASS PARTIAL ONLY direct-SrcB Work pilots/faults;FULL SUITE SKIPPED":"PASS OFFLINE direct-SrcB actual image:all types/widths/groups/modes/two changed-input jobs,original output bit exact+independent FP64,ALL128 UNPACK lanes bounded/fresh,224B tail overwritten each job,config only after existing drain,NO hot copy/extra drains/NoC/owner range;NO NATIVE OR SPEED QUALIFICATION");
}
#endif
