/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_HYBRID_VERIFIED_MODEL_H
#define PIONEER_Q38_HYBRID_VERIFIED_MODEL_H
#if !defined(PIONEER_Q38_BFP4_PREPARATION_DIAGNOSTIC) || PIONEER_Q38_BFP4_PREPARATION_DIAGNOSTIC != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) \
 || defined(PIONEER_Q38_BF16_FULL_CLIENT) || defined(PIONEER_Q38_BF16_FULL_NATIVE) \
 || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_DRIVER_BUILD_TAG) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Hybrid verified model requires the isolated preparation role
#endif
#ifndef PIONEER_BFP4_IMPLEMENTATION_SHA256
#error Hybrid verified model requires the frozen BFP4 converter fingerprint
#endif
#include "PioneerQ38HybridPreparedPages.h"
#include "PioneerQ38NativeBfpBandSource.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

// Retain the SAME authenticated original, geometry, both cache readers and
// preverified pages for a future diagnostic session. No device handle,
// model arithmetic, upload, ownership, launch or Work authority here.
class PioneerQ38HybridVerifiedModel {
 using Cache=PioneerModelWeightCache;
public:
 using Pages=PioneerQ38HybridPreparedPages<PioneerQwen38Gguf,PioneerQwen38PackedPlan,PioneerQ38NativeBfpBandSource>;
private:
 int fd=-1;struct stat identity={};std::unique_ptr<FILE,decltype(&fclose)> file{nullptr,&fclose};
 PioneerQwen38Gguf model;PioneerQwen38PackedPlan packed;
 std::unique_ptr<PioneerQ38CanonicalModelPlan> canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();
 Cache cache8;PioneerQ38NativeBfpBandSource eight{cache8};Pages pages;
 uint64_t epoch=0;bool attempted=false,prepared=false;
 static bool Same(const struct stat& a,const struct stat& b){
  if(a.st_dev!=b.st_dev || a.st_ino!=b.st_ino || a.st_size!=b.st_size || a.st_mtime!=b.st_mtime
   || a.st_ctime!=b.st_ctime || a.st_mode!=b.st_mode || a.st_uid!=b.st_uid || a.st_nlink!=b.st_nlink)return false;
#if defined(__APPLE__)
  return a.st_mtimespec.tv_nsec==b.st_mtimespec.tv_nsec && a.st_ctimespec.tv_nsec==b.st_ctimespec.tv_nsec;
#else
  return a.st_mtim.tv_nsec==b.st_mtim.tv_nsec && a.st_ctim.tv_nsec==b.st_ctim.tv_nsec;
#endif
 }
 static bool Digest(const char* hex,Cache::Digest& out){
  if(!hex || strlen(hex)!=64 || strspn(hex,"0123456789abcdef")!=64)return false;
  auto nibble=[](char c){return unsigned(c<='9'?c-'0':c-'a'+10);};
  for(unsigned i=0;i<32;i++)out[i]=uint8_t(nibble(hex[2*i])*16+nibble(hex[2*i+1]));
  return true;
 }
 static bool PrivateDirectory(const char* path){
  struct stat info={};return path && path[0]=='/' && !lstat(path,&info) && S_ISDIR(info.st_mode)
   && info.st_uid==geteuid() && (info.st_mode&07777)==0700;
 }
 bool Current()const{
  struct stat now={};return fd>=0 && file && epoch && model.Ready() && model.Stable() && model.Epoch()==epoch
   && !fstat(fd,&now) && Same(identity,now);
 }
 static bool Fail(std::string& error,const char* message){if(error.empty())error=message;return false;}
public:
 PioneerQ38HybridVerifiedModel()=default;
 PioneerQ38HybridVerifiedModel(const PioneerQ38HybridVerifiedModel&)=delete;
 PioneerQ38HybridVerifiedModel& operator=(const PioneerQ38HybridVerifiedModel&)=delete;
 bool Prepare(const char* directory4,const char* directory8,std::string& error,
  const std::function<void(unsigned,unsigned)>& progress={}){
  if(attempted)return Fail(error,"Hybrid model preparation is one-shot.");
  attempted=true;error.clear();
  if(!PrivateDirectory(directory4) || !PrivateDirectory(directory8))return Fail(error,"Both dedicated cache directories must already exist and be private.");
  const int opened=open("/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf",O_RDONLY|O_CLOEXEC|O_NOFOLLOW);
  if(opened<0)return Fail(error,"Original model could not be opened.");
  if(fstat(opened,&identity) || !S_ISREG(identity.st_mode) || identity.st_uid!=geteuid() || identity.st_nlink!=1
   || (identity.st_mode&0022) || uint64_t(identity.st_size)!=PioneerQwen38Gguf::FullBytes){
   close(opened);return Fail(error,"Original model identity refused.");
  }
  FILE* stream=fdopen(opened,"rb");if(!stream){close(opened);return Fail(error,"Original model stream refused.");}
  file.reset(stream);fd=opened;if(!model.Open(file.get()))return Fail(error,"Original model metadata refused.");
  epoch=model.Epoch();Cache::Digest digest={},fingerprint={};
  if(!Cache::HashFile(fd,PioneerQwen38Gguf::FullBytes,digest,error) || !Current()
   || digest!=PioneerQ38NativeBfpBandSource::OriginalPin() || !Digest(PIONEER_BFP4_IMPLEMENTATION_SHA256,fingerprint))
   return Fail(error,"Original model hash or frozen converter fingerprint refused.");
  if(!packed.Build(model,8192,true,true) || !Current() || !canonical->Build())return Fail(error,"Model geometry refused.");
  if(!cache8.Open(directory8,error) || !Current())return Fail(error,"BFP8 cache refused.");
  auto live=[this]{return Current();};
  if(!eight.Bind(model,digest,live,error) || !pages.Bind(model,packed,*canonical,eight,digest,fingerprint,directory4,live,error)
   || !pages.VerifyAll(error,progress) || !pages.AllVerified() || !Current())return Fail(error,"Hybrid cache verification failed.");
  prepared=true;error.clear();return true;
 }
 bool Ready()const{return prepared && Current() && pages.AllVerified();}
 Pages* PreparedPages(){return Ready()?&pages:nullptr;}
 const PioneerQ38CanonicalModelPlan* Canonical()const{return Ready()?canonical.get():nullptr;}
};
#endif
