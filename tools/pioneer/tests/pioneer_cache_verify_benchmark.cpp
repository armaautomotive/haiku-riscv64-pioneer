/* SPDX-License-Identifier: MIT */
// Synthetic local cache fixture only: not a model, card, or throughput test.
#include "../PioneerModelWeightCache.h"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>
using Cache=PioneerModelWeightCache;
int main(int argc,char** argv){
 if(argc!=3 || (std::strcmp(argv[2],"create") && std::strcmp(argv[2],"verify")))return 2;
 Cache cache;std::string error;if(!cache.Open(argv[1],error)){std::fprintf(stderr,"%s\n",error.c_str());return 1;}
 Cache::Key key;key.sourceSHA256=Cache::Hash("synthetic",9);key.conversionSHA256=Cache::Hash("fixture",7);
 key.entry=1;key.bytes=128ULL*1024*1024+17;
 auto ready=[](){return true;};
 if(!std::strcmp(argv[2],"create")){
  auto produce=[](uint64_t offset,unsigned count,uint8_t* out,std::string&){
   for(unsigned i=0;i<count;i++)out[i]=uint8_t(((offset+i)*71)^((offset+i)>>8)^((offset+i)>>16));return true;
  };
  Cache::Reader reader;bool hit=false;
  if(!cache.GetOrCreate(key,ready,produce,reader,hit,error) || hit){std::fprintf(stderr,"create: %s\n",error.c_str());return 1;}
  std::puts("PASS synthetic cache created");return 0;
 }
 for(unsigned i=0;i<4;i++){
  Cache::Reader reader;const auto begin=std::chrono::steady_clock::now();
  if(!cache.OpenExisting(key,ready,reader,error)){std::fprintf(stderr,"verify: %s\n",error.c_str());return 1;}
  const double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count();
  std::printf("sample=%u seconds=%.6f bytes=%llu\n",i,seconds,(unsigned long long)key.bytes);
 }
 return 0;
}
