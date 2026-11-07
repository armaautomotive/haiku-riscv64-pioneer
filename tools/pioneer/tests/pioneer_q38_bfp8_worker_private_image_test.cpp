/* SPDX-License-Identifier: MIT */
// Compiles locally against pinned, PRIVATE prior real-model diagnostic data.
// The resulting binary contains learned values and must never be published.
#define PIONEER_BFP8_WORKER_IMAGE_ONLY 1
#include "pioneer_q38_bfp8_worker_leaf_image_test.cpp"
#include "PioneerNativeBfpModelBandData.h"
#include "PioneerNativeBfpModelBandMetadata.h"
int main(int argc,char** argv){
 assert(argc==2);
 for(unsigned sample=0;sample<3;sample++){
  WorkerImage image(argv[1],63,5120);
  for(unsigned tile=0;tile<160;tile++){
   for(unsigned at=0;at<1088;at+=4){
    const uint32_t word=PioneerNativeBfpModelBandData::Word(sample,5120,0x90000+tile*1104+at);
    for(unsigned band=0;band<2;band++)for(unsigned byte=0;byte<4;byte++)
     image.transfer.weights[(band*160+tile)*1088+at+byte]=uint8_t(word>>(byte*8));
   }
   for(unsigned at=0;at<64;at+=4){
    const uint32_t word=PioneerNativeBfpModelBandData::Word(sample,5120,0x124000+tile*80+at);
    for(unsigned byte=0;byte<4;byte++)image.transfer.root[tile*64+at+byte]=uint8_t(word>>(byte*8));
   }
  }
  assert(image.Execute());const auto* expected=PioneerNativeBfpModelBandMetadata::Golden(sample);assert(expected);
  for(unsigned band=0;band<2;band++)for(unsigned row=0;row<32;row++)
   assert(image.l1[image.Index(W::Output+(band*32+row)*4)]==expected[row]);
  printf("PASS actual OFFBOARD BFP8 worker RV32 real-model sample%u: 64 prior-predicted words, ROOT-L1/GDDR simulation; NO DEVICE\n",sample);
 }
}
