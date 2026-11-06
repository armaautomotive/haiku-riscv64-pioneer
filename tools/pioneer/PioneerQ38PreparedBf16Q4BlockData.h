/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_PREPARED_BF16_Q4_BLOCK_DATA_H
#define PIONEER_Q38_PREPARED_BF16_Q4_BLOCK_DATA_H
#include "PioneerQ38Q4FpuCompactData.h"
#include "PioneerQ38Q4EscapeTiles.h"
// Isolated comparison PREPARATION, before device Open. Original raw rows ->
// existing cache-format primaries plus dense escape masks. No float weight
// conversion, CPU model inference/reference, cache writer or device access.
// Dense masks here are NOT a change to the model's sparse on-disk recipe.
struct PioneerQ38PreparedBf16Q4BlockData {
 static bool Prepare(const uint8_t* original,unsigned bytes,uint8_t* out,unsigned capacity){
  if(bytes!=4608 || capacity<5632 || !PioneerQ38Q4EscapeTiles::Separate(original,bytes,out,5632))return false;
  std::array<uint8_t,4608> compact{};std::array<uint8_t,5632> next{};
  if(!PioneerQ38Q4FpuCompactData::Prepare(original,bytes,compact.data(),compact.size()))return false;
  for(unsigned half=0;half<2;half++){
   std::array<uint8_t,2304> tile{};std::array<uint8_t,2816> encoded{};
   memcpy(tile.data(),compact.data()+4096+half*256,256);memcpy(tile.data()+256,compact.data()+half*2048,2048);
   if(!PioneerQ38Q4EscapeTiles::Encode(tile.data(),tile.size(),encoded.data(),encoded.size()))return false;
   memcpy(next.data()+half*2304,encoded.data(),2304);memcpy(next.data()+4608+half*512,encoded.data()+2304,512);
  }
  memcpy(out,next.data(),next.size());return true;
 }
};
#endif
