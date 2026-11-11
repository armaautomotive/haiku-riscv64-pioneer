/* SPDX-License-Identifier: MIT */
// Simulated QGWA worker receipts into actual linked ROOT receipt instructions.
#define main pioneer_unused_head_worker_main
#include "pioneer_q38_hybrid_head_worker_image_test.cpp"
#undef main
#include "pioneer_q38_hybrid_linked_receipt.h"
static void Test(const char* worker,const char* leaf,HybridLinkedReceipt& root,HybridLinkedReceipt& old,
 unsigned width,unsigned bands,bool head){
 auto image=std::make_unique<HybridImage>(worker,leaf,head?1:119,width,bands);image->Boot();
#ifdef PIONEER_Q38_QGWE_BATCH_DENSE_WORKER
 if(!head)image->Put(image->regs[8]-1136,50); // synthetic history only
#else
 if(!head)image->Put(image->regs[8]-1548,50); // synthetic history only
#endif
 const unsigned indices[]={2,3,4,5,1,8,4,10};uint32_t before[8],after[8],proof[64],job[64];
 for(unsigned i=0;i<8;i++)before[i]=image->transfer.counters[indices[i]];
 if(head)StageHead(*image,false);else image->Stage(50,1,width==5120?2:4);
 assert(image->Finish(1));
 for(unsigned i=0;i<64;i++){proof[i]=image->Get(0x83f00+i*4);job[i]=image->Get(HW::Job+i*4);}
 for(unsigned i=0;i<8;i++)after[i]=image->transfer.counters[indices[i]];
 assert(root.Run(proof,job,image->boot.data(),before,after));
 assert(!old.Run(proof,job,image->boot.data(),before,after));
 for(unsigned word:{0U,8U,19U,20U,22U,23U,28U,32U,48U,49U,50U,56U,57U}){
  proof[word]^=1;assert(!root.Run(proof,job,image->boot.data(),before,after));proof[word]^=1;
 }
 after[0]++;assert(!root.Run(proof,job,image->boot.data(),before,after));
}
int main(int argc,char** argv){
 assert(argc==5);HybridLinkedReceipt root(argv[3]),old(argv[4]);
 for(unsigned bands:{1U,2U,3U,8U})Test(argv[1],argv[2],root,old,5120,bands,true);
 for(unsigned width:{5120U,17408U})Test(argv[1],argv[2],root,old,width,2,false);
#ifdef PIONEER_Q38_QGWE_BATCH_DENSE_WORKER
 puts("PASS simulated QGWE worker receipts accepted by emitted QGWE ROOT predicate, old ROOT rejected, mutated proofs refused; NO FULL ROOT OR DEVICE");
#else
 puts("PASS simulated QGWA worker receipts accepted by emitted QGWA ROOT predicate, old ROOT rejected, mutated proofs refused; NO FULL ROOT OR DEVICE");
#endif
 return 0;
}
