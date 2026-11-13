/* SPDX-License-Identifier: MIT */
// Read-only real model/cache comparison, never a driver or device test.
#if !defined(PIONEER_Q38_REAL_SMALL_RECURRENT_CACHE_CHECK) || PIONEER_Q38_REAL_SMALL_RECURRENT_CACHE_CHECK != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__)
#error Small recurrent cache check is isolated offline DATA only
#endif
#include "../PioneerQ38NativeBfpBandSource.h"
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <memory>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc,char** argv){
 if(argc!=4 || std::strcmp(argv[1],"--verify-small-recurrent-bfp8") || argv[2][0]!='/' || argv[3][0]!='/')return 2;
 using Cache=PioneerModelWeightCache;using Band=PioneerQ38NativeBfpBandSource;
 const int fd=open(argv[2],O_RDONLY|O_CLOEXEC|O_NOFOLLOW);if(fd<0)return 1;
 struct stat st{};if(fstat(fd,&st) || !S_ISREG(st.st_mode) || uint64_t(st.st_size)!=PioneerQwen38Gguf::FullBytes){close(fd);return 1;}
 FILE* raw=fdopen(fd,"rb");if(!raw){close(fd);return 1;}std::unique_ptr<FILE,decltype(&fclose)> file(raw,&fclose);
 Cache::Digest sha{};std::string error;std::puts("SMALL_RECURRENT_CACHE_CHECK authenticating same-open model; NO DEVICE");std::fflush(stdout);
 if(!Cache::HashFile(fd,PioneerQwen38Gguf::FullBytes,sha,error) || sha!=Band::OriginalPin())return 1;
 PioneerQwen38Gguf model;if(!model.Open(file.get()) || !model.Stable())return 1;
 Cache cache;if(!cache.OpenExistingDirectory(argv[3],error))return 1;
 Band prepared(cache);if(!prepared.Bind(model,sha,[&model]{return model.Ready() && model.Stable();},error))return 1;
 std::array<uint8_t,16*4096> actual{},fresh{};unsigned tensors=0,pages=0;
 for(unsigned layer=0;layer<64;layer++)if(layer%4!=3)for(unsigned role:{10U,11U}){
  PioneerQ38Tensor expected;if(!PioneerQ38Expected(layer,role,expected))return 1;
  const auto* found=model.Find(expected.name.c_str());if(!found)return 1;
  const unsigned tensor=unsigned(found-model.Tensors().data());
  for(unsigned page=0;page<85;){
   const unsigned count=std::min(16U,85-page),bytes=count*4096;
   if(!prepared.ReadPaddedSmallPages(tensor,page,count,actual.data(),actual.size(),error)
    || !PioneerQ38NativeBfp8WeightTiles::ReadRange(model,tensor,uint64_t(page)*4096,bytes,fresh.data(),fresh.size())
    || std::memcmp(actual.data(),fresh.data(),bytes)){
    std::fprintf(stderr,"small cache mismatch/refusal layer=%u role=%u page=%u: %s\n",layer,role,page,error.c_str());return 1;
   }
   page+=count;pages+=count;
  }
  tensors++;
 }
 if(tensors!=96 || pages!=8160 || !model.Stable())return 1;
 std::printf("PASS_REAL_SMALL_RECURRENT_CACHE tensors=%u pages=%u bytes=%llu; NO DEVICE OR SPEED\n",
  tensors,pages,(unsigned long long)pages*4096);return 0;
}
