/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_DEFER_AFFINE_IMAGE_TEST_MODEL_ONLY
#include "pioneer_q38_defer_affine_image_test.cpp"
#include "../PioneerQ38PreparedQ4.h"
struct PreparedBlockImage:DeferredAffineImage {
 mutable DeferredAffineImage original;
 PreparedBlockImage(const char* reference,const char* candidate,unsigned type,unsigned k,unsigned groups,bool work)
  :DeferredAffineImage(candidate,type,k,groups),original(reference,type,k,groups) {
  if(type!=12 || !work)return; // checked retains ORIGINAL transport bytes
  for(auto& bytes:tiles) {
   std::vector<uint8_t> encoded(PioneerQ38PreparedQ4::Bytes,0xcc);
   assert(PioneerQ38PreparedQ4::Prepare(bytes.data(),unsigned(bytes.size()),encoded.data(),unsigned(encoded.size())));bytes.swap(encoded);
  }
  tile=tiles[0];memory[0x74008]=unsigned(tile.size());
  for(unsigned w=0;w<tile.size()/4;w++)memory[0x90000+w*4]=Word(tile,w*4);
  for(auto& p:packets)if(p.tile>=0){p.address=base+unsigned(p.tile)*unsigned(tile.size());p.bytes=unsigned(tile.size());}
 }
 bool CorrectRows()const override {
  original.vector=vector;original.scales=scales;original.matrixInstructions=matrixInstructions;
  memcpy(original.buffers,buffers,sizeof(buffers));return original.CorrectRows();
 }
};
int main(int argc,char** argv) {
 assert(argc==3 || (argc==4 && !strcmp(argv[3],"--q4-work-pilot")));setvbuf(stdout,nullptr,_IOLBF,0);
 for(unsigned type:{8U,12U,14U})for(unsigned k:{5120U,6144U,17408U})
 for(unsigned groups:{1U,3U,16U})for(bool work:{false,true}) {
  if(argc==4 && (type!=12 || k!=5120 || groups!=1 || !work))continue;
  printf("PREPARED_BLOCK_CASE type%u K%u groups%u work%u\n",type,k,groups,unsigned(work));
  DeferredAffineImage old(argv[1],type,k,groups);PreparedBlockImage next(argv[1],argv[2],type,k,groups,work);
  assert(old.Boot() && next.Boot());old.memory[0x74038]=next.memory[0x74038]=unsigned(work);
  for(unsigned job=1;job<=2;job++) {
   if(job==2){old.NextPackets();next.NextPackets();FreshInputs(old);FreshInputs(next);}
   old.Poison();next.Poison();unsigned before=old.steps,after=next.steps;
   assert(old.Job(job) && next.Job(job));
   assert(old.requests==next.requests && old.responses==next.responses && old.matrixInstructions==next.matrixInstructions
    && old.sumWrites==next.sumWrites && old.sumReads==next.sumReads && old.dotProbeReads==next.dotProbeReads
    && old.vectorOps==next.vectorOps && old.drains==next.drains && old.rawReads==next.rawReads);
   for(unsigned row=0;row<groups*16;row++)assert(old.buffers[4096+row]==next.buffers[4096+row]);
   if(type==12 && work && groups==16 && job==2)
    printf("OFFLINE_PREPARED_BLOCK K%u RV%u/%u vector_ops%u/%u drains%u/%u;EXACT_OUTPUT_DIFFERENT_WIRE_NOT_NATIVE_OR_TOKEN_RATE\n",k,old.steps-before,next.steps-after,old.vectorOps,next.vectorOps,old.drains,next.drains);
  }
  old.memory[0x74038]=next.memory[0x74038]=0;assert(old.Job(3,2) && next.Job(3,2));
 }
 for(unsigned word:{0U,1U,15U,16U,31U,32U,127U,128U,254U,255U}) {
  PreparedBlockImage bad(argv[1],argv[2],12,5120,1,true);assert(bad.Boot());bad.memory[0x74038]=1;bad.Poison();
  bad.tiles[0][2048+word*3+2]|=0x80;
  assert(!bad.Job(1) && !bad.memory.at(0x74144));unsigned requests=bad.requests;
  for(unsigned step=0;step<1000;step++)bad.StepBlock();assert(bad.requests==requests);
 }
 for(unsigned fault:{1U,5U,40U,208U}) {
  PreparedBlockImage bad(argv[1],argv[2],12,17408,3,true);assert(bad.Boot());bad.memory[0x74038]=1;bad.Poison();bad.faultAt=fault;
  assert(!bad.Job(1) && bad.pending && bad.requests==fault && bad.responses==fault-1 && !bad.memory.at(0x74144));
  unsigned requests=bad.requests;for(unsigned step=0;step<1000;step++)bad.StepBlock();assert(bad.requests==requests);
 }
 puts(argc==4?"PASS PARTIAL ONLY prepared-block actual image pilot+faults;FULL SUITE SKIPPED":"PASS OFFLINE ACTUAL prepared-block:original FP32+FP64 exact,all formats/widths/groups/modes/two changed inputs,unchanged vector/matrix/drains/quant/cache/STOP,checked ORIGINAL layout,10 malformed metadata+4 packet faults park before completion;NO_NATIVE_SOURCE_LAYOUT_OWNER_OR_SPEED_QUALIFICATION");
}
