// CPU-only metadata inventory;not a GPU bank/arena fit or quality proof.
#include "PioneerModelWeightCache.h"
#include "PioneerQ38NativeWeightTiles.h"
#include <cassert>
#include <cstdio>
int main(int argc,char** argv) {
  assert(argc==2);FILE* f=fopen(argv[1],"rb");assert(f);
  PioneerQwen38Gguf source;assert(source.Open(f));
  PioneerModelWeightCache::Digest digest;std::string error;
  assert(PioneerModelWeightCache::HashFile(fileno(f),PioneerQwen38Gguf::FullBytes,digest,error));
  assert(PioneerModelWeightCache::Hex(digest)=="c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747");
  uint64_t count[3]={},bytes[3]={},total=0,largest=0;std::string name;
  for(const auto& tensor:source.Tensors()) {
    PioneerQ38NativeWeightTiles::Layout layout;
    if(!PioneerQ38NativeWeightTiles::Describe(tensor,layout))continue;
    unsigned index=tensor.type==8?0:tensor.type==12?1:2;
    count[index]++;bytes[index]+=layout.bytes;total+=layout.bytes;
    if(layout.bytes>largest) { largest=layout.bytes;name=tensor.name; }
  }
  for(unsigned i=0;i<3;i++)printf("SOURCE_TYPE%u QUANTIZED_MATRIX_COUNT%llu NATIVE_PAYLOAD_BYTES%llu\n",i==0?8:i==1?12:14,(unsigned long long)count[i],(unsigned long long)bytes[i]);
  printf("TOTAL_NATIVE_PAYLOAD_BYTES%llu LARGEST_ENTRY_BYTES%llu NAME%s;METADATA_ONLY_NO_FULL_CONVERSION_BANK_FIT_OR_GPU_USE\n",(unsigned long long)total,(unsigned long long)largest,name.c_str());
  assert(!fclose(f));
}
