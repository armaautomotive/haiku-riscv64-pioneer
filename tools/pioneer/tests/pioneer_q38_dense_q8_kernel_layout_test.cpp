/* SPDX-License-Identifier: MIT */
// Independent fixed-size reconstruction vs host proposal; no live device.
#define PIONEER_Q38_QGW8_LAYOUT_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_KERNEL_LAYOUT_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_CANONICAL_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION 1
#define PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT 1
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define PIONEER_BANKS_MOCK_ONLY 1
#include "pioneer_resident_banks_test.cpp"
#include "../PioneerQ38DenseQ8KernelLayoutInspection.h"
#include "../PioneerQ38HybridDenseQ8MatrixPlan.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionJobs.h"
#include <memory>
#ifndef PIONEER_Q38_DENSE_Q8_KERNEL_LAYOUT_FIXTURE_ONLY
int main(){
 using Fixed=PioneerQ38DenseQ8KernelLayoutInspection;using Host=PioneerQ38HybridDenseQ8MatrixPlan;
 auto source=std::make_unique<PioneerQ38CanonicalModelPlan>();auto parent=std::make_unique<Fixed::Parent>();
 auto refused=std::make_unique<Fixed>();assert(!refused->Build(*source,*parent) && !refused->Ready());
 assert(source->Build() && parent->Build(*source));assert(!refused->Build(*source,*parent));
 auto fixed=std::make_unique<Fixed>();Host::Proposal host;
 assert(fixed->Build(*source,*parent) && fixed->Ready() && !fixed->Build(*source,*parent) && Host::Build(*source,host));
 assert(fixed->Count()==4678 && host.parts.size()==4678 && !fixed->At(4678) && !fixed->Used(7));
 unsigned converted=0;uint64_t holes=0;
 for(unsigned i=0;i<fixed->Count();i++){
  const auto* a=fixed->At(i);const auto* old=parent->At(i);const auto& b=host.parts[i];assert(a && old);
  assert(a->original==b.original && a->bank==b.bank && a->offset==b.offset && a->bytes==b.bytes
   && a->allocated==b.allocated && a->encoding==b.encoding && a->offset==old->offset);
  if(a->encoding==Fixed::DenseQ8Bfp8){converted++;holes+=old->allocated-a->allocated;}
  else assert(a->bytes==old->bytes && a->allocated==old->allocated && a->encoding==old->encoding);
 }
 for(unsigned b=0;b<7;b++)assert(fixed->Used(b)==parent->Used(b) && fixed->Used(b)==host.used[b]);
 assert(converted==336 && holes==73400320);
 auto physical=std::make_unique<PioneerQ38CanonicalModelPlan>();
 assert(physical->BuildDenseQ8Inspection(*fixed) && physical->DenseQ8InspectionLayout()
  && !physical->OriginalFormatLayout() && !physical->Qgw8InspectionLayout() && physical->Count()==source->Count());
 unsigned recurrent=0;
 for(unsigned i=0;i<fixed->Count();i++){
  const auto* a=source->Part(i);const auto* b=physical->Part(i);const auto* q=fixed->At(i);assert(a && b && q);
  assert(a->kind==b->kind && a->layer==b->layer && a->role==b->role && a->type==b->type
   && a->firstRow==b->firstRow && a->rows==b->rows && a->rowBytes==b->rowBytes
   && b->bank==q->bank && b->offset==q->offset && b->bytes==q->bytes && b->allocated==q->allocated);
  if(b->kind==physical->RecurrentKind){const auto& r=physical->RecurrentRegions()[recurrent++];assert(r.bank==b->bank && r.offset==b->offset);}
 }
 assert(recurrent==672);for(unsigned b=0;b<7;b++)assert(physical->Used(b)==fixed->Used(b));
 // The old scalar-Q8 job builder must not mistake shorter BFP8 bytes for
 // the original scalar format. A distinct QGWD recipe remains necessary.
 auto scalar=std::make_unique<PioneerQ38ProjectionJobs>();assert(!scalar->Build(*physical,3,5,0));
 unsigned denseIndex=0;while(denseIndex<fixed->Count() && fixed->At(denseIndex)->encoding!=Fixed::DenseQ8Bfp8)denseIndex++;
 assert(denseIndex<fixed->Count());auto* corrupt=const_cast<Fixed::Part*>(fixed->At(denseIndex));const auto saved=*corrupt;
 for(unsigned mode=0;mode<4;mode++){
  if(mode==0)corrupt->offset+=4096;else if(mode==1)corrupt->bytes++;
  else if(mode==2)corrupt->allocated+=4096;else corrupt->encoding=Fixed::Original;
  auto rejected=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(!rejected->BuildDenseQ8Inspection(*fixed) && !rejected->Ready());
  *corrupt=saved;
 }
 printf("PASS fixed dense Q8 layout matches %u host parts, %u converted stripes, %llu reserved hole bytes; NO DEVICE\n",
  fixed->Count(),converted,(unsigned long long)holes);
}
#endif
