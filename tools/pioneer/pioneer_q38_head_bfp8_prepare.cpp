/* SPDX-License-Identifier: MIT */
// Explicit, offline output-head cache preparation; no device or inference.
#if !defined(PIONEER_Q38_HEAD_BLOCK_FLOAT_DIAGNOSTIC) || PIONEER_Q38_HEAD_BLOCK_FLOAT_DIAGNOSTIC != 1
#error Head preparation requires the separate CPU-only diagnostic role
#endif
#ifndef PIONEER_HEAD_BFP8_IMPLEMENTATION_SHA256
#error Head preparation requires the exact converter build fingerprint
#endif
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define PIONEER_BANKS_MOCK_ONLY 1
#include "PioneerQ38HeadBfp8Preparation.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

static bool Same(const struct stat& a,const struct stat& b){
 if(a.st_dev!=b.st_dev || a.st_ino!=b.st_ino || a.st_size!=b.st_size || a.st_mtime!=b.st_mtime
  || a.st_ctime!=b.st_ctime || a.st_mode!=b.st_mode || a.st_uid!=b.st_uid || a.st_nlink!=b.st_nlink)return false;
#if defined(__APPLE__)
 return a.st_mtimespec.tv_nsec==b.st_mtimespec.tv_nsec && a.st_ctimespec.tv_nsec==b.st_ctimespec.tv_nsec;
#else
 return a.st_mtim.tv_nsec==b.st_mtim.tv_nsec && a.st_ctim.tv_nsec==b.st_ctim.tv_nsec;
#endif
}
struct AuthenticatedSource {
 PioneerQwen38Gguf& model;int fd;struct stat identity;
 bool Stable()const{struct stat current={};return model.Stable() && !fstat(fd,&current) && Same(identity,current);}
 bool Ready()const{return model.Ready() && Stable();}
 uint64_t Epoch()const{return model.Epoch();}
 const std::vector<PioneerQ38Tensor>& Tensors()const{return model.Tensors();}
 bool ReadRow(unsigned tensor,unsigned row,float* output,unsigned capacity){
  return Ready() && model.ReadRow(tensor,row,output,capacity) && Ready();
 }
};
struct AuthenticatedPlan {
 const PioneerQwen38PackedPlan& plan;
 bool MatchesSource(const AuthenticatedSource& source)const{return source.Ready() && plan.MatchesSource(source.model);}
 bool Tiled()const{return plan.Tiled();}unsigned DeltaSlots()const{return plan.DeltaSlots();}
 const std::vector<PioneerQ38PackedPart>& Parts()const{return plan.Parts();}
};
static bool ParseDigest(const char* hex,PioneerModelWeightCache::Digest& result){
 if(!hex || strlen(hex)!=64 || strspn(hex,"0123456789abcdef")!=64)return false;
 auto digit=[](char c){return unsigned(c<='9'?c-'0':c-'a'+10);};
 for(unsigned i=0;i<32;i++)result[i]=uint8_t(digit(hex[i*2])*16+digit(hex[i*2+1]));
 return true;
}
int main(int argc,char** argv){
 if(argc!=3 || (strcmp(argv[1],"--prepare-head-bfp8") && strcmp(argv[1],"--verify-head-bfp8")) || argv[2][0]!='/'){
  fprintf(stderr,"Usage: %s --prepare-head-bfp8|--verify-head-bfp8 ABSOLUTE_DEDICATED_CACHE\n",argv[0]);return 2;
 }
 const bool verifyOnly=!strcmp(argv[1],"--verify-head-bfp8");
 int fd=open("/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf",O_RDONLY|O_CLOEXEC|O_NOFOLLOW);if(fd<0)return 1;
 struct stat identity={};if(fstat(fd,&identity) || !S_ISREG(identity.st_mode) || identity.st_uid!=geteuid()
  || identity.st_nlink!=1 || (identity.st_mode&0022) || uint64_t(identity.st_size)!=PioneerQwen38Gguf::FullBytes){close(fd);return 1;}
 FILE* stream=fdopen(fd,"rb");if(!stream){close(fd);return 1;}
 std::unique_ptr<FILE,decltype(&fclose)> file(stream,&fclose);PioneerQwen38Gguf model;
 if(!model.Open(file.get()))return 1;
 AuthenticatedSource source{model,fd,identity};PioneerModelWeightCache::Digest digest={},fingerprint={};std::string error;
 puts("Authenticating full original model; CPU only, no P100A access.");fflush(stdout);
 if(!PioneerModelWeightCache::HashFile(fd,PioneerQwen38Gguf::FullBytes,digest,error) || !source.Ready()
  || PioneerModelWeightCache::Hex(digest)!="c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747"
  || !ParseDigest(PIONEER_HEAD_BFP8_IMPLEMENTATION_SHA256,fingerprint)){
  fprintf(stderr,"Original model authentication or converter fingerprint failed: %s\n",error.c_str());return 1;
 }
 PioneerQwen38PackedPlan plan;if(!plan.Build(model,8192,true,true))return 1;
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();if(!canonical->Build())return 1;
 AuthenticatedPlan checked{plan};using Preparation=PioneerQ38HeadBfp8Preparation<AuthenticatedSource,AuthenticatedPlan>;
 Preparation::Statistics stats;
 const bool okay=Preparation::Run(source,checked,*canonical,digest,fingerprint,argv[2],verifyOnly,stats,error,
  [](unsigned done,unsigned total,bool hit){
   printf("Head BFP8 fragments %u/%u; last %s\n",done,total,hit?"verified cache hit":"newly prepared");fflush(stdout);
  });
 if(!okay){fprintf(stderr,"Head BFP8 preparation stopped: %s\n",error.c_str());return 1;}
 printf("Head BFP8 %s: %u verified fragments, %u hits, %llu bytes; no P100A access or speed claim.\n",
  verifyOnly?"verification":"preparation",stats.fragments,stats.hits,(unsigned long long)stats.bytes);
 return fflush(stdout)?1:0;
}
