/* SPDX-License-Identifier: MIT */
// Actual QGWB dispatcher and BFP8 leaf; synthetic NoC, FPU and layer history.
#define PIONEER_Q38_HYBRID_WORKER_MACHINE_ONLY
#define PIONEER_TEST_BFP8 1
#include "pioneer_q38_hybrid_worker_image_test.cpp"
#include <memory>
static void StageDense(HybridImage& image,unsigned role,bool corrupt=false){
 assert(image.pc==image.waitPC && image.regs[8]==0x70000);image.Put(image.regs[8]-1136,3);
 auto& leaf=image.transfer.job;leaf.words[11]=1;
 uint32_t j[64]={};j[0]=HW::Magic;j[1]=1;j[2]=1;j[3]=image.boot[3];j[4]=image.boot[4];j[5]=3;j[7]=2025;
 for(unsigned i=0;i<8;i++)j[8+i]=image.boot[8+i];for(unsigned i=0;i<16;i++)j[16+i]=leaf.words[i];
 j[32]=0;j[33]=role==5?12288:1024;j[34]=role;j[35]=8;
 assert(HW::DenseJob(j) && HW::JobShape(j,image.boot.data(),0,0));if(corrupt)j[33]--;
 for(unsigned i=0;i<64;i++)image.Put(HW::Job+i*4,j[i]);image.Put(HW::Doorbell,1);
}
int main(int argc,char** argv){
 assert(argc==3);
 for(unsigned role=5;role<=7;role++){
  auto image=std::make_unique<HybridImage>(argv[1],argv[2],1,5120,2);image->Boot();
  const unsigned indices[]={2,3,4,5,1,8,4,10};uint32_t before[8],after[8],proof[64],job[64];
  for(unsigned i=0;i<8;i++)before[i]=image->transfer.counters[indices[i]];
  StageDense(*image,role);assert(image->Finish(1) && image->native.Clean());
  const auto* golden=PioneerNativeBfpBandGolden(5120);assert(golden);
  for(unsigned row=0;row<64;row++)assert(image->Get(W::Output+row*4)==golden[row%32]);
  for(unsigned i=0;i<64;i++){proof[i]=image->Get(0x83f00+i*4);job[i]=image->Get(HW::Job+i*4);}
  for(unsigned i=0;i<8;i++)after[i]=image->transfer.counters[indices[i]];
  assert(PioneerQ38HybridMatrixReceipt::Matrix(proof,job,image->boot.data(),before,after));
  assert(image->native.matrixOps==2*16*160 && image->stores==64);
 }
 auto invalid=std::make_unique<HybridImage>(argv[1],argv[2],1,5120,2);invalid->Boot();
 StageDense(*invalid,5,true);assert(!invalid->Finish(1,true) && !invalid->native.matrixOps);
 puts("PASS actual QGWB dense BFP8 matrix jobs and receipts for Q/G,K,V; synthetic history/NoC/FPU; NO HARDWARE OR SPEED");
}
