/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_SMALL_RECURRENT_BFP8_PLAN_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_KERNEL_LAYOUT_FIXTURE_ONLY 1
#define PIONEER_Q38_RECURRENT_Q8_MATRIX_PLAN_INSPECTION 1
#include "pioneer_q38_dense_q8_kernel_layout_test.cpp"
#include "../PioneerQ38HybridSmallRecurrentBfp8Plan.h"
#include "../PioneerQ38SmallRecurrentKernelLayoutInspection.h"
#include "../PioneerQ38SmallRecurrentLedgerInspection.h"

int main(){
 using C=PioneerQ38CanonicalModelPlan;using Fixed=PioneerQ38SmallRecurrentKernelLayoutInspection;
 auto canonical=std::make_unique<C>();assert(canonical->Build());
 auto ancestor=std::make_unique<Fixed::Parent::Parent::Parent>();auto dense=std::make_unique<Fixed::Parent::Parent>();
 auto parent=std::make_unique<Fixed::Parent>();auto fixed=std::make_unique<Fixed>();
 assert(ancestor->Build(*canonical) && dense->Build(*canonical,*ancestor)
  && parent->Build(*canonical,*dense,*ancestor) && fixed->Build(*canonical,*parent,*dense,*ancestor));
 PioneerQ38HybridSmallRecurrentBfp8Plan::Proposal host;
 assert(PioneerQ38HybridSmallRecurrentBfp8Plan::Build(*canonical,host) && host.parts.size()==fixed->Count());
 unsigned count=0;
 for(unsigned i=0;i<fixed->Count();i++){
  const auto* a=fixed->At(i);const auto& b=host.parts[i];assert(a);
  assert(a->original==b.original && a->bank==b.bank && a->offset==b.offset && a->bytes==b.bytes
   && a->allocated==b.allocated && a->encoding==b.encoding);
  if(a->encoding==Fixed::SmallRecurrentBfp8)count++;
 }
 assert(count==96 && !fixed->Build(*canonical,*parent,*dense,*ancestor));
 for(unsigned b=0;b<7;b++)assert(fixed->Used(b)==host.used[b]);
 const unsigned perBank=unsigned(PioneerResidentBanks::Capacity/4096);
 std::vector<uint8_t> ledger(PioneerResidentBanks::Pages,0);
 std::vector<uint32_t> hashes(PioneerResidentBanks::Pages,0);
 for(unsigned i=0;i<fixed->Count();i++)if(canonical->Part(i)->kind==C::WeightKind){
  const auto* part=fixed->At(i);const unsigned first=part->bank*perBank+part->offset/4096;
  for(unsigned n=0;n<part->allocated/4096;n++){
   assert(!ledger[first+n]);ledger[first+n]=1;hashes[first+n]=(first+n)*2654435761U;
  }
 }
 uint64_t digest=UINT64_C(14695981039346656037);unsigned covered=0,firstUsed=UINT32_MAX,hole=UINT32_MAX;
 for(unsigned i=0;i<ledger.size();i++)if(ledger[i]){
  if(firstUsed==UINT32_MAX)firstUsed=i;digest=(digest^i)*UINT64_C(1099511628211);
  digest=(digest^hashes[i])*UINT64_C(1099511628211);covered++;
 }else if(hole==UINT32_MAX)hole=i;
 using Ledger=PioneerQ38SmallRecurrentLedgerInspection;
 assert(covered==5067109 && firstUsed!=UINT32_MAX && hole!=UINT32_MAX);
 assert(Ledger::Verify(*canonical,*fixed,ledger.data(),hashes.data(),ledger.size(),digest));
 ledger[firstUsed]=0;assert(!Ledger::Verify(*canonical,*fixed,ledger.data(),hashes.data(),ledger.size(),digest));ledger[firstUsed]=1;
 ledger[hole]=1;assert(!Ledger::Verify(*canonical,*fixed,ledger.data(),hashes.data(),ledger.size(),digest));ledger[hole]=0;
 hashes[firstUsed]^=1;assert(!Ledger::Verify(*canonical,*fixed,ledger.data(),hashes.data(),ledger.size(),digest));hashes[firstUsed]^=1;
 auto* altered=const_cast<Fixed::Part*>(parent->At(0));const auto saved=*altered;altered->offset+=4096;
 auto refused=std::make_unique<Fixed>();assert(!refused->Build(*canonical,*parent,*dense,*ancestor) && !refused->Ready());
 *altered=saved;assert(!refused->Build(*canonical,*parent,*dense,*ancestor));
 puts("PASS independent fixed reconstruction and address-sorted ledger for 96 relocated small matrices; SYNTHETIC NO NATIVE/DEVICE/SPEED");
}
