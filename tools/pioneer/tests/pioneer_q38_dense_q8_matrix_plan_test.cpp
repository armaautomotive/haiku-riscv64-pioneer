/* SPDX-License-Identifier: MIT */
// Fixed-model geometry only: no model file, runtime owner or card.
#define PIONEER_Q38_DENSE_Q8_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION 1
#define PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT 1
#define PIONEER_Q38_GDN_MUTABLE_STATE
#define PIONEER_BANKS_MOCK_ONLY
#include "../PioneerQ38HybridHeadLayout.h"
#include "pioneer_resident_banks_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38CanonicalModelPlan.h"
#include "../PioneerQ38HybridDenseQ8MatrixPlan.h"
#include <cassert>
#include <cstdio>
#include <memory>
using Canonical=PioneerQ38CanonicalModelPlan;using Plan=PioneerQ38HybridDenseQ8MatrixPlan;
struct Mutated {
 static constexpr uint64_t Capacity=Canonical::Capacity;
 static constexpr unsigned WeightKind=Canonical::WeightKind;
 const Canonical& source;unsigned target,fault;mutable PioneerQ38CanonicalPart copy={};
 bool Ready()const{return source.Ready();}unsigned Count()const{return source.Count();}
 uint64_t Used(unsigned bank)const{return source.Used(bank);}
 const PioneerQ38CanonicalPart* Part(unsigned index)const{
  const auto* p=source.Part(index);if(!p)return nullptr;copy=*p;
  if(index==target){
   if(fault==0)copy.role=8;if(fault==1)copy.type=12;if(fault==2)copy.firstRow+=32;
   if(fault==3)copy.rows-=32;if(fault==4)copy.rowBytes++;if(fault==5)copy.bank=7;
   if(fault==6)copy.offset+=4096;if(fault==7)copy.allocated+=4096;
  }
  return &copy;
 }
};
int main(){
 auto source=std::make_unique<Canonical>();assert(source->Build());
 Plan::Parent::Proposal prior;Plan::Proposal candidate;
 assert(Plan::Parent::Build(*source,prior) && Plan::Build(*source,candidate));
 assert(candidate.parts.size()==source->Count());unsigned fragments=0;uint64_t saved=0,rows=0,next[7]={};
 for(unsigned i=0;i<source->Count();i++){
  const auto* p=source->Part(i);const auto& old=prior.parts[i];const auto& q=candidate.parts[i];
  assert(p && q.original==i && q.bank==p->bank && q.offset==next[q.bank]);next[q.bank]+=q.allocated;
  if(p->kind==Canonical::WeightKind && p->layer<64 && p->layer%4==3 && p->role>=5 && p->role<=7){
   assert(p->type==8 && q.encoding==Plan::DenseQ8Bfp8 && q.bytes==uint64_t(p->rows)*5440
    && old.bytes==uint64_t(p->rows)*5760 && q.allocated<=old.allocated);
   fragments++;saved+=old.allocated-q.allocated;rows+=p->rows;
  }else assert(q.encoding==old.encoding && q.bytes==old.bytes && q.allocated==old.allocated);
 }
 assert(fragments==336 && rows==16*(12288+1024+1024));
 for(unsigned b=0;b<7;b++)assert(next[b]==candidate.used[b] && next[b]<=prior.used[b] && next[b]<=Canonical::Capacity);
 auto empty=std::make_unique<Canonical>();const auto before=candidate.used;const auto count=candidate.parts.size();
 assert(!Plan::Build(*empty,candidate) && candidate.used==before && candidate.parts.size()==count);
 unsigned target=source->Count();
 for(unsigned i=0;i<source->Count();i++){
  const auto* p=source->Part(i);
  if(p && p->kind==Canonical::WeightKind && p->layer==3 && p->role==5){target=i;break;}
 }
 assert(target<source->Count());
 for(unsigned fault=0;fault<8;fault++){
  Mutated malformed{*source,target,fault};
  assert(!Plan::Build(malformed,candidate) && candidate.used==before && candidate.parts.size()==count);
 }
 std::printf("PASS dense Q8 BFP8 proposed geometry fragments=%u rows=%llu allocation_saved=%llu bytes; NO model/device/speed\n",
  fragments,(unsigned long long)rows,(unsigned long long)saved);
}
