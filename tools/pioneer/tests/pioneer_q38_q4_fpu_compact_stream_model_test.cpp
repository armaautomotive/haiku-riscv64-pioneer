/* SPDX-License-Identifier: MIT */
// Private full-K original samples;no learned bytes or predictions printed.
#define PIONEER_Q4_COMPACT_STREAM_IMAGE_ONLY
#include "pioneer_q38_q4_fpu_compact_stream_image_test.cpp"
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
 assert(argc==4 && argv[3][0]=='/');static_cast<void>(&NoMore);setvbuf(stdout,nullptr,_IOLBF,0);
 const int fd=open(argv[3],O_RDONLY|O_CLOEXEC|O_NOFOLLOW);assert(fd>=0);FILE* f=fdopen(fd,"rb");assert(f);
 struct stat before{},after{};assert(!fstat(fd,&before) && S_ISREG(before.st_mode) && before.st_nlink==1 && before.st_uid==getuid());
 puts("COMPACT_FULL_K_AUTH_SAME_OPEN_FULL_ORIGINAL_READ_ONLY_NO_CACHE_OR_DEVICE");
 PioneerModelWeightCache::Digest digest;std::string error;
 assert(PioneerModelWeightCache::HashFile(fd,PioneerQwen38Gguf::FullBytes,digest,error)
  && PioneerModelWeightCache::Hex(digest)=="c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747");
 auto source=std::make_unique<PioneerQwen38Gguf>();assert(source->Open(f));const auto epoch=source->Epoch();unsigned cases=0;
 for(const char* name:{"blk.0.ffn_gate.weight","blk.0.ffn_up.weight","blk.0.ffn_down.weight"}){
  unsigned index=~0U;for(unsigned i=0;i<source->Tensors().size();i++)if(source->Tensors()[i].name==name){index=i;break;}
  assert(index!=~0U);const auto& t=source->Tensors()[index];const unsigned k=unsigned(t.inputs);assert(t.type==12 && SL::Width(k) && !(t.outputs%32));
  for(unsigned first:{0U,unsigned((t.outputs/2)&~uint64_t(31)),unsigned(t.outputs)-32}){
   CompactStreamData data(k,0);
   for(unsigned block=0;block<k/256;block++)for(unsigned row=0;row<32;row++)
    assert(source->ReadPacked(index,(uint64_t(first+row)*(k/256)+block)*144,144,data.blocks[block].original.data()+row*144,144));
   assert(source->Stable() && source->Epoch()==epoch);data.Refresh();
   auto image=std::make_unique<CompactStreamRV32>(k==5120?argv[1]:argv[2],k);data.Stage(image->bus);
   assert(image->Execute() && image->Matches(data.expected));cases++;
   printf("PASS_PRIVATE_ACTUAL_COMPACT_FULL_K tensor%u width%u first%u twoJobs_all64WordsExact_persistentAccumulator_actualNoC_L1_RV32;NO_REAL_PROMPT_QUALITY_OR_SPEED\n",index,k,first);
  }
 }
 assert(cases==9 && source->Stable() && source->Epoch()==epoch && !fstat(fd,&after) && Same(before,after) && !fclose(f));
 puts("PASS_PRIVATE_COMPACT_FULL_K_STREAM cases9_jobs18_words576_originalWeights2654208_sameOpenFullSHA_epochNanoIdentityStable_NO_NATIVE_MODEL_QUALITY_OR_SPEED");
}
