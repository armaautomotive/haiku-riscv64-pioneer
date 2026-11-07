/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_BFP4_WORKER_JOB_H
#define PIONEER_Q38_BFP4_WORKER_JOB_H
#ifdef PIONEER_DRAM_SWEEP_FREESTANDING
typedef __UINT32_TYPE__ uint32_t;
#else
#include <stdint.h>
#endif
#include "PioneerNativeBfp4DramBandLayout.h"
#include "PioneerQ38Q4FpuCachedLayout.h"
// Isolated NEW job DATA for future paired 32-row BFP4 workers. No old Q4
// format alias, runtime selection, initialized-weight proof or ownership grant.
// The existing BF16 producer leaves the vector in ROOT L1, so these packets
// never move intermediate activations over PCIe or require a GDDR input copy.
// This 16-word leaf fits the existing capsule; layer, role, row and position
// must be independently checked against the sealed OUTER job by its owner.
struct PioneerQ38Bfp4WorkerJob {
 using L=PioneerNativeBfp4DramBandLayout;
 using Existing=PioneerQ38Q4FpuCachedLayout;
 enum : unsigned {Magic=0x314a3442,Version=1,Words=16,Output=0x84000,RootInput=Existing::InputBase,RootRoute=Existing::InputRoot};
 struct Job {uint32_t words[Words];};
 struct Read {uint32_t source,destination,bytes,route;};
 static bool Shape(const Job& job){
  const auto* w=job.words;const unsigned width=w[2],bands=w[4],worker=w[6];
  if(w[0]!=Magic || w[1]!=Version || (width!=5120 && width!=17408) || w[3]<0x04000000 || (w[3]&63)
   || !bands || bands>8 || !Existing::Route(w[5]) || !worker || worker>=120 || w[7]!=Existing::Peer(worker)
   || w[8]!=PioneerQ38Bf16Wire::Magic || w[9]!=RootInput || w[10]!=RootRoute || !w[11] || w[11]>=0x7fffffff
   || w[12]!=Output || w[13] || w[14] || w[15])return false;
  const unsigned oneBand=width==5120?92160U:313344U;volatile unsigned bytes=0;
  for(unsigned i=0;i<bands;i++)bytes=bytes+oneBand;
  return w[3]<=0xc4000000U-bytes;
 }
private:
 friend class PioneerQ38Bfp4WorkerLoader;
 // Only the loader may use its already validated private job copy here.
 static bool SelectedPacket(const Job& job,unsigned band,unsigned operand,unsigned tile,Read& output){
  if(band>=job.words[4] || operand>1 || tile>=L::Tiles(job.words[2]))return false;
  L::Read fixed;if(!L::Packet(job.words[2],operand,tile,fixed))return false;
  const unsigned oneBand=job.words[2]==5120?92160U:313344U;volatile unsigned bandOffset=0;
  for(unsigned i=0;i<band;i++)bandOffset=bandOffset+oneBand;
  Read next={operand?RootInput+tile*L::InputPayload:
    job.words[3]+bandOffset+tile*L::WeightPayload,
   fixed.destination,fixed.bytes,operand?unsigned(RootRoute):job.words[5]};
  output=next;return true;
 }
public:
 static bool Packet(const Job& job,unsigned band,unsigned operand,unsigned tile,Read& output){
  return Shape(job) && SelectedPacket(job,band,operand,tile,output);
 }
};
static_assert(PioneerQ38Bfp4WorkerJob::Output+8*32*4<=PioneerNativeBfp4DramBandLayout::Weight-PioneerNativeBfp4DramBandLayout::Header,
 "all 256 worker output words disjoint from BFP4 staging");
static_assert(sizeof(PioneerQ38Bfp4WorkerJob::Job)==64 && PioneerQ38Bfp4WorkerJob::RootInput+17408*2<=0x180000,
 "leaf stays in existing 16-word capsule and ROOT vector is in L1");
#endif
