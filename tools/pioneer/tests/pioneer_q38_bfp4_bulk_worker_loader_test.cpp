/* SPDX-License-Identifier: MIT */
// Actual bulk loader versus the existing loader's synthetic byte layout.
#define PIONEER_BFP4_WORKER_LOADER_TEST_ONLY 1
#define PIONEER_Q38_BFP4_BULK_LOAD_EXPERIMENT 1
#include "pioneer_q38_bfp4_worker_loader_test.cpp"
#include "pioneer_q38_bfp4_bulk_io_fixture.h"
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
