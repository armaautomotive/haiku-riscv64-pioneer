/* SPDX-License-Identifier: MIT */
// Actual QGW8 worker and BFP8 leaf instructions against synthetic NoC/FPU.
// Injects a fake completed-layer count: NOT a 64-layer lifecycle or device.
#define PIONEER_Q38_HYBRID_WORKER_MACHINE_ONLY
#define PIONEER_TEST_BFP8 1
#include "pioneer_q38_hybrid_worker_image_test.cpp"
#include <fcntl.h>
#include <unistd.h>

static void StageHead(HybridImage& image,bool corrupt,const std::array<uint32_t,64>* generated=nullptr){
 assert(image.pc==image.waitPC && image.regs[8]==0x70000);
 // This offset is bound to the pinned exact worker binary by the runner.
 // Synthetic injection tests only the output job after the omitted layers.
 image.Put(image.regs[8]-1548,64);
 auto& leaf=image.transfer.job;leaf.words[11]=1;
 uint32_t j[64]={};j[0]=HW::Magic;j[1]=1;j[2]=HW::GlobalMatrix;j[3]=image.boot[3];j[4]=image.boot[4];j[5]=64;j[7]=2025;
 for(unsigned i=0;i<8;i++)j[8+i]=image.boot[8+i];for(unsigned i=0;i<16;i++)j[16+i]=leaf.words[i];
 j[32]=32;j[33]=HW::Vocabulary;j[34]=0;j[35]=HW::JobWitness;
 if(generated){
  for(unsigned i=0;i<64;i++)j[i]=(*generated)[i];
  for(unsigned i=0;i<16;i++)image.transfer.job.words[i]=j[16+i];
 }
 assert(HW::JobShape(j,image.boot.data(),0,0));if(corrupt)j[33]--;
 for(unsigned i=0;i<64;i++)image.Put(HW::Job+i*4,j[i]);image.Put(HW::Doorbell,1);
}
int main(int argc,char** argv){
 assert(argc>=3 && argc<=5);std::array<uint32_t,64> generated={};
 if(argc>=4){std::ifstream file(argv[3],std::ios::binary);file.read(reinterpret_cast<char*>(generated.data()),sizeof(generated));assert(file.gcount()==sizeof(generated));assert(file.peek()==EOF);}
 {
  const unsigned bands=argc>=4?8:2,first=argc>=4?generated[32]:32,count=bands*32;
  auto image=std::make_unique<HybridImage>(argv[1],argv[2],1,5120,bands);image->Boot();
  const unsigned indices[]={2,3,4,5,1,8,4,10};std::array<uint32_t,8> before={},after={};
  for(unsigned i=0;i<8;i++)before[i]=image->transfer.counters[indices[i]];
  StageHead(*image,false,argc>=4?&generated:nullptr);
  assert(image->Finish(1) && image->native.Clean());
  const auto* golden=PioneerNativeBfpBandGolden(5120);assert(golden);unsigned best=0;
  for(unsigned row=0;row<count;row++){
   const unsigned bits=image->Get(W::Output+row*4);assert(bits==golden[row%32]);
   if(HW::Better(bits,first+row,golden[best%32],first+best))best=row;
  }
  const unsigned expected[]={HW::PartialMagic,1,64,0,1,0,first,count,first+best,golden[best%32],1,2025,248320,5120,8,1};
  for(unsigned i=0;i<16;i++)assert(image->Get(HW::Partial+i*4)==expected[i]);
  std::array<uint32_t,64> proof={},job={};
  for(unsigned i=0;i<64;i++){proof[i]=image->Get(0x83f00+i*4);job[i]=image->Get(HW::Job+i*4);}
  for(unsigned i=0;i<8;i++)after[i]=image->transfer.counters[indices[i]];
  assert(PioneerQ38HybridMatrixReceipt::Matrix(proof.data(),job.data(),image->boot.data(),before.data(),after.data()));
  for(unsigned word=0;word<64;word++)for(unsigned bit=0;bit<32;bit++){
   auto damaged=proof;damaged[word]^=uint32_t(1U)<<bit;
   assert(!PioneerQ38HybridMatrixReceipt::Matrix(damaged.data(),job.data(),image->boot.data(),before.data(),after.data()));
  }
  if(argc==5){
   assert(argv[4][0]=='/');std::array<uint32_t,288> fixture={};
   for(unsigned i=0;i<64;i++){fixture[i]=job[i];fixture[64+i]=image->boot[i];fixture[128+i]=proof[i];fixture[192+i]=image->Get(HW::Partial+i*4);}
   for(unsigned i=0;i<8;i++){fixture[256+i]=before[i];fixture[264+i]=after[i];}
   for(unsigned i=0;i<16;i++)fixture[272+i]=image->Get(HW::Status+i*4);
   int fd=open(argv[4],O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);assert(fd>=0);
   assert(write(fd,fixture.data(),sizeof(fixture))==sizeof(fixture) && !fsync(fd) && !close(fd));
  }
  assert(image->native.matrixOps==bands*16*160 && image->stores==count);
 }
 {
  auto image=std::make_unique<HybridImage>(argv[1],argv[2],1,5120);image->Boot();StageHead(*image,true);
  assert(!image->Finish(1,true) && !image->native.matrixOps && !image->transfer.issues && !image->stores);
 }
 printf("PASS actual QGW8 head worker/leaf: %u synthetic BFP8 outputs and argmax receipt%s, 2048 proof-bit mutations refused, corrupted job refused before work; injected layer history, NO lifecycle/model/device/TPS\n",argc>=4?256U:64U,argc>=4?" using actual planned job":"");
}
