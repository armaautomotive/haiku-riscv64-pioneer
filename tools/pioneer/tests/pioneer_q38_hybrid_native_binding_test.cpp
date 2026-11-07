/* SPDX-License-Identifier: MIT */
// Real tracked receipt feeding the real native bind/call-scope with simulated
// operating system and bank endpoints. No card, initialization or execution.
#define main pioneer_original_binding_fixture_main
#include "pioneer_q38_token_native_binding_test.cpp"
#undef main
#ifdef PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION
static_assert(PioneerQ38TokenNativeProfile==7 && PioneerQ38TokenPrograms::Family==0x37505451
 && PioneerQ38SelectedGlobalWorker::Magic==0x37574751,
 "native inspection must bind the QGW7 program and worker grammar together");
#else
static_assert(PioneerQ38TokenNativeProfile==5 && PioneerQ38TokenPrograms::Family==0x35505451
 && PioneerQ38SelectedGlobalWorker::Magic==0x35574751,
 "ordinary hybrid inspection must retain the QGW5 identity");
#endif

int main(){
 std::vector<uint32> hashes(PioneerResidentBanks::Pages);
 Fixture f;auto& c=*f.c;std::fill(f.learned.begin(),f.learned.end(),0);
 c.hashes=hashes.data();c.initialized=f.learned.data();c.span={};
 c.q38HybridReceipt=new PioneerQ38HybridUploadReceipt;
 auto& receipt=*c.q38HybridReceipt;
 assert(receipt.Begin(&c,&f.cookie,c.hashes,c.initialized,f.cookie.team,sReloadGeneration,
  c.banks,f.Dram(),f.learned.size()));
 unsigned bank=0,length=0,spans=0;uint64 offset=0,total=0;
 while(receipt.Next(bank,offset,length)){
  assert(receipt.Before(&c,c.hashes,c.initialized,&c.span,bank,offset,length));
  const unsigned first=unsigned(uint64(bank)*(PioneerResidentBanks::Capacity/4096)+offset/4096);
  for(unsigned p=0;p<length/4096;p++){
   const unsigned index=first+p;const uint32 hash=index*2654435761U;
   f.learned[index]=1;hashes[index]=hash;assert(receipt.RecordPage(index,hash));
  }
  c.span.saved=c.span.ready=c.span.dirty=true;c.span.nonce=++spans;
  assert(receipt.After());total+=length;
 }
 c.span.closed=true;c.span.ready=c.span.dirty=false;
 assert(spans==22005 && total==UINT64_C(21137039360) && receipt.Seal(c.span));
 assert(receipt.Matches(&c,&f.cookie,c.hashes,c.initialized,f.cookie.team,sReloadGeneration));
 auto probe=std::make_unique<PioneerQ38CanonicalModelPlan>();
 const bool derived=receipt.DeriveBoundLayout(*probe,&c,&f.cookie,c.hashes,c.initialized,f.cookie.team,sReloadGeneration);
 const bool compatible=derived && PioneerQ38TokenStartupPlan::WorkerLayoutCompatible(*probe);
 const bool pages=derived && PioneerQ38TokenStartupPlan::CanonicalLearnedPagesPresent(*probe,c.initialized,PioneerResidentBanks::Pages);
 PioneerResidentMutableRange ranges[7]={};
 const bool runtime=derived && PioneerQ38RuntimeRanges(*probe,ranges);
 bool scope=false;
 {PioneerQ38TokenCallScope call(&f.cookie);scope=call.Context()==&c && c.io.TokenModelSourceScope(true);}
 assert(derived && compatible && pages && runtime && scope);
 unsigned converted=0;PioneerQ38CanonicalPart* changed=nullptr;
 for(unsigned i=0;i<probe->Count();i++){
  const auto* part=probe->Part(i);
  if(part->kind!=probe->WeightKind || part->layer<48 || part->layer>60 || part->role<2 || part->role>4)continue;
  const uint64 width=part->role==4?17408:5120;
  assert(part->type==12 && part->rowBytes==width*9/16 && part->bytes==uint64(part->rows)*width*17/16);
  converted++;if(!changed)changed=const_cast<PioneerQ38CanonicalPart*>(part);
 }
 assert(converted==13*3*7 && changed);
 const auto saved=*changed;
 auto refuses=[&]{
  PioneerResidentMutableRange untouched[7];std::memset(untouched,0xa5,sizeof(untouched));
  PioneerResidentMutableRange before[7];std::memcpy(before,untouched,sizeof(before));
  assert(!PioneerQ38RuntimeRanges(*probe,untouched) && !std::memcmp(before,untouched,sizeof(before)));
  *changed=saved;
 };
 changed->type=8;refuses();changed->rowBytes++;refuses();changed->bytes--;refuses();
 changed->allocated+=4096;refuses();changed->layer=47;refuses();changed->role=5;refuses();
 const bool bound=f.Bind();
 assert(bound && f.Binding() && c.q38TokenNative && !c.q38TokenNative->Unsafe());
 assert(c.q38TokenNative->MatchesReservation(c.banks,f.Dram()));
 f.Inert();
 puts("PASS real sealed hybrid receipt to non-loadable native bind; synthetic ledger/OS/banks only; NO DEVICE OR INFERENCE");
}
