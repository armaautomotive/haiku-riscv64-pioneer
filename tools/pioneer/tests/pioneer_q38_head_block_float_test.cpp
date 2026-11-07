/* SPDX-License-Identifier: MIT */
// Exact synthetic Q6 row decode -> experimental head BFP bands. NO DEVICE.
#define PIONEER_Q38_HEAD_BLOCK_FLOAT_DIAGNOSTIC 1
#define PIONEER_NATIVE_WEIGHT_TILES_FIXTURE_ONLY
#include "pioneer_q38_native_weight_tiles_test.cpp"
#include "../PioneerQ38HeadBlockFloatDiagnostic.h"

struct HeadSource:Source {
 HeadSource():Source(14,5120,64){
  PioneerQ38Tensor expected;assert(PioneerQ38Expected(64,1,expected));
  tensors[0].name=expected.name;tensors[0].outputs=expected.outputs;tensors[0].bytes=expected.bytes;
 }
 bool ReadRow(unsigned index,unsigned row,float* output,unsigned capacity){
  if(index || row>=64 || capacity<5120)return false;
  std::array<uint8_t,4200> packed={};if(!ReadPacked(index,uint64_t(row)*4200,4200,packed.data(),packed.size()))return false;
  for(unsigned block=0;block<20;block++)if(!PioneerDecodeQ6K(packed.data()+block*210,output+block*256))return false;
  return true;
 }
};
int main(){
 using namespace PioneerQ38HeadBlockFloatDiagnostic;
 unsigned compared=0;
 for(auto format:{Codec::Format::Bfp4B,Codec::Format::Bfp8B})for(unsigned first:{0U,32U}){
  HeadSource source,reference;std::vector<uint8_t> actual;
  assert(PackBand(source,0,first,format,actual) && source.reads==32 && actual.size()==160*Codec::Bytes(format));
  for(unsigned col=0;col<160;col++){
   std::array<float,1024> tile={};std::array<uint8_t,Codec::MaxBytes> expected={};
   assert(PioneerQ38NativeWeightTiles::ReadFloatTile(reference,0,uint64_t(first/32)*160+col,tile.data(),tile.size()));
   assert(Codec::PackTile(format,tile.data(),tile.size(),expected.data(),expected.size()));
   assert(!memcmp(actual.data()+col*Codec::Bytes(format),expected.data(),Codec::Bytes(format)));compared++;
  }
 }
 for(unsigned bad=0;bad<13;bad++){
  HeadSource source;unsigned index=0,first=0;auto format=Codec::Format::Bfp4B;
  if(bad==0)index=1;if(bad==1)first=1;if(bad==2)first=248320;
  if(bad==3)format=Codec::Format(0);if(bad==4)source.tensors[0].name="token_embd.weight";
  if(bad==5)source.tensors[0].type=12;if(bad==6)source.tensors[0].rank=1;
  if(bad==7)source.tensors[0].inputs--;if(bad==8)source.tensors[0].outputs--;
  if(bad==9)source.tensors[0].bytes--;if(bad==10)source.ready=false;
  if(bad==11)source.fail=5;if(bad==12)source.change=5;
  std::vector<uint8_t> output(7,0xa5),saved=output;
  assert(!PackBand(source,index,first,format,output) && output==saved);
  if(bad<11)assert(!source.reads);
 }
 HeadSource rounding;std::vector<uint8_t> output(7,0xa5),saved=output;
 assert(!std::fesetround(FE_UPWARD));assert(!PackBand(rounding,0,0,Codec::Format::Bfp4B,output) && output==saved && !rounding.reads);
 assert(!std::fesetround(FE_TONEAREST));
 printf("PASS Q6 output-head BFP4/BFP8 conversion: %u exact synthetic tiles, 13 input/source failures and rounding refusal; NO original model, device, model quality or speed\n",compared);
}
