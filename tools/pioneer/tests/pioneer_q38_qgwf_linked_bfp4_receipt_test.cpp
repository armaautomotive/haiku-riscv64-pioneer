/* SPDX-License-Identifier: MIT */
// Actual QGWF worker/ROOT receipt instructions on mocked fabric only.
#define PIONEER_Q38_HYBRID_WORKER_MACHINE_ONLY
#include "pioneer_q38_hybrid_worker_image_test.cpp"
#include "pioneer_q38_hybrid_linked_receipt.h"
int main(int argc,char** argv){
 assert(argc==5);HybridLinkedReceipt root(argv[3]),old(argv[4]);
 for(unsigned width:{5120U,17408U}){
  HybridImage image(argv[1],argv[2],119,width);image.Boot();
  const unsigned indices[]={2,3,4,5,1,8,4,10};uint32_t before[8],after[8],proof[64],job[64];
  for(unsigned i=0;i<8;i++)before[i]=image.transfer.counters[indices[i]];
  image.Stage(0,1,width==5120?2:4);assert(image.Finish(1));
  for(unsigned i=0;i<64;i++){proof[i]=image.Get(0x83f00+i*4);job[i]=image.Get(HW::Job+i*4);}
  for(unsigned i=0;i<8;i++)after[i]=image.transfer.counters[indices[i]];
  assert(root.Run(proof,job,image.boot.data(),before,after));
  assert(!old.Run(proof,job,image.boot.data(),before,after));
  for(unsigned word:{0U,8U,19U,20U,22U,23U,28U,32U,48U,49U,50U,56U,57U}){
   proof[word]^=1;assert(!root.Run(proof,job,image.boot.data(),before,after));proof[word]^=1;
  }
  after[0]++;assert(!root.Run(proof,job,image.boot.data(),before,after));
 }
 puts("PASS simulated QGWF BFP4 worker receipts accepted by emitted QGWF ROOT; old ROOT and mutations rejected; NO DEVICE OR TPS");
}
