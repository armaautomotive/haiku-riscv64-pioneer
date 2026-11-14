/* SPDX-License-Identifier: MIT */
// Synthetic Q4_K embedding and optional pinned model-derived matrix bytes for
// the offline ISA test ONLY. Never a native driver, Work path, device transfer
// or fallback for an original GGUF.
#if !defined(PIONEER_Q38_SYNTHETIC_EMBEDDING_TEST) || PIONEER_Q38_SYNTHETIC_EMBEDDING_TEST != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) \
 || defined(PIONEER_Q38_BF16_FULL_NATIVE) || defined(PIONEER_Q38_BF16_FULL_CLIENT) \
 || defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) || defined(PIONEER_Q38_GLOBAL_WORKER_IMAGE)
#error Synthetic embedding bytes require their isolated host-test implementation
#endif
#if defined(PIONEER_Q38_SYNTHETIC_RECURRENT_MATRIX_TEST) && \
 (!defined(PIONEER_Q38_SYNTHETIC_QGW8_LAYER_TEST) || PIONEER_Q38_SYNTHETIC_QGW8_LAYER_TEST != 1)
#error Synthetic recurrent weights require the isolated synthetic layer fixture
#endif
#if defined(PIONEER_Q38_SYNTHETIC_GATE_MATRIX_TEST) && \
 (PIONEER_Q38_SYNTHETIC_GATE_MATRIX_TEST != 1 || !defined(PIONEER_Q38_SYNTHETIC_RECURRENT_MATRIX_TEST))
#error Synthetic gate weights require the isolated recurrent matrix fixture
#endif
#if defined(PIONEER_Q38_SYNTHETIC_QKV_MATRIX_TEST) && \
 (PIONEER_Q38_SYNTHETIC_QKV_MATRIX_TEST != 1 || !defined(PIONEER_Q38_SYNTHETIC_GATE_MATRIX_TEST))
#error Synthetic QKV weights require the isolated gate fixture
#endif
#if defined(PIONEER_Q38_MODEL_QKV_OBSERVER_TEST) && \
 (PIONEER_Q38_MODEL_QKV_OBSERVER_TEST != 1 || !defined(PIONEER_Q38_MODEL_RECURRENT_OBSERVER_TEST) \
 || !defined(PIONEER_Q38_SYNTHETIC_QKV_MATRIX_TEST))
#error Model QKV bytes require the isolated model recurrent QKV fixture
#endif
#if defined(PIONEER_Q38_SYNTHETIC_CONV_STATE_TEST) && \
 (PIONEER_Q38_SYNTHETIC_CONV_STATE_TEST != 1 || !defined(PIONEER_Q38_SYNTHETIC_QKV_MATRIX_TEST))
#error Synthetic Conv state requires the isolated QKV fixture
#endif
#if defined(PIONEER_Q38_SYNTHETIC_GDN_WEIGHT_TEST) && \
 (PIONEER_Q38_SYNTHETIC_GDN_WEIGHT_TEST != 1 || !defined(PIONEER_Q38_SYNTHETIC_CONV_STATE_TEST))
#error Synthetic GDN weights require the isolated Conv fixture
#endif
#if defined(PIONEER_Q38_SYNTHETIC_OUTPUT_MATRIX_TEST) && \
 (PIONEER_Q38_SYNTHETIC_OUTPUT_MATRIX_TEST != 1 || !defined(PIONEER_Q38_SYNTHETIC_GDN_WEIGHT_TEST))
#error Synthetic output matrix requires the isolated GDN fixture
#endif
#if defined(PIONEER_Q38_SYNTHETIC_FFN_MATRIX_TEST) && \
 (PIONEER_Q38_SYNTHETIC_FFN_MATRIX_TEST != 1 || !defined(PIONEER_Q38_SYNTHETIC_OUTPUT_MATRIX_TEST))
#error Synthetic FFN matrix requires the isolated output matrix fixture
#endif
#if defined(PIONEER_Q38_SYNTHETIC_FFN_UP_MATRIX_TEST) && \
 (PIONEER_Q38_SYNTHETIC_FFN_UP_MATRIX_TEST != 1 || !defined(PIONEER_Q38_SYNTHETIC_FFN_MATRIX_TEST))
#error Synthetic FFN up matrix requires the isolated FFN matrix fixture
#endif
#if defined(PIONEER_Q38_SYNTHETIC_FFN_DOWN_MATRIX_TEST) && \
 (PIONEER_Q38_SYNTHETIC_FFN_DOWN_MATRIX_TEST != 1 || !defined(PIONEER_Q38_SYNTHETIC_FFN_UP_MATRIX_TEST))
#error Synthetic FFN down matrix requires the isolated FFN up fixture
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
#ifdef PIONEER_Q38_SYNTHETIC_RECURRENT_MATRIX_TEST
#if PIONEER_Q38_SYNTHETIC_RECURRENT_MATRIX_TEST != 1
#error Synthetic recurrent matrix bytes require the exact test selector
#endif
#define PIONEER_Q38_RECURRENT_Q8_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_SMALL_RECURRENT_BFP8_PLAN_INSPECTION 1
#include "../PioneerQ38HybridSmallRecurrentBfp8Plan.h"
#include "../PioneerBlackholeBlockFloat.h"
#include <vector>
#endif
#endif
#include <cstdio>
#include <cstdlib>
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
#ifdef PIONEER_Q38_SYNTHETIC_OUTPUT_MATRIX_TEST
 uint32_t postNormRoute=0,postNormAddress=0;
#endif
#ifdef PIONEER_Q38_SYNTHETIC_RECURRENT_MATRIX_TEST
 uint32_t matrixRoute[2]={},matrixAddress[2]={};std::vector<uint8_t> matrix[2];
#ifdef PIONEER_Q38_SYNTHETIC_GATE_MATRIX_TEST
 struct Gate {uint32_t route,address;std::vector<uint8_t> bytes;};std::vector<Gate> gate;
#ifdef PIONEER_Q38_SYNTHETIC_CONV_STATE_TEST
 std::vector<Gate> convState,convWeight;
#ifdef PIONEER_Q38_SYNTHETIC_GDN_WEIGHT_TEST
 std::vector<Gate> gdnWeight;
#endif
#ifdef PIONEER_Q38_SYNTHETIC_FFN_MATRIX_TEST
 std::vector<Gate> ffnWeight;
#endif
#endif
#endif
#endif
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
#ifdef PIONEER_Q38_SYNTHETIC_OUTPUT_MATRIX_TEST
  found=0;for(unsigned i=0;i<canonical->Count();i++){
   const auto* p=canonical->Part(i);const auto& q=proposal.parts[i];
   if(p->kind!=canonical->WeightKind || p->layer || p->role!=1)continue;
   if(++found!=1 || q.bank>=7 || q.bytes!=20480 || q.offset>UINT32_MAX-canonical->Base)return source->ready=false;
   source->postNormRoute=PioneerQ38ProjectionRoute(q.bank);source->postNormAddress=canonical->Base+q.offset;
  }
  if(found!=1)return source->ready=false;
#endif
#ifdef PIONEER_Q38_SYNTHETIC_RECURRENT_MATRIX_TEST
  PioneerQ38HybridSmallRecurrentBfp8Plan::Proposal physical;
  if(!PioneerQ38HybridSmallRecurrentBfp8Plan::Build(*canonical,physical))return source->ready=false;
  found=0;
  for(unsigned i=0;i<canonical->Count();i++){
   const auto* p=canonical->Part(i);const auto& q=physical.parts[i];
   if(p->kind!=canonical->WeightKind || p->layer!=0 || (p->role!=10 && p->role!=11))continue;
   const unsigned role=p->role-10;
   if(++found>2 || !source->matrix[role].empty() || q.bank>=7 || q.encoding!=PioneerQ38HybridSmallRecurrentBfp8Plan::SmallRecurrentBfp8
    || q.bytes!=348160 || q.offset>UINT32_MAX-canonical->Base)return source->ready=false;
   source->matrixRoute[role]=PioneerQ38ProjectionRoute(q.bank);source->matrixAddress[role]=canonical->Base+q.offset;
   source->matrix[role].resize(q.bytes);
   for(unsigned band=0;band<2;band++)for(unsigned tile=0;tile<160;tile++){
    float values[1024];for(unsigned row=0;row<32;row++)for(unsigned col=0;col<32;col++){
     const unsigned global=band*32+row;values[row*32+col]=global>=48?0.f:float(int((global*7+tile*3+col*5+role*3)%17)-8)*0.0625f;
    }
    if(!PioneerBlackholeBlockFloat::PackTile(PioneerBlackholeBlockFloat::Format::Bfp8B,values,1024,
       source->matrix[role].data()+(band*160+tile)*1088,1088))return source->ready=false;
   }
  }
  if(found!=2)return source->ready=false;
#ifdef PIONEER_Q38_SYNTHETIC_GATE_MATRIX_TEST
  for(unsigned i=0;i<canonical->Count();i++){
   const auto* p=canonical->Part(i);const auto& q=physical.parts[i];
   if(p->kind!=canonical->WeightKind || p->layer!=0 || (p->role!=5
#ifdef PIONEER_Q38_SYNTHETIC_QKV_MATRIX_TEST
    && p->role!=6
#endif
#ifdef PIONEER_Q38_SYNTHETIC_OUTPUT_MATRIX_TEST
    && p->role!=13
#endif
   ))continue;
   if(q.bank>=7 || q.encoding!=PioneerQ38HybridRecurrentQ8MatrixPlan::RecurrentQ8Bfp8
    || !p->rows || (p->rows&31) || q.bytes!=p->rows*(p->role==13?6528U:5440U) || q.offset>UINT32_MAX-canonical->Base)return source->ready=false;
   Source::Gate part{PioneerQ38ProjectionRoute(q.bank),canonical->Base+q.offset,{}};part.bytes.resize(q.bytes);
   const unsigned tiles=p->role==13?192:160;
   for(unsigned band=0;band<p->rows/32;band++)for(unsigned t=0;t<tiles;t++){
    float values[1024];for(unsigned row=0;row<32;row++)for(unsigned col=0;col<32;col++){
     const unsigned global=p->firstRow+band*32+row;values[row*32+col]=float(int((global*3+t*7+col*5+(p->role-5)*3)%17)-8)*0.0625f;
    }
    if(!PioneerBlackholeBlockFloat::PackTile(PioneerBlackholeBlockFloat::Format::Bfp8B,values,1024,
      part.bytes.data()+(band*tiles+t)*1088,1088))return source->ready=false;
   }
#ifdef PIONEER_Q38_MODEL_QKV_OBSERVER_TEST
   if(p->role==6){
    const char* path=std::getenv("Q38_TEST_MODEL_QKV");if(!path || path[0]!='/')return source->ready=false;
    FILE* file=std::fopen(path,"rb");if(!file)return source->ready=false;
    uint32_t h[8];const uint32_t expected[]={0x31525051,1,5,5120,10240,320,160,1088};
    bool valid=std::fread(h,4,8,file)==8 && !std::memcmp(h,expected,sizeof(h)) && !(p->firstRow&31);
    const size_t bandBytes=160*1088,recordBytes=bandBytes+32*4;
    for(unsigned band=0;band<p->rows/32 && valid;band++){
     const size_t offset=32+size_t(p->firstRow/32+band)*recordBytes;
     valid=std::fseek(file,long(offset),SEEK_SET)==0
      && std::fread(part.bytes.data()+band*bandBytes,1,bandBytes,file)==bandBytes;
    }
    if(std::fclose(file))valid=false;if(!valid)return source->ready=false;
   }
#endif
   source->gate.push_back(std::move(part));
  }
  if(source->gate.size()!=
#ifdef PIONEER_Q38_SYNTHETIC_OUTPUT_MATRIX_TEST
   21
#elif defined(PIONEER_Q38_SYNTHETIC_QKV_MATRIX_TEST)
   14
#else
   7
#endif
  )return source->ready=false;
#ifdef PIONEER_Q38_SYNTHETIC_CONV_STATE_TEST
  for(unsigned i=0;i<canonical->Count();i++){
   const auto* p=canonical->Part(i);const auto& q=physical.parts[i];
   if(p->layer || !((p->kind==canonical->ConvKind) || (p->kind==canonical->WeightKind && p->role==12)))continue;
   if(q.bank>=7 || q.encoding!=PioneerQ38HybridModelPlan::Original || !q.bytes || q.offset>UINT32_MAX-canonical->Base)return source->ready=false;
   Source::Gate part{PioneerQ38ProjectionRoute(q.bank),canonical->Base+q.offset,{}};part.bytes.resize(q.bytes);
   if(p->kind==canonical->ConvKind){
    if(p->rowBytes!=12 || q.bytes!=p->rows*12)return source->ready=false;
    source->convState.push_back(std::move(part));
   }else{
    if(p->rowBytes!=16 || q.bytes!=p->rows*16)return source->ready=false;
    for(unsigned j=0;j<q.bytes;j+=16){part.bytes[j+14]=0x80;part.bytes[j+15]=0x3f;}
    source->convWeight.push_back(std::move(part));
   }
  }
  if(source->convState.size()!=7 || source->convWeight.size()!=7)return source->ready=false;
#ifdef PIONEER_Q38_SYNTHETIC_FFN_MATRIX_TEST
  for(unsigned i=0;i<canonical->Count();i++){
   const auto* p=canonical->Part(i);const auto& q=physical.parts[i];
   if(p->kind!=canonical->WeightKind || p->layer || (p->role!=2
#ifdef PIONEER_Q38_SYNTHETIC_FFN_UP_MATRIX_TEST
    && p->role!=3
#ifdef PIONEER_Q38_SYNTHETIC_FFN_DOWN_MATRIX_TEST
    && p->role!=4
#endif
#endif
   ))continue;
   const unsigned tiles=p->role==4?544U:160U;
   if(q.bank>=7 || q.encoding!=PioneerQ38HybridModelPlan::Bfp4 || !p->rows || (p->rows&31)
    || q.bytes!=p->rows*(tiles*18U) || q.offset>UINT32_MAX-canonical->Base)return source->ready=false;
   Source::Gate part{PioneerQ38ProjectionRoute(q.bank),canonical->Base+q.offset,{}};part.bytes.resize(q.bytes);
   for(unsigned band=0;band<p->rows/32;band++)for(unsigned tile=0;tile<tiles;tile++){
    float values[1024];for(unsigned row=0;row<32;row++)for(unsigned col=0;col<32;col++){
     const unsigned global=p->firstRow+band*32+row;values[row*32+col]=float(int((global*3+tile*7+col*5+(p->role-2)*2)%7)-3)*0.125f;
    }
    if(!PioneerBlackholeBlockFloat::PackTile(PioneerBlackholeBlockFloat::Format::Bfp4B,values,1024,
       part.bytes.data()+(band*tiles+tile)*576,576))return source->ready=false;
   }
   source->ffnWeight.push_back(std::move(part));
  }
  if(source->ffnWeight.size()!=
#ifdef PIONEER_Q38_SYNTHETIC_FFN_UP_MATRIX_TEST
#ifdef PIONEER_Q38_SYNTHETIC_FFN_DOWN_MATRIX_TEST
   21
#else
   14
#endif
#else
   7
#endif
  )return source->ready=false;
#endif
#ifdef PIONEER_Q38_SYNTHETIC_GDN_WEIGHT_TEST
  for(unsigned i=0;i<canonical->Count();i++){
   const auto* p=canonical->Part(i);const auto& q=physical.parts[i];
   if(p->kind!=canonical->WeightKind || p->layer || p->role<7 || p->role>9)continue;
   if(q.bank>=7 || q.encoding!=PioneerQ38HybridModelPlan::Original || q.offset>UINT32_MAX-canonical->Base
    || q.bytes!=(p->role==9?512U:192U))return source->ready=false;
   Source::Gate part{PioneerQ38ProjectionRoute(q.bank),canonical->Base+q.offset,{}};part.bytes.resize(q.bytes);
   for(unsigned j=0;j<q.bytes;j+=4){
    const uint32_t bits=p->role==7?0xbf800000U:p->role==9?0x3f800000U:0U;
    for(unsigned b=0;b<4;b++)part.bytes[j+b]=uint8_t(bits>>(8*b));
   }
   source->gdnWeight.push_back(std::move(part));
  }
  if(source->gdnWeight.size()!=3)return source->ready=false;
#endif
#endif
#endif
#ifdef PIONEER_Q38_MODEL_RECURRENT_OBSERVER_TEST
  const char* names[]={"Q38_TEST_MODEL_ALPHA","Q38_TEST_MODEL_BETA"};
  for(unsigned role=0;role<2;role++){
   const char* path=std::getenv(names[role]);if(!path || path[0]!='/')return source->ready=false;
   FILE* input=std::fopen(path,"rb");if(!input)return source->ready=false;
   uint32_t h[8];bool valid=std::fread(h,4,8,input)==8 && h[0]==0x31525051 && h[1]==1
    && h[3]==5120 && h[4]==64 && h[5]==2 && h[6]==160 && h[7]==1088;
   for(unsigned band=0;band<2 && valid;band++){
    valid=std::fread(source->matrix[role].data()+band*160*1088,1,160*1088,input)==160*1088;
    uint32_t ignored[32];if(valid)valid=std::fread(ignored,4,32,input)==32;
   }
   if(valid)valid=std::fgetc(input)==EOF && !std::ferror(input);
   if(std::fclose(input))valid=false;
   if(!valid)return source->ready=false;
  }
#endif
#endif
 }
#endif
 return source->ready;
}
bool Q38EmbeddingRawTestBridge::Ready()const{return source && source->ready && !source->revoked;}
void Q38EmbeddingRawTestBridge::Revoke(){if(source)source->revoked=true;}
bool Q38EmbeddingRawTestBridge::Read(unsigned route,uint32_t address,uint8_t* out,unsigned count){
 uint32_t geometry[8]={};
#ifdef PIONEER_Q38_SYNTHETIC_QGW8_LAYER_TEST
#ifdef PIONEER_Q38_SYNTHETIC_RECURRENT_MATRIX_TEST
#ifdef PIONEER_Q38_SYNTHETIC_GATE_MATRIX_TEST
#ifdef PIONEER_Q38_SYNTHETIC_CONV_STATE_TEST
#ifdef PIONEER_Q38_SYNTHETIC_FFN_MATRIX_TEST
 for(const auto& part:source->ffnWeight)if(Ready() && out && count && route==part.route && address>=part.address
  && address-part.address<=part.bytes.size() && count<=part.bytes.size()-(address-part.address)){
  memcpy(out,part.bytes.data()+(address-part.address),count);return true;
 }
#endif
#ifdef PIONEER_Q38_SYNTHETIC_GDN_WEIGHT_TEST
 for(const auto& part:source->gdnWeight)if(Ready() && out && count && route==part.route && address>=part.address
  && address-part.address<=part.bytes.size() && count<=part.bytes.size()-(address-part.address)){
  memcpy(out,part.bytes.data()+(address-part.address),count);return true;
 }
#endif
 for(const auto* parts:{&source->convState,&source->convWeight})for(const auto& part:*parts)
  if(Ready() && out && count && route==part.route && address>=part.address
   && address-part.address<=part.bytes.size() && count<=part.bytes.size()-(address-part.address)){
   memcpy(out,part.bytes.data()+(address-part.address),count);return true;
  }
#endif
 for(const auto& part:source->gate)if(Ready() && out && count && route==part.route && address>=part.address
  && address-part.address<=part.bytes.size() && count<=part.bytes.size()-(address-part.address)){
  memcpy(out,part.bytes.data()+(address-part.address),count);return true;
 }
#endif
 for(unsigned role=0;role<2;role++)if(Ready() && out && count && route==source->matrixRoute[role] && address>=source->matrixAddress[role]
  && address-source->matrixAddress[role]<=source->matrix[role].size() && count<=source->matrix[role].size()-(address-source->matrixAddress[role])){
  memcpy(out,source->matrix[role].data()+(address-source->matrixAddress[role]),count);return true;
 }
#endif
#ifdef PIONEER_Q38_SYNTHETIC_OUTPUT_MATRIX_TEST
 if(Ready() && out && route==source->postNormRoute && ((address==source->postNormAddress && count==16384)
  || (address==source->postNormAddress+16384 && count==4096))){
  for(unsigned i=0;i<count;i++)out[i]=(i&3)==2?0x80:(i&3)==3?0x3f:0;return true;
 }
#endif
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
#ifdef PIONEER_Q38_SYNTHETIC_CONV_STATE_TEST
bool Q38EmbeddingRawTestBridge::PutSyntheticConv(unsigned route,uint32_t address,uint32_t value){
 if(!Ready() || (address&3))return false;
 for(auto& part:source->convState)if(route==part.route && address>=part.address
  && address-part.address<=part.bytes.size() && 4<=part.bytes.size()-(address-part.address)){
  for(unsigned b=0;b<4;b++)part.bytes[address-part.address+b]=uint8_t(value>>(b*8));return true;
 }
 return false;
}
#endif
bool Q38EmbeddingRawTestBridge::OriginalBlock(unsigned token,unsigned block,uint8_t* out,unsigned count){
 if(!Ready() || token!=Token || block>=Blocks || !out || count!=BlockBytes)return false;
 uint8_t bytes[BlockBytes];Block(token,block,bytes);memcpy(out,bytes,sizeof(bytes));return true;
}
