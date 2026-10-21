/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_DIRECT_SRCB_IMAGE_MODEL_ONLY
#include "pioneer_q38_direct_srcb_image_test.cpp"
struct Batch4Image:DirectSrcBImage {
 std::vector<uint8_t> fetchCode;
 std::vector<Packet> originalPackets;
 unsigned copyReads=0,copyWrites=0;
 Batch4Image(const char* path,unsigned type,unsigned k,unsigned groups,unsigned worker=69):DirectSrcBImage(path,type,k,groups,worker) {
  fetchCode=Read((std::string(path)+"/fetch.bin").c_str());assert(!fetchCode.empty() && fetchCode.size()<=0x700 && !(fetchCode.size()&3));
  for(unsigned w=0;w<fetchCode.size()/4;w++)l1[Index(0x70000+w*4)]=Word(fetchCode,w*4);
  originalPackets=packets;
 }
 bool CodeAddress(uint32_t at)const override{return Range(at,0x70000,unsigned(fetchCode.size())) || DirectSrcBImage::CodeAddress(at);}
 void Prepare(bool work) {
  assert(!pending && pendingAccumulator.empty());memory[0x74038]=work;packets=originalPackets;
  if(work) {
   packets.resize(inputReads);
   for(unsigned t=0;t<tiles.size();t+=4) {
    const auto& original=originalPackets.at(inputReads+t);
    packets.push_back({original.address,original.route,0x98000,4*unsigned(tile.size()),int(t)});
   }
  }
  copyReads=copyWrites=0;
  for(unsigned at=0;at<0x3500;at+=4)memory[0x98000+at]=0x7fc1dead;
  memory[0x97ffc]=memory[0x9b500]=0xdec0adfe;
  Poison();PoisonTail();
 }
 uint32_t Load(uint32_t at)override {
  if(Range(at,0x70000,unsigned(fetchCode.size())))return l1[Index(at)];
  if(Range(at,0x98000,0x3500)) {
   assert(Active() && !pending && !rawPending && phaseQueue.empty() && memory.count(at));
   unsigned t=copyReads/(tile.size()/4),word=copyReads%(tile.size()/4);
   assert(at==0x98000+(t%4)*tile.size()+word*4 && memory.at(at)==Word(tiles.at(t),word*4));copyReads++;return memory.at(at);
  }
  if(at==0xffb20040 && Active()) {
   doorbellRead=true;
   if(pending && ++waits>delay && requests!=faultAt) {
    const auto& p=packets.at(requests-1);
    for(unsigned offset=0;offset<p.bytes;offset+=4) {
     uint32_t value=p.tile<0?root.at(p.address+offset):Word(tiles.at(unsigned(p.tile)+offset/tile.size()),offset%tile.size());
     memory[p.destination+offset]=value^((corrupt && offset==256)?0x01010101U:0);
    }
    pending=false;responses++;
   }
   return pending?1:0;
  }
  return DirectSrcBImage::Load(at);
 }
 void Store(uint32_t at,uint32_t value)override {
  if(Active() && Range(at,0x90000,unsigned(tile.size()))) {
   assert(!pending && !rawPending && phaseQueue.empty() && copyReads==copyWrites+1 && at==0x90000+(copyWrites%(tile.size()/4))*4);
   unsigned t=copyWrites/(tile.size()/4),word=copyWrites%(tile.size()/4);assert(value==Word(tiles.at(t),word*4));
   memory[at]=value;copyWrites++;return;
  }
  // Original initial command frame is still staged,then overwritten by
  // the bounded batch frame BEFORE any issue. NO permission to ISSUE it.
  if(Active() && requests==inputReads && (at==0xffb2000c || at==0xffb20020)
   && value==(at==0xffb2000c?0x90000:unsigned(tile.size()))) {
   assert(!pending);unsigned w=(at-0xffb20000)/4;command[w]=value;readback[w]=false;return;
  }
  DirectSrcBImage::Store(at,value);
 }
};
#ifndef PIONEER_Q38_BATCH4_IMAGE_MODEL_ONLY
int main(int argc,char** argv) {
 assert(argc==3 || (argc==4 && !strcmp(argv[3],"--work-pilot")));setvbuf(stdout,nullptr,_IOLBF,0);
 for(unsigned type:{8U,12U,14U})for(unsigned k:{5120U,6144U,17408U})for(unsigned groups:{1U,3U,16U})for(bool work:{false,true}) {
  if(argc==4 && (k!=5120 || groups!=1 || !work))continue;
  printf("BATCH4_CASE type%u K%u groups%u work%u\n",type,k,groups,unsigned(work));
  DirectSrcBImage old(argv[1],type,k,groups);Batch4Image next(argv[2],type,k,groups);
  assert(old.Boot() && next.Boot());old.memory[0x74038]=work;
  for(unsigned job=1;job<=2;job++) {
   if(job==2){old.NextPackets();next.NextPackets();FreshInputs(old);FreshInputs(next);}
   old.Poison();old.PoisonTail();next.Prepare(work);
   assert(old.Job(job) && next.Job(job));
   assert(old.matrixInstructions==next.matrixInstructions && old.sumWrites==next.sumWrites && old.sumReads==next.sumReads
    && old.vectorOps==next.vectorOps && old.drains==next.drains && old.pendingAccumulator.empty() && next.pendingAccumulator.empty());
   assert(old.requests-old.inputReads==(next.requests-next.inputReads)*(work?4U:1U));
   assert(next.copyReads==(work?groups*(k/(type==8?32:256))*next.tile.size()/4:0U) && next.copyWrites==next.copyReads);
   assert(next.memory.at(0x97ffc)==0xdec0adfe && next.memory.at(0x9b500)==0xdec0adfe);
   for(unsigned row=0;row<groups*16;row++)assert(old.buffers[4096+row]==next.buffers[4096+row]);
  }
  old.memory[0x74038]=next.memory[0x74038]=0;assert(old.Job(3,2) && next.Job(3,2));
 }
 for(unsigned fault:{1U,4U,5U,30U,55U}) {
  Batch4Image bad(argv[2],12,17408,3);assert(bad.Boot());bad.Prepare(true);bad.faultAt=fault;
  assert(!bad.Job(1) && bad.pending && bad.requests==fault && bad.responses==fault-1 && !bad.memory.at(0x74144));
  unsigned requests=bad.requests,copies=bad.copyWrites;for(unsigned step=0;step<1000;step++)bad.StepBlock();assert(bad.requests==requests && bad.copyWrites==copies);
 }
 puts(argc==4?"PASS PARTIAL ONLY batch4 Work pilots+NoC faults;FULL SUITE SKIPPED":"PASS OFFLINE ACTUAL batch4 peer:all types/K/groups/modes/two changed inputs,exact original FP32+independent FP64,ALL scratch words fresh/in-order/bounded,unchanged Tensix operations/drains/rounding,quarter weight requests+same bytes,NoC uncertainty parks without later copy/retry;NO root/controller/native/speed qualification");
}
#endif
