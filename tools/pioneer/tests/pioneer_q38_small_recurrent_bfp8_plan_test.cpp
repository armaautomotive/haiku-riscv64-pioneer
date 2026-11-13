/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_SMALL_RECURRENT_BFP8_PLAN_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_KERNEL_LAYOUT_FIXTURE_ONLY 1
#define PIONEER_Q38_RECURRENT_Q8_MATRIX_PLAN_INSPECTION 1
#include "pioneer_q38_dense_q8_kernel_layout_test.cpp"
#include "../PioneerQ38HybridSmallRecurrentBfp8Plan.h"

struct AlteredSource {
 using C=PioneerQ38CanonicalModelPlan;
 static constexpr uint64_t Capacity=C::Capacity;
 enum {WeightKind=C::WeightKind};
 const C& source;unsigned index;PioneerQ38CanonicalPart altered;
 bool Ready()const{return source.Ready();}unsigned Count()const{return source.Count();}
 uint64_t Used(unsigned bank)const{return source.Used(bank);}
 const PioneerQ38CanonicalPart* Part(unsigned i)const{return i==index?&altered:source.Part(i);}
};

int main(){
 using C=PioneerQ38CanonicalModelPlan;using Parent=PioneerQ38HybridRecurrentQ8MatrixPlan;
 using Plan=PioneerQ38HybridSmallRecurrentBfp8Plan;
 auto source=std::make_unique<C>();Plan::Proposal refused;refused.used[0]=123;
 assert(!Plan::Build(*source,refused) && refused.parts.empty() && refused.used[0]==123);
 assert(source->Build());Parent::Proposal old;Plan::Proposal proposal;
 assert(Parent::Build(*source,old) && Plan::Build(*source,proposal));
 auto end=old.used;unsigned count=0,first=source->Count();
 for(unsigned i=0;i<source->Count();i++){
  const auto* p=source->Part(i);const auto& a=old.parts[i];const auto& b=proposal.parts[i];assert(p);
  const bool target=p->kind==C::WeightKind && p->layer<64 && p->layer%4!=3 && (p->role==10 || p->role==11);
  assert(a.original==b.original && a.bank==b.bank);
  if(target){
   if(first==source->Count())first=i;
   assert(b.offset==end[b.bank] && b.offset>=old.used[b.bank] && !(b.offset&4095));
   assert(b.bytes==Plan::Bytes && b.allocated==Plan::Bytes && b.encoding==Plan::SmallRecurrentBfp8);
   assert(p->rows==48 && a.bytes==48*5760 && b.bytes==64*5440);
   end[b.bank]+=b.allocated;assert(end[b.bank]<=C::Capacity);count++;
  }else assert(a.offset==b.offset && a.bytes==b.bytes && a.allocated==b.allocated && a.encoding==b.encoding);
 }
 assert(count==96 && first<source->Count() && end==proposal.used);
 for(unsigned fault=0;fault<3;fault++){
  AlteredSource altered{*source,first,*source->Part(first)};
  if(fault==0)altered.altered.rows=47;else if(fault==1)altered.altered.type=12;else altered.altered.firstRow=1;
  Plan::Proposal unchanged;unchanged.used[0]=123;
  assert(!Plan::Build(altered,unchanged) && unchanged.parts.empty() && unchanged.used[0]==123);
 }
 std::printf("PASS 96 padded recurrent alpha/beta allocations; %u appended bytes; original addresses preserved; NO CACHE/DEVICE/WORK/SPEED\n",count*Plan::Bytes);
}
