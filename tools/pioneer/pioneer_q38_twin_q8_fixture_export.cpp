/* SPDX-License-Identifier: MIT */
// One offline shard and nothing else. Never opens an accelerator device.
#include "PioneerQ38TwinQ8ProjectionFixture.h"
#include "PioneerModelWeightCache.h"
#include <fcntl.h>
#include <unistd.h>

namespace F=PioneerQ38TwinQ8ProjectionFixture;
using Cache=PioneerModelWeightCache;

static bool Same(const struct stat& a,const struct stat& b){
 if(a.st_dev!=b.st_dev || a.st_ino!=b.st_ino || a.st_size!=b.st_size
  || a.st_mtime!=b.st_mtime || a.st_ctime!=b.st_ctime)return false;
#if defined(__APPLE__)
 return a.st_mtimespec.tv_nsec==b.st_mtimespec.tv_nsec && a.st_ctimespec.tv_nsec==b.st_ctimespec.tv_nsec;
#else
 return a.st_mtim.tv_nsec==b.st_mtim.tv_nsec && a.st_ctim.tv_nsec==b.st_ctim.tv_nsec;
#endif
}
int main(int argc,char** argv){
 if(argc!=3 || argv[1][0]!='/' || argv[2][0]!='/')return 2;
 const int fd=open(argv[1],O_RDONLY|O_NOFOLLOW|O_CLOEXEC|O_NONBLOCK);
 if(fd<0)return 1;
 std::unique_ptr<FILE,decltype(&fclose)> file(fdopen(fd,"rb"),&fclose);
 if(!file){close(fd);return 1;}
 struct stat initial={};
 if(fstat(fd,&initial) || !S_ISREG(initial.st_mode))return 1;
 std::string error;Cache::Digest digest={};
 if(!Cache::HashFile(fd,PioneerQwen38Gguf::FullBytes,digest,error)
  || Cache::Hex(digest)!="c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747")return 1;
 PioneerQwen38Gguf model;if(!model.Open(file.get()))return 1;
 auto ready=[&](){struct stat now={};return !fstat(fd,&now) && Same(initial,now) && model.Stable();};
 std::unique_ptr<const F::Fixture> data;
 if(!F::Build(model,ready,data) || !ready()
  || Cache::Hex(Cache::Hash(data->raw.data(),F::Storage))!="a6a28c1abe3d8475b90c2fb6c79b796a19e3f5ecce0f96066cdd41a59f0aa005"
  || Cache::Hex(Cache::Hash(data->native.data(),F::Storage))!="a3fa3db0700afe50fc55e60b0b525a84a96343a14a2a11fca8713bf34197a68c")return 1;
 // Publication is into a fresh private stage, never an existing path.
 const int dest=open(argv[2],O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC|O_NOFOLLOW,0600);
 if(dest<0)return 1;
 FILE* output=fdopen(dest,"wb");
 if(!output){close(dest);return 1;}
 const bool wrote=fwrite(data->raw.data(),1,F::Storage,output)==F::Storage
  && fwrite(data->native.data(),1,F::Storage,output)==F::Storage && !fflush(output) && !fsync(dest);
 const bool closed=!fclose(output);
 return wrote && closed && ready()?0:1;
}
