/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_BFP4_FRAGMENT_CACHE_H
#define PIONEER_Q38_BFP4_FRAGMENT_CACHE_H
#include "PioneerQ38Bfp4FragmentSource.h"
#include "PioneerModelWeightCache.h"

// Separate derived-weight cache for this EXPERIMENTAL representation only.
// Callers must authenticate the original same-open model and compute the
// implementation fingerprint from all participating converter/codec source.
// A cache hit is DATA, not proof of quality, card ownership or Work support.
// This diagnostic is not linked into the working Q4 runtime.
template<class Source> class PioneerQ38Bfp4FragmentCache {
 using Cache=PioneerModelWeightCache;
 PioneerQ38Bfp4FragmentSource<Source> converter;Cache cache;Cache::Reader reader;Cache::Key key;
 bool attempted=false,ready=false;
 static bool Fail(std::string& error,const char* text){error=text;return false;}
public:
 bool Ready()const{return ready && converter.Ready() && reader.IsOpen();}
 bool Prepare(Source& source,const PioneerQ38PackedPart& part,const Cache::Digest& authenticatedSource,
  const Cache::Digest& implementation,const char* directory,bool& hit,std::string& error){
  if(attempted)return Fail(error,"BFP4 fragment cache preparation is one-shot.");
  attempted=true;hit=false;
  if(!std::any_of(authenticatedSource.begin(),authenticatedSource.end(),[](uint8_t v){return v!=0;})
   || !std::any_of(implementation.begin(),implementation.end(),[](uint8_t v){return v!=0;}))
   return Fail(error,"BFP4 cache requires source and implementation identities.");
  if(!converter.Bind(source,part) || !converter.Ready() || !directory)return Fail(error,"BFP4 source geometry or directory refused.");
  const std::string recipe="pioneer-q38-ffn-bfp4b-v1/32x32-face-major/64-exponents+512-mantissas/"
   "Q4K-decode-then-BFP4B-RNE/no-cpu-products/source-code-sha256="+Cache::Hex(implementation);
  key.sourceSHA256=authenticatedSource;key.conversionSHA256=Cache::Hash(recipe.data(),recipe.size());
  key.entry=(uint64_t(part.layer)<<48)|(uint64_t(part.role)<<40)|(uint64_t(part.firstRow)<<16)|part.rows;
  key.bytes=part.allocated;
  if(!cache.Open(directory,error))return false;
  auto live=[this]{return converter.Ready();};
  auto produce=[this](uint64_t offset,unsigned bytes,uint8_t* out,std::string& why){
   if(!out || (offset&4095) || !bytes || (bytes&4095) || bytes>Cache::ChunkBytes || offset>key.bytes || bytes>key.bytes-offset)
    return Fail(why,"Invalid BFP4 cache conversion range.");
   for(unsigned at=0;at<bytes;at+=4096)if(!converter.ReadPage(unsigned((offset+at)/4096),out+at,4096))
    return Fail(why,"BFP4 source changed or conversion failed.");
   return converter.Ready();
  };
  if(!cache.GetOrCreate(key,live,produce,reader,hit,error) || !converter.Ready() || !reader.IsOpen() || reader.Size()!=key.bytes)return false;
  ready=true;error.clear();return true;
 }
 bool ReadPage(unsigned page,uint8_t* out,unsigned capacity,std::string& error){
  if(!out || capacity<4096 || page>=key.bytes/4096)return Fail(error,"Invalid BFP4 cache page.");
  if(!Ready()){ready=false;return Fail(error,"BFP4 cache source not ready.");}
  std::array<uint8_t,4096> staged={};
  if(!reader.Read(uint64_t(page)*4096,4096,staged.data(),staged.size(),error) || !Ready()){
   ready=false;return Fail(error,"BFP4 cache read or source stability failed.");
  }
  std::memcpy(out,staged.data(),staged.size());return true;
 }
};
#endif
