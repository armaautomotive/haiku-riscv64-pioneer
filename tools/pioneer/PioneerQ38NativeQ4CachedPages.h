/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_NATIVE_Q4_CACHED_PAGES_H
#define PIONEER_Q38_NATIVE_Q4_CACHED_PAGES_H
#include "PioneerQ38NativeQ4WeightCache.h"
// Read-only upload adapter,NOT a device transport or a source allocation.
// The caller owns/outlives the prepared metadata and SAME authenticated
// original. A future session requires AllPrepared BEFORE accelerator.Open.
// One verified64KiB window avoids rehashing that chunk for each4KiB page.
// Every buffered read STILL checks source and the open file identity;it can
// NEVER build,repair,convert or serve an unprepared/mismatched fragment.
class PioneerQ38NativeQ4CachedPages {
 using Cache=PioneerModelWeightCache;
 PioneerQ38NativeQ4WeightCache* weights=nullptr;
 Cache cache;Cache::Ready sourceReady;
 std::unique_ptr<Cache::Reader> reader;
 Cache::Key current{};
 std::array<uint8_t,Cache::ChunkBytes> window{};
 uint64_t windowOffset=0,windowFills=0;
 unsigned active=UINT32_MAX,windowBytes=0;
 bool attempted=false;
 static bool Fail(std::string& error,const char* why){error=why;return false;}
 static bool Same(const Cache::Key& a,const Cache::Key& b){return a.sourceSHA256==b.sourceSHA256
  && a.conversionSHA256==b.conversionSHA256 && a.entry==b.entry && a.bytes==b.bytes;}
 bool Ready(unsigned fragment,const Cache::Key& key)const{
  Cache::Key actual;return sourceReady && sourceReady() && weights
   && weights->KeyFor(fragment,false,actual) && Same(actual,key);
 }
public:
 bool ReadyFor(const PioneerQ38NativeQ4WeightCache& source)const{
  return weights==&source && source.AllPrepared() && sourceReady && sourceReady();
 }
 bool Bind(const char* directory,PioneerQ38NativeQ4WeightCache& source,const Cache::Ready& ready,std::string& error){
  if(attempted)return Fail(error,"Native Q4 cached-page binding is one-shot.");attempted=true;
  if(!ready || !ready() || !source.OriginalPlan().Ready() || !cache.Open(directory,error))return false;
  weights=&source;sourceReady=ready;return ready();
 }
 uint64_t WindowFills()const{return windowFills;}
 bool ReadPage(unsigned fragment,unsigned page,bool correction,uint8_t* output,unsigned capacity,std::string& error){
  Cache::Key key;PioneerQ38NativeQ4WeightCache::CorrectionProposal side{};
  if(!output || capacity<4096 || !weights || !weights->KeyFor(fragment,false,key) || !Ready(fragment,key)
   || !weights->Correction(fragment,side))return Fail(error,"Native Q4 upload source/fragment is not prepared.");
  const auto& parts=weights->OriginalPlan().Parts();if(fragment>=parts.size())return Fail(error,"Invalid Q4 fragment.");
  const auto& p=parts[fragment];const unsigned bytes=correction?side.bytes:p.bytes,allocated=correction?side.allocated:p.allocated;
  const uint64_t relative=uint64_t(page)*4096;
  if(page>=allocated/4096 || relative>=bytes)return Fail(error,"Invalid Q4 upload page.");
  if(active!=fragment || !Same(current,key)){
   reader.reset();active=UINT32_MAX;windowBytes=0;auto next=std::make_unique<Cache::Reader>();
   if(!cache.OpenExisting(key,[&]{return Ready(fragment,key);},*next,error))return false;
   reader=std::move(next);current=key;active=fragment;
  }
  if(!reader || !reader->IsOpen())return Fail(error,"Native Q4 bundle changed during upload.");
  std::array<uint8_t,4096> stage{};const unsigned count=unsigned(std::min(UINT64_C(4096),uint64_t(bytes)-relative));
  const uint64_t start=(correction?uint64_t(p.bytes):0)+relative;
  for(unsigned copied=0;copied<count;){
   const uint64_t at=start+copied,first=at/Cache::ChunkBytes*Cache::ChunkBytes;
   if(!windowBytes || windowOffset!=first){
    windowBytes=0;
    if(first>=key.bytes)return Fail(error,"Native Q4 upload exceeds atomic bundle.");
    const unsigned n=unsigned(std::min(uint64_t(Cache::ChunkBytes),key.bytes-first));
    if(!reader->Read(first,n,window.data(),window.size(),error))return false;
    windowOffset=first;windowBytes=n;windowFills++;
   }
   const unsigned within=unsigned(at-windowOffset);
   if(within>=windowBytes)return Fail(error,"Native Q4 upload window is incomplete.");
   const unsigned take=std::min(count-copied,windowBytes-within);
   memcpy(stage.data()+copied,window.data()+within,take);copied+=take;
  }
  if(!Ready(fragment,key) || !reader->IsOpen())return Fail(error,"Native Q4 source/cache changed during upload.");
  memcpy(output,stage.data(),stage.size());return true;
 }
};
#endif
