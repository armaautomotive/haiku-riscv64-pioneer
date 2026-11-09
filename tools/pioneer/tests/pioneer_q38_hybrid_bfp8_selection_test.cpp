/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38HybridBfp8Selection.h"
#include <cassert>
#include <cstdio>
int main(){
 unsigned selected=0,qgw8=0,dense=0,total=0;uint64_t omittedBytes=0,denseBytes=0;
 for(unsigned layer=0;layer<=64;layer++)for(unsigned role=0;;role++){
  PioneerQ38Tensor tensor;if(!PioneerQ38Expected(layer,role,tensor))break;
  const bool wanted=layer>=48 && layer<=60 && role>=2 && role<=4;
  const bool wantedQgw8=layer>=50 && layer<=60 && role>=2 && role<=4;
  const bool wantedDense=layer<64 && layer%4==3 && role>=5 && role<=7;
  assert(PioneerQ38HybridBfp8Selection::Match(tensor)==wanted);
  assert(PioneerQ38HybridBfp8Selection::MatchQgw8(tensor)==wantedQgw8);
  assert(PioneerQ38HybridBfp8Selection::MatchDenseQ8(tensor)==wantedDense);
  total++;selected+=wanted;qgw8+=wantedQgw8;dense+=wantedDense;
  if(wantedDense){
   PioneerQ38NativeBfp8WeightTiles::Layout layout;assert(PioneerQ38NativeBfp8WeightTiles::Describe(tensor,layout));denseBytes+=layout.bytes;
   auto bad=tensor;bad.type++;assert(!PioneerQ38HybridBfp8Selection::MatchDenseQ8(bad));
   bad=tensor;bad.rank++;assert(!PioneerQ38HybridBfp8Selection::MatchDenseQ8(bad));
   bad=tensor;bad.inputs++;assert(!PioneerQ38HybridBfp8Selection::MatchDenseQ8(bad));
   bad=tensor;bad.outputs++;assert(!PioneerQ38HybridBfp8Selection::MatchDenseQ8(bad));
   bad=tensor;bad.bytes++;assert(!PioneerQ38HybridBfp8Selection::MatchDenseQ8(bad));
  }
  if(wanted && !wantedQgw8){
   PioneerQ38NativeBfp8WeightTiles::Layout layout;
   assert(PioneerQ38NativeBfp8WeightTiles::Describe(tensor,layout));omittedBytes+=layout.bytes;
  }
  if(wanted){
   auto bad=tensor;bad.type++;assert(!PioneerQ38HybridBfp8Selection::Match(bad));
   bad=tensor;bad.rank++;assert(!PioneerQ38HybridBfp8Selection::Match(bad));
   bad=tensor;bad.inputs++;assert(!PioneerQ38HybridBfp8Selection::Match(bad));
   bad=tensor;bad.outputs++;assert(!PioneerQ38HybridBfp8Selection::Match(bad));
   bad=tensor;bad.bytes++;assert(!PioneerQ38HybridBfp8Selection::Match(bad));
  }
 }
 assert(total==851 && selected==39 && qgw8==33 && omittedBytes==UINT64_C(568197120)
  && dense==48 && denseBytes==UINT64_C(1247805440));
 puts("PASS 851 tensors: old 39/QGW8 33 FFNs, 48 dense Q8 tensors; malformed shapes refused; NO MODEL OR DEVICE");
}
