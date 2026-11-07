/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_NATIVE_BFP4_DRAM_BAND_H
#define PIONEER_NATIVE_BFP4_DRAM_BAND_H
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Unqualified BFP4 GDDR band is not a driver, Work or CPU-reference capability
#endif
#include "PioneerNativeBfp4DramBandLayout.h"
#include "PioneerNativeDramBandLoader.h"
#include "PioneerQ38NativeBfp4Band.h"
// Reuse the previously exercised queued loader and the physically verified
// BFP4 instruction producer, substituting ONLY private descriptor addresses.
// Neither source authentication, physical ownership, GDDR contents, model
// quality nor use in Work follows from these fixed offboard DATA operations.
struct PioneerNativeBfp4DramBand {
 using L=PioneerNativeBfp4DramBandLayout;
 using Loader=PioneerNativeDramBandLoaderFor<L>;
 template<class E> struct Descriptors {
  E& emitter;unsigned tiles,weight=0,input=0;bool active=false,valid=true;
  explicit Descriptors(E& e,unsigned k):emitter(e),tiles(k/32){}
  void Op(unsigned value){if(valid)emitter.Op(value);}
  void Drain(){if(valid)emitter.Drain();}
  void Config(unsigned index,unsigned value){
   if(!valid)return;
   if(index==112 && value==0x00200015)active=true;
   if(index==76){
    if(active){
     if(weight>=tiles || value!=PioneerQ38NativeBfp4Band::Weight/16-1+weight*(PioneerQ38NativeBfp4Band::WeightStride/16)){valid=false;return;}
     value=L::Weight/16-1+weight++*(L::WeightStride/16);
    }else{
     if(value!=PioneerQ38NativeBfpMatvec::WeightAddress/16-1){valid=false;return;}value=L::Weight/16-1;
    }
   }
   if(index==124){
    if(active){
     if(input>=tiles || value!=PioneerQ38NativeBfp4Band::Input/16-1+input*(PioneerQ38NativeBfp4Band::InputStride/16)){valid=false;return;}
     value=L::Input/16-1+input++*(L::InputStride/16);
    }else{
     if(value!=PioneerQ38NativeBfpMatvec::InputAddress/16-1){valid=false;return;}value=L::Input/16-1;
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
