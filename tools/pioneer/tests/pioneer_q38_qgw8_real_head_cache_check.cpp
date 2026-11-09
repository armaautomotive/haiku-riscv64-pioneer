/* SPDX-License-Identifier: MIT */
// Offline real-weight DATA comparison. No model inference or device access.
#if !defined(PIONEER_Q38_REAL_HEAD_CACHE_CHECK) || PIONEER_Q38_REAL_HEAD_CACHE_CHECK != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__)
#error Real head cache check is separate offline diagnostic DATA only
#endif
#define PIONEER_Q38_HEAD_BLOCK_FLOAT_DIAGNOSTIC 1
#define PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION 1
#define PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT 1
#define PIONEER_Q38_QGW8_LAYOUT_INSPECTION 1
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define PIONEER_BANKS_MOCK_ONLY 1
#include "../PioneerQ38HeadBfp8FragmentCache.h"
#include "../PioneerQ38HybridHeadLayout.h"
#include "../PioneerQ38HybridHeadModelPlan.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38CanonicalModelPlan.h"
#include <array>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <memory>
#include <sys/stat.h>
#include <unistd.h>

namespace H=PioneerQ38HybridHeadLayout;namespace W=PioneerQ38HybridHeadWorkerLayout;
using Cache=PioneerModelWeightCache;using Canonical=PioneerQ38CanonicalModelPlan;using Plan=PioneerQ38HybridHeadModelPlan;
static Cache::Digest Parse(const char* text){
 Cache::Digest out={};if(!text || strlen(text)!=64 || strspn(text,"0123456789abcdef")!=64)return out;
 auto nibble=[](char c){return unsigned(c<='9'?c-'0':c-'a'+10);};
 for(unsigned i=0;i<32;i++)out[i]=uint8_t(nibble(text[2*i])*16+nibble(text[2*i+1]));return out;
}
static std::array<uint32_t,64> RootBoot(){
 std::array<uint32_t,64>b={};b[0]=H::C::Magic;b[1]=1;b[2]=64;b[3]=129;b[4]=b[16]=0x14000;b[5]=119;b[6]=8192;b[7]=64;
 for(unsigned i=0;i<4;i++)b[8+i*2]=i+1;for(unsigned i=40;i<46;i++)b[i]=H::C::BootWord(i);return b;
}
static std::array<uint32_t,64> WorkerBoot(const std::array<uint32_t,64>& root,unsigned worker){
 std::array<uint32_t,64>b={};b[0]=W::Magic;b[1]=1;b[2]=64;b[3]=worker;b[4]=W::Peer(worker);b[5]=b[16]=0x14000;b[6]=8192;b[7]=64;
 for(unsigned i=0;i<8;i++)b[8+i]=root[8+i];for(unsigned i=40;i<=50;i++)b[i]=W::BootWord(i);return b;
}
int main(int argc,char** argv){
 if(argc!=4 || strcmp(argv[1],"--verify-cached-head") || argv[2][0]!='/' || argv[3][0]!='/')return 2;
 setvbuf(stdout,nullptr,_IOLBF,0);
 const int fd=open(argv[2],O_RDONLY|O_CLOEXEC|O_NOFOLLOW);if(fd<0)return 1;
 struct stat info={};if(fstat(fd,&info) || !S_ISREG(info.st_mode) || uint64_t(info.st_size)!=PioneerQwen38Gguf::FullBytes){close(fd);return 1;}
 FILE* raw=fdopen(fd,"rb");if(!raw){close(fd);return 1;}
 std::unique_ptr<FILE,decltype(&fclose)> file(raw,&fclose);Cache::Digest sourceHash={};std::string error;
 puts("QGW8_HEAD_CACHE_CHECK authenticating real model; OFFLINE_NO_DEVICE_OR_INFERENCE");
 if(!Cache::HashFile(fd,PioneerQwen38Gguf::FullBytes,sourceHash,error)
  || Cache::Hex(sourceHash)!="c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747"){
  fputs("QGW8_HEAD_CHECK_REFUSED original hash; NO DEVICE\n",stderr);return 1;
 }
 puts("QGW8_HEAD_CACHE_CHECK original hash verified");
 PioneerQwen38Gguf source;if(!source.Open(file.get()) || !source.Stable()){
  fputs("QGW8_HEAD_CHECK_REFUSED original metadata; NO DEVICE\n",stderr);return 1;
 }
 PioneerQwen38PackedPlan packed;if(!packed.Build(source,8192,true,true)){
  fputs("QGW8_HEAD_CHECK_REFUSED original plan; NO DEVICE\n",stderr);return 1;
 }
 auto canonical=std::make_unique<Canonical>();Plan::Proposal proposal;
 if(!canonical->Build() || !Plan::Build(*canonical,proposal) || packed.Parts().size()!=canonical->Count()
  || proposal.parts.size()!=canonical->Count()){
  fputs("QGW8_HEAD_CHECK_REFUSED canonical plan; NO DEVICE\n",stderr);return 1;
 }
 const auto root=RootBoot();if(!H::C::BootShape(root.data()))return 1;
 std::array<uint32_t,128> h={};std::array<unsigned,7> parts={};parts.fill(UINT32_MAX);
 unsigned work=UINT32_MAX,norm=UINT32_MAX,heads=0;
 for(unsigned i=0;i<canonical->Count();i++){
  if(!source.Stable() || !packed.MatchesSource(source) || packed.Parts().size()!=canonical->Count())return 1;
  const auto* p=canonical->Part(i);const auto& old=packed.Parts()[i];const auto& q=proposal.parts[i];
  if(!p || q.original!=i || p->kind!=old.kind || p->layer!=old.layer || p->role!=old.role
   || p->bank!=old.bank || p->firstRow!=old.firstRow || p->rows!=old.rows){
   fprintf(stderr,"QGW8_HEAD_CHECK_REFUSED original part index=%u; NO DEVICE\n",i);return 1;
  }
  if(p->kind==Canonical::WorkspaceKind && p->bank==0)work=i;
  if(p->kind==Canonical::WeightKind && p->layer==64 && p->role==2)norm=i;
  if(p->kind==Canonical::WeightKind && p->layer==64 && p->role==1){
   if(p->bank>=7 || parts[p->bank]!=UINT32_MAX || q.encoding!=Plan::HeadBfp8 || q.bytes!=uint64_t(p->rows)*5440)return 1;
   parts[p->bank]=i;auto* s=h.data()+H::Stripe+p->bank*H::StripeWords;
   const uint32_t values[]={p->bank,Canonical::Base+q.offset,p->firstRow,p->rows,5440,q.bytes,q.allocated,139};
   for(unsigned j=0;j<8;j++)s[j]=values[j];heads++;
  }
 }
 if(work>=canonical->Count() || norm>=canonical->Count() || heads!=7){
  fputs("QGW8_HEAD_CHECK_REFUSED head geometry; NO DEVICE\n",stderr);return 1;
 }
 const auto& wp=proposal.parts[work];const auto& np=proposal.parts[norm];const unsigned workspace=Canonical::Base+wp.offset;
 const uint32_t fixed[]={H::Magic,1,128,0,workspace,workspace+H::C::HeadOffset,np.bank,Canonical::Base+np.offset,5120,248320,973,9,119,8,5440,20480};
 for(unsigned i=0;i<16;i++){h[i]=fixed[i];h[16+i]=root[16+i];}for(unsigned i=0;i<8;i++)h[32+i]=root[8+i];
 const uint32_t witness[]={W::Magic,H::C::Magic,64,0,32,8,0,1};for(unsigned i=0;i<8;i++)h[40+i]=witness[i];
 if(!H::Shape(h.data(),root.data(),0,workspace)){
  fputs("QGW8_HEAD_CHECK_REFUSED head shape; NO DEVICE\n",stderr);return 1;
 }
 std::vector<uint8_t> coverage(248320,0);
 for(unsigned ordinal=0;ordinal<H::Jobs;ordinal++){
  const unsigned bank=ordinal%7,worker=ordinal%119+1;const auto boot=WorkerBoot(root,worker);std::array<uint32_t,64> job={};
  if(!W::BootShape(boot.data()) || !H::Job(h.data(),ordinal,0,0,boot.data(),job.data())){
   fprintf(stderr,"QGW8_HEAD_CHECK_REFUSED job %u; NO DEVICE\n",ordinal);return 1;
  }
  const auto* stripe=h.data()+H::Stripe+bank*8;const unsigned first=job[32],count=job[20]*32;
  if(first<stripe[2] || first+count>stripe[2]+stripe[3] || job[21]!=PioneerQ38ProjectionRoute(bank)
   || job[19]!=stripe[1]+((first-stripe[2])/32)*174080){
   fprintf(stderr,"QGW8_HEAD_CHECK_REFUSED job bounds %u; NO DEVICE\n",ordinal);return 1;
  }
  for(unsigned row=first;row<first+count;row++)if(coverage[row]++)return 1;
 }
 for(uint8_t c:coverage)if(c!=1)return 1;
 puts("QGW8_HEAD_CACHE_CHECK 973 jobs cover the exact vocabulary; DATA_ONLY");
 // The Mac cache is intentionally keyed with its host converter identity;
 // the Haiku-target key is different even when payloads have matched.
 const auto implementation=Parse("44c4d4b8727890b9a880f2a0e6954a2e2cb306200700f047eec85944e2066bed");
 uint64_t bytes=0;unsigned pages=0;std::vector<uint8_t> cached(16*4096),fresh(4096);
 for(unsigned bank=0;bank<7;bank++){
  if(!source.Stable() || !packed.MatchesSource(source) || packed.Parts().size()!=canonical->Count())return 1;
  const unsigned i=parts[bank];if(i>=packed.Parts().size())return 1;const auto part=packed.Parts()[i];
  PioneerQ38HeadBfp8FragmentCache<PioneerQwen38Gguf> cache;
  PioneerQ38HeadBfp8FragmentSource<PioneerQwen38Gguf> converter;
  if(!cache.OpenPrepared(source,part,sourceHash,implementation,argv[3],error) || !converter.Bind(source,part)){
   fprintf(stderr,"QGW8_HEAD_CHECK_REFUSED cache bank=%u: %s; NO DEVICE\n",bank,error.c_str());return 1;
  }
  const unsigned count=converter.Pages();if(!count)return 1;
  for(unsigned at=0;at<count;){
   const unsigned n=std::min(16U,count-at);
   if(!cache.ReadPages(at,n,cached.data(),unsigned(cached.size()),error))return 1;
   for(unsigned j=0;j<n;j++)if(!converter.ReadPage(at+j,fresh.data(),unsigned(fresh.size()))
    || memcmp(cached.data()+j*4096,fresh.data(),4096)){
    fprintf(stderr,"QGW8_HEAD_CACHE_MISMATCH bank=%u page=%u; NO DEVICE\n",bank,at+j);return 1;
   }
   pages+=n;bytes+=uint64_t(n)*4096;at+=n;
  }
  printf("QGW8_HEAD_CACHE bank=%u pages=%u rows=%u; OFFLINE\n",bank,count,part.rows);
 }
 if(!source.Stable() || !packed.MatchesSource(source))return 1;
 printf("QGW8_HEAD_CACHE_MATCH pages=%u bytes=%llu jobs=973 covered_rows=248320; OFFLINE_DATA_ONLY_NO_DEVICE_INFERENCE_OR_SPEED\n",
  pages,(unsigned long long)bytes);return fflush(stdout)?1:0;
}
