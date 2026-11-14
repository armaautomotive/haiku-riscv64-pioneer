/* SPDX-License-Identifier: MIT */
// Synthetic receipt lifecycle only: never touches a card.
#define PIONEER_Q38_SMALL_RECURRENT_BFP8_PLAN_INSPECTION 1
#define PIONEER_Q38_SMALL_RECURRENT_RESIDENT_REBIND_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_KERNEL_LAYOUT_FIXTURE_ONLY 1
#define PIONEER_Q38_RECURRENT_Q8_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_QGW8_LAYOUT_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_KERNEL_LAYOUT_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_CANONICAL_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION 1
#define PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT 1
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define PIONEER_BANKS_MOCK_ONLY 1
#include "../PioneerQ38SmallRecurrentUploadReceiptInspection.h" // receipt must not require the host proposal header
#include "../PioneerQ38SmallRecurrentWorkspaceReceipt.h"
#include "../PioneerQ38DenseQ8MatrixManifest.h"
#include "../PioneerQ38RecurrentQ8MatrixWorkspace.h"
#include "pioneer_q38_dense_q8_kernel_layout_test.cpp"

int main(int argc,char** argv){
 assert(argc==1 || (argc==2 && !strcmp(argv[1],"--rebind")));
 BanksMock io;auto proof=BankProof(io);PioneerResidentBanks banks;assert(banks.Bind(proof));
 std::vector<uint8_t> ledger(PioneerResidentBanks::Pages,0);std::vector<uint32_t> hashes(PioneerResidentBanks::Pages,0);
 using Receipt=PioneerQ38SmallRecurrentUploadReceiptInspection;auto receipt=std::make_unique<Receipt>();
 int context=0,cookie=0,foreign=0;assert(receipt->Begin(&context,&cookie,hashes.data(),ledger.data(),17,42,banks,proof,ledger.size()));
 PioneerQ38SpanLease span;assert(!receipt->Seal(span));unsigned bank=0,length=0,spans=0,last=0;uint64_t offset=0,pages=0;
 while(receipt->Next(bank,offset,length)){
  const unsigned first=unsigned(uint64_t(bank)*(PioneerResidentBanks::Capacity/4096)+offset/4096),count=length/4096;
  assert(!spans || first>=last);last=first+count;
  assert(!receipt->Before(&foreign,hashes.data(),ledger.data(),&span,bank,offset,length));
  assert(!receipt->Before(&context,hashes.data(),ledger.data(),&span,bank,offset+4096,length));
  assert(receipt->Before(&context,hashes.data(),ledger.data(),&span,bank,offset,length));
  for(unsigned i=0;i<count;i++){
   const uint32_t hash=(first+i)*2654435761U;ledger[first+i]=1;hashes[first+i]=hash;
   assert(receipt->RecordPage(first+i,hash));
  }
  span.saved=span.ready=span.dirty=true;span.nonce=++spans;assert(receipt->After());pages+=count;
 }
 assert(spans==21475 && pages==5067109);span.closed=true;span.ready=span.dirty=false;
 assert(receipt->Seal(span) && receipt->Sealed() && receipt->Revalidate(&context,&cookie,hashes.data(),ledger.data(),17,42));
 assert(!receipt->Revalidate(&foreign,&cookie,hashes.data(),ledger.data(),17,42));
 assert(receipt->MatchesBanks(&banks,&proof) && receipt->MatchesLease(&span));
 assert(!receipt->MatchesBanks(nullptr,&proof) && !receipt->MatchesLease(nullptr));
 if(argc==2){
  int nextContext=0;auto nextProof=std::make_unique<tt_dram_samples_result>(proof);
  auto nextBanks=std::make_unique<PioneerResidentBanks>();assert(nextBanks->Bind(*nextProof));
  auto nextLedger=ledger;auto nextHashes=hashes;PioneerQ38SpanLease foreignSpan;
  unsigned hole=0;while(hole<ledger.size() && ledger[hole])hole++;assert(hole<ledger.size());
  const auto attempt=[&](const void* fd,uint64_t gen,const PioneerQ38SpanLease* retained){
   return receipt->RebindInspection(&context,fd,hashes.data(),ledger.data(),17,gen,retained,&nextContext,
    nextHashes.data(),nextLedger.data(),*nextBanks,*nextProof,nextLedger.size());
  };
  assert(!attempt(&foreign,42,&span) && !attempt(&cookie,43,&span) && !attempt(&cookie,42,&foreignSpan));
  assert(!receipt->RebindInspection(&context,&cookie,hashes.data(),ledger.data(),17,42,&span,&nextContext,
   reinterpret_cast<const uint32_t*>(nextLedger.data()),nextLedger.data(),*nextBanks,*nextProof,nextLedger.size()));
  nextLedger[hole]=1;assert(!attempt(&cookie,42,&span));nextLedger[hole]=0;
  nextHashes[0]^=1;assert(!attempt(&cookie,42,&span));nextHashes[0]^=1;
  nextProof->translated[0][0]^=1;assert(!attempt(&cookie,42,&span));nextProof->translated[0][0]^=1;
  assert(receipt->Matches(&context,&cookie,hashes.data(),ledger.data(),17,42) && !banks.Quarantined());
  assert(attempt(&cookie,42,&span) && !receipt->Matches(&context,&cookie,hashes.data(),ledger.data(),17,42)
   && receipt->Matches(&nextContext,&cookie,nextHashes.data(),nextLedger.data(),17,42)
   && receipt->MatchesLease(&span) && receipt->Revalidate(&nextContext,&cookie,nextHashes.data(),nextLedger.data(),17,42));
  std::vector<uint8_t>().swap(ledger);std::vector<uint32_t>().swap(hashes);
  assert(receipt->Revalidate(&nextContext,&cookie,nextHashes.data(),nextLedger.data(),17,42));
  int thirdContext=0;auto thirdProof=proof;PioneerResidentBanks thirdBanks;assert(thirdBanks.Bind(thirdProof));
  auto thirdLedger=nextLedger;auto thirdHashes=nextHashes;
  assert(receipt->RebindInspection(&nextContext,&cookie,nextHashes.data(),nextLedger.data(),17,42,&span,&thirdContext,
   thirdHashes.data(),thirdLedger.data(),thirdBanks,thirdProof,thirdLedger.size()));
  std::vector<uint8_t>().swap(nextLedger);std::vector<uint32_t>().swap(nextHashes);
  nextBanks.reset();nextProof.reset();
  assert(receipt->Matches(&thirdContext,&cookie,thirdHashes.data(),thirdLedger.data(),17,42)
   && receipt->MatchesLease(&span) && receipt->Revalidate(&thirdContext,&cookie,thirdHashes.data(),thirdLedger.data(),17,42));
  auto logical=std::make_unique<PioneerQ38CanonicalModelPlan>();
  assert(receipt->DeriveLogical(*logical,&thirdContext,&cookie,thirdHashes.data(),thirdLedger.data(),17,42)
   && receipt->MatchesLogical(*logical,&thirdContext,&cookie,thirdHashes.data(),thirdLedger.data(),17,42));
  std::array<uint32_t,Receipt::RecurrentWords> workspace={};
  assert(receipt->RecurrentWorkspace(workspace.data(),workspace.size(),&thirdContext,&cookie,
   thirdHashes.data(),thirdLedger.data(),17,42));
  puts("PASS two offline small-recurrent receipt rebinds; NO device reservation or lifecycle authority");return 0;
 }
 auto logical=std::make_unique<PioneerQ38CanonicalModelPlan>();
 assert(receipt->DeriveLogical(*logical,&context,&cookie,hashes.data(),ledger.data(),17,42));
 assert(logical->Qgw8InspectionLayout() && receipt->MatchesLogical(*logical,&context,&cookie,hashes.data(),ledger.data(),17,42));
 assert(!receipt->MatchesLogical(*logical,&foreign,&cookie,hashes.data(),ledger.data(),17,42));
 using Workspace=PioneerQ38SmallRecurrentWorkspace;using Bridge=PioneerQ38SmallRecurrentWorkspaceReceipt;
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();Workspace::Plan::Proposal proposal;
 assert(canonical->Build() && Workspace::Plan::Build(*canonical,proposal));
 using DM=PioneerQ38DenseQ8MatrixManifest;DM::Plan::Proposal densePlan;
 assert(DM::Plan::Build(*canonical,densePlan));
 std::array<uint32_t,DM::Words> denseExpected{},denseActual{};denseActual.fill(0xa5a5a5a5);
 const auto denseUntouched=denseActual;assert(DM::Build(*canonical,densePlan,denseExpected.data(),denseExpected.size()));
 assert(!receipt->DenseManifest(denseActual.data(),denseActual.size(),&context,&foreign,hashes.data(),ledger.data(),17,42) && denseActual==denseUntouched);
 assert(!receipt->DenseManifest(hashes.data(),DM::Words,&context,&cookie,hashes.data(),ledger.data(),17,42));
 assert(receipt->DenseManifest(denseActual.data(),denseActual.size(),&context,&cookie,hashes.data(),ledger.data(),17,42) && denseActual==denseExpected);
 using RW=PioneerQ38RecurrentQ8MatrixWorkspace;RW::M::Plan::Proposal recurrentPlan;
 assert(RW::M::Plan::Build(*canonical,recurrentPlan));
 std::array<uint32_t,RW::Words> recurrentExpected{},recurrentActual{};recurrentActual.fill(0xa5a5a5a5);
 const auto recurrentUntouched=recurrentActual;assert(RW::Build(*canonical,recurrentPlan,recurrentExpected.data(),recurrentExpected.size()));
 assert(!receipt->RecurrentWorkspace(recurrentActual.data(),recurrentActual.size(),&context,&foreign,hashes.data(),ledger.data(),17,42) && recurrentActual==recurrentUntouched);
 assert(!receipt->RecurrentWorkspace(hashes.data(),RW::Words,&context,&cookie,hashes.data(),ledger.data(),17,42));
 assert(receipt->RecurrentWorkspace(recurrentActual.data(),recurrentActual.size(),&context,&cookie,hashes.data(),ledger.data(),17,42) && recurrentActual==recurrentExpected);
 std::array<uint32_t,Workspace::Words> actual{},expected{};actual.fill(0xa5a5a5a5);
 assert(Workspace::Build(*canonical,proposal,expected.data(),expected.size()));
 assert(!receipt->BuildWorkspace(&foreign,&cookie,hashes.data(),ledger.data(),17,42,actual.data(),actual.size()));
 for(auto value:actual)assert(value==0xa5a5a5a5);
 assert(!receipt->BuildWorkspace(&context,&cookie,hashes.data(),ledger.data(),17,43,actual.data(),actual.size()));
 assert(!receipt->BuildWorkspace(&context,&cookie,hashes.data(),ledger.data(),17,42,actual.data(),actual.size()-1));
 assert(!receipt->BuildWorkspace(&context,&cookie,hashes.data(),ledger.data(),17,42,hashes.data(),actual.size()));
 assert(receipt->BuildWorkspace(&context,&cookie,hashes.data(),ledger.data(),17,42,actual.data(),actual.size()) && actual==expected);
 actual.fill(0xa5a5a5a5);
 assert(!Bridge::Build(*receipt,*canonical,proposal,&foreign,&cookie,hashes.data(),ledger.data(),17,42,actual.data(),actual.size()));
 for(auto value:actual)assert(value==0xa5a5a5a5);
 assert(!Bridge::Build(*receipt,*canonical,proposal,&context,&cookie,hashes.data(),ledger.data(),17,42,
  reinterpret_cast<uint32_t*>(proposal.parts.data()),Workspace::Words));
 assert(Bridge::Build(*receipt,*canonical,proposal,&context,&cookie,hashes.data(),ledger.data(),17,42,actual.data(),actual.size()) && actual==expected);
 const auto preserved=actual;proposal.parts[0].offset+=4096;
 assert(!Bridge::Build(*receipt,*canonical,proposal,&context,&cookie,hashes.data(),ledger.data(),17,42,actual.data(),actual.size()) && actual==preserved);
 proposal.parts[0].offset-=4096;
 unsigned hole=0;while(hole<ledger.size() && ledger[hole])hole++;assert(hole<ledger.size());
 ledger[hole]=1;
 // Matches is a constant-time owner identity check, not a ledger audit.
 // The explicit lifecycle Revalidate must detect the damaged ledger.
 assert(receipt->Matches(&context,&cookie,hashes.data(),ledger.data(),17,42));
 assert(!receipt->Revalidate(&context,&cookie,hashes.data(),ledger.data(),17,42) && banks.Quarantined());
 assert(!receipt->Sealed());
 assert(!receipt->BuildWorkspace(&context,&cookie,hashes.data(),ledger.data(),17,42,actual.data(),actual.size()) && actual==preserved);
 assert(!Bridge::Build(*receipt,*canonical,proposal,&context,&cookie,hashes.data(),ledger.data(),17,42,actual.data(),actual.size()) && actual==preserved);
 BanksMock io2;auto proof2=BankProof(io2);PioneerResidentBanks banks2;assert(banks2.Bind(proof2));
 std::fill(ledger.begin(),ledger.end(),0);std::fill(hashes.begin(),hashes.end(),0);
 auto aborted=std::make_unique<Receipt>();assert(aborted->Begin(&context,&cookie,hashes.data(),ledger.data(),17,43,banks2,proof2,ledger.size()));
 PioneerQ38SpanLease second;assert(aborted->Next(bank,offset,length));
 assert(aborted->Before(&context,hashes.data(),ledger.data(),&second,bank,offset,length));aborted->Abort();
 assert(banks2.Quarantined() && !aborted->Next(bank,offset,length) && !aborted->Sealed());
 puts("PASS physical-order 5067109-page synthetic receipt; mutation and uncertain transfer quarantine; NO DEVICE/UPLOAD/SPEED");
}
