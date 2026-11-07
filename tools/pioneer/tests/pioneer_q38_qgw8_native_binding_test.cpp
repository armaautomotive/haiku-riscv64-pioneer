/* SPDX-License-Identifier: MIT */
// Real native bind/scope and QGW8 receipt with simulated OS/bank endpoints.
// No device, source authentication, initialization, launch, or inference.
#define main pioneer_original_binding_fixture_main
#include "pioneer_q38_token_native_binding_test.cpp"
#undef main
static_assert(PioneerQ38TokenNativeProfile==8 && PioneerQ38TokenPrograms::Family==0x38505451
 && PioneerQ38SelectedGlobalWorker::Magic==0x38574751,"distinct QGW8 fake-tag native profile required");

int main(){
 std::vector<uint32> hashes(PioneerResidentBanks::Pages);
 Fixture f;auto& c=*f.c;std::fill(f.learned.begin(),f.learned.end(),0);
 c.hashes=hashes.data();c.initialized=f.learned.data();c.span={};
 c.q38Qgw8Receipt=new PioneerQ38Qgw8UploadReceipt;
 auto& receipt=*c.q38Qgw8Receipt;
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
 assert(spans==22035 && total==UINT64_C(21167624192) && receipt.Seal(c.span));
 auto probe=std::make_unique<PioneerQ38CanonicalModelPlan>();
 assert(receipt.DeriveBoundCanonical(*probe,&c,&f.cookie,c.hashes,c.initialized,f.cookie.team,sReloadGeneration)
  && probe->Qgw8InspectionLayout() && PioneerQ38TokenStartupPlan::WorkerLayoutCompatible(*probe)
  && PioneerQ38TokenStartupPlan::CanonicalLearnedPagesPresent(*probe,c.initialized,PioneerResidentBanks::Pages));
 PioneerResidentMutableRange ranges[7]={};assert(PioneerQ38RuntimeRanges(*probe,ranges));
 const auto epoch=sQ38TokenModelEpoch;
 c.q38Qgw8Receipt=nullptr;assert(!f.Bind() && !c.q38TokenNative && sQ38TokenModelEpoch==epoch);
 c.q38Qgw8Receipt=&receipt;c.hashes=hashes.data()+1;
 assert(!f.Bind() && !c.q38TokenNative && sQ38TokenModelEpoch==epoch);c.hashes=hashes.data();
 sReloadGeneration++;assert(!f.Bind() && !c.q38TokenNative && sQ38TokenModelEpoch==epoch);sReloadGeneration--;
 c.q38HybridReceipt=new PioneerQ38HybridUploadReceipt;
 assert(!f.Bind() && !c.q38TokenNative && sQ38TokenModelEpoch==epoch);
 delete c.q38HybridReceipt;c.q38HybridReceipt=nullptr;
 assert(f.Bind() && f.Binding() && c.q38TokenNative && !c.q38TokenNative->Unsafe()
  && c.q38TokenNative->MatchesReservation(c.banks,f.Dram()));
 c.q38Qgw8Receipt=nullptr;assert(!f.Binding());c.q38Qgw8Receipt=&receipt;assert(f.Binding());
 f.Inert();
 puts("PASS QGW8 sealed receipt through actual fake-tag native bind and reservation; simulated only, NO DEVICE OR INFERENCE");
}
