/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_HEAD_BLOCK_FLOAT_DIAGNOSTIC 1
#include "pioneer_q38_head_bfp8_cpu_backend.h"
#include "pioneer_q38_head_bfp8_cpu_project.h"
#include "../PioneerModelWeightCache.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <memory>

bool Q38HeadBfp8CpuObserve(const char* path,const std::vector<uint32_t>& input,std::vector<uint32_t>& output){
 if(!path || path[0]!='/' || input.size()!=5120)return false;
 int fd=open(path,O_RDONLY|O_CLOEXEC|O_NOFOLLOW);if(fd<0)return false;
 struct stat info={};if(fstat(fd,&info) || !S_ISREG(info.st_mode) || uint64_t(info.st_size)!=PioneerQwen38Gguf::FullBytes){close(fd);return false;}
 FILE* stream=fdopen(fd,"rb");if(!stream){close(fd);return false;}
 std::unique_ptr<FILE,decltype(&fclose)> file(stream,&fclose);
 PioneerModelWeightCache::Digest digest={};std::string error;
 if(!PioneerModelWeightCache::HashFile(fd,PioneerQwen38Gguf::FullBytes,digest,error)
  || PioneerModelWeightCache::Hex(digest)!="c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747")return false;
 PioneerQwen38Gguf model;if(!model.Open(file.get()) || !model.Stable())return false;
 const auto* head=model.Find("output.weight");if(!head)return false;
 const unsigned index=unsigned(head-model.Tensors().data());
 std::vector<uint32_t> result;
 if(!Q38HeadBfp8CpuProject(model,index,input,0,248320/32,result,
  [](unsigned done,unsigned total){if(done%256==0 || done==total){
   printf("CPU_HEAD_BFP8 bands%u/%u; software-only\n",done,total);fflush(stdout);
  }}) || result.size()!=248320 || !model.Stable())return false;
 output=std::move(result);return true;
}
