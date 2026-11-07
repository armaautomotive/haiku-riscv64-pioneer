/* SPDX-License-Identifier: MIT */
// Actual bulk loader versus the existing loader's synthetic byte layout.
#define PIONEER_BFP4_WORKER_LOADER_TEST_ONLY 1
#define PIONEER_Q38_BFP4_BULK_LOAD_EXPERIMENT 1
#include "pioneer_q38_bfp4_worker_loader_test.cpp"
#include "../PioneerQ38Bfp4BulkWorkerLoader.h"

struct BulkIO:IO {
 unsigned bulkIssues=0,dropCompletion=0;bool corrupted=false;
 BulkIO(const W::Job& j,unsigned b):IO(j,b){}
 bool Local(unsigned a)const{
  return a>=L::Weight-L::Header && a+4<=PioneerQ38Bfp4BulkWorkerLoader::Scratch+PioneerQ38Bfp4BulkWorkerLoader::ScratchBytes;
 }
 unsigned Read(unsigned a){
  if(Local(a)){events++;return l1[a]|unsigned(l1[a+1])<<8|unsigned(l1[a+2])<<16|unsigned(l1[a+3])<<24;}
  return IO::Read(a);
 }
 void Write(unsigned a,unsigned value){
  if(Local(a)){
   events++;for(unsigned i=0;i<4;i++)l1[a+i]=uint8_t(value>>(8*i));return;
  }
  if(a!=0xffb20040){IO::Write(a,value);return;}
  events++;assert(value==1 && command[3]==PioneerQ38Bfp4BulkWorkerLoader::Scratch);
  const unsigned groups=L::Tiles(job.words[2])/16,operand=bulkIssues/groups,batch=bulkIssues%groups;
  assert(operand<2 && command[8]==16*(operand?L::InputPayload:L::WeightPayload));
  W::Read expected={};assert(W::Packet(job,band,operand,batch*16,expected));
  assert(command[0]==expected.source && command[2]==expected.route && command[5]==job.words[7]);
  const auto& source=operand?root:weights;const unsigned base=operand?W::RootInput:job.words[3];
  assert(expected.source>=base && expected.source-base+command[8]<=source.size());
  std::memcpy(l1.data()+command[3],source.data()+expected.source-base,command[8]);
  bulkIssues++;if(dropCompletion==bulkIssues)return;
  counters[2]++;counters[3]+=command[8]/64;counters[4]++;counters[5]++;counters[14]++;
  if(corrupted)counters[3]++;
 }
 void ExactBulk()const{
  assert(bulkIssues==2*L::Tiles(job.words[2])/16);
  for(unsigned operand=0;operand<2;operand++)for(unsigned tile=0;tile<L::Tiles(job.words[2]);tile++){
   W::Read p={};assert(W::Packet(job,band,operand,tile,p));
   for(unsigned i=0;i<L::Header;i++)assert(l1[p.destination-L::Header+i]==0);
   const auto& source=operand?root:weights;const unsigned base=operand?W::RootInput:job.words[3];
   assert(!std::memcmp(l1.data()+p.destination,source.data()+p.source-base,p.bytes));
  }
  for(unsigned i=0;i<L::Weight-L::Header;i++)assert(l1[i]==0xa7);
  for(unsigned i=L::WeightEnd(job.words[2]);i<L::Input-L::Header;i++)assert(l1[i]==0xa7);
  for(unsigned i=L::InputEnd(job.words[2]);i<PioneerQ38Bfp4BulkWorkerLoader::Scratch;i++)assert(l1[i]==0xa7);
  for(unsigned i=PioneerQ38Bfp4BulkWorkerLoader::Scratch+PioneerQ38Bfp4BulkWorkerLoader::ScratchBytes;i<l1.size();i++)assert(l1[i]==0xa7);
 }
};
int main(){
 unsigned cases=0;
 for(unsigned worker:{1U,63U,119U})for(unsigned width:{5120U,17408U})for(unsigned band:{0U,1U}){
  auto job=Make(worker,width);IO original(job,band);WorkerLoader old;
  assert(old.Run(original,job,band));original.Exact();
  BulkIO bulk(job,band);PioneerQ38Bfp4BulkWorkerLoader loader;
  assert(loader.Run(bulk,job,band));bulk.ExactBulk();
  for(unsigned op=0;op<2;op++)for(unsigned tile=0;tile<L::Tiles(width);tile++){
   W::Read p={};assert(W::Packet(job,band,op,tile,p));
   assert(!std::memcmp(original.l1.data()+p.destination-L::Header,bulk.l1.data()+p.destination-L::Header,L::Header+p.bytes));
  }
  if(worker==119 && band==0)printf("SYNTHETIC_BULK_EVENTS width%u old%u bulk%u old_requests%u bulk_requests%u; NOT HARDWARE TIME\n",
   width,original.events,bulk.events,original.issues,bulk.bulkIssues);
  const unsigned before=bulk.events;assert(!loader.Run(bulk,job,band) && bulk.events==before);cases++;
 }
 for(unsigned fault=0;fault<3;fault++){
  auto job=Make(1,5120);BulkIO io(job,0);PioneerQ38Bfp4BulkWorkerLoader loader;
  if(fault==0)io.dropCompletion=1;else if(fault==1)io.corrupted=true;else job.words[0]=0;
  assert(!loader.Run(io,job,0));const unsigned before=io.events;
  assert(!loader.Run(io,job,0) && io.events==before);
 }
 printf("PASS synthetic BFP4 bulk loader %u cases: same staged bytes, 16x fewer NoC requests, failure stops; NO DEVICE OR SPEED\n",cases);
}
