/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_MUTABLE_STATE
#define PIONEER_BANKS_MOCK_ONLY
#include "pioneer_resident_banks_test.cpp"
#define PIONEER_Q38_FIXTURE_ONLY
#include "pioneer_qwen38_gguf_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnBanks.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38SpanAuthority.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38CohortProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38StreamProtocol.h"
#include <memory>
#include <type_traits>
template<class T,class=void> struct PublicZeroCommit:std::false_type {};
template<class T> struct PublicZeroCommit<T,std::void_t<decltype(std::declval<T&>().CommitInitialization(
 std::declval<const PioneerQ38GdnIdentity&>(),0U,0U,1U))>>:std::true_type {};
static_assert(!PublicZeroCommit<PioneerQ38GdnBanks>::value,"Only the complete state-zero controller may commit bank-owner progress");
static const PioneerQ38GdnIdentity Identity={1,2,3,4};
struct GdnBankFixture {
 BanksMock io;
 tt_dram_samples_result proof=BankProof(io);
 PioneerResidentBanks banks;
 std::vector<uint8_t> initialized=std::vector<uint8_t>(PioneerResidentBanks::Pages,0);
 std::unique_ptr<PioneerQ38GdnBanks> state=std::make_unique<PioneerQ38GdnBanks>();
 GdnBankFixture() { assert(banks.Bind(proof)); }
 bool Bind(const std::vector<PioneerQ38GdnRegion>& r) {
  return state->Bind(banks,proof,Identity,initialized.data(),unsigned(initialized.size()),r.data(),unsigned(r.size()));
 }
 void Inert() const { assert(!io.Mock::writes && !io.requests); }
};
static std::vector<PioneerQ38GdnRegion> Regions(const PioneerQwen38PackedPlan& p) {
 std::vector<PioneerQ38GdnRegion> r;
 for (const auto& part:p.Parts()) if (part.kind==PIONEER_Q38_DELTA_STATE)
  r.push_back({part.layer,part.role,part.firstRow,part.rows,part.bank,part.proposedAddress-PioneerQwen38PackedPlan::Base});
 return r;
}
int main()
{
 (void)&Input; (void)&Prepare; (void)&Upload; (void)&Close;
 Fixture modelFile; PioneerQwen38Gguf model; assert(model.Open(modelFile.file));
 PioneerQwen38PackedPlan plan; assert(plan.Build(model,8192,true,true)); auto regions=Regions(plan);
 assert(regions.size()==672);
 {
  GdnBankFixture f;
  for (unsigned field=0;field<4;field++) {
   auto bad=Identity; if (!field) bad.session=0; if (field==1) bad.sequence=0; if (field==2) bad.modelEpoch=0; if (field==3) bad.driverGeneration=0;
   assert(!f.state->Bind(f.banks,f.proof,bad,f.initialized.data(),unsigned(f.initialized.size()),regions.data(),unsigned(regions.size())));
  }
  for (unsigned bank=0;bank<7;bank++) f.initialized[bank*786432]=1; // unrelated learned pages
  auto frozen=f.initialized; assert(f.Bind(regions) && !f.Bind(regions));
  unsigned initializedPages=0;
  for (unsigned region=0;region<regions.size();region++) {
   unsigned pages=0; assert(f.state->InitializationPages(Identity,region,pages) && pages==regions[region].heads*16);
   for (unsigned firstPage=0;firstPage<pages;firstPage+=256) {
    unsigned count=std::min(256U,pages-firstPage); PioneerQ38GdnBankExtent e={};
    assert(f.state->InitializeSpan(Identity,region,firstPage,count,e) && e.bank==regions[region].bank
     && e.address==PioneerResidentBanks::Base+regions[region].offset+firstPage*4096 && e.pages==count && e.bytes==count*4096);
    initializedPages+=count;
   }
   PioneerQ38GdnBankExtent out={99,99,99,99,99,99};
   assert(!f.state->InitializeSpan(Identity,region,pages,1,out) && !f.state->InitializeSpan(Identity,region,0,257,out)
    && !f.state->InitializeSpan(Identity,region,0,0,out) && out.address==99);
  }
  assert(initializedPages==73728);
  // Binary-search exclusion must agree with a separate naive interval
  // oracle, including unaligned byte ranges and exact touching edges.
  uint32_t random=0x13579bdf;
  for (unsigned trial=0;trial<10000;trial++) {
   random=random*1664525U+1013904223U; unsigned bank=random%7;
   random=random*1664525U+1013904223U; uint64_t offset=uint64_t(random%786432)*4096+(random&63);
   random=random*1664525U+1013904223U; unsigned bytes=1+random%1048576;
   bool expected=bytes<=PioneerResidentBanks::Capacity-offset;
   for (const auto& r:regions) if (r.bank==bank && offset<uint64_t(r.offset)+uint64_t(r.heads)*65536 && r.offset<offset+bytes) expected=false;
   assert(f.banks.ImmutableRange(f.proof,bank,offset,bytes)==expected);
  }
  for (unsigned layer=0;layer<64;layer++) if (layer%4!=3) for (unsigned slot=0;slot<2;slot++) for (unsigned worker=0;worker<96;worker++) {
   PioneerQ38GdnBankExtent e={}; PioneerQ38DeltaHalfProposal expected={};
   assert(f.state->InitializeHalf(Identity,layer,slot,worker,e) && plan.DeltaHalf(layer,slot,worker/2,worker%2,expected));
   assert(e.bank==expected.bank && e.address==expected.proposedAddress && e.route==PioneerQ38SpanRoute(e.bank)
    && e.pages==8 && e.bytes==32768 && e.index==e.bank*786432+(e.address-PioneerResidentBanks::Base)/4096);
   for (unsigned p=0;p<8;p++) {
    uint32_t address=99,route=99,index=99;
    assert(!f.banks.Page(f.proof,e.bank,e.address-PioneerResidentBanks::Base+p*4096,address,route,index)
     && address==99 && route==99 && index==99 && !f.initialized[e.index+p]);
   }
   assert(!f.banks.Range(f.proof,e.bank,e.address-PioneerResidentBanks::Base,4));
   PioneerQ38SpanExtent span; assert(!PioneerQ38SpanAuthorize(f.banks,f.proof,f.initialized.data(),unsigned(f.initialized.size()),
    e.bank,e.address-PioneerResidentBanks::Base,32768,span));
  }
  assert(f.initialized==frozen); // never tag uninitialized/reserved state as learned data
  for (unsigned bank=0;bank<7;bank++) {
   uint32_t a=0,r=0,p=0; assert(f.banks.Page(f.proof,bank,0,a,r,p) && f.banks.Range(f.proof,bank,0,4));
   assert(!f.banks.Range(f.proof,bank,0,5)); // legacy limit unchanged
  }
  // Even a maliciously all-one initialized bitmap cannot grant a learned
  // matrix access to a range whose FIRST page is outside mutable state.
  auto reserved=regions[0]; assert(reserved.offset>=4096);
  unsigned first=reserved.bank*786432+reserved.offset/4096-1;
  for (unsigned p=0;p<32;p++) f.initialized[first+p]=1;
  tt_q38_cohort_part part={}; part.bank=reserved.bank; part.offset=reserved.offset-4096; part.groups=1;
  PioneerQ38CohortExtent cohort;
  assert(!PioneerQ38CohortAuthorizePart(f.banks,f.proof,f.initialized.data(),unsigned(f.initialized.size()),8,5120,part,1,cohort));
  tt_q38_stream_probe stream={}; stream.version=TT_Q38_STREAM_ABI; stream.type=8; stream.inputs=5120;
  stream.bank=reserved.bank; stream.offset=reserved.offset-4096;
  uint32_t zero=PioneerInt8Operand(0); zero|=zero<<16;
  for (unsigned w=0;w<stream.inputs/2;w++) stream.activation[w]=zero;
  PioneerQ38StreamExtent streamExtent;
  assert(PioneerQ38StreamValid(stream) && !PioneerQ38StreamAuthorize(f.banks,f.proof,f.initialized.data(),unsigned(f.initialized.size()),stream,streamExtent));
  auto foreign=Identity; foreign.driverGeneration++;
  PioneerQ38GdnBankExtent out={99,99,99,99,99,99};
  assert(!f.state->InitializeHalf(foreign,0,0,0,out) && out.address==99);
  for (unsigned field=0;field<3;field++) {
   assert(!f.state->InitializeHalf(Identity,field==0?64:0,field==1?2:0,field==2?96:0,out));
  }
  auto copy=f.proof; uint32_t a=99,r=99,p=99;
  assert(!f.banks.MutablePage(copy,f.state.get(),reserved.bank,reserved.offset,a,r,p));
  assert(!f.banks.MutablePage(f.proof,&plan,reserved.bank,reserved.offset,a,r,p));
  f.proof.source[1]^=1; assert(!f.state->InitializeHalf(Identity,0,0,0,out)); f.proof.source[1]^=1;
  f.state->Fault(); assert(f.state->Unsafe() && f.banks.Quarantined() && !f.state->InitializeHalf(Identity,0,0,0,out));
  for (unsigned bank=0;bank<7;bank++) assert(!f.banks.Page(f.proof,bank,0,a,r,p));
  f.Inert();
 }
 // Bad placements cannot partially publish a reservation.
 for (unsigned fault=0;fault<15;fault++) {
  GdnBankFixture f; auto bad=regions;
  if (fault==0) bad.pop_back();
  if (fault==1) bad[0].layer=3;
  if (fault==2) bad[0].slot=2;
  if (fault==3) bad[0].firstHead=48;
  if (fault==4) bad[0].heads=0;
  if (fault==5) bad[0].heads=49;
  if (fault==6) bad[0].bank=7;
  if (fault==7) bad[0].offset++;
  if (fault==8) bad[0].offset=UINT32_MAX-65535;
  if (fault==9) bad[0].offset=uint32_t(PioneerResidentBanks::Capacity-65536);
  if (fault==10) bad[1]=bad[0];
  if (fault==11) { bad[7].bank=bad[0].bank; bad[7].offset=bad[0].offset; }
  if (fault>=12) {
   const auto& r=bad[fault==12?0:fault==13?bad.size()/2:bad.size()-1];
   f.initialized[r.bank*786432+r.offset/4096+(fault==14?r.heads*16-1:0)]=1;
  }
  assert(!f.Bind(bad));
  // Valid bind succeeding proves no claim leaked on the failed attempt.
  std::fill(f.initialized.begin(),f.initialized.end(),0); assert(f.Bind(regions)); f.Inert();
 }
 // Raw interlock also checks null/empty/overflow/overlap/all pages, and
 // only the exact reservation owner can derive its private pages.
 {
  GdnBankFixture f; PioneerResidentMutableRange r={0,16,16};
  assert(!f.banks.ReserveMutable(f.proof,nullptr,&r,1,f.initialized.data(),unsigned(f.initialized.size())));
  assert(!f.banks.ReserveMutable(f.proof,&f,&r,0,f.initialized.data(),unsigned(f.initialized.size())));
  assert(!f.banks.ReserveMutable(f.proof,&f,&r,1,nullptr,unsigned(f.initialized.size())));
  assert(!f.banks.ReserveMutable(f.proof,&f,&r,1,f.initialized.data(),unsigned(f.initialized.size()-1)));
  PioneerResidentMutableRange pair[]={{0,16,16},{0,31,2}};
  assert(!f.banks.ReserveMutable(f.proof,&f,pair,2,f.initialized.data(),unsigned(f.initialized.size())));
  assert(f.banks.ReserveMutable(f.proof,&f,&r,1,f.initialized.data(),unsigned(f.initialized.size())));
  uint32_t a=99,b=99,c=99;
  assert(f.banks.MutablePage(f.proof,&f,0,16*4096,a,b,c) && a==0x04010000 && b==786 && c==16);
  assert(!f.banks.MutablePage(f.proof,&f,0,15*4096,a,b,c) && !f.banks.MutablePage(f.proof,&f,0,32*4096,a,b,c));
  assert(!f.banks.Range(f.proof,0,15*4096,2) && !f.banks.Range(f.proof,0,31*4096,2));
  assert(f.banks.ImmutableRange(f.proof,0,16*4096-1,1) && !f.banks.ImmutableRange(f.proof,0,16*4096-1,2));
  assert(!f.banks.ImmutableRange(f.proof,0,32*4096-1,1) && f.banks.ImmutableRange(f.proof,0,32*4096,1));
  f.Inert();
 }
 puts("PASS OFFLINE kernel GDN bank owner:ALL9216 half-head extents/288MiB,same bank/routes/source/epoch owner,fresh-page atomic reservation,15 placement/initialized faults inert,weight upload/read/whole learned spans exclude state,legacy<=4 unchanged,sticky all-bank quarantine;NO card I/O/initialization/driver capability");
}
