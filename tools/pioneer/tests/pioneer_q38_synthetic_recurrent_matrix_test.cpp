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
 unsigned found=0,route=0,address=0;
 for(unsigned i=0;i<original->Count();i++){
  const auto* p=original->Part(i);if(p->kind!=original->WeightKind || p->layer!=0 || p->role!=10)continue;
  const auto& q=proposal.parts[i];assert(++found==1 && q.bytes==348160);route=PioneerQ38ProjectionRoute(q.bank);address=original->Base+q.offset;
 }
 assert(found==1);std::array<uint8_t,1088> bytes;std::array<float,1024> values;
 for(unsigned band=0;band<2;band++)for(unsigned tile=0;tile<160;tile++){
  assert(source.Read(route,address+(band*160+tile)*1088,bytes.data(),bytes.size()));
  assert(PioneerBlackholeBlockFloat::UnpackTile(PioneerBlackholeBlockFloat::Format::Bfp8B,bytes.data(),bytes.size(),values.data(),values.size()));
  for(unsigned row=0;row<32;row++)for(unsigned col=0;col<32;col++){
   const unsigned global=band*32+row;const float expected=global>=48?0.f:float(int((global*7+tile*3+col*5)%17)-8)*0.0625f;
   assert(values[row*32+col]==expected);
  }
 }
 bytes.fill(0xa5);const auto before=bytes;
 assert(!source.Read(route^1,address,bytes.data(),bytes.size()) && bytes==before);
 assert(!source.Read(route,address+348160-100,bytes.data(),bytes.size()) && bytes==before);
 source.Revoke();assert(!source.Read(route,address,bytes.data(),bytes.size()) && bytes==before);
 puts("PASS 320 synthetic recurrent BFP8 tiles, padded rows and source bounds; NO MODEL OR DEVICE");
}
