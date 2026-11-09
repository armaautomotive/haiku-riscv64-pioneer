/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_SRCA_PROBE_H
#define PIONEER_Q38_SRCA_PROBE_H
// Disposable, fixed-fixture qualification only. This is not an ownership
// protocol, a model execution path, or permission to launch on hardware.
#include "PioneerQ38DenseBf16Matrix.h"
#include "PioneerQ38DenseBf16Stage.h"

struct PioneerQ38SrcaProbe {
 enum : unsigned { Values=0xa0000, Probabilities=0x82000, Output=0x92000 };
 template<class E> struct BaseEmitter {
  E& inner;unsigned base;
  void Op(unsigned v){inner.Op(v);}
  void Drain(){inner.Drain();}
  void Config(unsigned i,unsigned v){inner.Config(i,i==84?0x00400000|base:v);}
 };
 // Each pass uses the same fixed positive FP16 input and probability 1.0.
 // Run the production four-product PV path, first with the original register
 // setting and then with the hypothesized setting. Return the raw Dst words;
 // numerical comparison is deliberately outside this on-card program.
 template<class IO,class E> static bool Run(IO& io,E& emitter){
  for(unsigned pass=0;pass<2;pass++){
   BaseEmitter<E> selected{emitter,pass?0x40U:0x80U};
   PioneerQ38DenseBf16ValuesCachedStage<IO> stage(io,Values,Probabilities,1,0);
   if(!PioneerQ38DenseBf16Matrix::ValuesCached(selected,stage))return false;
   for(unsigned lane=0;lane<32;lane++)
    io.Write(Output+(pass*32+lane)*4,io.Result(lane));
   io.Fence();
  }
  return true;
 }
};
#endif
