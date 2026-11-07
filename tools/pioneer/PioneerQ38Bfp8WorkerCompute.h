/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_BFP8_WORKER_COMPUTE_H
#define PIONEER_Q38_BFP8_WORKER_COMPUTE_H
#include "PioneerQ38Bfp8WorkerLoader.h"
#include "PioneerQ38NativeBfpBand.h"
// Exact four-phase full-K BFP8 stream, with descriptors translated from the
// earlier preloaded-L1 layout to the previously physically tested C64 layout.
struct PioneerQ38Bfp8WorkerCompute {
 using L=PioneerQ38Bfp8WorkerLayout;
 template<class E> struct Descriptors {
  E& emitter;unsigned tiles,weight=0,input=0,active=0,valid=1;
  Descriptors(E& e,unsigned k):emitter(e),tiles(k/32){}
  void Op(unsigned value){if(valid)emitter.Op(value);}void Drain(){if(valid)emitter.Drain();}
  void Config(unsigned index,unsigned value){
   if(!valid)return;
   if(index==112 && value==0x00200015)active=1;
   if(index==76){
    if(active){
     if(weight>=tiles || value!=PioneerQ38NativeBfpBand::Weight/16-1+weight*(PioneerQ38NativeBfpBand::WeightStride/16)){valid=0;return;}
     value=L::Weight/16-1+weight++*(L::WeightStride/16);
    }else{if(value!=PioneerQ38NativeBfpMatvec::WeightAddress/16-1){valid=0;return;}value=L::Weight/16-1;}
   }
   if(index==124){
    if(active){
     if(input>=tiles || value!=PioneerQ38NativeBfpBand::Input/16-1+input*(PioneerQ38NativeBfpBand::InputStride/16)){valid=0;return;}
     value=L::Input/16-1+input++*(L::InputStride/16);
    }else{if(value!=PioneerQ38NativeBfpMatvec::InputAddress/16-1){valid=0;return;}value=L::Input/16-1;}
   }
   emitter.Config(index,value);
  }
  bool Complete()const{return valid && active && weight==tiles && input==tiles;}
 };
 template<class E>static bool Compute(E& emitter,unsigned k){
  if(!L::Width(k))return false;
  Descriptors<E> translated(emitter,k);
  return PioneerQ38NativeBfpBand::Run<4>(translated,k) && translated.Complete();
 }
};
#endif
