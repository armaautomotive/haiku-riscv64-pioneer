/* SPDX-License-Identifier: MIT */
// CPU-only comparison against a separately produced, trace-pinned prompt
// input/Q4 result. No card access, file mutation or model-quality claim.
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__)
#error BFP4 prompt comparison is separate diagnostic-only CPU data
#endif
#include "PioneerBfp4PromptQuality.h"
#include "PioneerModelWeightCache.h"
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

static bool Same(const struct stat& a,const struct stat& b){
 if(a.st_dev!=b.st_dev || a.st_ino!=b.st_ino || a.st_size!=b.st_size || a.st_mtime!=b.st_mtime || a.st_ctime!=b.st_ctime)return false;
#if defined(__APPLE__)
 return a.st_mtimespec.tv_nsec==b.st_mtimespec.tv_nsec && a.st_ctimespec.tv_nsec==b.st_ctimespec.tv_nsec;
#else
 return a.st_mtim.tv_nsec==b.st_mtim.tv_nsec && a.st_ctim.tv_nsec==b.st_ctim.tv_nsec;
#endif
}
static bool ReadPinned(const char* path,size_t bytes,const char* sha,std::vector<uint8_t>& output){
 int fd=open(path,O_RDONLY|O_CLOEXEC|O_NOFOLLOW);if(fd<0)return false;
 struct stat before={},after={};bool okay=!fstat(fd,&before) && S_ISREG(before.st_mode) && before.st_uid==geteuid()
  && before.st_nlink==1 && (before.st_mode&07777)==0600 && uint64_t(before.st_size)==bytes;
 std::vector<uint8_t> data(bytes);size_t done=0;
 while(okay && done<bytes){ssize_t n=read(fd,data.data()+done,bytes-done);if(n<=0)okay=false;else done+=size_t(n);}
 okay=okay && !fstat(fd,&after) && Same(before,after);
 const bool closed=!close(fd);if(!okay || !closed || done!=bytes || PioneerModelWeightCache::Hex(PioneerModelWeightCache::Hash(data.data(),data.size()))!=sha)return false;
 output=std::move(data);return true;
}
static uint32_t Word(const uint8_t* p){return uint32_t(p[0])|uint32_t(p[1])<<8|uint32_t(p[2])<<16|uint32_t(p[3])<<24;}
int main(int argc,char** argv){
 if(argc!=2 || strlen(argv[1])!=64 || strspn(argv[1],"0123456789abcdef")!=64){fputs("Use PROMPT_INPUT_SHA256; CPU-only.\n",stderr);return 2;}
 const char* inputPath="/boot/home/pioneer-bfp4-prompt-quality-20261106-a/prompt-input.bin";
 const char* samplePath="/boot/home/pioneer-bfp4-real-band-export-20261106-a/private/band%u.rbp4";
 const char* pins[]={"edbc6d7cadcd85d0114ff16a5c44afb3627296497a7a0e03f76efa41da5ac7dd",
  "59f05959ef8486021eefd0b7e02e083b6f18a7eef747f15d5e24e6e87c587695",
  "036bcee72fc9bb84fedb9c6f93a6d22358f9d7c90c610cc5c19159f002a587e3"};
 const unsigned firsts[]={0,8704,17376};std::vector<uint8_t> input;
 if(!ReadPinned(inputPath,16+(5120+96)*4,argv[1],input) || Word(input.data())!=0x31464951 || Word(input.data()+4)!=1
  || Word(input.data()+8)!=248319 || Word(input.data()+12)!=5120)return 1;
 std::array<uint32_t,5120> realInput={};std::array<uint32_t,96> original={};
 for(unsigned i=0;i<5120;i++)realInput[i]=Word(input.data()+16+i*4);
 for(unsigned i=0;i<96;i++)original[i]=Word(input.data()+16+(5120+i)*4);
 for(unsigned sample=0;sample<3;sample++){
  char path[160];snprintf(path,sizeof(path),samplePath,sample);std::vector<uint8_t> data;
  if(!ReadPinned(path,102544,pins[sample],data) || Word(data.data())!=0x34504252 || Word(data.data()+4)!=1
   || Word(data.data()+8)!=5120 || Word(data.data()+12)!=firsts[sample])return 1;
  PioneerBfp4PromptQuality::Result result={};
  if(!PioneerBfp4PromptQuality::Compute(data.data()+16,160*576,realInput.data(),original.data()+sample*32,result))return 1;
  printf("BFP4_PROMPT_COMPARE sample%u first%u outputs32 relative_l2%.9g max_abs%.9g; CPU_DIAGNOSTIC_NOT_MODEL_QUALITY_OR_SPEED\n",
   sample,firsts[sample],result.relativeL2,result.maxAbsolute);
  if(fflush(stdout))return 1;
 }
 puts("PASS_BFP4_PROMPT_COMPARE samples3 prompt_position0 token248319; NO_DEVICE_NO_FULL_MODEL_QUALITY_OR_SPEED");
 return fflush(stdout)?1:0;
}
