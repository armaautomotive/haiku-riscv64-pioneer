/* SPDX-License-Identifier: MIT */
#include "PioneerQ38RecurrentSmallBfp8WorkerRole.h"
#if !defined(PIONEER_Q38_RECURRENT_SMALL_BFP8_WORKER_INSPECTION) || PIONEER_Q38_RECURRENT_SMALL_BFP8_WORKER_INSPECTION != 1
#error Small recurrent ROOT jobs are isolated inspection only
#endif
#ifndef PIONEER_Q38_RECURRENT_SMALL_ROOT_JOBS_H
#define PIONEER_Q38_RECURRENT_SMALL_ROOT_JOBS_H
// ROOT record is seven bank triples (address, first row, row count). Exactly
// one bank contains the complete two-band padded tensor. Data only: the
// native owner must authenticate, upload and preserve record and weights.
struct PioneerQ38RecurrentSmallRootJobs {
 enum {BandBytes=174080,Workers=2};
 template<class IO>
#ifdef PIONEER_Q38_QGWD_ROOT_IMAGE
 static __attribute__((always_inline)) inline bool Build(IO& io,unsigned layer,unsigned role){
#else
 static bool Build(IO& io,unsigned layer,unsigned role){
#endif
  if(layer>=64 || layer%4==3 || (role!=10 && role!=11))return false;
  unsigned chosen=7,address=0;
  for(unsigned bank=0;bank<7;bank++){
   const unsigned a=io.Table(bank,0),first=io.Table(bank,1),rows=io.Table(bank,2);
   if(!a && !first && !rows)continue;
   if(chosen!=7 || first || rows!=64 || a<0x04000000 || (a&4095) || a>0xc4000000U-2*BandBytes)return false;
   chosen=bank;address=a;
  }
  if(chosen==7)return false;
  for(unsigned bank=0;bank<7;bank++)for(unsigned field=0;field<3;field++){
   const unsigned expected=bank==chosen?(field==0?address:field==2?64U:0U):0;
   if(io.Table(bank,field)!=expected)return false;
  }
  for(unsigned w=0;w<Workers;w++){
   io.Output(w,0,address+w*BandBytes);io.Output(w,1,chosen);io.Output(w,2,w*32);io.Output(w,3,1);
  }
  return true;
 }
};
#endif
