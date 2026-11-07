/* SPDX-License-Identifier: MIT */
// Synthetic Q4_K bytes for the offline ISA test ONLY. Never a model source,
// native driver, Work path, device transfer or fallback for an original GGUF.
#if !defined(PIONEER_Q38_SYNTHETIC_EMBEDDING_TEST) || PIONEER_Q38_SYNTHETIC_EMBEDDING_TEST != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) \
 || defined(PIONEER_Q38_BF16_FULL_NATIVE) || defined(PIONEER_Q38_BF16_FULL_CLIENT) \
 || defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) || defined(PIONEER_Q38_GLOBAL_WORKER_IMAGE)
#error Synthetic embedding bytes require their isolated host-test implementation
#endif
#define PIONEER_Q38_EMBEDDING_RAW_TEST_BRIDGE 1
#include "pioneer_q38_embedding_raw_test_bridge.h"
#include "../PioneerQ38GlobalFeedbackLayout.h"
#include <cstdio>
#include <cstring>

namespace {
constexpr unsigned Token=248319,Blocks=20,TileBytes=2304,BlockBytes=144;
void Block(unsigned token,unsigned block,uint8_t (&bytes)[BlockBytes]){
 bytes[0]=0;bytes[1]=0x3c;bytes[2]=0;bytes[3]=0; // finite half scales: d=1, dmin=0
 for(unsigned i=4;i<16;i++)bytes[i]=0x11;
 for(unsigned i=16;i<BlockBytes;i++)bytes[i]=uint8_t(token*13U+block*37U+i*19U);
}
}
struct Q38EmbeddingRawTestBridge::Source {bool ready=false,revoked=false;};
Q38EmbeddingRawTestBridge::Q38EmbeddingRawTestBridge():source(new Source){}
Q38EmbeddingRawTestBridge::~Q38EmbeddingRawTestBridge(){delete source;}
bool Q38EmbeddingRawTestBridge::Open(const char* path){
 if(!source || source->ready || source->revoked || !path)return false;
 FILE* file=fopen(path,"rb");if(!file)return false;
 char bytes[64]={};const size_t n=fread(bytes,1,sizeof(bytes),file);bool okay=!ferror(file);if(fclose(file)!=0)okay=false;
 static constexpr char signature[]="SYNTHETIC_Q4K_EMBEDDING_V1\n";
 source->ready=okay && n==sizeof(signature)-1 && !memcmp(bytes,signature,sizeof(signature)-1);return source->ready;
}
bool Q38EmbeddingRawTestBridge::Ready()const{return source && source->ready && !source->revoked;}
void Q38EmbeddingRawTestBridge::Revoke(){if(source)source->revoked=true;}
bool Q38EmbeddingRawTestBridge::Read(unsigned route,uint32_t address,uint8_t* out,unsigned count){
 uint32_t geometry[8]={};
 if(!Ready() || !out || count!=TileBytes || !PioneerQ38GlobalFeedbackLayout::Describe(Token,0,0,0xc0000000U,geometry)
  || route!=PioneerQ38ProjectionRoute(geometry[4]) || address<geometry[5]
  || address-geometry[5]>=Blocks*TileBytes || (address-geometry[5])%TileBytes)return false;
 const unsigned tile=(address-geometry[5])/TileBytes,first=Token-geometry[6];
 for(unsigned row=0;row<16;row++){
  uint8_t block[BlockBytes];Block(first+row,tile,block);
  for(unsigned i=0;i<BlockBytes;i++)out[i*16+row]=block[i];
 }
 return true;
}
bool Q38EmbeddingRawTestBridge::OriginalBlock(unsigned token,unsigned block,uint8_t* out,unsigned count){
 if(!Ready() || token!=Token || block>=Blocks || !out || count!=BlockBytes)return false;
 uint8_t bytes[BlockBytes];Block(token,block,bytes);memcpy(out,bytes,sizeof(bytes));return true;
}
