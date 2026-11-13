/* SPDX-License-Identifier: MIT */
// Synthetic source verification only; no native owner, device or model.
#define PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION 1
#define PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT 1
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define PIONEER_Q38_RECURRENT_Q8_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_SMALL_RECURRENT_BFP8_PLAN_INSPECTION 1
#define PIONEER_Q38_EMBEDDING_RAW_TEST_BRIDGE 1
#include "../PioneerQ38HybridSmallRecurrentBfp8Plan.h"
#include "../PioneerBlackholeBlockFloat.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38CanonicalModelPlan.h"
#include "pioneer_q38_embedding_raw_test_bridge.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <memory>
int main(int argc,char** argv){
 assert(argc==2);Q38EmbeddingRawTestBridge source;assert(source.Open(argv[1]) && source.Ready());
 auto original=std::make_unique<PioneerQ38CanonicalModelPlan>();PioneerQ38HybridSmallRecurrentBfp8Plan::Proposal proposal;
 assert(original->Build() && PioneerQ38HybridSmallRecurrentBfp8Plan::Build(*original,proposal));
 unsigned found=0,route[2]={},address[2]={};
 for(unsigned i=0;i<original->Count();i++){
  const auto* p=original->Part(i);if(p->kind!=original->WeightKind || p->layer!=0 || (p->role!=10 && p->role!=11))continue;
  const auto& q=proposal.parts[i];assert(++found<=2 && q.bytes==348160);const unsigned role=p->role-10;route[role]=PioneerQ38ProjectionRoute(q.bank);address[role]=original->Base+q.offset;
 }
 assert(found==2);std::array<uint8_t,1088> bytes;std::array<float,1024> values;
#ifdef PIONEER_Q38_SYNTHETIC_GATE_MATRIX_TEST
 unsigned fragments=0,gateRows=0;
 for(unsigned i=0;i<original->Count();i++){
  const auto* p=original->Part(i);const auto& q=proposal.parts[i];
  if(p->kind!=original->WeightKind || p->layer || p->role!=5)continue;
  assert(q.encoding==PioneerQ38HybridRecurrentQ8MatrixPlan::RecurrentQ8Bfp8 && p->firstRow==gateRows);
  for(unsigned band=0;band<p->rows/32;band++)for(unsigned tile=0;tile<160;tile++){
   assert(source.Read(PioneerQ38ProjectionRoute(q.bank),original->Base+q.offset+(band*160+tile)*1088,bytes.data(),bytes.size()));
   assert(PioneerBlackholeBlockFloat::UnpackTile(PioneerBlackholeBlockFloat::Format::Bfp8B,bytes.data(),bytes.size(),values.data(),values.size()));
   for(unsigned row=0;row<32;row++)for(unsigned col=0;col<32;col++){
    const unsigned global=p->firstRow+band*32+row;const float expected=float(int((global*3+tile*7+col*5)%17)-8)*0.0625f;
    assert(values[row*32+col]==expected);
   }
  }
  gateRows+=p->rows;fragments++;
 }
 assert(fragments==7 && gateRows==6144);
 puts("PASS synthetic role5 BFP8 source 6144 rows across seven banks; NO MODEL OR DEVICE");
#ifdef PIONEER_Q38_SYNTHETIC_QKV_MATRIX_TEST
 fragments=0;unsigned qkvRows=0;
 for(unsigned i=0;i<original->Count();i++){
  const auto* p=original->Part(i);const auto& q=proposal.parts[i];
  if(p->kind!=original->WeightKind || p->layer || p->role!=6)continue;
  assert(q.encoding==PioneerQ38HybridRecurrentQ8MatrixPlan::RecurrentQ8Bfp8 && p->firstRow==qkvRows);
  for(unsigned band=0;band<p->rows/32;band++)for(unsigned tile=0;tile<160;tile++){
   assert(source.Read(PioneerQ38ProjectionRoute(q.bank),original->Base+q.offset+(band*160+tile)*1088,bytes.data(),bytes.size()));
   assert(PioneerBlackholeBlockFloat::UnpackTile(PioneerBlackholeBlockFloat::Format::Bfp8B,bytes.data(),bytes.size(),values.data(),values.size()));
   for(unsigned row=0;row<32;row++)for(unsigned col=0;col<32;col++){
    const unsigned global=p->firstRow+band*32+row;const float expected=float(int((global*3+tile*7+col*5+3)%17)-8)*0.0625f;
    assert(values[row*32+col]==expected);
   }
  }
  qkvRows+=p->rows;fragments++;
 }
 assert(fragments==7 && qkvRows==10240);
 puts("PASS synthetic role6 BFP8 source 10240 rows across seven banks; NO MODEL OR DEVICE");
#ifdef PIONEER_Q38_SYNTHETIC_CONV_STATE_TEST
 unsigned states=0,weights=0;
 for(unsigned i=0;i<original->Count();i++){
  const auto* p=original->Part(i);const auto& q=proposal.parts[i];
  if(p->layer || !((p->kind==original->ConvKind) || (p->kind==original->WeightKind && p->role==12)))continue;
  const unsigned route=PioneerQ38ProjectionRoute(q.bank),address=original->Base+q.offset;
  if(p->kind==original->ConvKind){
   uint8_t state[12];assert(source.Read(route,address,state,sizeof(state)));
   for(uint8_t b:state)assert(!b);
   assert(source.PutSyntheticConv(route,address,0x3f000000) && source.Read(route,address,state,sizeof(state))
    && state[0]==0 && state[1]==0 && state[2]==0 && state[3]==0x3f);
   assert(!source.PutSyntheticConv(route,address+q.bytes,1));states++;
  }else{
   uint8_t weight[16];assert(source.Read(route,address,weight,sizeof(weight)));
   for(unsigned j=0;j<14;j++)assert(!weight[j]);assert(weight[14]==0x80 && weight[15]==0x3f);
   assert(!source.PutSyntheticConv(route,address,1));weights++;
  }
 }
 assert(states==7 && weights==7);puts("PASS synthetic Conv weights and bounded mutable state across seven banks; NO MODEL OR DEVICE");
#ifdef PIONEER_Q38_SYNTHETIC_GDN_WEIGHT_TEST
 unsigned gdnWeights=0;
 for(unsigned i=0;i<original->Count();i++){
  const auto* p=original->Part(i);const auto& q=proposal.parts[i];
  if(p->kind!=original->WeightKind || p->layer || p->role<7 || p->role>9)continue;
  const unsigned r=PioneerQ38ProjectionRoute(q.bank),a=original->Base+q.offset;
  const uint32_t expected=p->role==7?0xbf800000U:p->role==9?0x3f800000U:0U;
  for(unsigned at=0;at<q.bytes;at+=4){uint8_t raw[4];assert(source.Read(r,a+at,raw,4));
   const uint32_t word=uint32_t(raw[0])|uint32_t(raw[1])<<8|uint32_t(raw[2])<<16|uint32_t(raw[3])<<24;assert(word==expected);}
  uint8_t outside[4]={1,2,3,4};assert(!source.Read(r,a+q.bytes-2,outside,4));
  assert(!source.PutSyntheticConv(r,a,0));gdnWeights++;
 }
 assert(gdnWeights==3);puts("PASS synthetic GDN a, dt bias and norm weight bounds; NO MODEL OR DEVICE");
#endif
#endif
#endif
#endif
 for(unsigned role=0;role<2;role++)for(unsigned band=0;band<2;band++)for(unsigned tile=0;tile<160;tile++){
  assert(source.Read(route[role],address[role]+(band*160+tile)*1088,bytes.data(),bytes.size()));
  assert(PioneerBlackholeBlockFloat::UnpackTile(PioneerBlackholeBlockFloat::Format::Bfp8B,bytes.data(),bytes.size(),values.data(),values.size()));
  for(unsigned row=0;row<32;row++)for(unsigned col=0;col<32;col++){
   const unsigned global=band*32+row;const float expected=global>=48?0.f:float(int((global*7+tile*3+col*5+role*3)%17)-8)*0.0625f;
   assert(values[row*32+col]==expected);
  }
 }
 bytes.fill(0xa5);const auto before=bytes;
 for(unsigned role=0;role<2;role++){
  assert(!source.Read(route[role]^1,address[role],bytes.data(),bytes.size()) && bytes==before);
  assert(!source.Read(route[role],address[role]+348160-100,bytes.data(),bytes.size()) && bytes==before);
 }
 source.Revoke();assert(!source.Read(route[0],address[0],bytes.data(),bytes.size()) && bytes==before);
 puts("PASS 640 synthetic recurrent BFP8 tiles, padded rows and source bounds; NO MODEL OR DEVICE");
}
