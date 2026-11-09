/* SPDX-License-Identifier: MIT */
// Real authenticated output-head bands through the actual emitted QGW8 RV32
// worker in a simulated fabric/FPU. Injected preceding layer history and
// synthetic BF16 input: not hardware, full inference, quality or speed.
#define PIONEER_Q38_HYBRID_WORKER_MACHINE_ONLY
#define PIONEER_TEST_BFP8 1
#define PIONEER_Q38_HEAD_BLOCK_FLOAT_DIAGNOSTIC 1
#include "pioneer_q38_hybrid_worker_image_test.cpp"
#include "../PioneerQ38HeadBlockFloatDiagnostic.h"
#include "../PioneerBfp4PromptQuality.h"
#include "../PioneerModelWeightCache.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc,char** argv){
 if(argc!=4 || argv[1][0]!='/' || argv[2][0]!='/' || argv[3][0]!='/')return 2;
 int fd=open(argv[3],O_RDONLY|O_CLOEXEC|O_NOFOLLOW);if(fd<0)return 1;
 struct stat info={};if(fstat(fd,&info) || !S_ISREG(info.st_mode) || uint64_t(info.st_size)!=PioneerQwen38Gguf::FullBytes){close(fd);return 1;}
 FILE* file=fdopen(fd,"rb");if(!file){close(fd);return 1;}
 PioneerModelWeightCache::Digest digest={};std::string error;
 puts("QGW8_REAL_HEAD_WORKER authenticating original; SIMULATED_NO_DEVICE");fflush(stdout);
 if(!PioneerModelWeightCache::HashFile(fd,PioneerQwen38Gguf::FullBytes,digest,error)
  || PioneerModelWeightCache::Hex(digest)!="c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747"){fclose(file);return 1;}
 PioneerQwen38Gguf model;if(!model.Open(file) || !model.Stable()){fclose(file);return 1;}
 const auto* head=model.Find("output.weight");if(!head){fclose(file);return 1;}
 const unsigned index=unsigned(head-model.Tensors().data());
 std::array<uint32_t,5120> input={};
 for(unsigned i=0;i<input.size();i++){
  float x=float(int((i*73)%257)-128)*0.00390625f;std::memcpy(&input[i],&x,4);
 }
 for(unsigned first:{0U,70240U,108128U,248288U}){
  std::vector<uint8_t> packed;std::array<uint32_t,32> expected={};
  if(!PioneerQ38HeadBlockFloatDiagnostic::PackBand(model,index,first,PioneerBlackholeBlockFloat::Format::Bfp8B,packed)
   || !PioneerBfp4PromptQuality::PredictFormat(PioneerBlackholeBlockFloat::Format::Bfp8B,packed.data(),packed.size(),input.data(),5120,expected)
   || !model.Stable()){fclose(file);return 1;}
  auto image=std::make_unique<HybridImage>(argv[1],argv[2],1,5120,1);
  assert(image->transfer.weights.size()==packed.size());image->transfer.weights=packed;
  for(unsigned i=0;i<5120;i++){
   const uint32_t bits=input[i];const uint16_t bf=uint16_t((bits+0x7fff+((bits>>16)&1))>>16);
   image->transfer.root[i*2]=uint8_t(bf);image->transfer.root[i*2+1]=uint8_t(bf>>8);
  }
  image->Boot();assert(image->pc==image->waitPC && image->regs[8]==0x70000);
  image->Put(image->regs[8]-1548,64); // pinned worker, synthetic layer completion
  auto& leaf=image->transfer.job;leaf.words[11]=1;
  uint32_t j[64]={};j[0]=HW::Magic;j[1]=1;j[2]=HW::GlobalMatrix;j[3]=image->boot[3];j[4]=image->boot[4];j[5]=64;j[7]=2025;
  for(unsigned i=0;i<8;i++)j[8+i]=image->boot[8+i];for(unsigned i=0;i<16;i++)j[16+i]=leaf.words[i];
  j[32]=first;j[33]=HW::Vocabulary;j[34]=0;j[35]=HW::JobWitness;
  assert(HW::JobShape(j,image->boot.data(),0,0));
  for(unsigned i=0;i<64;i++)image->Put(HW::Job+i*4,j[i]);image->Put(HW::Doorbell,1);
  if(!image->Finish(1) || !image->native.Clean()){fclose(file);return 1;}
  unsigned best=0;
  for(unsigned row=0;row<32;row++){
   unsigned actual=image->Get(W::Output+row*4);
   if(actual!=expected[row]){fprintf(stderr,"QGW8_REAL_HEAD_MISMATCH first%u row%u actual%08x expected%08x;SIMULATED_NO_DEVICE\n",first,row,actual,expected[row]);fclose(file);return 1;}
   if(HW::Better(actual,first+row,expected[best],first+best))best=row;
  }
  if(image->Get(HW::Partial+8*4)!=first+best || image->Get(HW::Partial+9*4)!=expected[best] || !model.Stable()){fclose(file);return 1;}
  printf("QGW8_REAL_HEAD_WORKER first%u rows32 exact32 argmax%u;SIMULATED_NO_DEVICE\n",first,first+best);fflush(stdout);
 }
 puts("PASS_REAL_HEAD_QGW8_WORKER 128 real-weight outputs; SYNTHETIC_INPUT_SIMULATED_FABRIC_NOT_DEVICE_OR_SPEED");
 return fclose(file)?1:0;
}
