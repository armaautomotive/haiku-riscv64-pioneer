/* SPDX-License-Identifier: MIT */
// Actual native source validator and startup serializer;synthetic learned
// bytes/bank endpoints only. No driver,card execution or speed measurement.
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define PIONEER_BANKS_MOCK_ONLY
#define Input PreparedBanksInput
#include "pioneer_resident_banks_test.cpp"
#undef Input
#define PIONEER_Q38_PREPARED_UPLOAD_FIXTURE_ONLY
#include "pioneer_q38_prepared_q4_upload_plan_test.cpp"
#include "../PioneerQ38TokenStartupPlan.h"
#include <sys/wait.h>
#include <unistd.h>
template<class F>static void Refusal(F check){
 fflush(nullptr);const auto pid=fork();assert(pid>=0);
 if(!pid){check();_exit(0);}
 int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));
}
int main(){
 (void)&PreparedBanksInput;(void)&Prepare;(void)&Upload;(void)&Close;
 setvbuf(stdout,nullptr,_IOLBF,0);
 Fixture fixture;PioneerQwen38Gguf model;PioneerQwen38PackedPlan original;
 assert(model.Open(fixture.file) && original.Build(model,8192,true,true));
 Weights weights(original);Pages pages(weights);Plan upload;std::string error;
 assert(upload.Build(weights,pages,error));
 PioneerQ38PreparedQ4RootTables<Plan> tables;assert(tables.Build(upload,model,error));
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 BanksMock io;auto proof=BankProof(io);PioneerResidentBanks banks;assert(banks.Bind(proof));
 std::vector<uint8_t> ledger(PioneerResidentBanks::Pages);std::array<uint32_t,1344> sizes{};unsigned sides=0;
 for(const auto& span:upload.Spans()){
  const unsigned first=span.bank*786432+(span.address-original.Base)/4096;
  std::fill(ledger.begin()+first,ledger.begin()+first+span.allocated/4096,1);
  if(span.kind==Plan::Correction)sizes[sides++]=span.bytes;
 }
 assert(sides==1344);
 unsigned context=1,cookie=2,hashes=3;
 auto source=std::make_unique<PioneerQ38PreparedNativeSource>();
 auto begin=[&](PioneerQ38PreparedNativeSource& s){return s.Begin(&context,&cookie,&hashes,ledger.data(),7,9,banks,proof,
  sizes.data(),sizes.size(),ledger.size());};
 assert(!source->Ready() && !source->Seal());
 const auto& side=*std::find_if(upload.Spans().begin(),upload.Spans().end(),[](const auto& s){return s.kind==Plan::Correction;});
 const unsigned page=side.bank*786432+(side.address-original.Base)/4096;
 for(uint8_t bad:{uint8_t(0),uint8_t(2),uint8_t(255)}){
  ledger[page]=bad;auto refused=std::make_unique<PioneerQ38PreparedNativeSource>();assert(!begin(*refused) && !refused->Ready());
 }
 ledger[page]=1;assert(begin(*source) && !begin(*source) && !source->Ready());
 using R=PioneerQ38PreparedQ4RootLayout;std::array<uint32_t,R::Bytes/4> table{};unsigned refusals=3;
 assert(tables.Read(0,2,0,reinterpret_cast<uint8_t*>(table.data()),R::Bytes));
 for(unsigned field=0;field<37;field++){
  Refusal([&]{auto invalid=table;const unsigned at=field<16?field:field<32?16+field-16:
   field<36?R::LedgerOffset/4+field-32:R::Bytes/4-1;
   invalid[at]^=0x80000001U;assert(!source->Accept(0,invalid.data(),invalid.size()) && !source->Ready() && !source->Seal());});refusals++;
 }
 assert(!source->Accept(1,table.data(),table.size()) && !source->Accept(0,table.data(),table.size()-1));refusals+=2;
 for(unsigned i=0;i<192;i++){
  assert(tables.Read(i/3,i%3+2,0,reinterpret_cast<uint8_t*>(table.data()),R::Bytes));
  assert(source->Accept(i,table.data(),table.size()) && source->AcceptedPhases()==i+1 && !source->Ready());
 }
 assert(source->Seal() && source->Ready() && !source->Seal() && source->Matches(&context,&cookie,&hashes,ledger.data(),7,9)
  && !source->Matches(&cookie,&cookie,&hashes,ledger.data(),7,9) && !source->Matches(&context,&hashes,&hashes,ledger.data(),7,9)
  && !source->Matches(&context,&cookie,&hashes,ledger.data()+1,7,9) && !source->Matches(&context,&cookie,&hashes,ledger.data(),8,9)
  && !source->Matches(&context,&cookie,&hashes,ledger.data(),7,10) && source->MatchesPlan(*canonical));
 auto startup=std::make_unique<PioneerQ38TokenStartupPlan>();
 std::array<uint32_t,1920> boots{};for(unsigned i=0;i<boots.size();i++)boots[i]=0x76540001+i;
 const PioneerQ38GdnIdentity id={1,2,3,4};const uint32_t prompt[]={17,31};
 assert(startup->BindPreparedSource(*source) && !startup->BindPreparedSource(*source)
  && startup->Build(*canonical,0,id,boots.data(),prompt,2,128,true,248318,248317));
 std::array<uint8_t,16384> actual{},expected{},published{};unsigned chunks=0;
 for(unsigned layer=0;layer<64;layer++){
  auto originalPackage=std::make_unique<PioneerQ38TokenPackage>();
  assert(originalPackage->Build(*canonical,layer,0,id,boots.data(),0,startup->Workspace()+0x600000));
  for(unsigned at=0;at<0x80000;at+=actual.size()){
   assert(startup->ReadPublishedPackage(layer,at,actual.data(),actual.size())
    && originalPackage->Read(at,expected.data(),expected.size()));
   for(unsigned role=2;role<=4;role++){
    const unsigned first=R::PhaseOffset(role),end=first+R::Bytes;
    assert(tables.Read(layer,role,0,reinterpret_cast<uint8_t*>(table.data()),R::Bytes));
    for(unsigned i=0;i<actual.size();i++)if(at+i>=first && at+i<end)
     expected[i]=uint8_t(table[(at+i-first)/4]>>(((at+i-first)&3)*8));
   }
   assert(actual==expected);chunks++;
   if(at==0x70000)assert(startup->ReadPackage(layer,at,published.data(),published.size()) && published==actual);
  }
 }
 actual.fill(0x5a);const auto untouched=actual;
 assert(!source->Overlay(64,0,actual.data(),64) && actual==untouched
  && !source->Overlay(0,1,actual.data(),64) && actual==untouched
  && !source->Overlay(0,0,reinterpret_cast<uint8_t*>(source.get()),64));
 source->Abandon();assert(!source->Ready() && !startup->ReadPublishedPackage(0,0,actual.data(),64) && actual==untouched);
 assert(!io.Mock::writes && !io.requests);
 printf("PASS_PREPARED_NATIVE_SOURCE all1344_retained_side_spans all192_phases_22848_canonical_workers startup_chunks%u refusal_cases%u "
  "all_other_package_bytes_unchanged no_bank_packets source_loss_revokes_startup;SYNTHETIC_NO_CARD_OR_TPS\n",chunks,refusals);
}
