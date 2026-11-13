/* SPDX-License-Identifier: MIT */
#define PIONEER_NATIVE_WEIGHT_TILES_FIXTURE_ONLY
#include "pioneer_q38_native_weight_tiles_test.cpp"
#define PIONEER_Q38_RECURRENT_SMALL_BFP8_PREPARER 1
#include "../PioneerQ38RecurrentSmallBfp8Selection.h"
int main(){
 unsigned total=0,selected=0;
 for(unsigned layer=0;layer<=64;layer++)for(unsigned role=0;;role++){
  PioneerQ38Tensor t;if(!PioneerQ38Expected(layer,role,t))break;total++;
  const bool expected=layer<64 && layer%4!=3 && (role==10 || role==11);
  assert(PioneerQ38RecurrentSmallBfp8Selection::Match(t)==expected);
  if(expected){
   selected++;assert(t.inputs==5120 && t.outputs==48 && t.type==8);
   for(unsigned field=0;field<5;field++){
    auto bad=t;if(field==0)bad.type++;else if(field==1)bad.rank++;else if(field==2)bad.inputs++;
    else if(field==3)bad.outputs++;else bad.bytes++;
    assert(!PioneerQ38RecurrentSmallBfp8Selection::Match(bad));
   }
  }
 }
 assert(total==851 && selected==96);
 Source source(8,5120,48);PioneerQ38NativeBfp8WeightTiles::Layout layout;
 assert(PioneerQ38NativeBfp8WeightTiles::Describe(source.tensors[0],layout));
 assert(layout.rowTiles==2 && layout.columnTiles==160 && layout.bytes==2*160*1088);
 for(unsigned column:{0U,79U,159U}){
  std::array<float,1024> original{},decoded{};std::array<uint8_t,1088> packed{};
  const unsigned tile=160+column;
  assert(PioneerQ38NativeWeightTiles::ReadFloatTile(source,0,tile,original.data(),original.size()));
  assert(PioneerQ38NativeBfp8WeightTiles::ReadRange(source,0,uint64_t(tile)*1088,1088,packed.data(),packed.size()));
  assert(PioneerBlackholeBlockFloat::UnpackTile(PioneerBlackholeBlockFloat::Format::Bfp8B,packed.data(),packed.size(),decoded.data(),decoded.size()));
  for(unsigned row=16;row<32;row++)for(unsigned c=0;c<32;c++)assert(original[row*32+c]==0 && decoded[row*32+c]==0);
 }
 puts("PASS 96 separate alpha/beta selections and padded last BFP8 rows; synthetic source, NO device or speed");
}
