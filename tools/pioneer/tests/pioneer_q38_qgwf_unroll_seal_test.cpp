/* SPDX-License-Identifier: MIT */
// Actual emitted QGWF instructions with synthetic endpoints only.
#define PIONEER_Q38_HYBRID_WORKER_MACHINE_ONLY
#include "pioneer_q38_hybrid_worker_image_test.cpp"
#include <fstream>

static unsigned Symbol(const char* folder,const char* name){
 std::ifstream in(std::string(folder)+"/program.symbols");std::string a,k,n;
 while(in>>a>>k>>n)if(n==name)return unsigned(std::stoul(a,nullptr,16));
 assert(false);return 0;
}
static std::unique_ptr<HybridImage> Enter(const char* folder,const char* leaf){
 auto image=std::make_unique<HybridImage>(folder,leaf,1,5120);image->Boot();image->Stage(0,1,2);
 const unsigned seal=Symbol(folder,"q38_mixed_seal");assert(seal);
 while(image->pc!=seal && image->pc!=image->faultPC)image->StepWorker();
 assert(image->pc==seal && !image->transfer.issues && !image->stores);return image;
}
static unsigned Clean(const char* folder,const char* leaf){
 auto image=Enter(folder,leaf);const unsigned back=image->regs[1],before=image->steps;
 while(image->pc!=back && image->pc!=image->faultPC)image->StepWorker();
 assert(image->pc==back && !image->transfer.issues && !image->stores);return image->steps-before;
}
int main(int argc,char** argv){
 assert(argc==4);const unsigned baseline=Clean(argv[1],argv[3]),optimized=Clean(argv[2],argv[3]);
 assert(optimized<baseline && baseline-optimized==143);
 for(unsigned candidate=0;candidate<2;candidate++)for(unsigned which=0;which<2;which++)for(unsigned i=0;i<64;i++)for(uint32_t mask:{1U,0x80000000U,0xffffffffU}){
  auto image=Enter(argv[1+candidate],argv[3]);const unsigned address=(which?HW::Job:HW::Boot)+i*4;
  const unsigned back=image->regs[1];image->Put(address,image->Get(address)^mask);
  while(image->pc!=back && image->pc!=image->faultPC)image->StepWorker();
  assert(image->pc==image->faultPC && !image->transfer.issues && !image->stores);
 }
 printf("PASS emitted QGWF seal %u -> %u instructions and 768 single-word faults; synthetic only, NO DEVICE OR TPS\n",baseline,optimized);
}
