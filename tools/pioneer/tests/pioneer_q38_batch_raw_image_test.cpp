/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_BATCH4_IMAGE_MODEL_ONLY
#include "pioneer_q38_batch4_image_test.cpp"
struct BatchRawImage:Batch4Image {
 using Batch4Image::Batch4Image;
 unsigned metadataReads=0;
 mutable unsigned validatedTile=UINT32_MAX;
 uint32_t CurrentTileBase()const {
  assert(Active() && !pending && responses==requests && requests>inputReads && phaseQueue.empty());
  unsigned panels=type==8?2:16,t=matrixInstructions/(MatrixPhasesPerPanel()*panels),batch=type==8?4:2;
  assert(t<tiles.size() && packets.at(requests-1).tile==int((t/batch)*batch));
  uint32_t base=0x98000+(t%batch)*tile.size();
  if(validatedTile!=t) {
   for(unsigned w=0;w<tile.size()/4;w++)assert(memory.at(base+w*4)==Word(tiles.at(t),w*4));
   validatedTile=t;
  }
  return base;
 }
 uint32_t RawInputBase()const override {return Active()?CurrentTileBase():0x90000;}
 uint32_t Load(uint32_t at)override {
  if(Range(at,0x98000,0x3500)) {
   uint32_t base=CurrentTileBase();assert(!rawPending && at>=base && at-base+4<=tile.size() && !(at&3));
   metadataReads++;return memory.at(at);
  }
  return Batch4Image::Load(at);
 }
 void Store(uint32_t at,uint32_t value)override {
  // Eliminating the whole-tile copy means NO Work writes into raw90000.
  if(Active() && Range(at,0x90000,unsigned(tile.size())))assert(false);
  Batch4Image::Store(at,value);
 }
};
#ifndef PIONEER_Q38_BATCH_RAW_IMAGE_MODEL_ONLY
int main(int argc,char** argv){
 assert(argc==3 || (argc==4 && !strcmp(argv[3],"--work-pilot")));setvbuf(stdout,nullptr,_IOLBF,0);
 for(unsigned type:{8U,12U,14U})for(unsigned k:{5120U,6144U,17408U})for(unsigned groups:{1U,3U,16U})for(bool work:{false,true}) {
  if(argc==4 && (k!=5120 || groups!=1 || !work))continue;
  printf("BATCH_RAW_CASE type%u K%u groups%u work%u\n",type,k,groups,unsigned(work));
  Batch4Image old(argv[1],type,k,groups);BatchRawImage next(argv[2],type,k,groups);
  assert(old.Boot() && next.Boot());
  for(unsigned job=1;job<=2;job++) {
   if(job==2){old.NextPackets();next.NextPackets();FreshInputs(old);FreshInputs(next);}
   old.Prepare(work);next.Prepare(work);
   auto before=next.memory;
   assert(old.Job(job) && next.Job(job));
   assert(old.requests==next.requests && old.responses==next.responses && old.matrixInstructions==next.matrixInstructions
    && old.sumWrites==next.sumWrites && old.sumReads==next.sumReads && old.vectorOps==next.vectorOps && old.drains==next.drains
    && next.pendingAccumulator.empty() && !next.copyWrites && !next.copyReads);
   if(work)for(unsigned w=0;w<next.tile.size()/4;w++)assert(before.at(0x90000+w*4)==next.memory.at(0x90000+w*4));
   assert(next.memory.at(0x97ffc)==0xdec0adfe && next.memory.at(0x99a80)==0xdec0adfe && next.memory.at(0x9b500)==0xdec0adfe);
   for(unsigned w=0;w<64;w++)assert(old.memory.at(0x83f00+w*4)==next.memory.at(0x83f00+w*4));
   for(unsigned row=0;row<groups*16;row++)assert(old.buffers[4096+row]==next.buffers[4096+row]);
  }
  old.memory[0x74038]=next.memory[0x74038]=0;assert(old.Job(3,2) && next.Job(3,2));
 }
 for(unsigned fault:{1U,4U,5U,55U,106U}) {
  BatchRawImage bad(argv[2],12,17408,3);assert(bad.Boot());bad.Prepare(true);bad.faultAt=fault;
  assert(!bad.Job(1) && bad.pending && bad.requests==fault && bad.responses==fault-1 && !bad.memory.at(0x74144));
  unsigned requests=bad.requests,reads=bad.metadataReads;for(unsigned step=0;step<1000;step++)bad.StepBlock();
  assert(bad.requests==requests && bad.metadataReads==reads && !bad.copyWrites);
 }
 puts(argc==4?"PASS PARTIAL ONLY direct batch input Work pilots+NoC faults;FULL SUITE SKIPPED":"PASS OFFLINE ACTUAL direct bounded-batch input:all types/K/groups/modes/two changed inputs,exact old FP32+independent FP64,SAME NoC/matrix/SFPU/drains/rounding,EVERY whole tile fresh/completed/current,NO Work copy90000,uncertain NoC parks without later consumption/retry;NO native/controller/speed qualification");
}
#endif
