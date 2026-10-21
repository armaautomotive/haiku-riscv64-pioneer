/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_PREDECODED_Q4_IMAGE_MODEL_ONLY
#include "pioneer_q38_predecoded_q4_image_test.cpp"
struct StickyQ4Image:PredecodedQ4Image {
 bool freshSetup=false;
 unsigned benignFirstRestores=0;
 using PredecodedQ4Image::PredecodedQ4Image;
 bool DirectNativeQ8Sfpu()const override{return direct;} // SAME native SM7 SrcA/SFPU format contract
 void Instruction(uint32_t op)override {
  if(direct && op==0xb2000000)freshSetup=true;
  PredecodedQ4Image::Instruction(op);
 }
 void Store(uint32_t at,uint32_t value)override {
  if(direct && at==0xffef0000 && freshSetup) {
   assert(!directPending && !rawPending && phaseQueue.empty() && pendingAccumulator.empty() && drained);
   freshSetup=false;DeferredAffineImage::Store(at,value);return; // independently require ALL25 original startup pairs
  }
  if(direct && at==0xffef0000 && value==0x4231 && config[0]==0x4231 && setupConfiguration==25) {
   // Once at the FIRST block:write/read-back of the already active format.
   // This is NOT a new startup;no source,queue or pending-store ownership.
   assert(drained && !directPending && !rawPending && phaseQueue.empty() && pendingAccumulator.empty() && !ownerA && !ownerB);
   benignFirstRestores++;configWrites++;return;
  }
  PredecodedQ4Image::Store(at,value);
 }
};
int main(int argc,char** argv) {
 assert(argc==3 || (argc==4 && !strcmp(argv[3],"--q4-work-pilot")));setvbuf(stdout,nullptr,_IOLBF,0);
 for(unsigned type:{8U,12U,14U})for(unsigned k:{5120U,6144U,17408U})
 for(unsigned groups:{1U,3U,16U})for(bool work:{false,true}) {
  if(argc==4 && (type!=12 || k!=5120 || groups!=1 || !work))continue;
  printf("STICKY_Q4_CASE type%u K%u groups%u work%u\n",type,k,groups,unsigned(work));
  DeferredAffineImage old(argv[1],type,k,groups);StickyQ4Image next(argv[1],argv[2],type,k,groups,work);
  assert(old.Boot() && next.Boot());old.memory[0x74038]=next.memory[0x74038]=unsigned(work);
  for(unsigned job=1;job<=2;job++) {
   if(job==2){old.NextPackets();next.NextPackets();FreshInputs(old);FreshInputs(next);}
   old.Poison();next.Poison();unsigned before=old.steps,after=next.steps;
   assert(old.Job(job) && next.Job(job));
   assert(old.requests==next.requests && old.responses==next.responses && old.matrixInstructions==next.matrixInstructions
    && old.sumWrites==next.sumWrites && old.sumReads==next.sumReads && old.dotProbeReads==next.dotProbeReads);
   for(unsigned row=0;row<groups*16;row++)assert(old.buffers[4096+row]==next.buffers[4096+row]);
   if(type==12 && work) {
    assert(next.directUnpacks==job*groups*(k/16) && next.directReads==next.directUnpacks*256 && !next.nativeStoreOps && !next.moveInstructions);
    if(groups==16 && job==2)printf("OFFLINE_STICKY_Q4 K%u RV%u/%u vector_ops%u/%u drains%u/%u;NOT_NATIVE_OR_TOKEN_RATE\n",k,old.steps-before,next.steps-after,old.vectorOps,next.vectorOps,old.drains,next.drains);
   }
  }
  old.memory[0x74038]=next.memory[0x74038]=0;assert(old.Job(3,2) && next.Job(3,2));
 }
 for(unsigned fault=0;fault<4;fault++) {
  StickyQ4Image bad(argv[1],argv[2],12,5120,1,true);assert(bad.Boot());bad.memory[0x74038]=1;bad.Poison();
  if(fault==0)bad.omitUnpack=true;if(fault==1)bad.tiles[0][256]=16;
  if(fault==2)bad.omitCompletion=true;if(fault==3)bad.omitAdcReset=true;
  bool caught=false;try{(void)bad.Job(1);}catch(const PredecodedQ4Image::DirectHazard&){caught=true;}
  assert(caught && !bad.outputReads && !bad.memory.at(0x74144));
 }
 for(unsigned fault:{1U,5U,40U,208U}) {
  StickyQ4Image bad(argv[1],argv[2],12,17408,3,true);assert(bad.Boot());bad.memory[0x74038]=1;bad.Poison();bad.faultAt=fault;
  assert(!bad.Job(1) && bad.pending && bad.requests==fault && bad.responses==fault-1 && !bad.memory.at(0x74144));
  unsigned requests=bad.requests;for(unsigned step=0;step<1000;step++)bad.StepBlock();assert(bad.requests==requests);
 }
 puts(argc==4?"PASS PARTIAL ONLY sticky-Q4 actual image pilot+failure observers;FULL SUITE SKIPPED":"PASS OFFLINE ACTUAL sticky-Q4:original packed FP32+FP64 exact,all formats/widths/groups/modes/two changed inputs,metadata restore BEFORE each block,sticky native SM7 for16 panels,deferred ownership/STOP/failure observers;NO_NATIVE_LAYOUT_OWNER_OR_SPEED_QUALIFICATION");
}
