/* SPDX-License-Identifier: MIT */
// Select only the 96 alpha/beta matrices for the frozen BFP8 cache writer.
#if !defined(PIONEER_Q38_RECURRENT_SMALL_BFP8_PREPARER) || PIONEER_Q38_RECURRENT_SMALL_BFP8_PREPARER != 1 \
 || !defined(PIONEER_Q38_ALL_BFP8_CACHE) || PIONEER_Q38_ALL_BFP8_CACHE != 1
#error Small recurrent preparer requires its exact distinct all-BFP8 role
#endif
#include "PioneerQ38RecurrentSmallBfp8Selection.h"
#include <cstring>
static unsigned selectedTensors=0;
struct PioneerQ38RecurrentSmallBfp8Tiles {
 using Base=PioneerQ38NativeBfp8WeightTiles;using Layout=Base::Layout;
 static bool Describe(const PioneerQ38Tensor& tensor,Layout& out){
  if(!PioneerQ38RecurrentSmallBfp8Selection::Match(tensor))return false;
  Layout staged;if(!Base::Describe(tensor,staged) || staged.format!=PioneerBlackholeBlockFloat::Format::Bfp8B
   || staged.rowTiles!=2 || staged.columnTiles!=160 || staged.bytes!=348160)return false;
  selectedTensors++;out=staged;return true;
 }
 template<class Source>static bool ReadRange(Source& source,unsigned tensor,uint64_t offset,unsigned bytes,
  uint8_t* output,unsigned capacity){
  if(tensor>=source.Tensors().size() || !PioneerQ38RecurrentSmallBfp8Selection::Match(source.Tensors()[tensor]))return false;
  return Base::ReadRange(source,tensor,offset,bytes,output,capacity);
 }
};
#ifndef PIONEER_Q38_RECURRENT_SMALL_BFP8_TILE_TEST_ONLY
#define PioneerQ38NativeBfp8WeightTiles PioneerQ38RecurrentSmallBfp8Tiles
#define main PioneerQ38FrozenSmallRecurrentWriterMain
#include "pioneer_q38_native_weight_cache.cpp"
#undef main
#undef PioneerQ38NativeBfp8WeightTiles
int main(int argc,char** argv){
 if(argc!=4 || std::strcmp(argv[1],"--build") || argv[2][0]!='/' || argv[3][0]!='/'){
  fputs("Use --build ABSOLUTE_ORIGINAL_GGUF ABSOLUTE_DEDICATED_CACHE; CPU-only.\n",stderr);return 2;
 }
 const int result=PioneerQ38FrozenSmallRecurrentWriterMain(argc,argv);
 if(result || selectedTensors!=96){
  fprintf(stderr,"SMALL_RECURRENT_BFP8_PREPARATION_INCOMPLETE selected%u; NO DEVICE\n",selectedTensors);return 1;
 }
 puts("PASS_SMALL_RECURRENT_BFP8_PREPARATION 96 selected tensors, frozen BFP8 recipe; NO DEVICE OR SPEED");return 0;
}
#endif
