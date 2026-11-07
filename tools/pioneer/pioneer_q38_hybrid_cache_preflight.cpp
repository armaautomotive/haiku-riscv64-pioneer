/* SPDX-License-Identifier: MIT */
// Read-only, CPU-only preflight of the actual proposed hybrid pages. This is
// not a device owner, an uploader, model inference, or Work/Assistant mode.
#if !defined(PIONEER_Q38_BFP4_PREPARATION_DIAGNOSTIC) || PIONEER_Q38_BFP4_PREPARATION_DIAGNOSTIC != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Hybrid cache preflight requires its distinct CPU-only preparation role
#endif
#ifndef PIONEER_BFP4_IMPLEMENTATION_SHA256
#error Hybrid cache preflight requires the original BFP4 converter fingerprint
#endif
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define PIONEER_BANKS_MOCK_ONLY 1
#include "PioneerQ38HybridPreparedPages.h"
#include "PioneerQ38NativeBfpBandSource.h"
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
static bool ParseDigest(const char* hex,PioneerModelWeightCache::Digest& out){
 if(!hex || strlen(hex)!=64 || strspn(hex,"0123456789abcdef")!=64)return false;
 auto nibble=[](char c){return unsigned(c<='9'?c-'0':c-'a'+10);};
 for(unsigned i=0;i<32;i++)out[i]=uint8_t(nibble(hex[2*i])*16+nibble(hex[2*i+1]));
 return true;
}
static bool ExistingPrivateDirectory(const char* path){
 struct stat info={};return path && path[0]=='/' && !lstat(path,&info) && S_ISDIR(info.st_mode)
  && info.st_uid==geteuid() && (info.st_mode&07777)==0700;
}
int main(int argc,char** argv){
 if(argc!=4 || strcmp(argv[1],"--verify-prepared") || argv[2][0]!='/' || argv[3][0]!='/'){
  fputs("Use --verify-prepared ABSOLUTE_BFP4_CACHE ABSOLUTE_BFP8_CACHE; CPU-only.\n",stderr);return 2;
 }
 // Refuse missing or unsafe paths before authentication or cache.Open().
 if(!ExistingPrivateDirectory(argv[2]) || !ExistingPrivateDirectory(argv[3])){
  fputs("Both dedicated cache directories must already exist and be private.\n",stderr);return 1;
 }
 int fd=open("/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf",O_RDONLY|O_CLOEXEC|O_NOFOLLOW);if(fd<0)return 1;
 struct stat identity={};if(fstat(fd,&identity) || !S_ISREG(identity.st_mode) || identity.st_uid!=geteuid()
  || identity.st_nlink!=1 || (identity.st_mode&0022) || uint64_t(identity.st_size)!=PioneerQwen38Gguf::FullBytes){close(fd);return 1;}
 FILE* stream=fdopen(fd,"rb");if(!stream){close(fd);return 1;}
 std::unique_ptr<FILE,decltype(&fclose)> file(stream,&fclose);PioneerQwen38Gguf model;
 if(!model.Open(file.get()))return 1;
 const uint64_t epoch=model.Epoch();auto ready=[&]{struct stat now={};return model.Ready() && model.Stable()
  && model.Epoch()==epoch && !fstat(fd,&now) && Same(identity,now);};
 PioneerModelWeightCache::Digest digest={},fingerprint={};std::string error;
 puts("HYBRID_PREFLIGHT authenticating original; CPU-only, no accelerator");fflush(stdout);
 if(!PioneerModelWeightCache::HashFile(fd,PioneerQwen38Gguf::FullBytes,digest,error) || !ready()
  || digest!=PioneerQ38NativeBfpBandSource::OriginalPin()
  || !ParseDigest(PIONEER_BFP4_IMPLEMENTATION_SHA256,fingerprint)){
  fprintf(stderr,"HYBRID_PREFLIGHT_REFUSED original/fingerprint: %s\n",error.c_str());return 1;
 }
 PioneerQwen38PackedPlan packed;if(!packed.Build(model,8192,true,true) || !ready())return 1;
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();if(!canonical->Build())return 1;
 PioneerModelWeightCache cache8;
 if(!cache8.Open(argv[3],error) || !ready()){
  fprintf(stderr,"HYBRID_PREFLIGHT_REFUSED BFP8 cache: %s\n",error.c_str());return 1;
 }
 PioneerQ38NativeBfpBandSource eight(cache8);
 if(!eight.Bind(model,digest,ready,error)){
  fprintf(stderr,"HYBRID_PREFLIGHT_REFUSED BFP8 binding: %s\n",error.c_str());return 1;
 }
 PioneerQ38HybridPreparedPages<PioneerQwen38Gguf,PioneerQwen38PackedPlan,PioneerQ38NativeBfpBandSource> pages;
 if(!pages.Bind(model,packed,*canonical,eight,digest,fingerprint,argv[2],ready,error)
  || !pages.VerifyAll(error,[](unsigned done,unsigned total){if(done%21==0 || done==total){
   printf("HYBRID_PREFLIGHT verified %u/%u converted fragments\n",done,total);fflush(stdout);
  }}) || !pages.AllVerified() || !ready()){
  fprintf(stderr,"HYBRID_PREFLIGHT_REFUSED fragments: %s\n",error.c_str());return 1;
 }
 puts("PASS_HYBRID_PREFLIGHT 1071 BFP4 plus 273 BFP8 fragments, existing caches verified; NO_DEVICE_UPLOAD_MODEL_EXECUTION_OR_SPEED");
 return fflush(stdout)?1:0;
}
