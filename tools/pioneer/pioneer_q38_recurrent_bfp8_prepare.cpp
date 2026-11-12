/* SPDX-License-Identifier: MIT */
// Filter the unchanged frozen all-BFP8 writer to exactly 144 large
// recurrent Q8 tensors. CPU preparation only; never touches a device.
#if !defined(PIONEER_Q38_RECURRENT_Q8_BFP8_PREPARER) || PIONEER_Q38_RECURRENT_Q8_BFP8_PREPARER != 1 \
 || !defined(PIONEER_Q38_ALL_BFP8_CACHE) || PIONEER_Q38_ALL_BFP8_CACHE != 1
#error Recurrent Q8 preparer requires its exact distinct all-BFP8 role
#endif
#include "PioneerQ38RecurrentBfp8Selection.h"
#include <cstring>
static unsigned selectedTensors=0;
struct PioneerQ38RecurrentBfp8Tiles {
 using Base=PioneerQ38NativeBfp8WeightTiles;using Layout=Base::Layout;
 static bool Describe(const PioneerQ38Tensor& tensor,Layout& out){
  if(!PioneerQ38RecurrentBfp8Selection::Match(tensor))return false;
  Layout staged;if(!Base::Describe(tensor,staged) || staged.format!=PioneerBlackholeBlockFloat::Format::Bfp8B)return false;
  selectedTensors++;out=staged;return true;
 }
 template<class Source>static bool ReadRange(Source& source,unsigned tensor,uint64_t offset,unsigned bytes,
  uint8_t* output,unsigned capacity){return Base::ReadRange(source,tensor,offset,bytes,output,capacity);}
};
#ifndef PIONEER_Q38_RECURRENT_BFP8_TILE_TEST_ONLY
#define PioneerQ38NativeBfp8WeightTiles PioneerQ38RecurrentBfp8Tiles
#define main PioneerQ38FrozenRecurrentWriterMain
#include "pioneer_q38_native_weight_cache.cpp"
#undef main
#undef PioneerQ38NativeBfp8WeightTiles
int main(int argc,char** argv){
 if(argc!=4 || std::strcmp(argv[1],"--build") || argv[2][0]!='/' || argv[3][0]!='/'){
  fputs("Use --build ABSOLUTE_ORIGINAL_GGUF ABSOLUTE_DEDICATED_CACHE; CPU-only.\n",stderr);return 2;
 }
 const int result=PioneerQ38FrozenRecurrentWriterMain(argc,argv);
 if(result || selectedTensors!=144){
  fprintf(stderr,"RECURRENT_Q8_BFP8_PREPARATION_INCOMPLETE selected%u; NO DEVICE\n",selectedTensors);return 1;
 }
 puts("PASS_RECURRENT_Q8_BFP8_PREPARATION 144 selected tensors, frozen all-BFP8 recipe; NO DEVICE OR SPEED");return 0;
}
#endif
