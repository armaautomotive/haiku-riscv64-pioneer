/* SPDX-License-Identifier: MIT */
// One private experimental cache entry. No GPU, Work or inference authority.
#ifndef PIONEER_BFP4_IMPLEMENTATION_SHA256
#error Exact converter implementation fingerprint required
#endif
#define main PioneerFragmentPriorMain
#include "pioneer_q38_bfp4_fragment_probe.cpp"
#undef main
#include "PioneerQ38Bfp4FragmentCache.h"

struct CountingModel {
 PioneerQwen38Gguf& model;unsigned reads=0;
 bool Ready()const{return model.Ready();}bool Stable()const{return model.Stable();}uint64_t Epoch()const{return model.Epoch();}
 const auto& Tensors()const{return model.Tensors();}
 bool ReadRow(unsigned tensor,unsigned row,float* out,unsigned capacity){reads++;return model.ReadRow(tensor,row,out,capacity);}
};
static bool Fingerprint(const char* text,PioneerModelWeightCache::Digest& digest){
 if(!text || strlen(text)!=64 || strspn(text,"0123456789abcdef")!=64)return false;
 auto digit=[](char c){return unsigned(c<='9'?c-'0':c-'a'+10);};
 for(unsigned i=0;i<32;i++)digest[i]=uint8_t(digit(text[2*i])*16+digit(text[2*i+1]));
 return true;
}
int main(int argc,char** argv){
 if(argc!=2 || strcmp(argv[1],"--check-one-private-cache"))return 2;
 const char* directory="/boot/home/pioneer-bfp4-cache-probe-20261106-a";
 const char* cacheDirectory="/boot/home/pioneer-bfp4-cache-probe-20261106-a/cache";
 struct stat d={};if(lstat(directory,&d) || !S_ISDIR(d.st_mode) || d.st_uid!=geteuid() || (d.st_mode&07777)!=0700)return 1;
 PioneerModelWeightCache::Digest implementation={};if(!Fingerprint(PIONEER_BFP4_IMPLEMENTATION_SHA256,implementation))return 1;
 const char* path="/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf";int fd=open(path,O_RDONLY|O_CLOEXEC|O_NOFOLLOW);if(fd<0)return 1;
 struct stat info={};if(fstat(fd,&info) || !S_ISREG(info.st_mode) || info.st_uid!=geteuid() || info.st_nlink!=1
  || (info.st_mode&0022) || uint64_t(info.st_size)!=PioneerQwen38Gguf::FullBytes){close(fd);return 1;}
 FILE* stream=fdopen(fd,"rb");if(!stream){close(fd);return 1;}std::unique_ptr<FILE,decltype(&fclose)> file(stream,&fclose);
 PioneerQwen38Gguf model;if(!model.Open(file.get()))return 1;
 puts("BFP4_CACHE_PROBE: authenticating original; CPU_ONLY_NO_DEVICE");if(fflush(stdout))return 1;
 PioneerModelWeightCache::Digest digest={};std::string error;
 if(!PioneerModelWeightCache::HashFile(fd,PioneerQwen38Gguf::FullBytes,digest,error) || !model.Stable()
  || PioneerModelWeightCache::Hex(digest)!="c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747")return 1;
 PioneerQwen38PackedPlan plan;if(!plan.Build(model,8192,true,true) || !plan.MatchesSource(model))return 1;
 const PioneerQ38PackedPart* part=nullptr;
 for(const auto& candidate:plan.Parts())if(candidate.kind==PIONEER_Q38_WEIGHT && candidate.layer==0 && candidate.role==2 && candidate.firstRow==0){
  if(part)return 1;
  part=&candidate;
 }
 if(!part)return 1;
 CountingModel source{model};bool hit=true;
 PioneerQ38Bfp4FragmentCache<CountingModel> cold;
 if(!cold.Prepare(source,*part,digest,implementation,cacheDirectory,hit,error) || hit || source.reads!=part->rows)return 1;
 std::vector<uint8_t> sample;if(!ReadSample(0,"edbc6d7cadcd85d0114ff16a5c44afb3627296497a7a0e03f76efa41da5ac7dd",sample))return 1;
 std::array<uint8_t,4096> page={};unsigned copied=0;
 while(copied<92160){
  const unsigned within=copied%4096,count=std::min(4096-within,92160-copied);
  if(!cold.ReadPage(copied/4096,page.data(),page.size(),error) || memcmp(page.data()+within,sample.data()+16+copied,count))return 1;
  copied+=count;
 }
 const unsigned coldReads=source.reads;PioneerQ38Bfp4FragmentCache<CountingModel> warm;
 if(!warm.Prepare(source,*part,digest,implementation,cacheDirectory,hit,error) || !hit || source.reads!=coldReads
  || !warm.ReadPage(0,page.data(),page.size(),error) || memcmp(page.data(),sample.data()+16,4096) || source.reads!=coldReads)return 1;
 printf("PASS_BFP4_REAL_FRAGMENT_CACHE rows%u bytes%u cold_source_rows%u warm_source_rows0 sample0_exact92160; CPU_ONLY_NO_DEVICE_NO_WORK_OR_SPEED_CLAIM\n",
  part->rows,part->allocated,coldReads);
 return fflush(stdout)?1:0;
}
