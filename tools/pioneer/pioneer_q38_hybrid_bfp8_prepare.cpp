/* SPDX-License-Identifier: MIT */
// Run the existing pinned all-BFP8 writer verbatim with only its tensor
// selection narrowed. The inherited numerical ReadRange is the SAME codec;
// its original conversion recipe/fingerprint and cache keys remain valid.
#if !defined(PIONEER_Q38_HYBRID_BFP8_PREPARER) || PIONEER_Q38_HYBRID_BFP8_PREPARER != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Hybrid BFP8 preparer is a distinct CPU-only role
#endif
#if !defined(PIONEER_Q38_ALL_BFP8_CACHE) || PIONEER_Q38_ALL_BFP8_CACHE != 1
#error The frozen all-BFP8 converter must be selected
#endif
#include "PioneerQ38HybridBfp8Selection.h"
#include <cstring>
static unsigned selectedTensors=0;
struct PioneerQ38HybridBfp8Tiles {
 using Base=PioneerQ38NativeBfp8WeightTiles;
 using Layout=Base::Layout;
 static bool Describe(const PioneerQ38Tensor& tensor,Layout& out){
  if(!PioneerQ38HybridBfp8Selection::Match(tensor))return false;
  Layout stage;if(!Base::Describe(tensor,stage) || stage.format!=PioneerBlackholeBlockFloat::Format::Bfp8B)return false;
  selectedTensors++;out=stage;return true;
 }
 template<class Source> static bool ReadRange(Source& source,unsigned tensor,uint64_t offset,unsigned bytes,
  uint8_t* output,unsigned capacity){return Base::ReadRange(source,tensor,offset,bytes,output,capacity);}
};
// The included writer's only substituted operation is Describe's selection;
// its cache, same-open full hash, conversion, publication, and recipe remain
// the original source. The builder pins the original source fingerprint.
#ifndef PIONEER_Q38_HYBRID_BFP8_TILE_TEST_ONLY
#define PioneerQ38NativeBfp8WeightTiles PioneerQ38HybridBfp8Tiles
#define main PioneerQ38FrozenBfp8WriterMain
#include "pioneer_q38_native_weight_cache.cpp"
#undef main
#undef PioneerQ38NativeBfp8WeightTiles
int main(int argc,char** argv){
 if(argc!=4 || strcmp(argv[1],"--build") || argv[2][0]!='/' || argv[3][0]!='/'){
  fputs("Use --build ABSOLUTE_ORIGINAL_GGUF ABSOLUTE_DEDICATED_CACHE; CPU-only.\n",stderr);return 2;
 }
 const int result=PioneerQ38FrozenBfp8WriterMain(argc,argv);
 if(result || selectedTensors!=39){
  fprintf(stderr,"HYBRID_BFP8_PREPARATION_INCOMPLETE selected%u; NO DEVICE\n",selectedTensors);return 1;
 }
 puts("PASS_HYBRID_BFP8_PREPARATION 39 selected FFN tensors, frozen all-BFP8 recipe; NO DEVICE OR SPEED");
 return fflush(stdout)?1:0;
}
#endif
