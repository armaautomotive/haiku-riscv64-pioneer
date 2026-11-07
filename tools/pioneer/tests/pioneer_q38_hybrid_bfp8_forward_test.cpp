/* SPDX-License-Identifier: MIT */
#define PIONEER_NATIVE_WEIGHT_TILES_FIXTURE_ONLY
#include "pioneer_q38_native_weight_tiles_test.cpp"
#define PIONEER_Q38_HYBRID_BFP8_PREPARER 1
#define PIONEER_Q38_ALL_BFP8_CACHE 1
#define PIONEER_Q38_HYBRID_BFP8_TILE_TEST_ONLY 1
#include "../pioneer_q38_hybrid_bfp8_prepare.cpp"
int main(){
 Source source(12,5120,17408);PioneerQ38Tensor expected;
#ifdef PIONEER_Q38_HYBRID_BFP8_WINDOW11
 assert(PioneerQ38Expected(50,2,expected));
#else
 assert(PioneerQ38Expected(48,2,expected));
#endif
 source.tensors[0].name=expected.name;
 PioneerQ38HybridBfp8Tiles::Layout wrapped,original;
 assert(PioneerQ38HybridBfp8Tiles::Describe(source.tensors[0],wrapped));
 assert(PioneerQ38NativeBfp8WeightTiles::Describe(source.tensors[0],original));
 assert(wrapped.format==original.format && wrapped.bytes==original.bytes
  && wrapped.rowTiles==original.rowTiles && wrapped.columnTiles==original.columnTiles);
 for(uint64_t at:{UINT64_C(0),UINT64_C(59999),original.bytes-4096}){
  std::array<uint8_t,4096> a{},b{};
  assert(PioneerQ38HybridBfp8Tiles::ReadRange(source,0,at,a.size(),a.data(),a.size()));
  assert(PioneerQ38NativeBfp8WeightTiles::ReadRange(source,0,at,b.size(),b.data(),b.size()) && a==b);
 }
 const std::string recipe="pioneer-native-weight-tiles-v2-all-bfp8/outputs32-K32/faces16-row-major/zero-padding/"
  "Q4_K,Q8_0,Q6_K-to-BFP8_B/shared-exp-FP32-bias/align-truncate-then-RNE-saturate/"
  "no-subnormal-input/source-fingerprint=7db613bfdde463be7f933f25123a00dbe39928cbd8a070aa2481c6cb936753d0";
 assert(PioneerModelWeightCache::Hex(PioneerModelWeightCache::Hash(recipe.data(),recipe.size()))==
  "551c6713a31c749d0dad82e616c592f01aba6854af8781f6756b4f60d85bc897");
 puts("PASS filtered BFP8 wrapper: frozen reader byte-identical at 3 cross-tile positions; original cache recipe exact; SYNTHETIC NO DEVICE");
}
