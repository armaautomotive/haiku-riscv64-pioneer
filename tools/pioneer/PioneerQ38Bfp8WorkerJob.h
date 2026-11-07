/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_BFP8_WORKER_JOB_H
#define PIONEER_Q38_BFP8_WORKER_JOB_H
#ifdef PIONEER_DRAM_SWEEP_FREESTANDING
typedef __UINT32_TYPE__ uint32_t;
#else
#include <stdint.h>
#endif
#include "PioneerQ38Q4FpuCachedLayout.h"
// Distinct C64 layout follows prior real BFP8 GDDR tests. This is geometry,
// not proof of physical ownership, uploaded weights or producer lifetime.
struct PioneerQ38Bfp8WorkerLayout {
 enum : unsigned {Weight=0x90000,Input=0x12c000,Output=0x140000,Header=64,
  WeightPayload=1088,WeightStride=1152,InputPayload=64,InputStride=128};
 static bool Width(unsigned k){return k==5120 || k==17408;}
 static unsigned Tiles(unsigned k){return Width(k)?k/32:0;}
 struct Read {unsigned source,destination,bytes;};
 static bool Packet(unsigned k,unsigned operand,unsigned tile,Read& output){
  if(!Width(k) || operand>1 || tile>=Tiles(k))return false;
  Read next={operand?0x04100000+tile*InputPayload:0x04000000+tile*WeightPayload,
   operand?Input+tile*InputStride:Weight+tile*WeightStride,operand?InputPayload:WeightPayload};
  output=next;return true;
 }
};
class PioneerQ38Bfp8WorkerLoader;
struct PioneerQ38Bfp8WorkerJob {
 using L=PioneerQ38Bfp8WorkerLayout;using Existing=PioneerQ38Q4FpuCachedLayout;
 enum : unsigned {Magic=0x314a3842,Version=1,Words=16,Output=0x84000,RootInput=Existing::InputBase,RootRoute=Existing::InputRoot};
 struct Job {uint32_t words[Words];};struct Read {uint32_t source,destination,bytes,route;};
 static bool Shape(const Job& job){
  const auto* w=job.words;const unsigned width=w[2],bands=w[4],worker=w[6];
  if(w[0]!=Magic || w[1]!=Version || !L::Width(width) || w[3]<0x04000000 || (w[3]&63)
   || !bands || bands>8 || !Existing::Route(w[5]) || !worker || worker>=120 || w[7]!=Existing::Peer(worker)
   || w[8]!=PioneerQ38Bf16Wire::Magic || w[9]!=RootInput || w[10]!=RootRoute || !w[11] || w[11]>=0x7fffffff
   || w[12]!=Output || w[13] || w[14] || w[15])return false;
  const unsigned oneBand=width==5120?174080U:591872U;volatile unsigned bytes=0;
  for(unsigned i=0;i<bands;i++)bytes=bytes+oneBand;
  return w[3]<=0xc4000000U-bytes;
 }
private:
 friend class PioneerQ38Bfp8WorkerLoader;
 static bool SelectedPacket(const Job& job,unsigned band,unsigned operand,unsigned tile,Read& output){
  if(band>=job.words[4] || operand>1 || tile>=L::Tiles(job.words[2]))return false;
  L::Read fixed;if(!L::Packet(job.words[2],operand,tile,fixed))return false;
  const unsigned oneBand=job.words[2]==5120?174080U:591872U;volatile unsigned bandOffset=0;
  for(unsigned i=0;i<band;i++)bandOffset=bandOffset+oneBand;
  Read next={operand?RootInput+tile*L::InputPayload:job.words[3]+bandOffset+tile*L::WeightPayload,
   fixed.destination,fixed.bytes,operand?unsigned(RootRoute):job.words[5]};output=next;return true;
 }
public:
 static bool Packet(const Job& job,unsigned band,unsigned operand,unsigned tile,Read& output){
  return Shape(job) && SelectedPacket(job,band,operand,tile,output);
 }
};
static_assert(PioneerQ38Bfp8WorkerLayout::Weight-PioneerQ38Bfp8WorkerLayout::Header+544*PioneerQ38Bfp8WorkerLayout::WeightStride
 <=PioneerQ38Bfp8WorkerLayout::Input-PioneerQ38Bfp8WorkerLayout::Header,"BFP8 weight/input extent");
static_assert(PioneerQ38Bfp8WorkerLayout::Input-PioneerQ38Bfp8WorkerLayout::Header+544*PioneerQ38Bfp8WorkerLayout::InputStride
 <=PioneerQ38Bfp8WorkerLayout::Output && PioneerQ38Bfp8WorkerJob::Output+8*32*4<PioneerQ38Bfp8WorkerLayout::Weight-PioneerQ38Bfp8WorkerLayout::Header,
 "BFP8 input/output and worker result extents");
static_assert(sizeof(PioneerQ38Bfp8WorkerJob::Job)==64,"BFP8 job fits existing capsule");
#endif
