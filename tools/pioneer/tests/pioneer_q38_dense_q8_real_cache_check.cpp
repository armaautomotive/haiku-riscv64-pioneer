/* SPDX-License-Identifier: MIT */
// Compare real converted cache pages to fresh conversions; offline only.
#if !defined(PIONEER_Q38_REAL_DENSE_Q8_CACHE_CHECK) || PIONEER_Q38_REAL_DENSE_Q8_CACHE_CHECK != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__)
#error Real dense Q8 cache check is isolated offline DATA only
#endif
#define PIONEER_Q38_DENSE_Q8_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION 1
#define PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT 1
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define PIONEER_BANKS_MOCK_ONLY 1
#include "../PioneerQ38NativeBfpBandSource.h"
#include "../PioneerQwen38PackedPlan.h"
#include "../PioneerQ38HybridDenseQ8MatrixPlan.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38CanonicalModelPlan.h"
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <memory>
#include <sys/stat.h>
#include <unistd.h>
using Cache=PioneerModelWeightCache;using Canonical=PioneerQ38CanonicalModelPlan;using Plan=PioneerQ38HybridDenseQ8MatrixPlan;
int main(int argc,char** argv){
 if(argc!=4 || std::strcmp(argv[1],"--verify-dense-q8-bfp8") || argv[2][0]!='/' || argv[3][0]!='/')return 2;
 std::setvbuf(stdout,nullptr,_IOLBF,0);const int fd=open(argv[2],O_RDONLY|O_CLOEXEC|O_NOFOLLOW);if(fd<0)return 1;
 struct stat info={};if(fstat(fd,&info) || !S_ISREG(info.st_mode) || uint64_t(info.st_size)!=PioneerQwen38Gguf::FullBytes){close(fd);return 1;}
 FILE* raw=fdopen(fd,"rb");if(!raw){close(fd);return 1;}std::unique_ptr<FILE,decltype(&fclose)> file(raw,&fclose);
 Cache::Digest hash={};std::string error;puts("DENSE_Q8_CACHE_CHECK authenticating same-open model; NO DEVICE");
 if(!Cache::HashFile(fd,PioneerQwen38Gguf::FullBytes,hash,error) || hash!=PioneerQ38NativeBfpBandSource::OriginalPin())return 1;
 PioneerQwen38Gguf source;if(!source.Open(file.get()) || !source.Stable())return 1;
 PioneerQwen38PackedPlan packed;if(!packed.Build(source,8192,true,true))return 1;
 auto canonical=std::make_unique<Canonical>();Plan::Proposal proposal;
 if(!canonical->Build() || !Plan::Build(*canonical,proposal) || packed.Parts().size()!=canonical->Count())return 1;
 Cache cache;if(!cache.OpenExistingDirectory(argv[3],error)){std::fprintf(stderr,"cache refused: %s\n",error.c_str());return 1;}
 PioneerQ38NativeBfpBandSource prepared(cache);
 if(!prepared.Bind(source,hash,[&source]{return source.Ready() && source.Stable();},error))return 1;
 uint64_t rawBytes=0,paddedBytes=0;unsigned pages=0,fragments=0;
 std::vector<uint8_t> cached(16*4096),fresh(16*4096);
 for(unsigned i=0;i<canonical->Count();i++){
  if(!source.Stable() || !packed.MatchesSource(source) || packed.Parts().size()!=canonical->Count())return 1;
  const auto* original=canonical->Part(i);const auto& q=proposal.parts[i];const auto part=packed.Parts()[i];
  if(!original || q.original!=i || q.bank!=original->bank || part.kind!=original->kind || part.layer!=original->layer
   || part.role!=original->role || part.firstRow!=original->firstRow || part.rows!=original->rows)return 1;
  if(q.encoding!=Plan::DenseQ8Bfp8)continue;
  if(part.tensor>=source.Tensors().size() || (part.firstRow&31) || (part.rows&31) || (q.allocated&4095))return 1;
  const auto& tensor=source.Tensors()[part.tensor];const uint64_t bandBytes=uint64_t(tensor.inputs/32)*1088;
  if(tensor.type!=8 || tensor.inputs!=5120 || q.bytes!=uint64_t(part.rows/32)*bandBytes)return 1;
  const uint64_t base=uint64_t(part.firstRow/32)*bandBytes;const unsigned count=q.allocated/4096;
  for(unsigned at=0;at<count;){
   const unsigned n=std::min(16U,count-at);
   if(!prepared.ReadStripePages(part.tensor,part.firstRow,part.rows,at,n,cached.data(),unsigned(cached.size()),error)){
    std::fprintf(stderr,"dense cache refused fragment=%u page=%u: %s\n",i,at,error.c_str());return 1;
   }
   std::fill(fresh.begin(),fresh.end(),0);const uint64_t offset=uint64_t(at)*4096;
   const unsigned bytes=offset<q.bytes?unsigned(std::min(uint64_t(n)*4096,uint64_t(q.bytes)-offset)):0;
   if((bytes && !PioneerQ38NativeBfp8WeightTiles::ReadRange(source,part.tensor,base+offset,bytes,fresh.data(),unsigned(fresh.size())))
    || std::memcmp(cached.data(),fresh.data(),size_t(n)*4096)){
    std::fprintf(stderr,"dense cache mismatch fragment=%u page=%u\n",i,at);return 1;
   }
   pages+=n;paddedBytes+=uint64_t(n)*4096;at+=n;
  }
  rawBytes+=q.bytes;fragments++;
  if(fragments%21==0 || fragments==336)std::printf("DENSE_Q8_CACHE_CHECK fragments=%u/336 pages=%u\n",fragments,pages);
 }
 if(fragments!=336 || rawBytes!=UINT64_C(1247805440) || !source.Stable() || !packed.MatchesSource(source))return 1;
 std::printf("DENSE_Q8_CACHE_MATCH fragments=%u pages=%u raw=%llu padded=%llu; NO DEVICE OR SPEED\n",
  fragments,pages,(unsigned long long)rawBytes,(unsigned long long)paddedBytes);return std::fflush(stdout)?1:0;
}
