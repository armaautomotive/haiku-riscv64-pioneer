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
#ifdef PIONEER_Q38_SYNTHETIC_QGW8_LAYER_TEST
#if PIONEER_Q38_SYNTHETIC_QGW8_LAYER_TEST != 1
#error Synthetic QGW8 layer data requires the exact isolated test selector
#endif
#define PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION 1
#define PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT 1
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "../PioneerQ38HybridHeadModelPlan.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38CanonicalModelPlan.h"
#include <memory>
#endif
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
struct Q38EmbeddingRawTestBridge::Source {bool ready=false,revoked=false;
#ifdef PIONEER_Q38_SYNTHETIC_QGW8_LAYER_TEST
 uint32_t normRoute=0,normAddress=0;
#endif
};
Q38EmbeddingRawTestBridge::Q38EmbeddingRawTestBridge():source(new Source){}
Q38EmbeddingRawTestBridge::~Q38EmbeddingRawTestBridge(){delete source;}
bool Q38EmbeddingRawTestBridge::Open(const char* path){
 if(!source || source->ready || source->revoked || !path)return false;
 FILE* file=fopen(path,"rb");if(!file)return false;
 char bytes[64]={};const size_t n=fread(bytes,1,sizeof(bytes),file);bool okay=!ferror(file);if(fclose(file)!=0)okay=false;
 #ifdef PIONEER_Q38_SYNTHETIC_QGW8_LAYER_TEST
 static constexpr char signature[]="SYNTHETIC_Q4K_QGW8_LAYER_V1\n";
 #else
 static constexpr char signature[]="SYNTHETIC_Q4K_EMBEDDING_V1\n";
 #endif
 source->ready=okay && n==sizeof(signature)-1 && !memcmp(bytes,signature,sizeof(signature)-1);
#ifdef PIONEER_Q38_SYNTHETIC_QGW8_LAYER_TEST
 if(source->ready){
  auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();PioneerQ38HybridHeadModelPlan::Proposal proposal;
  if(!canonical->Build() || !PioneerQ38HybridHeadModelPlan::Build(*canonical,proposal))return source->ready=false;
  unsigned found=0;
  for(unsigned i=0;i<canonical->Count();i++){
   const auto* p=canonical->Part(i);const auto& q=proposal.parts[i];
   if(p->kind!=canonical->WeightKind || p->layer!=0 || p->role!=0)continue;
   if(++found!=1 || q.bank>=7 || q.encoding!=PioneerQ38HybridHeadModelPlan::Base::Original
    || q.bytes!=20480 || q.offset>UINT32_MAX-canonical->Base)return source->ready=false;
   source->normRoute=PioneerQ38ProjectionRoute(q.bank);source->normAddress=canonical->Base+q.offset;
  }
  if(found!=1)return source->ready=false;
 }
#endif
 return source->ready;
}
bool Q38EmbeddingRawTestBridge::Ready()const{return source && source->ready && !source->revoked;}
void Q38EmbeddingRawTestBridge::Revoke(){if(source)source->revoked=true;}
bool Q38EmbeddingRawTestBridge::Read(unsigned route,uint32_t address,uint8_t* out,unsigned count){
 uint32_t geometry[8]={};
#ifdef PIONEER_Q38_SYNTHETIC_QGW8_LAYER_TEST
 if(Ready() && out && route==source->normRoute && ((address==source->normAddress && count==16384)
  || (address==source->normAddress+16384 && count==4096))){
  // Test-only finite unit weights; never real model weights or worker results.
  for(unsigned i=0;i<count;i++)out[i]=(i&3)==2?0x80:(i&3)==3?0x3f:0;
  return true;
 }
#endif
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
