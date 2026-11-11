/* SPDX-License-Identifier: MIT */
// Real token native bind against the real QGWD receipt; synthetic endpoints.
#define main pioneer_original_binding_fixture_main
#include "pioneer_q38_token_native_binding_test.cpp"
#undef main
#ifdef PIONEER_Q38_QGWF_NATIVE_INSPECTION
static_assert(PioneerQ38TokenNativeProfile==13 && PioneerQ38TokenPrograms::Family==0x49505451
 && PioneerQ38SelectedGlobalWorker::Magic==0x46574751,"fake QGWF native profile required");
#elif defined(PIONEER_Q38_QGWE_NATIVE_INSPECTION)
static_assert(PioneerQ38TokenNativeProfile==13 && PioneerQ38TokenPrograms::Family==0x48505451
 && PioneerQ38SelectedGlobalWorker::Magic==0x45574751,"fake QGWE native profile required");
#else
static_assert(PioneerQ38TokenNativeProfile==13 && PioneerQ38TokenPrograms::Family==0x47505451
 && PioneerQ38SelectedGlobalWorker::Magic==0x44574751,"fake QGWD native profile required");
#endif
int main(){
 std::vector<uint32> hashes(PioneerResidentBanks::Pages);
 Fixture f;auto& c=*f.c;std::fill(f.learned.begin(),f.learned.end(),0);
 c.hashes=hashes.data();c.initialized=f.learned.data();c.span={};
 c.q38QgwdReceipt=new PioneerQ38DenseQ8UploadReceiptInspection;
 auto& receipt=*c.q38QgwdReceipt;
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
 assert(spans==21907 && total==UINT64_C(21094223872) && receipt.Seal(c.span));
 auto logical=std::make_unique<PioneerQ38CanonicalModelPlan>();
 assert(PioneerQ38QgwdNativeBinding::DeriveLogical(c,&f.cookie,sReloadGeneration,*logical)
  && !PioneerQ38TokenStartupPlan::CanonicalLearnedPagesPresent(*logical,c.initialized,PioneerResidentBanks::Pages)
  && Q38TokenNativeLearnedPresent(c,*logical));
 const auto epoch=sQ38TokenModelEpoch;
 c.q38QgwdReceipt=nullptr;assert(!f.Bind() && !c.q38TokenNative && sQ38TokenModelEpoch==epoch);
 c.q38QgwdReceipt=&receipt;c.hashes=hashes.data()+1;
 assert(!f.Bind() && !c.q38TokenNative && sQ38TokenModelEpoch==epoch);c.hashes=hashes.data();
 sReloadGeneration++;assert(!f.Bind() && !c.q38TokenNative && sQ38TokenModelEpoch==epoch);sReloadGeneration--;
 assert(f.Bind() && f.Binding() && c.q38TokenNative && !c.q38TokenNative->Unsafe()
  && c.q38TokenNative->MatchesReservation(c.banks,f.Dram()));
 c.q38QgwdReceipt=nullptr;assert(!f.Binding());c.q38QgwdReceipt=&receipt;assert(f.Binding());
 f.Inert();
 puts("PASS actual QGWD sealed receipt/native bind/reservation with gapped ledger; SIMULATED ONLY");
}
