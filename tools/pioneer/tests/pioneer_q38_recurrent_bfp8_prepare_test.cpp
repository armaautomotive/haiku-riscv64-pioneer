/* SPDX-License-Identifier: MIT */
#define PIONEER_NATIVE_WEIGHT_TILES_FIXTURE_ONLY
#include "pioneer_q38_native_weight_tiles_test.cpp"
#define PIONEER_Q38_RECURRENT_Q8_BFP8_PREPARER 1
#define PIONEER_Q38_ALL_BFP8_CACHE 1
#define PIONEER_Q38_RECURRENT_BFP8_TILE_TEST_ONLY 1
#include "../pioneer_q38_recurrent_bfp8_prepare.cpp"

int main(){
 unsigned total=0,selected=0;
 for(unsigned ordinal=0;ordinal<=64;ordinal++){
  const unsigned layer=ordinal?ordinal-1:64;
  for(unsigned role=0;;role++){
   PioneerQ38Tensor tensor;if(!PioneerQ38Expected(layer,role,tensor))break;total++;
   const bool expected=layer<64 && layer%4!=3 && (role==5 || role==6 || role==13);
   assert(PioneerQ38RecurrentBfp8Selection::Match(tensor)==expected);
   PioneerQ38RecurrentBfp8Tiles::Layout layout;
   assert(PioneerQ38RecurrentBfp8Tiles::Describe(tensor,layout)==expected);
   if(expected){
    selected++;assert(layout.bytes==tensor.outputs*(tensor.inputs/32)*34);
    for(unsigned field=0;field<5;field++){
     auto bad=tensor;
     if(field==0)bad.type++;else if(field==1)bad.rank++;else if(field==2)bad.inputs++;
     else if(field==3)bad.outputs++;else bad.bytes++;
     assert(!PioneerQ38RecurrentBfp8Selection::Match(bad));
    }
   }
  }
 }
 assert(total==851 && selected==144 && selectedTensors==144);
 for(unsigned role:{5U,6U,13U}){
  PioneerQ38Tensor expected;assert(PioneerQ38Expected(0,role,expected));
  Source source(8,unsigned(expected.inputs),unsigned(expected.outputs));source.tensors[0].name=expected.name;
  PioneerQ38RecurrentBfp8Tiles::Layout layout;assert(PioneerQ38RecurrentBfp8Tiles::Describe(source.tensors[0],layout));
  for(uint64_t at:{UINT64_C(0),UINT64_C(59999),layout.bytes-4096}){
   std::array<uint8_t,4096> wrapped{},base{};
   assert(PioneerQ38RecurrentBfp8Tiles::ReadRange(source,0,at,wrapped.size(),wrapped.data(),wrapped.size()));
   assert(PioneerQ38NativeBfp8WeightTiles::ReadRange(source,0,at,base.size(),base.data(),base.size()) && wrapped==base);
  }
 }
 const std::string recipe="pioneer-native-weight-tiles-v2-all-bfp8/outputs32-K32/faces16-row-major/zero-padding/"
  "Q4_K,Q8_0,Q6_K-to-BFP8_B/shared-exp-FP32-bias/align-truncate-then-RNE-saturate/"
  "no-subnormal-input/source-fingerprint=7db613bfdde463be7f933f25123a00dbe39928cbd8a070aa2481c6cb936753d0";
 assert(PioneerModelWeightCache::Hex(PioneerModelWeightCache::Hash(recipe.data(),recipe.size()))==
  "551c6713a31c749d0dad82e616c592f01aba6854af8781f6756b4f60d85bc897");
 puts("PASS exact 144 recurrent tensor selection, altered-metadata refusal and frozen BFP8 byte forwarding; SYNTHETIC NO MODEL OR DEVICE");
}
