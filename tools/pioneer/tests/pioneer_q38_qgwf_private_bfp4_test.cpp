/* SPDX-License-Identifier: MIT */
// Pinned real-derived BFP4 bands; simulated fabric, no model or device run.
#define PIONEER_Q38_HYBRID_WORKER_MACHINE_ONLY
#include "pioneer_q38_hybrid_worker_image_test.cpp"
int main(int argc,char** argv){
 assert(argc==4);const auto sample=Read(argv[3]);
 assert(sample.size()==102544 && Word(sample,0)==0x34504252 && Word(sample,4)==1 && Word(sample,8)==5120);
 for(unsigned peer:{1U,63U,119U}){
  HybridImage image(argv[1],argv[2],peer,5120);image.Boot();
  for(unsigned band=0;band<2;band++)std::memcpy(image.transfer.weights.data()+band*160*576,sample.data()+16,160*576);
  std::memcpy(image.transfer.root.data(),sample.data()+16+160*576,10240);
  image.Stage(0,1,2);assert(image.Finish(1));
  for(unsigned band=0;band<2;band++)for(unsigned row=0;row<32;row++)
   assert(image.Get(W::Output+(band*32+row)*4)==Word(sample,16+160*576+10240+row*4));
 }
 puts("PASS QGWF emitted BFP4 worker on one private real-derived band across 3 simulated peers, 192 exact words; NO DEVICE OR TPS");
}
