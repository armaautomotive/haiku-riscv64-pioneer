/* SPDX-License-Identifier: MIT */
// Two separate processes: the old writer creates a synthetic cache entry and
// the current reader verifies it. No model or accelerator is involved.
#ifdef PIONEER_TEST_FROZEN_BFP8_WRITER
#include "../frozen_bfp8/PioneerModelWeightCache.h"
#else
#include "../PioneerModelWeightCache.h"
#endif
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

using Cache=PioneerModelWeightCache;
int main(int argc,char** argv){
 assert(argc==2 && argv[1][0]=='/');std::string error;Cache cache;
#ifdef PIONEER_TEST_FROZEN_BFP8_WRITER
 assert(cache.Open(argv[1],error));
#else
 assert(cache.OpenExistingDirectory(argv[1],error));
#endif
 Cache::Key key;key.sourceSHA256=Cache::Hash("synthetic model",15);
 const char recipe[]="pioneer-native-weight-tiles-v2-all-bfp8/outputs32-K32/faces16-row-major/zero-padding/"
  "Q4_K,Q8_0,Q6_K-to-BFP8_B/shared-exp-FP32-bias/align-truncate-then-RNE-saturate/"
  "no-subnormal-input/source-fingerprint=7db613bfdde463be7f933f25123a00dbe39928cbd8a070aa2481c6cb936753d0";
 key.conversionSHA256=Cache::Hash(recipe,sizeof(recipe)-1);key.entry=73;key.bytes=Cache::ChunkBytes*2+173;
 assert(Cache::Hex(key.conversionSHA256)=="551c6713a31c749d0dad82e616c592f01aba6854af8781f6756b4f60d85bc897");
 auto byte=[](uint64_t i){return uint8_t((i*71)^(i>>8)^(i>>16));};
 auto ready=[](){return true;};Cache::Reader reader;
#ifdef PIONEER_TEST_FROZEN_BFP8_WRITER
 unsigned produced=0;bool hit=true;
 auto produce=[&](uint64_t offset,unsigned count,uint8_t* output,std::string&){
  produced++;for(unsigned i=0;i<count;i++)output[i]=byte(offset+i);return true;
 };
 assert(cache.GetOrCreate(key,ready,produce,reader,hit,error) && !hit && produced==3);
#else
 assert(cache.OpenExisting(key,ready,reader,error));
#endif
 for(uint64_t offset=0;offset<key.bytes;){
  const unsigned count=unsigned(std::min(uint64_t(17011),key.bytes-offset));
  std::vector<uint8_t> bytes(count+2,0xa7);assert(reader.Read(offset,count,bytes.data()+1,count,error));
  assert(bytes.front()==0xa7 && bytes.back()==0xa7);
  for(unsigned i=0;i<count;i++)assert(bytes[i+1]==byte(offset+i));
  offset+=count;
 }
 puts("PASS frozen BFP8 recipe synthetic cache data; no model or device");
}
