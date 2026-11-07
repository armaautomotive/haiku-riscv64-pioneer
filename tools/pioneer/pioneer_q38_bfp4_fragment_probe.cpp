/* SPDX-License-Identifier: MIT */
// CPU-only same-footprint conversion probe. Does not touch or authorize GPU.
#if !defined(PIONEER_Q38_BFP4_PREPARATION_DIAGNOSTIC) || PIONEER_Q38_BFP4_PREPARATION_DIAGNOSTIC != 1
#error Exact separate BFP4 preparation diagnostic required
#endif
#include "PioneerQ38Bfp4FragmentSource.h"
#include "PioneerModelWeightCache.h"
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <memory>
#include <sys/stat.h>
#include <unistd.h>
static bool ReadSample(unsigned sample,const char* pin,std::vector<uint8_t>& data){
 char path[160];snprintf(path,sizeof(path),"/boot/home/pioneer-bfp4-real-band-export-20261106-a/private/band%u.rbp4",sample);
 int fd=open(path,O_RDONLY|O_CLOEXEC|O_NOFOLLOW);if(fd<0)return false;
 struct stat a={},b={};bool okay=!fstat(fd,&a) && S_ISREG(a.st_mode) && a.st_uid==geteuid() && a.st_nlink==1 && (a.st_mode&07777)==0600 && a.st_size==102544;
 std::vector<uint8_t> next(102544);size_t done=0;
 while(okay && done<next.size()){ssize_t n=read(fd,next.data()+done,next.size()-done);if(n<=0)okay=false;else done+=size_t(n);}
 okay=okay && !fstat(fd,&b) && a.st_dev==b.st_dev && a.st_ino==b.st_ino && a.st_size==b.st_size
  && a.st_mtime==b.st_mtime && a.st_ctime==b.st_ctime && a.st_mtim.tv_nsec==b.st_mtim.tv_nsec && a.st_ctim.tv_nsec==b.st_ctim.tv_nsec;
 const bool closed=!close(fd);if(!okay || !closed || done!=next.size() || PioneerModelWeightCache::Hex(PioneerModelWeightCache::Hash(next.data(),next.size()))!=pin)return false;
 data=std::move(next);return true;
}
int main(int argc,char** argv){
 if(argc!=2 || strcmp(argv[1],"--check-ffn-fragment-layout"))return 2;
 const char* path="/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf";int fd=open(path,O_RDONLY|O_CLOEXEC|O_NOFOLLOW);if(fd<0)return 1;
 struct stat info={};if(fstat(fd,&info) || !S_ISREG(info.st_mode) || info.st_uid!=geteuid() || info.st_nlink!=1
  || (info.st_mode&0022) || uint64_t(info.st_size)!=PioneerQwen38Gguf::FullBytes){close(fd);return 1;}
 FILE* stream=fdopen(fd,"rb");if(!stream){close(fd);return 1;}std::unique_ptr<FILE,decltype(&fclose)> file(stream,&fclose);
 PioneerQwen38Gguf model;if(!model.Open(file.get()))return 1;
 puts("BFP4_FRAGMENT_PROBE: authenticating full original; CPU_ONLY_NO_DEVICE");if(fflush(stdout))return 1;
 PioneerModelWeightCache::Digest digest={};std::string error;
 if(!PioneerModelWeightCache::HashFile(fd,PioneerQwen38Gguf::FullBytes,digest,error) || !model.Stable()
  || PioneerModelWeightCache::Hex(digest)!="c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747")return 1;
 PioneerQwen38PackedPlan plan;if(!plan.Build(model,8192,true,true) || !plan.MatchesSource(model))return 1;
 unsigned next[64][3]={},counts[64][3]={},banks[64][3]={};uint64_t total=0;unsigned selected=0;
 for(const auto& part:plan.Parts()){
  if(part.kind!=PIONEER_Q38_WEIGHT || part.layer>=64 || part.role<2 || part.role>4)continue;
  PioneerQ38Bfp4FragmentSource<PioneerQwen38Gguf> converter;
  if(!converter.Bind(model,part) || part.firstRow!=next[part.layer][part.role-2] || (banks[part.layer][part.role-2]&(1U<<part.bank)))return 1;
  next[part.layer][part.role-2]+=part.rows;counts[part.layer][part.role-2]++;banks[part.layer][part.role-2]|=1U<<part.bank;
  total+=part.bytes;selected++;
 }
 for(unsigned layer=0;layer<64;layer++)for(unsigned role=0;role<3;role++)
  if(next[layer][role]!=(role==2?5120U:17408U) || counts[layer][role]!=7 || banks[layer][role]!=127)return 1;
 if(selected!=1344 || total!=UINT64_C(9625927680))return 1;
 const char* pins[]={"edbc6d7cadcd85d0114ff16a5c44afb3627296497a7a0e03f76efa41da5ac7dd",
  "59f05959ef8486021eefd0b7e02e083b6f18a7eef747f15d5e24e6e87c587695",
  "036bcee72fc9bb84fedb9c6f93a6d22358f9d7c90c610cc5c19159f002a587e3"};
 const unsigned firsts[]={0,8704,17376};
 for(unsigned sample=0;sample<3;sample++){
  std::vector<uint8_t> expected;if(!ReadSample(sample,pins[sample],expected))return 1;
  const PioneerQ38PackedPart* chosen=nullptr;
  for(const auto& part:plan.Parts())if(part.kind==PIONEER_Q38_WEIGHT && part.layer==0 && part.role==2
   && firsts[sample]>=part.firstRow && firsts[sample]+32<=part.firstRow+part.rows){if(chosen)return 1;chosen=&part;}
  if(!chosen)return 1;
  PioneerQ38Bfp4FragmentSource<PioneerQwen38Gguf> converter;if(!converter.Bind(model,*chosen))return 1;
  const uint64_t begin=uint64_t((firsts[sample]-chosen->firstRow)/32)*92160;unsigned copied=0;
  while(copied<92160){
   uint64_t offset=begin+copied;std::array<uint8_t,4096> page={};
   if(!converter.ReadPage(unsigned(offset/4096),page.data(),page.size()))return 1;
   const unsigned within=unsigned(offset%4096),count=std::min(4096-within,92160-copied);
   if(memcmp(page.data()+within,expected.data()+16+copied,count))return 1;
   copied+=count;
  }
  if(!model.Stable())return 1;
  printf("BFP4_FRAGMENT_MATCH sample%u first%u bytes92160; CPU_ONLY_NO_DEVICE\n",sample,firsts[sample]);if(fflush(stdout))return 1;
 }
 printf("PASS_BFP4_FFN_LAYOUT layers64 tensors192 fragments%u original_and_converted_bytes%llu bands3_exact; CPU_ONLY_NO_DEVICE_NO_WORK_OR_SPEED_CLAIM\n",selected,(unsigned long long)total);
 return fflush(stdout)?1:0;
}
