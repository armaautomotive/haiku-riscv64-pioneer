/* SPDX-License-Identifier: MIT */
// Geometry and frame construction for an offline four-worker experiment.
// No authorization to map, launch, scatter or install a native device path.
#include "PioneerQ38RecurrentSmallBfp8WorkerRole.h"
#if !defined(PIONEER_Q38_RECURRENT_SMALL_BFP8_WORKER_INSPECTION) || PIONEER_Q38_RECURRENT_SMALL_BFP8_WORKER_INSPECTION != 1
#error Small recurrent matrix schedule is inspection-only
#endif
#ifndef PIONEER_Q38_RECURRENT_SMALL_MATRIX_SCHEDULE_H
#define PIONEER_Q38_RECURRENT_SMALL_MATRIX_SCHEDULE_H
#include "PioneerQ38DenseQ8MatrixWorkerLayout.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionResultLayout.h"
struct PioneerQ38RecurrentSmallMatrixSchedule {
 enum {Count=4,Bands=2,RealRows=48,PaddedRows=64,BandBytes=174080};
 struct Part {uint32_t frame[64];uint32_t first,valid,offset;};
 static bool Build(unsigned layer,unsigned position,unsigned invocation,const uint32_t* bases,const uint32_t* banks,
  const uint32_t boot[Count][64],const uint32_t* previous,Part* output){
  namespace W=PioneerQ38DenseQ8MatrixWorkerLayout;using M=PioneerQ38Bfp8WorkerJob;
  if(!bases || !banks || !boot || !previous || !output || layer>=64 || layer%4==3 || position>=8192
   || !invocation || invocation>=0x7fffffff)return false;
  for(unsigned k=0;k<2;k++)if(banks[k]>=7 || (bases[k]&63) || bases[k]<0x04000000 || bases[k]>0xc4000000-2*BandBytes)return false;
  if(banks[0]==banks[1] && bases[0]<bases[1]+2*BandBytes && bases[1]<bases[0]+2*BandBytes)return false;
  Part staged[Count]={};
  for(unsigned k=0;k<Count;k++){
   const unsigned tensor=k/2,band=k%2,worker=k+1,role=10+tensor,sequence=previous[k]+1;
   if(!W::BootShape(boot[k]) || boot[k][3]!=worker || previous[k]>=0x7ffffffe)return false;
   Part& p=staged[k];auto* j=p.frame;
   j[0]=W::Magic;j[1]=sequence;j[2]=1;j[3]=worker;j[4]=boot[k][4];j[5]=layer;j[6]=position;j[7]=invocation;
   for(unsigned i=0;i<8;i++)j[8+i]=boot[k][8+i];
   const uint32_t leaf[]={M::Magic,M::Version,5120,bases[tensor]+band*BandBytes,1,
    PioneerQ38ProjectionRoute(banks[tensor]),worker,boot[k][4],PioneerQ38Bf16Wire::Magic,
    M::RootInput,M::RootRoute,sequence,M::Output,0,0,0};
   for(unsigned i=0;i<16;i++)j[16+i]=leaf[i];
   j[32]=band*32;j[33]=PaddedRows;j[34]=role;j[35]=8;
   p.first=band*32;p.valid=band?16:32;p.offset=PioneerQ38ProjectionResultOffset(role)+p.first*4;
   if(!W::JobShape(j,boot[k],previous[k],0))return false;
  }
  for(unsigned k=0;k<Count;k++)output[k]=staged[k];return true;
 }
 // Only authenticated output from the appropriate completed worker may be
 // passed by the caller. This helper itself neither authenticates nor does IO.
 static bool CopyRealRows(const Part& p,const uint32_t* source,unsigned count,uint32_t* out,unsigned capacity){
  if(!source || !out || count!=32 || capacity!=RealRows || (p.first!=0 && p.first!=32)
   || p.valid!=(p.first?16U:32U) || p.first+p.valid>capacity
   || p.frame[20]!=1 || p.frame[32]!=p.first || p.frame[33]!=PaddedRows
   || p.offset!=PioneerQ38ProjectionResultOffset(p.frame[34])+p.first*4
   || (p.frame[34]!=10 && p.frame[34]!=11))return false;
  uint32_t stage[32];for(unsigned i=0;i<p.valid;i++)stage[i]=source[i];
  for(unsigned i=0;i<p.valid;i++)out[p.first+i]=stage[i];return true;
 }
};
#endif
