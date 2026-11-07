/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_BFP4_WORKER_COMPUTE_H
#define PIONEER_Q38_BFP4_WORKER_COMPUTE_H
#include "PioneerQ38Bfp4WorkerLoader.h"
#include "PioneerQ38NativeBfp4Band.h"
// Exact previously tested BFP4 operation stream with this worker's private
// staging descriptors. Word-sized flags avoid unsupported byte stores on
// BabyRISCV. This is a diagnostic leaf, never a GPU ownership/Work grant.
struct PioneerQ38Bfp4WorkerCompute {
 using L=PioneerNativeBfp4DramBandLayout;
 template<class E> struct Descriptors {
  E& emitter;unsigned tiles,weight=0,input=0,active=0,valid=1;
  Descriptors(E& e,unsigned k):emitter(e),tiles(k/32){}
  void Op(unsigned value){if(valid)emitter.Op(value);}
  void Drain(){if(valid)emitter.Drain();}
  void Config(unsigned index,unsigned value){
   if(!valid)return;
   if(index==112 && value==0x00200015)active=1;
   if(index==76){
    if(active){
     if(weight>=tiles || value!=PioneerQ38NativeBfp4Band::Weight/16-1+weight*(PioneerQ38NativeBfp4Band::WeightStride/16)){valid=0;return;}
     value=L::Weight/16-1+weight++*(L::WeightStride/16);
    }else{
     if(value!=PioneerQ38NativeBfpMatvec::WeightAddress/16-1){valid=0;return;}value=L::Weight/16-1;
    }
   }
   if(index==124){
    if(active){
     if(input>=tiles || value!=PioneerQ38NativeBfp4Band::Input/16-1+input*(PioneerQ38NativeBfp4Band::InputStride/16)){valid=0;return;}
     value=L::Input/16-1+input++*(L::InputStride/16);
    }else{
     if(value!=PioneerQ38NativeBfpMatvec::InputAddress/16-1){valid=0;return;}value=L::Input/16-1;
    }
   }
   emitter.Config(index,value);
  }
  bool Complete()const{return valid && active && weight==tiles && input==tiles;}
 };
 template<class E> static bool Compute(E& emitter,unsigned k){
  if(!L::Width(k))return false;
  Descriptors<E> translated(emitter,k);
  return PioneerQ38NativeBfp4Band::Run(translated,k) && translated.Complete();
 }
};
#endif
