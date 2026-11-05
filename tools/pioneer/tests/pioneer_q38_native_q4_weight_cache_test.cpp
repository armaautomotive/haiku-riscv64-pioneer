/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_FIXTURE_ONLY 1
#include "pioneer_qwen38_gguf_test.cpp"
#include "../PioneerQ38NativeQ4WeightCache.h"
#include "../PioneerQ38NativeQ4CachedPages.h"
#include <fcntl.h>
#ifdef NDEBUG
#error Cache validation checks must remain enabled
#endif
using NC=PioneerQ38NativeQ4WeightCache;
using WC=PioneerModelWeightCache;
static unsigned Read32(const uint8_t* p){return p[0]|(unsigned(p[1])<<8)|(unsigned(p[2])<<16)|(unsigned(p[3])<<24);}
static void Write32(uint8_t* p,unsigned v){for(unsigned i=0;i<4;i++)p[i]=uint8_t(v>>(i*8));}
template<class Reader> static void Refused(Reader& cache,unsigned part,unsigned page,bool correction){
 std::array<uint8_t,4096> output;output.fill(0xa9);const auto before=output;std::string why;
 assert(!cache.ReadPage(part,page,correction,output.data(),output.size(),why) && output==before && !why.empty());
}
// Independent scalar original-nibble -> native integer and mask mapping.
// Only synthetic test expectations;NEVER part of a shader or Work loader.
static void Reference(const uint8_t* raw,uint8_t* primary,uint8_t* masks){
 memcpy(primary,raw,256);memset(primary+256,0,2048);memset(masks,0,512);
 for(unsigned panel=0;panel<16;panel++)for(unsigned i=0;i<256;i++){
  const unsigned at=256+(panel/4)*512+(panel&1)*256+i;
  const unsigned q=(raw[at]>>(panel&2?4:0))&15;const int z=int(q==15?14:q)-7;
  primary[256+panel*128+i/2]|=uint8_t((z<0?unsigned(-z)|8:unsigned(z))<<((i&1)*4));
  masks[panel*32+i/8]|=uint8_t(unsigned(q==15)<<(i&7));
 }
}
int main(int argc,char** argv){
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);
 const std::string directory=std::string(argv[1])+"/native-q4";
 Fixture fixture;PioneerQwen38Gguf source;assert(source.Open(fixture.file));PioneerQwen38PackedPlan plan;
 assert(plan.Build(source,8192,true,true));
 unsigned first=0;while(plan.Parts()[first].kind!=PIONEER_Q38_WEIGHT || source.Tensors()[plan.Parts()[first].tensor].type!=12)first++;
 const auto part=plan.Parts()[first];const auto tensor=source.Tensors()[part.tensor];
 const unsigned blocks=unsigned(tensor.inputs/256),rowBytes=blocks*144;
 assert(part.rows>=64);std::vector<uint8_t> rows(rowBytes*64);
 for(unsigned n=0;n<64;n++)for(unsigned b=0;b<blocks;b++){
  uint8_t* raw=rows.data()+n*rowBytes+b*144;
  for(unsigned i=0;i<16;i++)raw[i]=uint8_t(i*13+n*17+b*7);
  for(unsigned k=0;k<256;k++){
   const unsigned nonzero=n/16==0?256:n/16==1?1:n/16==2?239:240;
   const unsigned q=k<nonzero && ((n&15)==0 || n/16==0)?15:(k+n+b)%15;
   raw[16+(k/64)*32+k%32]|=uint8_t(q<<(k%64>=32?4:0));
  }
 }
 fixture.Write(tensor.offset+uint64_t(part.firstRow)*rowBytes,rows);assert(source.Open(fixture.file) && plan.Build(source,8192,true,true));
 const auto original=WC::Hash("SYNTHETIC-NOT-AUTHENTICATED-PUBLIC-MODEL",40);
 const auto recipe=NC::Recipe("synthetic-converter-fingerprint");const auto conversion=WC::Hash(recipe.data(),recipe.size());
 bool ready=true;auto sourceReady=[&]{return ready && source.Stable();};std::string error;
 NC cold;assert(cold.Bind(directory.c_str(),source,plan,original,conversion,sourceReady,error));
 assert(!cold.Bind(directory.c_str(),source,plan,original,conversion,sourceReady,error));
 assert(cold.NextFragment()==first && !cold.AllPrepared());Refused(cold,first,0,false);
 NC::Statistics stats;printf("NATIVE_Q4_CACHE_COLD syntheticPrimaryBytes%u\n",part.bytes);
 assert(cold.PrepareNext(stats,error));
 assert(stats.fragments==1 && !stats.manifestHits && !stats.bundleHits && stats.convertedTiles && !cold.AllPrepared());
 NC warm;assert(warm.Bind(directory.c_str(),source,plan,original,conversion,sourceReady,error));NC::Statistics hits;
 assert(warm.PrepareNext(hits,error) && hits.fragments==1 && hits.manifestHits==1 && hits.bundleHits==1 && !hits.convertedTiles && hits.bytes==stats.bytes);
 assert(warm.NextFragment()>first);
 PioneerQ38NativeQ4CachedPages upload;assert(upload.Bind(directory.c_str(),warm,sourceReady,error));
 // Preparation cancellation must leave the earlier complete recipe usable.
 bool keepGoing=true;const auto cancelledRecipe=WC::Hash("cancel-native-q4-test",21);
 NC cancelled;const std::string cancelledDirectory=std::string(argv[1])+"/cancelled-native-q4";
 assert(cancelled.Bind(cancelledDirectory.c_str(),source,plan,original,cancelledRecipe,[&]{return keepGoing && source.Stable();},error));
 NC::Statistics cancellation;
 assert(!cancelled.PrepareNext(cancellation,error,[&](uint64_t,uint64_t,bool verifying){if(!verifying)keepGoing=false;})
  && !cancellation.fragments && !cancelled.AllPrepared());Refused(cancelled,first,0,false);
 NC::CorrectionProposal correction{};assert(warm.Correction(first,correction));
 assert(correction.bank==part.bank && correction.proposedAddress==PioneerQwen38PackedPlan::Base+plan.Used(part.bank)
  && correction.allocated==((correction.bytes+4095)&~4095U));
 for(unsigned bank=0;bank<7;bank++)assert(warm.Used(bank)==plan.Used(bank)+(bank==part.bank?correction.allocated:0));
 const unsigned tiles=part.bytes/2304,indexBytes=(tiles*4+15)&~15U;
 std::vector<uint8_t> side(indexBytes,0);unsigned kinds[3]={};
 std::array<uint8_t,4096> originalPage{},actual{},expectedPage{};unsigned pageNumber=UINT32_MAX,cachedTile=UINT32_MAX;
 std::array<uint8_t,2304> raw{},native{};std::array<uint8_t,512> mask{};
 auto stage=[&](unsigned tile){
  unsigned copied=0;
  while(copied<raw.size()){
   const unsigned at=tile*2304+copied,page=at/4096,within=at%4096,take=std::min(unsigned(raw.size())-copied,4096-within);
   if(page!=pageNumber){assert(plan.ReadWeightPage(source,first,page,originalPage.data(),originalPage.size()));pageNumber=page;}
   memcpy(raw.data()+copied,originalPage.data()+within,take);copied+=take;
  }
  Reference(raw.data(),native.data(),mask.data());unsigned count=0;
  for(unsigned r=0;r<256;r++)count+=bool(mask[2*r]|mask[2*r+1]);
  const unsigned kind=!count?2:32+count*2<512?1:0,bytes=!count?0:kind==1?32+count*2:512;
  Write32(side.data()+tile*4,(kind<<30)|(count?unsigned(side.size()):0));kinds[kind]++;
  if(count){
   const unsigned start=unsigned(side.size());side.resize(start+((bytes+15)&~15U),0);
   if(!kind)memcpy(side.data()+start,mask.data(),512);
   else{unsigned at=32;for(unsigned r=0;r<256;r++)if(mask[2*r]|mask[2*r+1]){
    side[start+r/8]|=uint8_t(1U<<(r&7));side[start+at++]=mask[2*r];side[start+at++]=mask[2*r+1];
   }assert(at==bytes);}
  }
  cachedTile=tile;
 };
 for(unsigned page=0;page<part.allocated/4096;page++){
  expectedPage.fill(0);unsigned copied=0;const unsigned start=page*4096;
  while(copied<4096 && start+copied<part.bytes){
   const unsigned at=start+copied,tile=at/2304,within=at%2304,take=std::min(4096-copied,2304-within);
   if(cachedTile!=tile)stage(tile);
   memcpy(expectedPage.data()+copied,native.data()+within,take);copied+=take;
  }
  actual.fill(0xbb);assert(upload.ReadPage(first,page,false,actual.data(),actual.size(),error) && actual==expectedPage);
 }
 assert(kinds[0] && kinds[1] && kinds[2] && side.size()==correction.bytes);
 for(unsigned page=0;page<correction.allocated/4096;page++){
  expectedPage.fill(0);const unsigned at=page*4096,take=std::min(4096U,unsigned(side.size())-at);
  memcpy(expectedPage.data(),side.data()+at,take);
  assert(upload.ReadPage(first,page,true,actual.data(),actual.size(),error) && actual==expectedPage);
 }
 assert(upload.WindowFills()==(stats.bytes+65535)/65536);
 Refused(upload,first,part.allocated/4096,false);Refused(upload,first,correction.allocated/4096,true);Refused(upload,UINT32_MAX,0,false);
 Refused(warm,first,part.allocated/4096,false);Refused(warm,first,correction.allocated/4096,true);Refused(warm,UINT32_MAX,0,false);
 // Same complete generic cache checksum,but deliberately invalid manifest
 // semantics. It must not prepare/upload or start the bundle converter.
 const std::string badDirectory=std::string(argv[1])+"/invalid-native-q4";WC::Key manifestKey;
 assert(warm.KeyFor(first,true,manifestKey));WC bad;assert(bad.Open(badDirectory.c_str(),error));WC::Reader badReader;bool hit=false;
 assert(bad.GetOrCreate(manifestKey,sourceReady,[](uint64_t,unsigned n,uint8_t* out,std::string&){memset(out,0,n);return true;},badReader,hit,error));
 NC invalid;assert(invalid.Bind(badDirectory.c_str(),source,plan,original,conversion,sourceReady,error));NC::Statistics refused;
 assert(!invalid.PrepareNext(refused,error) && !refused.convertedTiles && !refused.fragments);Refused(invalid,first,0,true);
 // Upload cannot repair corruption,even after a valid bundle has been read.
 WC::Key key;assert(warm.KeyFor(first,false,key));const std::string path=directory+"/"+WC::EntryName(key);
 assert(upload.ReadPage(first,0,false,actual.data(),actual.size(),error)); // buffer the victim's UNCHANGED first chunk
 struct stat before{},after{};assert(!stat(path.c_str(),&before) && !chmod(path.c_str(),0600));
 int fd=open(path.c_str(),O_RDWR|O_NOFOLLOW);assert(fd>=0);uint8_t changed=0x57;
 assert(pwrite(fd,&changed,1,before.st_size-1)==1 && !fsync(fd) && !close(fd) && !chmod(path.c_str(),0400));
 Refused(warm,first,0,false);Refused(upload,first,0,false);assert(!stat(path.c_str(),&after) && before.st_ino==after.st_ino);
 ready=false;Refused(cold,first,0,false);ready=true;
 // The original caller cannot silently replace our owned full8192/two-slot
 // metadata snapshot by rebuilding its own plan after preparation.
 assert(plan.Build(source,1,false,false));
 assert(cold.OriginalPlan().Tiled() && cold.OriginalPlan().DeltaSlots()==2);
 NC unbound;assert(!unbound.Bind((std::string(argv[1])+"/untiled-refused").c_str(),source,plan,original,conversion,sourceReady,error));
 assert(source.Open(fixture.file));Refused(cold,first,0,true);Refused(upload,first,0,true);assert(!cold.AllPrepared());
 printf("PASS_NATIVE_Q4_PREPARED_WEIGHT_CACHE primaryPages%u correctionPages%u masksDense%u sparse%u zero%u nativeBytes%llu;"
  "EXACT originalQuantCodes separateRecipe ZERO_warm_conversion atomicBundle tailPadding readOnlyUpload sourceRevocation;"
  "SYNTHETIC_SOURCE_NO_NATIVE_MODEL_OR_TPS\n",part.allocated/4096,correction.allocated/4096,kinds[0],kinds[1],kinds[2],(unsigned long long)stats.bytes);
 (void)Read32;
}
