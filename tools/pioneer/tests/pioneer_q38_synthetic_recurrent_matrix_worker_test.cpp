/* SPDX-License-Identifier: MIT */
// Separate offline worker execution using synthetic weights and input only.
#define PIONEER_TEST_BFP8 1
#define PIONEER_Q38_DENSE_Q8_CONNECTED_TEST 1
#define PIONEER_Q38_DENSE_Q8_MATRIX_ROOT_INSPECTION 1
#define PIONEER_Q38_EMBEDDING_RAW_TEST_BRIDGE 1
#include "pioneer_q38_dense_q8_producer_image_test.cpp"
#include "../PioneerQ38DenseQ8MatrixRootFrame.h"
#include "pioneer_q38_embedding_raw_test_bridge.h"
#include <memory>
extern bool Q38SyntheticRecurrentAddress(unsigned& bank,unsigned& address);

int main(int argc,char** argv){
 assert(argc==4);Q38EmbeddingRawTestBridge source;assert(source.Open(argv[3]) && source.Ready());
 unsigned bank=0,address=0;assert(Q38SyntheticRecurrentAddress(bank,address));
 for(unsigned band=0;band<2;band++){
 DenseProducerImage image(argv[1],argv[2],5120,1,band+1);image.Boot();
 for(auto& word:image.input)word=0x3f800000;image.StageProducer(10,1,false,0);image.Check(1);
 // This worker's 256 inputs are actual producer output; the rest are the
 // same explicit unit-valued synthetic input, not generated model state.
 for(unsigned i=0;i<5120;i++){image.transfer.root[i*2]=0x80;image.transfer.root[i*2+1]=0x3f;}
 for(unsigned at=0;at<512;at+=4){
  const auto word=image.bank.at(image.descriptor[4]+image.descriptor[12]+at);
  for(unsigned b=0;b<4;b++)image.transfer.root[band*512+at+b]=uint8_t(word>>(8*b));
 }
 const uint32_t tuple[]={address+band*174080,bank,band*32,1};uint32_t job[64]={};
 assert(PioneerQ38DenseQ8MatrixRootFrame::Build(tuple,image.boot.data(),0,10,0,2025,1,job));
 image.RearmTestAccounting();image.steps=0;
 for(unsigned i=0;i<16;i++)image.transfer.job.words[i]=job[16+i];
 assert(source.Read(PioneerQ38ProjectionRoute(bank),tuple[0],image.transfer.weights.data(),unsigned(image.transfer.weights.size())));
 for(unsigned i=0;i<64;i++)image.Put(HW::Job+i*4,job[i]);image.Put(HW::Doorbell,2);
 assert(image.FinishCombined(2) && image.native.Clean() && image.native.matrixOps>0);
 for(unsigned row=0;row<32;row++){
  const unsigned global=band*32+row;int sum=0;
  if(global<48)for(unsigned tile=0;tile<160;tile++)for(unsigned col=0;col<32;col++)sum+=int((global*7+tile*3+col*5)%17)-8;
  const float expected=float(sum)*0.0625f;const float actual=PioneerAttentionFloat(image.Get(W::Output+row*4));
  assert(actual==expected);
 }
 }
 puts("PASS actual recurrent worker synthetic BFP8 matrix/two bands and 64 rows including padding vs independent exact reference; SEPARATE SIMULATED WORKERS, NO MODEL OR DEVICE/TPS");
}
