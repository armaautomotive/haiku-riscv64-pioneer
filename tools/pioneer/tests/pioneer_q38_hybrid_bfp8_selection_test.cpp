/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38HybridBfp8Selection.h"
#include <cassert>
#include <cstdio>
int main(){
 unsigned selected=0,total=0;
 for(unsigned layer=0;layer<=64;layer++)for(unsigned role=0;;role++){
  PioneerQ38Tensor tensor;if(!PioneerQ38Expected(layer,role,tensor))break;
  const bool wanted=layer>=48 && layer<=60 && role>=2 && role<=4;
  assert(PioneerQ38HybridBfp8Selection::Match(tensor)==wanted);total++;selected+=wanted;
  if(wanted){
   auto bad=tensor;bad.type++;assert(!PioneerQ38HybridBfp8Selection::Match(bad));
   bad=tensor;bad.rank++;assert(!PioneerQ38HybridBfp8Selection::Match(bad));
   bad=tensor;bad.inputs++;assert(!PioneerQ38HybridBfp8Selection::Match(bad));
   bad=tensor;bad.outputs++;assert(!PioneerQ38HybridBfp8Selection::Match(bad));
   bad=tensor;bad.bytes++;assert(!PioneerQ38HybridBfp8Selection::Match(bad));
  }
 }
 assert(total==851 && selected==39);
 puts("PASS fixed 851-tensor selection: 39 exact BFP8 FFN tensors, malformed selected shapes refused; NO MODEL OR DEVICE");
}
