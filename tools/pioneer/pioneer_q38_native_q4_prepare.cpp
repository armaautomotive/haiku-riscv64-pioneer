/* SPDX-License-Identifier: MIT */
// Explicit userspace PREPARATION ONLY. No driver/device,model inference,
// vocabulary,chat template,CPU reference,learned-product output or fallback.
#include "PioneerQ38NativeQ4WeightCache.h"
#include <fcntl.h>
#include <unistd.h>
#include <cassert>
#include <cstdio>
#if defined(NDEBUG) || !defined(PIONEER_NATIVE_Q4_PREPARATION_SHA256)
#error Independently fingerprinted preparation build with checks enabled required
#endif
using Cache=PioneerModelWeightCache;
using Native=PioneerQ38NativeQ4WeightCache;
using Codec=PioneerQ38Q4EscapeTiles;
static bool Same(const struct stat& a,const struct stat& b){
 if(a.st_dev!=b.st_dev || a.st_ino!=b.st_ino || a.st_size!=b.st_size || a.st_mtime!=b.st_mtime || a.st_ctime!=b.st_ctime)return false;
#if defined(__APPLE__)
 return a.st_mtimespec.tv_nsec==b.st_mtimespec.tv_nsec && a.st_ctimespec.tv_nsec==b.st_ctimespec.tv_nsec;
#else
 return a.st_mtim.tv_nsec==b.st_mtim.tv_nsec && a.st_ctim.tv_nsec==b.st_ctim.tv_nsec;
#endif
}
static unsigned Word(const uint8_t* p){return p[0]|(unsigned(p[1])<<8)|(unsigned(p[2])<<16)|(unsigned(p[3])<<24);}
static bool Read(Native& cache,unsigned fragment,bool side,unsigned at,unsigned count,uint8_t* output){
 unsigned copied=0;std::string error;std::array<uint8_t,4096> page{};
 while(copied<count){
  const unsigned position=at+copied,within=position%4096,take=std::min(count-copied,4096-within);
  if(!cache.ReadPage(fragment,position/4096,side,page.data(),page.size(),error))return false;
  memcpy(output+copied,page.data()+within,take);copied+=take;
 }
 return true;
}
static bool Sample(Native& cache,PioneerQwen38Gguf& model,unsigned fragment,unsigned tile){
 std::string error;
 const auto& plan=cache.OriginalPlan();const auto& part=plan.Parts()[fragment];Native::CorrectionProposal correction{};
 if(!cache.Correction(fragment,correction))return false;
 std::array<uint8_t,2816> encoded{};std::array<uint8_t,512> packed{};std::array<uint8_t,2304> decoded{},raw{};
 uint8_t indexBytes[4];
 if(!Read(cache,fragment,false,tile*2304,2304,encoded.data()) || !Read(cache,fragment,true,tile*4,4,indexBytes))return false;
 const unsigned index=Word(indexBytes),kind=index>>30,offset=index&0x3fffffff;Codec::Mask description;description.kind=kind;
 if(kind==Codec::Zero){if(offset)return false;}
 else if(kind==Codec::SparseRows){
  if(offset>correction.bytes || 32>correction.bytes-offset || !Read(cache,fragment,true,offset,32,packed.data()))return false;
  for(unsigned i=0;i<32;i++)description.nonzeroRows+=unsigned(__builtin_popcount(packed[i]));
  description.bytes=32+description.nonzeroRows*2;
  if(!description.nonzeroRows || description.bytes>=512 || description.bytes>correction.bytes-offset
   || !Read(cache,fragment,true,offset+32,description.bytes-32,packed.data()+32))return false;
 }else if(kind==Codec::Dense){
  if(offset>correction.bytes || 512>correction.bytes-offset || !Read(cache,fragment,true,offset,512,packed.data()))return false;
  description.bytes=512;description.nonzeroRows=Codec::NonzeroRows(packed.data());
 }else return false;
 if(!Codec::UnpackMask(packed.data(),description.bytes,description,encoded.data()+2304,512)
  || !Codec::Decode(encoded.data(),encoded.size(),decoded.data(),decoded.size()))return false;
 std::array<uint8_t,4096> page{};unsigned copied=0;
 while(copied<2304){
  const unsigned at=tile*2304+copied,within=at%4096,take=std::min(2304-copied,4096-within);
  if(!plan.ReadWeightPage(model,fragment,at/4096,page.data(),page.size()))return false;
  memcpy(raw.data()+copied,page.data()+within,take);copied+=take;
 }
 if(raw!=decoded)return false;
 // Tail pages never leak the adjacent bundle component into an upload.
 for(bool side:{false,true}){
  const unsigned bytes=side?correction.bytes:part.bytes,allocated=side?correction.allocated:part.allocated;
  if(!cache.ReadPage(fragment,allocated/4096-1,side,page.data(),page.size(),error))return false;
  if(bytes%4096)for(unsigned i=bytes%4096;i<4096;i++)if(page[i])return false;
 }
 return true;
}
int main(int argc,char** argv){
 assert(argc==4 && !strcmp(argv[3],"--prepare") && argv[1][0]=='/' && argv[2][0]=='/');setvbuf(stdout,nullptr,_IOLBF,0);
 const std::string fingerprint=PIONEER_NATIVE_Q4_PREPARATION_SHA256;
 assert(fingerprint.size()==64 && fingerprint.find_first_not_of("0123456789abcdef")==std::string::npos);
 const int fd=open(argv[1],O_RDONLY|O_CLOEXEC|O_NOFOLLOW);assert(fd>=0);FILE* file=fdopen(fd,"rb");assert(file);
 struct stat before{},after{};assert(!fstat(fd,&before) && S_ISREG(before.st_mode));Cache::Digest sha{};std::string error;
 puts("NATIVE_Q4_PREPARATION_AUTH_SAME_OPEN_ORIGINAL_READ_ONLY_NO_DEVICE");
 assert(Cache::HashFile(fd,PioneerQwen38Gguf::FullBytes,sha,error)
  && Cache::Hex(sha)=="c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747");
 auto model=std::make_unique<PioneerQwen38Gguf>();assert(model->Open(file) && model->Tensors().size()==851);
 auto plan=std::make_unique<PioneerQwen38PackedPlan>();assert(plan->Build(*model,8192,true,true));
 auto ready=[&]{return model->Stable() && !fstat(fd,&after) && Same(before,after);};
 const auto recipe=Native::Recipe(fingerprint);const auto conversion=Cache::Hash(recipe.data(),recipe.size());
 auto cache=std::make_unique<Native>();assert(cache->Bind(argv[2],*model,*plan,sha,conversion,ready,error));
 Native::Statistics stats;unsigned samples=0;uint64_t blocks=0,primary=0,extra=0;puts("NATIVE_Q4_PREPARATION_ATOMIC_CACHE_ONLY_BEGIN");
 while(!cache->AllPrepared()){
  const unsigned fragment=cache->NextFragment();assert(fragment<plan->Parts().size());
  if(!cache->PrepareNext(stats,error)){fprintf(stderr,"NATIVE_Q4_PREPARATION_FAILED %s\n",error.c_str());return 1;}
  const auto& part=plan->Parts()[fragment];const unsigned tiles=part.bytes/2304;
  for(unsigned tile:{0U,tiles/2,tiles-1}){assert(Sample(*cache,*model,fragment,tile));samples++;}
  Native::CorrectionProposal correction{};assert(cache->Correction(fragment,correction));
  blocks+=tiles;primary+=part.bytes;extra+=correction.allocated;
  printf("NATIVE_Q4_PREPARATION fragments%llu blocks%llu bundleBytes%llu manifestHits%llu bundleHits%llu convertedTiles%llu;AGGREGATES_ONLY\n",
   (unsigned long long)stats.fragments,(unsigned long long)blocks,(unsigned long long)stats.bytes,
   (unsigned long long)stats.manifestHits,(unsigned long long)stats.bundleHits,(unsigned long long)stats.convertedTiles);
 }
 assert(stats.fragments==1351 && blocks==4488320 && samples==4053 && ready());
 for(unsigned bank=0;bank<7;bank++){
  assert(cache->Used(bank)<=PioneerQwen38PackedPlan::Capacity);
  printf("NATIVE_Q4_PREPARED_BANK%u original%llu corrections%llu total%llu remaining%llu;NO_NATIVE_ALLOCATION_AUTHORITY\n",bank,
   (unsigned long long)plan->Used(bank),(unsigned long long)(cache->Used(bank)-plan->Used(bank)),
   (unsigned long long)cache->Used(bank),(unsigned long long)(PioneerQwen38PackedPlan::Capacity-cache->Used(bank)));
 }
 assert(!fstat(fd,&after) && Same(before,after) && !fclose(file));
 printf("CLOSED_REAL_NATIVE_Q4_PREPARATION fragments%llu allOriginalBlocks%llu primaryBytes%llu extraAllocated%llu sampledLosslessRoundTrips%u "
  "manifestHits%llu bundleHits%llu convertedTiles%llu recipe%s;ALL851_full8192_twoState_preserved_NO_GPU_MODEL_GENERATION_OR_TPS\n",
  (unsigned long long)stats.fragments,(unsigned long long)blocks,(unsigned long long)primary,(unsigned long long)extra,samples,
  (unsigned long long)stats.manifestHits,(unsigned long long)stats.bundleHits,(unsigned long long)stats.convertedTiles,fingerprint.c_str());
}
