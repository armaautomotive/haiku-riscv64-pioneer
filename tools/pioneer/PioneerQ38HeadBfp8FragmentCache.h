/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_HEAD_BFP8_FRAGMENT_CACHE_H
#define PIONEER_Q38_HEAD_BFP8_FRAGMENT_CACHE_H
#include "PioneerQ38HeadBfp8FragmentSource.h"
#include "PioneerModelWeightCache.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

// Separately keyed DERIVED output-head data. A cache hit is not model
// authentication, quality approval, GPU ownership, or Work capability.
template<class Source>class PioneerQ38HeadBfp8FragmentCache {
 using Cache=PioneerModelWeightCache;
 PioneerQ38HeadBfp8FragmentSource<Source> converter;Cache cache;Cache::Reader reader;Cache::Key key;
 bool attempted=false,ready=false;
 static bool Fail(std::string& error,const char* why){error=why;return false;}
 bool Initialize(Source& source,const PioneerQ38PackedPart& part,const Cache::Digest& sourceHash,
  const Cache::Digest& implementation,const char* directory,bool existingOnly,std::string& error){
  if(attempted)return Fail(error,"Head BFP8 cache is one-shot.");attempted=true;
  if(!std::any_of(sourceHash.begin(),sourceHash.end(),[](uint8_t v){return v!=0;})
   || !std::any_of(implementation.begin(),implementation.end(),[](uint8_t v){return v!=0;}))
   return Fail(error,"Head BFP8 cache requires source and implementation identities.");
  if(!directory || !converter.Bind(source,part) || !converter.Ready())return Fail(error,"Head source geometry or directory refused.");
  const std::string recipe="pioneer-q38-output-head-bfp8b-v1/32x32-face-major/64-exponents+1024-mantissas/"
   "Q6K-decode-then-BFP8B-RNE/no-cpu-products/source-code-sha256="+Cache::Hex(implementation);
  key.sourceSHA256=sourceHash;key.conversionSHA256=Cache::Hash(recipe.data(),recipe.size());
  key.entry=(UINT64_C(64)<<48)|(UINT64_C(1)<<40)|(uint64_t(part.firstRow)<<16)|part.rows;
  key.bytes=uint64_t(converter.Pages())*4096;
  if(!existingOnly)return cache.Open(directory,error);
  int fd=open(directory,O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);struct stat before={},after={};
  if(fd<0 || fstat(fd,&before) || !S_ISDIR(before.st_mode) || before.st_uid!=geteuid() || (before.st_mode&07777)!=0700){
   if(fd>=0)close(fd);return Fail(error,"Prepared head BFP8 cache directory is missing or unsafe.");
  }
  const bool opened=cache.Open(directory,error);
  const bool unchanged=!lstat(directory,&after) && S_ISDIR(after.st_mode) && before.st_dev==after.st_dev && before.st_ino==after.st_ino;
  close(fd);if(!unchanged)return Fail(error,"Prepared head BFP8 cache directory changed while opening.");
  return opened;
 }
public:
 bool Ready()const{return ready && converter.Ready() && reader.IsOpen();}
 bool OpenPrepared(Source& source,const PioneerQ38PackedPart& part,const Cache::Digest& sourceHash,
  const Cache::Digest& implementation,const char* directory,std::string& error){
  if(!Initialize(source,part,sourceHash,implementation,directory,true,error))return false;
  if(!cache.OpenExisting(key,[this]{return converter.Ready();},reader,error)
   || !converter.Ready() || !reader.IsOpen() || reader.Size()!=key.bytes)return false;
  ready=true;error.clear();return true;
 }
 bool Prepare(Source& source,const PioneerQ38PackedPart& part,const Cache::Digest& sourceHash,
  const Cache::Digest& implementation,const char* directory,bool& hit,std::string& error){
  hit=false;if(!Initialize(source,part,sourceHash,implementation,directory,false,error))return false;
  auto live=[this]{return converter.Ready();};
  auto produce=[this](uint64_t offset,unsigned bytes,uint8_t* out,std::string& why){
   if(!out || (offset&4095) || !bytes || (bytes&4095) || bytes>Cache::ChunkBytes || offset>key.bytes || bytes>key.bytes-offset)
    return Fail(why,"Invalid head BFP8 cache conversion range.");
   for(unsigned at=0;at<bytes;at+=4096)if(!converter.ReadPage(unsigned((offset+at)/4096),out+at,4096))
    return Fail(why,"Head BFP8 source changed or conversion failed.");
   return converter.Ready();
  };
  if(!cache.GetOrCreate(key,live,produce,reader,hit,error) || !converter.Ready() || !reader.IsOpen() || reader.Size()!=key.bytes)return false;
  ready=true;error.clear();return true;
 }
 bool ReadPages(unsigned page,unsigned count,uint8_t* out,unsigned capacity,std::string& error){
  if(!out || !count || count>Cache::ChunkBytes/4096 || capacity<count*4096
   || page>=key.bytes/4096 || count>key.bytes/4096-page
   || reinterpret_cast<uintptr_t>(out)>UINTPTR_MAX-count*4096)return Fail(error,"Invalid head BFP8 cache page range.");
  if(!Ready()){ready=false;return Fail(error,"Head BFP8 cache source not ready.");}
  std::vector<uint8_t> staged(count*4096);
  if(!reader.Read(uint64_t(page)*4096,unsigned(staged.size()),staged.data(),unsigned(staged.size()),error) || !Ready()){
   ready=false;return Fail(error,"Head BFP8 cache read or source stability failed.");
  }
  std::memcpy(out,staged.data(),staged.size());return true;
 }
 bool ReadPage(unsigned page,uint8_t* out,unsigned capacity,std::string& error){return ReadPages(page,1,out,capacity,error);}
};
#endif
