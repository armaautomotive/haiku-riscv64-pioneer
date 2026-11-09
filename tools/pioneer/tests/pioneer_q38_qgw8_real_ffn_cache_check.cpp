/* SPDX-License-Identifier: MIT */
// Offline real-weight DATA comparison. No model inference or device access.
#if !defined(PIONEER_Q38_REAL_FFN_CACHE_CHECK) || PIONEER_Q38_REAL_FFN_CACHE_CHECK != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__)
#error Real FFN cache check is separate offline diagnostic DATA only
#endif
#define PIONEER_Q38_BFP4_PREPARATION_DIAGNOSTIC 1
#define PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION 1
#define PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT 1
#define PIONEER_Q38_QGW8_LAYOUT_INSPECTION 1
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define PIONEER_BANKS_MOCK_ONLY 1
#include "../PioneerQ38Bfp4FragmentCache.h"
#include "../PioneerQ38HybridHeadModelPlan.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38CanonicalModelPlan.h"
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <memory>
#include <sys/stat.h>
#include <unistd.h>

using Cache=PioneerModelWeightCache;using Canonical=PioneerQ38CanonicalModelPlan;
using Plan=PioneerQ38HybridHeadModelPlan;using Base=PioneerQ38HybridModelPlan;
static Cache::Digest Parse(const char* text){
 Cache::Digest out={};if(!text || strlen(text)!=64 || strspn(text,"0123456789abcdef")!=64)return out;
 auto nibble=[](char c){return unsigned(c<='9'?c-'0':c-'a'+10);};
 for(unsigned i=0;i<32;i++)out[i]=uint8_t(nibble(text[2*i])*16+nibble(text[2*i+1]));return out;
}
int main(int argc,char** argv){
 if(argc!=4 || strcmp(argv[1],"--verify-cached-ffn-bfp4") || argv[2][0]!='/' || argv[3][0]!='/')return 2;
 setvbuf(stdout,nullptr,_IOLBF,0);
 const int fd=open(argv[2],O_RDONLY|O_CLOEXEC|O_NOFOLLOW);if(fd<0)return 1;
 struct stat info={};if(fstat(fd,&info) || !S_ISREG(info.st_mode) || uint64_t(info.st_size)!=PioneerQwen38Gguf::FullBytes){close(fd);return 1;}
 FILE* raw=fdopen(fd,"rb");if(!raw){close(fd);return 1;}
 std::unique_ptr<FILE,decltype(&fclose)> file(raw,&fclose);Cache::Digest sourceHash={};std::string error;
 puts("QGW8_FFN_CACHE_CHECK authenticating real model; OFFLINE_NO_DEVICE_OR_INFERENCE");
 if(!Cache::HashFile(fd,PioneerQwen38Gguf::FullBytes,sourceHash,error)
  || Cache::Hex(sourceHash)!="c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747"){
  fputs("QGW8_FFN_CHECK_REFUSED original hash; NO DEVICE\n",stderr);return 1;
 }
 puts("QGW8_FFN_CACHE_CHECK original hash verified");
 PioneerQwen38Gguf source;if(!source.Open(file.get()) || !source.Stable())return 1;
 PioneerQwen38PackedPlan packed;if(!packed.Build(source,8192,true,true))return 1;
 auto canonical=std::make_unique<Canonical>();Plan::Proposal proposal;
 if(!canonical->Build() || !Plan::Build(*canonical,proposal) || packed.Parts().size()!=canonical->Count()
  || proposal.parts.size()!=canonical->Count())return 1;
 std::vector<uint32_t> manifest(Plan::ManifestWords,0);
 if(!Plan::Manifest(*canonical,proposal,manifest.data(),unsigned(manifest.size())))return 1;
 unsigned ffn=0,bfp4=0,bfp8=0;
 for(unsigned i=0;i<canonical->Count();i++){
  if(!source.Stable() || !packed.MatchesSource(source) || packed.Parts().size()!=canonical->Count())return 1;
  const auto* p=canonical->Part(i);const auto& old=packed.Parts()[i];const auto& q=proposal.parts[i];
  if(!p || q.original!=i || p->kind!=old.kind || p->layer!=old.layer || p->role!=old.role
   || p->bank!=old.bank || p->firstRow!=old.firstRow || p->rows!=old.rows)return 1;
  if(p->kind!=Canonical::WeightKind || p->layer>=64 || p->role<2 || p->role>4)continue;
  const unsigned at=(p->layer*3+p->role-2)*7+p->bank;
  if(at>=manifest.size() || manifest[at]!=Canonical::Base+q.offset || q.bank!=p->bank)return 1;
  if(q.encoding==Base::Bfp4)bfp4++;else if(q.encoding==Base::Bfp8)bfp8++;else return 1;ffn++;
 }
 if(ffn!=1344 || bfp4!=1113 || bfp8!=231)return 1;
 puts("QGW8_FFN_CACHE_CHECK 1344 planned FFN stripe coordinates verified; DATA_ONLY");
 // Mac and Haiku use deliberately distinct converter identities. This
 // checks only the documented Mac-keyed BFP4 cache; no rekey or writes.
 const auto implementation=Parse("57f8d22c609d2ba9b9a65be5be19452c35f937ec33f69e65aecd4033b703a39c");
 uint64_t bytes=0;unsigned pages=0,fragments=0;std::vector<uint8_t> cached(16*4096),fresh(4096);
 for(unsigned i=0;i<canonical->Count();i++){
  if(!source.Stable() || !packed.MatchesSource(source) || packed.Parts().size()!=canonical->Count())return 1;
  const auto& q=proposal.parts[i];if(q.encoding!=Base::Bfp4)continue;
  const auto part=packed.Parts()[i];PioneerQ38Bfp4FragmentCache<PioneerQwen38Gguf> cache;
  PioneerQ38Bfp4FragmentSource<PioneerQwen38Gguf> converter;
  if(!cache.OpenPrepared(source,part,sourceHash,implementation,argv[3],error) || !converter.Bind(source,part)){
   fprintf(stderr,"QGW8_FFN_CHECK_REFUSED fragment=%u: %s; NO DEVICE\n",i,error.c_str());return 1;
  }
  const unsigned count=converter.Pages();if(!count || uint64_t(count)*4096!=q.allocated)return 1;
  for(unsigned at=0;at<count;){
   const unsigned n=std::min(16U,count-at);
   if(!cache.ReadPages(at,n,cached.data(),unsigned(cached.size()),error))return 1;
   for(unsigned j=0;j<n;j++)if(!converter.ReadPage(at+j,fresh.data(),unsigned(fresh.size()))
    || memcmp(cached.data()+j*4096,fresh.data(),4096)){
    fprintf(stderr,"QGW8_FFN_CACHE_MISMATCH fragment=%u page=%u; NO DEVICE\n",i,at+j);return 1;
   }
   pages+=n;bytes+=uint64_t(n)*4096;at+=n;
  }
  fragments++;
  if(fragments%21==0 || fragments==1113)printf("QGW8_FFN_CACHE fragments=%u/1113 pages=%u; OFFLINE\n",fragments,pages);
 }
 if(fragments!=1113 || bytes!=UINT64_C(7971471360) || !source.Stable() || !packed.MatchesSource(source))return 1;
 printf("QGW8_FFN_CACHE_MATCH fragments=%u pages=%u bytes=%llu; BFP4_ONLY_OFFLINE_NO_DEVICE_INFERENCE_OR_SPEED\n",
  fragments,pages,(unsigned long long)bytes);return fflush(stdout)?1:0;
}
