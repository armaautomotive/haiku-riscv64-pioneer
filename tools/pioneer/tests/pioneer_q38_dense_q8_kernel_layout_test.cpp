/* SPDX-License-Identifier: MIT */
// Independent fixed-size reconstruction vs host proposal; no live device.
#define PIONEER_Q38_QGW8_LAYOUT_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_KERNEL_LAYOUT_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION 1
#define PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT 1
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define PIONEER_BANKS_MOCK_ONLY 1
#include "pioneer_resident_banks_test.cpp"
#include "../PioneerQ38DenseQ8KernelLayoutInspection.h"
#include "../PioneerQ38HybridDenseQ8MatrixPlan.h"
#include <memory>
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
 printf("PASS fixed dense Q8 layout matches %u host parts, %u converted stripes, %llu reserved hole bytes; NO DEVICE\n",
  fixed->Count(),converted,(unsigned long long)holes);
}
