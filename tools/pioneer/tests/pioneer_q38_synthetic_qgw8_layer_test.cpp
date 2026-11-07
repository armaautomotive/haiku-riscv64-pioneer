/* SPDX-License-Identifier: MIT */
// Synthetic input fixture only; no model, native owner or device access.
#define PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION 1
#define PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT 1
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define PIONEER_Q38_EMBEDDING_RAW_TEST_BRIDGE 1
#include "../PioneerQ38HybridHeadModelPlan.h"
#include "../PioneerQ38GlobalFeedbackLayout.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38CanonicalModelPlan.h"
#include "pioneer_q38_embedding_raw_test_bridge.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <memory>

int main(int argc,char** argv){
 assert(argc==3);
 Q38EmbeddingRawTestBridge wrong;assert(!wrong.Open(argv[2]) && !wrong.Ready());
 Q38EmbeddingRawTestBridge source;assert(source.Open(argv[1]) && source.Ready());
 auto original=std::make_unique<PioneerQ38CanonicalModelPlan>();PioneerQ38HybridHeadModelPlan::Proposal proposal;
 assert(original->Build() && PioneerQ38HybridHeadModelPlan::Build(*original,proposal));
 unsigned found=0,route=0,address=0;
 for(unsigned i=0;i<original->Count();i++){
  const auto* p=original->Part(i);if(p->kind!=original->WeightKind || p->layer!=0 || p->role!=0)continue;
  const auto& q=proposal.parts[i];assert(++found==1 && q.bytes==20480);
  route=PioneerQ38ProjectionRoute(q.bank);address=original->Base+q.offset;
 }
 assert(found==1);std::array<uint8_t,16384> bytes={};
 assert(source.Read(route,address,bytes.data(),16384));
 for(unsigned i=0;i<bytes.size();i++)assert(bytes[i]==((i&3)==2?0x80:(i&3)==3?0x3f:0));
 bytes.fill(0xa5);assert(source.Read(route,address+16384,bytes.data(),4096));
 for(unsigned i=0;i<4096;i++)assert(bytes[i]==((i&3)==2?0x80:(i&3)==3?0x3f:0));
 for(unsigned i=4096;i<bytes.size();i++)assert(bytes[i]==0xa5);
 bytes.fill(0xa5);const auto before=bytes;
 assert(!source.Read(route^1,address,bytes.data(),16384) && bytes==before);
 assert(!source.Read(route,address+4,bytes.data(),16384) && bytes==before);
 assert(!source.Read(route,address,bytes.data(),4096) && bytes==before);
 source.Revoke();assert(!source.Read(route,address,bytes.data(),16384) && bytes==before);
 puts("PASS isolated synthetic QGW8 layer fixture and out-of-range/refused reads; NO MODEL OR DEVICE");
}
