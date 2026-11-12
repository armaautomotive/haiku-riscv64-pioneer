/* SPDX-License-Identifier: MIT */
// Offline geometry only: no cache conversion, device or inference.
#define PIONEER_Q38_DENSE_Q8_KERNEL_LAYOUT_FIXTURE_ONLY 1
#define PIONEER_Q38_RECURRENT_Q8_MATRIX_PLAN_INSPECTION 1
#include "pioneer_q38_dense_q8_kernel_layout_test.cpp"
#include "../PioneerQ38HybridRecurrentQ8MatrixPlan.h"
#include "../PioneerQ38RecurrentQ8KernelLayoutInspection.h"
#include "../PioneerQ38RecurrentQ8LedgerInspection.h"
#include "../PioneerQ38NativeBfp8WeightTiles.h"

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
 using C=PioneerQ38CanonicalModelPlan;using Old=PioneerQ38HybridDenseQ8MatrixPlan;
 using New=PioneerQ38HybridRecurrentQ8MatrixPlan;
 auto source=std::make_unique<C>();New::Proposal untouched;untouched.used[0]=123;
 assert(!New::Build(*source,untouched) && untouched.parts.empty() && untouched.used[0]==123);
 assert(source->Build());Old::Proposal old;New::Proposal next;
 assert(Old::Build(*source,old) && New::Build(*source,next));
 using Fixed=PioneerQ38RecurrentQ8KernelLayoutInspection;
 auto ancestor=std::make_unique<Fixed::Parent::Parent>();auto parent=std::make_unique<Fixed::Parent>();
 auto fixed=std::make_unique<Fixed>();
 assert(ancestor->Build(*source) && parent->Build(*source,*ancestor) && fixed->Build(*source,*parent,*ancestor));
 assert(fixed->Count()==next.parts.size());
 for(unsigned i=0;i<fixed->Count();i++){
  const auto* a=fixed->At(i);const auto& b=next.parts[i];assert(a && a->original==b.original
   && a->bank==b.bank && a->offset==b.offset && a->bytes==b.bytes && a->allocated==b.allocated && a->encoding==b.encoding);
 }
 auto* corrupt=const_cast<Fixed::Part*>(parent->At(0));assert(corrupt);const auto saved=*corrupt;
 corrupt->offset+=4096;auto rejected=std::make_unique<Fixed>();
 assert(!rejected->Build(*source,*parent,*ancestor) && !rejected->Ready() && !rejected->At(0));*corrupt=saved;
 assert(!fixed->Build(*source,*parent,*ancestor)); // one-shot, including after success
 const unsigned perBank=PioneerResidentBanks::Capacity/4096;
 std::vector<uint8_t> ledger(PioneerResidentBanks::Pages,0);
 std::vector<uint32_t> hashes(PioneerResidentBanks::Pages,0);
 uint64_t digest=UINT64_C(14695981039346656037);unsigned firstUsed=UINT32_MAX;
 for(unsigned i=0;i<source->Count();i++)if(source->Part(i)->kind==C::WeightKind){
  const auto* part=fixed->At(i);const unsigned base=part->bank*perBank+part->offset/4096;
  for(unsigned n=0;n<part->allocated/4096;n++){
   const unsigned at=base+n;assert(!ledger[at]);ledger[at]=1;hashes[at]=at*2654435761U;
   digest=(digest^at)*UINT64_C(1099511628211);digest=(digest^hashes[at])*UINT64_C(1099511628211);
   if(firstUsed==UINT32_MAX)firstUsed=at;
  }
 }
 assert(PioneerQ38RecurrentQ8LedgerInspection::Verify(*source,*fixed,ledger.data(),hashes.data(),ledger.size(),digest));
 ledger[firstUsed]=0;assert(!PioneerQ38RecurrentQ8LedgerInspection::Verify(*source,*fixed,ledger.data(),hashes.data(),ledger.size(),digest));ledger[firstUsed]=1;
 unsigned hole=0;while(hole<ledger.size() && ledger[hole])hole++;assert(hole<ledger.size());
 ledger[hole]=1;assert(!PioneerQ38RecurrentQ8LedgerInspection::Verify(*source,*fixed,ledger.data(),hashes.data(),ledger.size(),digest));ledger[hole]=0;
 hashes[firstUsed]^=1;assert(!PioneerQ38RecurrentQ8LedgerInspection::Verify(*source,*fixed,ledger.data(),hashes.data(),ledger.size(),digest));
 assert(old.parts.size()==4678 && next.parts.size()==old.parts.size() && next.used==old.used);
 unsigned converted=0,small=0;uint64_t holes=0,rows=0;
 for(unsigned i=0;i<source->Count();i++){
  const auto* p=source->Part(i);const auto& a=old.parts[i];const auto& b=next.parts[i];assert(p);
  assert(a.original==b.original && a.bank==b.bank && a.offset==b.offset && b.bytes<=a.bytes && b.allocated<=a.allocated);
  const bool recurrent=p->kind==C::WeightKind && p->layer<64 && p->layer%4!=3;
  const bool target=recurrent && (p->role==5 || p->role==6 || p->role==13);
  if(target){
   assert(b.encoding==New::RecurrentQ8Bfp8 && a.encoding==PioneerQ38HybridModelPlan::Original);
   assert(p->type==8 && !(p->rows&31) && b.bytes==uint64_t(p->rows)*(p->role==13?6528U:5440U));
   PioneerQ38Tensor tensor;tensor.rank=2;tensor.type=8;tensor.inputs=p->role==13?6144:5120;
   tensor.outputs=p->role==5?6144:p->role==6?10240:5120;
   PioneerQ38NativeBfp8WeightTiles::Layout tiles;
   assert(PioneerQ38NativeBfp8WeightTiles::Describe(tensor,tiles)
    && tiles.columnTiles==tensor.inputs/32 && tiles.rowTiles==tensor.outputs/32
    && tiles.bytes==tensor.outputs*uint64_t(p->role==13?6528U:5440U));
   converted++;rows+=p->rows;holes+=a.allocated-b.allocated;
  }else{
   assert(a.bytes==b.bytes && a.allocated==b.allocated && a.encoding==b.encoding);
   if(recurrent && (p->role==10 || p->role==11)){assert(p->type==8 && p->rows==48);small++;}
  }
 }
 assert(converted==1008 && rows==UINT64_C(1032192) && small==96 && holes>UINT64_C(340000000));
 unsigned first=0;while(first<source->Count()){
  const auto* p=source->Part(first);if(p && p->kind==C::WeightKind && p->layer==0 && p->role==13)break;first++;
 }
 assert(first<source->Count());
 for(unsigned fault=0;fault<2;fault++){
  AlteredSource changed{*source,first,*source->Part(first)};
  if(fault==0)changed.altered.firstRow+=32;else changed.altered.type=14;
  New::Proposal refused;refused.used[0]=123;
  assert(!New::Build(changed,refused) && refused.parts.empty() && refused.used[0]==123);
 }
 printf("PASS offline recurrent Q8 geometry: %u stripes, %llu rows, %llu reserved hole bytes; 96 small projections unchanged; NO DEVICE\n",
  converted,(unsigned long long)rows,(unsigned long long)holes);
}
