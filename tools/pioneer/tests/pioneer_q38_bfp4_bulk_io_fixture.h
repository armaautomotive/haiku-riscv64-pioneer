/* SPDX-License-Identifier: MIT */
// Host simulator only; expects the existing worker-loader IO fixture.
#ifndef PIONEER_Q38_BFP4_BULK_IO_FIXTURE_H
#define PIONEER_Q38_BFP4_BULK_IO_FIXTURE_H
#include "../PioneerQ38Bfp4BulkWorkerLoader.h"
struct BulkIO:IO {
 unsigned bulkIssues=0,dropCompletion=0;bool corrupted=false;
 BulkIO(const W::Job& j,unsigned b,std::vector<uint8_t>* shared=nullptr):IO(j,b,shared){}
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
  const unsigned groups=L::Tiles(job.words[2])/16,operand=(bulkIssues/groups)%2,batch=bulkIssues%groups;
  if(autoBand)band=bulkIssues/(2*groups);
  assert(band<job.words[4] && command[8]==16*(operand?L::InputPayload:L::WeightPayload));
  W::Read expected={};assert(W::Packet(job,band,operand,batch*16,expected));
  assert(command[0]==expected.source && command[2]==expected.route && command[5]==job.words[7]);
  const auto& source=operand?root:weights;const unsigned base=operand?W::RootInput:job.words[3];
  assert(expected.source>=base && expected.source-base+command[8]<=source.size());
  std::memcpy(l1.data()+command[3],source.data()+expected.source-base,command[8]);
  bulkIssues++;issues++;if(dropCompletion==bulkIssues)return;
  counters[2]++;counters[3]+=command[8]/64;counters[4]++;counters[5]++;counters[14]++;completions++;
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
#endif
