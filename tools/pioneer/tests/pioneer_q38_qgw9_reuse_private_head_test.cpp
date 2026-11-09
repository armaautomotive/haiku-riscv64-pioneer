/* SPDX-License-Identifier: MIT */
// Actual experimental worker in simulated fabric, with PRIVATE real head data.
// No ROOT/lifetime/device authority. Never publish resulting binary.
#define PIONEER_Q38_HYBRID_WORKER_MACHINE_ONLY
#define PIONEER_TEST_BFP8 1
#include "pioneer_q38_hybrid_worker_image_test.cpp"
#include "PioneerNativeBfpModelBandData.h"
#include "PioneerNativeBfpModelBandMetadata.h"

int main(int argc,char** argv){
 assert(argc==3);
 for(unsigned sample=0;sample<3;sample++)for(unsigned peer:{1U,63U,119U}){
  auto image=std::make_unique<HybridImage>(argv[1],argv[2],peer,5120,2);image->Boot();
  for(unsigned tile=0;tile<160;tile++){
   for(unsigned at=0;at<1088;at+=4){
    const uint32_t word=PioneerNativeBfpModelBandData::Word(sample,5120,0x90000+tile*1104+at);
    for(unsigned band=0;band<2;band++)for(unsigned byte=0;byte<4;byte++)
     image->transfer.weights[(band*160+tile)*1088+at+byte]=uint8_t(word>>(byte*8));
   }
   for(unsigned at=0;at<64;at+=4){
    const uint32_t word=PioneerNativeBfpModelBandData::Word(sample,5120,0x124000+tile*80+at);
    for(unsigned byte=0;byte<4;byte++)image->transfer.root[tile*64+at+byte]=uint8_t(word>>(byte*8));
   }
  }
  image->Put(image->regs[8]-1548,64); // injected history, not a full model.
  auto& leaf=image->transfer.job;leaf.words[11]=1;uint32_t j[64]={};
  j[0]=HW::Magic;j[1]=1;j[2]=HW::GlobalMatrix;j[3]=image->boot[3];j[4]=image->boot[4];j[5]=64;j[7]=2025;
  for(unsigned i=0;i<8;i++)j[8+i]=image->boot[8+i];for(unsigned i=0;i<16;i++)j[16+i]=leaf.words[i];
  j[32]=PioneerNativeBfpModelBandMetadata::First(sample);j[33]=HW::Vocabulary;j[34]=0;j[35]=HW::JobWitness;
  assert(HW::JobShape(j,image->boot.data(),0,0));
  for(unsigned i=0;i<64;i++)image->Put(HW::Job+i*4,j[i]);image->Put(HW::Doorbell,1);
  assert(image->Finish(1));
#ifdef PIONEER_Q38_QGW8_BFP8_BATCH_WORKER
  assert(image->transfer.issues==322);
#else
  assert(image->inputPreserved && image->transfer.issues==480);
#endif
  const auto* expected=PioneerNativeBfpModelBandMetadata::Golden(sample);assert(expected);
  for(unsigned row=0;row<64;row++)assert(image->Get(W::Output+row*4)==expected[row%32]);
 }
 puts("PASS emitted worker: 3 private real-head bands x 3 peers, 576 exact words and reduced reads; SIMULATED FABRIC, NO DEVICE OR OWNERSHIP GRANT");
}
