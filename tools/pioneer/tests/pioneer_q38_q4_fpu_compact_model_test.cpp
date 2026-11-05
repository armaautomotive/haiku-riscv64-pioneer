/* SPDX-License-Identifier: MIT */
// Private original-model reads. No original/derived numeric arrays printed.
#define PIONEER_Q38_COMPACT_FPU_IMAGE_MACHINE_ONLY
#include "pioneer_q38_q4_fpu_compact_image_test.cpp"
#include "../PioneerQwen38Gguf.h"
#include "../PioneerModelWeightCache.h"
#include <fcntl.h>
#include <unistd.h>
static bool Same(const struct stat& a,const struct stat& b){
 if(a.st_dev!=b.st_dev || a.st_ino!=b.st_ino || a.st_uid!=b.st_uid || a.st_gid!=b.st_gid || a.st_mode!=b.st_mode || a.st_nlink!=b.st_nlink || a.st_size!=b.st_size || a.st_mtime!=b.st_mtime || a.st_ctime!=b.st_ctime)return false;
#if defined(__APPLE__)
 return a.st_mtimespec.tv_nsec==b.st_mtimespec.tv_nsec && a.st_ctimespec.tv_nsec==b.st_ctimespec.tv_nsec;
#else
 return a.st_mtim.tv_nsec==b.st_mtim.tv_nsec && a.st_ctim.tv_nsec==b.st_ctim.tv_nsec;
#endif
}
int main(int argc,char** argv){
 assert(argc==3 && argv[2][0]=='/');setvbuf(stdout,nullptr,_IOLBF,0);
 const int fd=open(argv[2],O_RDONLY|O_CLOEXEC|O_NOFOLLOW);assert(fd>=0);FILE* file=fdopen(fd,"rb");assert(file);
 struct stat before{},after{};assert(!fstat(fd,&before) && S_ISREG(before.st_mode) && before.st_nlink==1 && before.st_uid==getuid());
 puts("COMPACT_FPU_AUTH_SAME_OPEN_FULL_ORIGINAL_READ_ONLY_NO_CACHE_OR_DEVICE");
 PioneerModelWeightCache::Digest digest;std::string error;
 assert(PioneerModelWeightCache::HashFile(fd,PioneerQwen38Gguf::FullBytes,digest,error)
  && PioneerModelWeightCache::Hex(digest)=="c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747");
 auto source=std::make_unique<PioneerQwen38Gguf>();assert(source->Open(file) && source->Tensors().size()==851);
 const auto epoch=source->Epoch();unsigned tensors=0,cases=0;
 for(unsigned tensor=0;tensor<source->Tensors().size();tensor++){
  const auto& t=source->Tensors()[tensor];if(t.type!=12)continue;
  assert(t.rank==2 && t.outputs%32==0 && t.inputs%256==0);tensors++;
  const unsigned blocks=unsigned(t.inputs/256),rowBytes=blocks*144;
  for(unsigned first:{0U,unsigned((t.outputs/2)&~uint64_t(31)),unsigned(t.outputs)-32})for(unsigned block:{0U,blocks/2,blocks-1}){
   Q4PipelineFixture data(0);
   for(unsigned row=0;row<32;row++)assert(source->ReadPacked(tensor,uint64_t(first+row)*rowBytes+block*144,144,data.original.data()+row*144,144));
   assert(source->Stable() && source->Epoch()==epoch);data.Refresh();std::array<uint8_t,4608> compact;
   assert(PioneerQ38Q4FpuCompactData::Prepare(data.original.data(),data.original.size(),compact.data(),compact.size()));
   CompactFpuRV32 image(argv[1]);image.native.StageCompact(compact,data.input);image.native.delay[0]=100003;image.native.delay[1]=7;
   assert(image.Execute() && image.Matches(data.expected));cases++;
  }
  printf("PASS_PRIVATE_COMPACT_FPU_ACTUAL tensor%u K%u outputs%u sampledBlocks9 actualJobs18 all576WordsExact_oldFpuBf16Oracle;NO_REAL_PROMPT_OR_TPS\n",tensor,unsigned(t.inputs),unsigned(t.outputs));
 }
 assert(tensors==193 && cases==1737 && source->Stable() && source->Epoch()==epoch && !fstat(fd,&after) && Same(before,after));
 assert(!fclose(file));
 printf("PASS_PRIVATE_COMPACT_FPU_ORIGINAL tensors%u sampledBlocks%u actualJobs%u returnedWords%u sameOpenFullSHA_epochAndIdentityStable;diagnosticBF16_inputs_notRealGenerationQualityOrSpeed\n",tensors,cases,cases*2,cases*64);
}
