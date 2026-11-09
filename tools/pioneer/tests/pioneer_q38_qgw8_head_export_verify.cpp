/* SPDX-License-Identifier: MIT */
// Independently re-read original Q6 tiles and verify PRIVATE diagnostic data.
// CPU-only; no device, driver, Work inference, model quality or speed claim.
#if !defined(PIONEER_QGW8_HEAD_EXPORT_VERIFY) || PIONEER_QGW8_HEAD_EXPORT_VERIFY != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__)
#error Separate offline head export verifier required
#endif
#include "../PioneerQ38NativeWeightTiles.h"
#include "../PioneerBfp4PromptQuality.h"
#include "../PioneerModelWeightCache.h"
#include "PioneerNativeBfpModelBandData.h" // PRIVATE; never commit
#include "PioneerNativeBfpModelBandMetadata.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <memory>
#include <sys/stat.h>
#include <unistd.h>

using Cache=PioneerModelWeightCache;
using Data=PioneerNativeBfpModelBandData;
using Meta=PioneerNativeBfpModelBandMetadata;
static bool HashFile(FILE* f,uint64_t size,const char* expected){
 Cache::Digest digest={};std::string error;
 return f && Cache::HashFile(fileno(f),size,digest,error) && Cache::Hex(digest)==expected;
}
static FILE* Open(const char* name,uint64_t size){
 if(!name || name[0]!='/')return nullptr;int fd=open(name,O_RDONLY|O_CLOEXEC|O_NOFOLLOW);if(fd<0)return nullptr;
 struct stat s={};if(fstat(fd,&s) || !S_ISREG(s.st_mode) || uint64_t(s.st_size)!=size){close(fd);return nullptr;}
 FILE* f=fdopen(fd,"rb");if(!f)close(fd);return f;
}
int main(int argc,char** argv){
 if(argc!=3)return 2;
 std::unique_ptr<FILE,decltype(&fclose)> modelFile(Open(argv[1],PioneerQwen38Gguf::FullBytes),&fclose);
 constexpr unsigned snapBytes=(16+2*5120+1)*4;
 std::unique_ptr<FILE,decltype(&fclose)> snap(Open(argv[2],snapBytes),&fclose);
 puts("HEAD_EXPORT_VERIFY authenticating inputs; CPU_ONLY_NO_DEVICE");fflush(stdout);
 struct stat snapshotBefore={},snapshotAfter={},modelBefore={},modelAfter={};
 if(!modelFile || !snap || fstat(fileno(snap.get()),&snapshotBefore) || fstat(fileno(modelFile.get()),&modelBefore))return 1;
 if(!HashFile(modelFile.get(),PioneerQwen38Gguf::FullBytes,"c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747")
  || !HashFile(snap.get(),snapBytes,"ce1ed4778227d6deb4bc0fd85440facf2a680c7d60b2e7d92c60e74abd73c98a"))return 1;
 std::vector<uint32_t> words(snapBytes/4);
 if(fseeko(snap.get(),0,SEEK_SET) || fread(words.data(),1,snapBytes,snap.get())!=snapBytes
  || words[0]!=0x38484351 || words[1]!=1 || words[2]!=248319 || words[3]!=64 || words[4]!=5120 || words[5]!=2
  || words[6]!=70253 || words[7]!=70253 || words.back()!=0x454e4448
  || fstat(fileno(snap.get()),&snapshotAfter) || snapshotBefore.st_dev!=snapshotAfter.st_dev
  || snapshotBefore.st_ino!=snapshotAfter.st_ino || snapshotBefore.st_size!=snapshotAfter.st_size
  || snapshotBefore.st_mtime!=snapshotAfter.st_mtime || snapshotBefore.st_ctime!=snapshotAfter.st_ctime)return 1;
 for(unsigned i=8;i<16;i++)if(words[i])return 1;
 PioneerQwen38Gguf model;if(!model.Open(modelFile.get()) || !model.Stable() || fstat(fileno(modelFile.get()),&modelAfter)
  || modelBefore.st_dev!=modelAfter.st_dev || modelBefore.st_ino!=modelAfter.st_ino || modelBefore.st_size!=modelAfter.st_size
  || modelBefore.st_mtime!=modelAfter.st_mtime || modelBefore.st_ctime!=modelAfter.st_ctime)return 1;
 const auto* head=model.Find("output.weight");const auto* norm=model.Find("output_norm.weight");
 if(!head || !norm || head->inputs!=5120 || head->outputs!=248320 || head->type!=14
  || norm->type!=0 || norm->inputs!=5120 || norm->outputs!=1 || Meta::Width!=5120 || Meta::Rows!=248320 || Meta::Samples!=3)return 1;
 unsigned tensor=unsigned(head-model.Tensors().data());if(tensor!=Meta::Tensor)return 1;
 std::vector<float> scaleWeights(5120),hidden(5120);std::array<uint32_t,5120> normalized={};
 if(!model.ReadRow(unsigned(norm-model.Tensors().data()),0,scaleWeights.data(),scaleWeights.size()))return 1;
 double sum=0;for(unsigned i=0;i<5120;i++){
  uint32_t b=words[16+5120+i];if((b&0x7f800000)==0x7f800000)return 1;
  std::memcpy(&hidden[i],&b,4);sum+=double(hidden[i]*hidden[i]);
 }
 const float factor=1.f/std::sqrt(float(sum/5120)+1e-6f);if(!std::isfinite(factor) || !(factor>0))return 1;
 for(unsigned i=0;i<5120;i++){float v=(hidden[i]*factor)*scaleWeights[i];if(!std::isfinite(v))return 1;std::memcpy(&normalized[i],&v,4);}
 const unsigned starts[]={0,70240,108128};using Codec=PioneerBlackholeBlockFloat;
 for(unsigned sample=0;sample<3;sample++){
  if(Meta::First(sample)!=starts[sample] || !Meta::Golden(sample) || !Meta::PayloadSHA(sample))return 1;
  std::vector<uint8_t> weights(160*1104,0),inputs(160*80,0),packed(160*1088,0);
  for(unsigned tile=0;tile<160;tile++){
   std::array<float,1024> values={};uint8_t* p=packed.data()+tile*1088;
   if(!PioneerQ38NativeWeightTiles::ReadFloatTile(model,tensor,uint64_t(starts[sample]/32)*160+tile,values.data(),values.size())
    || !Codec::PackTile(Codec::Format::Bfp8B,values.data(),values.size(),p,1088))return 1;
   std::memcpy(weights.data()+tile*1104+16,p,1088);
   for(unsigned k=0;k<32;k++){
    uint32_t b=normalized[tile*32+k];uint16_t bf=(b&0x7f800000)?uint16_t((b+0x7fff+((b>>16)&1))>>16):uint16_t((b>>16)&0x8000);
    inputs[tile*80+16+k*2]=uint8_t(bf);inputs[tile*80+17+k*2]=uint8_t(bf>>8);
   }
  }
  for(unsigned i=0;i<weights.size();i+=4){uint32_t w=0;std::memcpy(&w,weights.data()+i,4);if(Data::Word(sample,5120,0x8fff0+i)!=w)return 1;}
  for(unsigned i=0;i<inputs.size();i+=4){uint32_t w=0;std::memcpy(&w,inputs.data()+i,4);if(Data::Word(sample,5120,0x123ff0+i)!=w)return 1;}
  std::array<uint32_t,32> expected={};
  if(!PioneerBfp4PromptQuality::PredictFormat(Codec::Format::Bfp8B,packed.data(),packed.size(),normalized.data(),5120,expected)
   || std::memcmp(expected.data(),Meta::Golden(sample),128))return 1;
  weights.insert(weights.end(),inputs.begin(),inputs.end());
  if(Cache::Hex(Cache::Hash(weights.data(),weights.size()))!=Meta::PayloadSHA(sample) || !model.Stable())return 1;
  printf("HEAD_EXPORT_VERIFY sample%u first%u payload_bytes%zu reference_words32;CPU_ONLY_NO_DEVICE\n",sample,starts[sample],weights.size());fflush(stdout);
 }
 puts("PASS_HEAD_EXPORT_VERIFIED three head bands, independent tile reader, normalized input and 96 reference words;NO_DEVICE_OR_SPEED");
 return 0;
}
