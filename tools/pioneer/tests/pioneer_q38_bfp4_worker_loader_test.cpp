/* SPDX-License-Identifier: MIT */
// Explicit simulated NoC endpoints only. Never a GPU/runtime test.
#if defined(PIONEER_TEST_BFP8)
#define PIONEER_Q38_BFP8_WORKER_DIAGNOSTIC 1
#include "../PioneerQ38Bfp8WorkerLoader.h"
using W=PioneerQ38Bfp8WorkerJob;using WorkerLoader=PioneerQ38Bfp8WorkerLoader;
#else
#define PIONEER_Q38_BFP4_WORKER_DIAGNOSTIC 1
#include "../PioneerQ38Bfp4WorkerLoader.h"
using W=PioneerQ38Bfp4WorkerJob;using WorkerLoader=PioneerQ38Bfp4WorkerLoader;
#endif
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
using L=W::L;
static W::Job Make(unsigned worker,unsigned width,unsigned bands=2){
 W::Job j={{W::Magic,W::Version,width,0x04000000,bands,786,worker,W::Existing::Peer(worker),PioneerQ38Bf16Wire::Magic,
  W::RootInput,W::RootRoute,7,W::Output,0,0,0}};
 assert(W::Shape(j));return j;
}
struct IO {
 W::Job job;unsigned band,events=0,issues=0,completions=0,headers=0,scalar=0,fault=0,drop=99;
#if (defined(PIONEER_TEST_BFP8) && defined(PIONEER_Q38_BFP8_BATCH_INPUT_EXPERIMENT)) || defined(PIONEER_Q38_BFP4_BATCH_INPUT_EXPERIMENT)
 unsigned scatterWrites=0,dropScatter=0,rootDelay=0,maxRootPending=0,delayedRootPolls=0;bool stallRoot=false;
#endif
 std::array<uint32_t,10> command;std::array<uint32_t,16> counters;std::vector<W::Read> pending;
#ifdef PIONEER_Q38_BFP8_INCOMING_WATCH_EXPERIMENT
 std::array<uint32_t,12> incoming;
#endif
 std::vector<uint8_t> ownedL1,weights,root;std::vector<uint8_t>& l1;bool autoBand=false;
 IO(const W::Job& j,unsigned b,std::vector<uint8_t>* shared=nullptr):job(j),band(b),ownedL1(shared?0:0x180000,0xa7),
  weights(size_t(j.words[4])*L::Tiles(j.words[2])*L::WeightPayload),root(j.words[2]*2),l1(shared?*shared:ownedL1){
  assert(l1.size()==0x180000);
  command.fill(0xcccccccc);counters.fill(UINT32_MAX-17);
#ifdef PIONEER_Q38_BFP8_INCOMING_WATCH_EXPERIMENT
  incoming.fill(UINT32_MAX-17);
#endif
  for(size_t i=0;i<weights.size();i++)weights[i]=uint8_t((i*17+i/L::WeightPayload*13)^0x8b);
  for(size_t i=0;i<root.size();i++)root[i]=uint8_t((i*19+i/64*7)^0x31);
 }
 bool Header(unsigned a)const{
  for(unsigned op=0;op<2;op++){
   const unsigned start=(op?L::Input:L::Weight)-L::Header,stride=op?L::InputStride:L::WeightStride;
   if(a>=start && a-start<L::Tiles(job.words[2])*stride && (a-start)%stride<L::Header)return true;
  }
  return false;
 }
 bool InputScratch(unsigned a)const{
  return a>=L::Input-L::Header && a-(L::Input-L::Header)<L::Tiles(job.words[2])*L::InputStride;
 }
 void Flush(){
#if (defined(PIONEER_TEST_BFP8) && defined(PIONEER_Q38_BFP8_BATCH_INPUT_EXPERIMENT)) || defined(PIONEER_Q38_BFP4_BATCH_INPUT_EXPERIMENT)
  assert(pending.size()==16 || (!pending.empty() && pending.size()<=3 && pending[0].route==W::RootRoute));
#else
  assert(pending.size()==16);
#endif
  for(const auto& p:pending){
   assert(!(p.source&63) && !(p.destination&63) && !(p.bytes&63));
   if(p.route==W::RootRoute){assert(p.source>=W::RootInput && p.source-W::RootInput+p.bytes<=root.size());std::memcpy(l1.data()+p.destination,root.data()+p.source-W::RootInput,p.bytes);}
   else {assert(p.route==job.words[5] && p.source>=job.words[3] && p.source-job.words[3]+p.bytes<=weights.size());
    std::memcpy(l1.data()+p.destination,weights.data()+p.source-job.words[3],p.bytes);}
   counters[2]++;counters[3]+=p.bytes/64;counters[4]++;counters[5]++;counters[14]++;completions++;
  }
  pending.clear();if(fault==5)counters[2]++;if(fault==6)counters[3]++;if(fault==7)counters[1]++;
 }
 unsigned Read(unsigned a){
  events++;
#ifdef PIONEER_Q38_BFP8_INCOMING_WATCH_EXPERIMENT
  for(unsigned n=0;n<2;n++)for(unsigned i=0;i<6;i++){
   const unsigned offsets[]={0x2d8,0x2dc,0x2e0,0x2e4,0x2f0,0x2f4};
   if(a==0xffb20000+n*0x10000+offsets[i])return incoming[n*6+i];
  }
#endif
  if(Header(a) || InputScratch(a)){
#if (defined(PIONEER_TEST_BFP8) && defined(PIONEER_Q38_BFP8_BATCH_INPUT_EXPERIMENT)) || defined(PIONEER_Q38_BFP4_BATCH_INPUT_EXPERIMENT)
   if(InputScratch(a))assert(pending.empty() || pending[0].route!=W::RootRoute);
#endif
   return l1[a]|unsigned(l1[a+1])<<8|unsigned(l1[a+2])<<16|unsigned(l1[a+3])<<24;}
  const unsigned xy=job.words[7],x=xy&63,y=xy>>6,xi=x<6?0:x<12?1:2,yi=y<6?0:1;
  const unsigned xs=(x<6?x:x<12?x-6:x-12)*5,ys=(y<6?y:y-6)*5;
  if(a==0xffb20044)return (17<<12)|(12<<19)|xy|(fault==1?0x20:0);
  if(a==0xffb20048)return 0x10000;if(a==0xffb20148)return xy;
  if(a==0xffb20118+xi*4)return x<<xs;if(a==0xffb20130+yi*4)return y<<ys;
  if(a==0xffb20150 || a==0xffb20154)return 0;
  if(a==0xffb20100)return fault==2?0x4000:0x14000;if(a==0xffb20068)return fault==3?1:0;
  if(a==0xffb20064){if((pending.size()==16
#if (defined(PIONEER_TEST_BFP8) && defined(PIONEER_Q38_BFP8_BATCH_INPUT_EXPERIMENT)) || defined(PIONEER_Q38_BFP4_BATCH_INPUT_EXPERIMENT)
   || (!pending.empty() && pending[0].route==W::RootRoute && !rootDelay)
#endif
   ) && fault!=4
#if (defined(PIONEER_TEST_BFP8) && defined(PIONEER_Q38_BFP8_BATCH_INPUT_EXPERIMENT)) || defined(PIONEER_Q38_BFP4_BATCH_INPUT_EXPERIMENT)
   && !(stallRoot && !pending.empty() && pending[0].route==W::RootRoute)
#endif
   )Flush();
#if (defined(PIONEER_TEST_BFP8) && defined(PIONEER_Q38_BFP8_BATCH_INPUT_EXPERIMENT)) || defined(PIONEER_Q38_BFP4_BATCH_INPUT_EXPERIMENT)
   else if(!pending.empty() && pending[0].route==W::RootRoute && rootDelay){rootDelay--;delayedRootPolls++;}
#endif
   return 0x10101010-unsigned(pending.size());}
  if(a==0xffb20040)return fault==8 && issues?2:0;
  if(a==0xffb20840 || a==0xffb21040 || a==0xffb21840)return fault==9?1:0;
  if(a>=0xffb20240 && a<0xffb202c0)return fault==10 && issues?1:0;
  if(a>=0xffb20200 && a<0xffb20240)return counters[(a-0xffb20200)/4];
  if(a>=0xffb20000 && a<0xffb20028)return command[(a-0xffb20000)/4];
  scalar++;assert(false);return 0;
 }
 void Write(unsigned a,unsigned value){
  events++;
#if (defined(PIONEER_TEST_BFP8) && defined(PIONEER_Q38_BFP8_BATCH_INPUT_EXPERIMENT)) || defined(PIONEER_Q38_BFP4_BATCH_INPUT_EXPERIMENT)
  if(InputScratch(a))assert(pending.empty() || pending[0].route!=W::RootRoute);
#endif
  if(Header(a)){headers++;assert(!value);if(fault!=11 || headers!=1)std::memset(l1.data()+a,0,4);return;}
#if (defined(PIONEER_TEST_BFP8) && defined(PIONEER_Q38_BFP8_BATCH_INPUT_EXPERIMENT)) || defined(PIONEER_Q38_BFP4_BATCH_INPUT_EXPERIMENT)
  if(InputScratch(a)){
   scatterWrites++;if(scatterWrites!=dropScatter)for(unsigned byte=0;byte<4;byte++)l1[a+byte]=uint8_t(value>>(byte*8));
   return;
  }
#endif
  if(a==0xffb20040){
   const unsigned tiles=L::Tiles(job.words[2]);
#if (defined(PIONEER_TEST_BFP8) && defined(PIONEER_Q38_BFP8_BATCH_INPUT_EXPERIMENT)) || defined(PIONEER_Q38_BFP4_BATCH_INPUT_EXPERIMENT)
   const unsigned chunks=(job.words[2]*2+16383)/16384,perBand=tiles+chunks;
   if(autoBand)band=issues/perBand;
   const unsigned index=issues%perBand;W::Read expected={};
   if(index<tiles)assert(W::Packet(job,band,0,index,expected));
   else {const unsigned offset=(index-tiles)*16384,remaining=job.words[2]*2-offset;
    expected={W::RootInput+offset,L::Input+offset,remaining<16384?remaining:16384,W::RootRoute};}
   assert(value==1 && pending.size()<16);
#else
   unsigned operand=(issues/tiles)%2,tile=issues%tiles;
#if defined(PIONEER_Q38_BFP4_INPUT_REUSE_EXPERIMENT) || defined(PIONEER_Q38_BFP8_INPUT_REUSE_EXPERIMENT)
   if(autoBand && issues>=2*tiles){band=1+(issues-2*tiles)/tiles;operand=0;tile=(issues-2*tiles)%tiles;}
   else
#endif
   if(autoBand)band=issues/(2*tiles);
   assert(value==1 && pending.size()<16);W::Read expected={};
   assert(W::Packet(job,band,operand,tile,expected));
#endif
   if(!(command[0]==expected.source && command[2]==expected.route && command[3]==expected.destination && command[5]==job.words[7] && command[8]==expected.bytes))
    fprintf(stderr,"BFP_LOADER_PACKET_MISMATCH issues=%u band=%u got=%x,%x,%x,%x,%u expected=%x,%x,%x,%x,%u\n",
     issues,band,command[0],command[2],command[3],command[5],command[8],
     expected.source,expected.route,expected.destination,job.words[7],expected.bytes);
   assert(command[0]==expected.source && command[2]==expected.route && command[3]==expected.destination && command[5]==job.words[7] && command[8]==expected.bytes);
   assert(!command[1] && !command[4] && !command[6] && !command[7] && !command[9]);
#if (defined(PIONEER_TEST_BFP8) && defined(PIONEER_Q38_BFP8_BATCH_INPUT_EXPERIMENT)) || defined(PIONEER_Q38_BFP4_BATCH_INPUT_EXPERIMENT)
   if(expected.route==W::RootRoute && pending.empty())rootDelay=3;
#endif
   pending.push_back(expected);
#if (defined(PIONEER_TEST_BFP8) && defined(PIONEER_Q38_BFP8_BATCH_INPUT_EXPERIMENT)) || defined(PIONEER_Q38_BFP4_BATCH_INPUT_EXPERIMENT)
   if(expected.route==W::RootRoute)maxRootPending=std::max(maxRootPending,unsigned(pending.size()));
#endif
   issues++;return;
  }
  assert(a>=0xffb20000 && a<0xffb20028);const unsigned index=(a-0xffb20000)/4;
  if(!issues && index==drop)return;command[index]=value;
 }
 void Fence(){events++;}
 void Exact()const{
  assert(issues==
#if (defined(PIONEER_TEST_BFP8) && defined(PIONEER_Q38_BFP8_BATCH_INPUT_EXPERIMENT)) || defined(PIONEER_Q38_BFP4_BATCH_INPUT_EXPERIMENT)
   L::Tiles(job.words[2])+(job.words[2]*2+16383)/16384
#else
   2*L::Tiles(job.words[2])
#endif
   && completions==issues && pending.empty() && !scalar);
  std::vector<uint8_t> expected(l1.size(),0xa7);
  for(unsigned op=0;op<2;op++)for(unsigned tile=0;tile<L::Tiles(job.words[2]);tile++){
   W::Read p={};assert(W::Packet(job,band,op,tile,p));std::memset(expected.data()+p.destination-L::Header,0,L::Header);
   const auto* source=op?root.data()+p.source-W::RootInput:weights.data()+p.source-job.words[3];
   std::memcpy(expected.data()+p.destination,source,p.bytes);
  }
  assert(l1==expected);
 }
};
#ifndef PIONEER_BFP4_WORKER_LOADER_TEST_ONLY
int main(){
 unsigned valid=0,refused=0;
 for(unsigned worker=1;worker<120;worker++)for(unsigned width:{5120U,17408U}){
  auto job=Make(worker,width);IO io(job,1);WorkerLoader loader;
  assert(loader.Run(io,job,1));io.Exact();
#if (defined(PIONEER_TEST_BFP8) && defined(PIONEER_Q38_BFP8_BATCH_INPUT_EXPERIMENT)) || defined(PIONEER_Q38_BFP4_BATCH_INPUT_EXPERIMENT)
  assert(io.delayedRootPolls>=3 && io.maxRootPending==(width==5120?1U:3U));
#endif
  const auto before=io.events;assert(!loader.Run(io,job,1) && io.events==before);valid++;
 }
 for(unsigned fault=1;fault<=11;fault++){
  auto job=Make(1,5120);IO io(job,0);io.fault=fault;WorkerLoader loader;assert(!loader.Run(io,job,0));
  const auto before=io.events;assert(!loader.Run(io,job,0) && io.events==before && !io.scalar);refused++;
 }
 for(unsigned drop=0;drop<10;drop++){
  auto job=Make(1,5120);IO io(job,0);io.drop=drop;WorkerLoader loader;assert(!loader.Run(io,job,0) && !io.issues);
  const auto before=io.events;assert(!loader.Run(io,job,0) && io.events==before);refused++;
 }
#if (defined(PIONEER_TEST_BFP8) && defined(PIONEER_Q38_BFP8_BATCH_INPUT_EXPERIMENT)) || defined(PIONEER_Q38_BFP4_BATCH_INPUT_EXPERIMENT)
 for(unsigned width:{5120U,17408U}){
  auto job=Make(1,width);IO io(job,0);io.dropScatter=1;WorkerLoader loader;
  assert(!loader.Run(io,job,0) && io.scatterWrites==1);
  const auto before=io.events;assert(!loader.Run(io,job,0) && io.events==before);refused++;
 }
 {
  auto job=Make(1,17408);IO io(job,0);io.stallRoot=true;WorkerLoader loader;
  assert(!loader.Run(io,job,0) && io.maxRootPending==3 && !io.pending.empty() && !io.scatterWrites);
  const auto before=io.events;assert(!loader.Run(io,job,0) && io.events==before);refused++;
 }
#endif
 for(unsigned word=0;word<W::Words;word++){
  auto job=Make(1,5120);IO io(job,0);auto invalid=job;invalid.words[word]=UINT32_MAX;WorkerLoader loader;
  assert(!loader.Run(io,invalid,0) && !io.events);refused++;
 }
 printf("PASS BFP%u paired-worker loader: %u valid/%u refused; all119 peers, two widths, GDDR weight/ROOT-L1 BF16 input, exact L1+guards/counters, failure stops IO; NO DEVICE OR WORK\n",L::WeightPayload==576?4U:8U,valid,refused);
}
#endif
