/* SPDX-License-Identifier: MIT */
// Real existing controllers+frozen RV32 zero instructions,simulated IO
// ONLY. Serial role transitions,NOT a native scope/launch/producer grant.
#define PIONEER_Q38_FFN_ZERO_CONTROLLER_MODEL_ONLY
#include "pioneer_q38_ffn_zero_controller_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38GraphMutableBanks.h"
struct JointZeroFixture {
 FfnZeroControlIO io;PanelOwners owner;PioneerOutputBufferLease transfer;PioneerBulkUploadLease bulk;PioneerWideOutputLease wide;
 PioneerQ38FfnZeroLease fl;PioneerQ38FfnZeroResult fr;PioneerQ38StateZeroLease gl;PioneerQ38StateZeroResult gr;
 const PioneerQ38GdnIdentity identity={1,2,3,4};
 std::unique_ptr<PioneerQ38GraphMutableBanks> joint=std::make_unique<PioneerQ38GraphMutableBanks>();
 std::unique_ptr<PioneerQ38CanonicalModelPlan> canonical;
 std::vector<uint8_t> learned=std::vector<uint8_t>(PioneerResidentBanks::Pages,0);
 std::vector<uint8_t> savedLearned;
 PioneerQ38GdnBanks* gdn=nullptr;PioneerQ38FfnBanks* ffn=nullptr;
 explicit JointZeroFixture(const char* image,bool fixedModel=false,unsigned ffnBank=0):io(image),owner(io) {
  assert(PioneerPrepareOutputBuffer(io,owner.host,transfer)
   && PioneerBulkUploadPrepare(io,owner.dram,owner.noc,owner.host,owner.store,transfer,bulk)
   && PioneerPrepareWideOutput(io,owner.host,transfer,wide));
  if(fixedModel) {
   canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
   for(unsigned i=0;i<canonical->Count();i++){const auto& p=*canonical->Part(i);if(p.kind!=PioneerQ38CanonicalModelPlan::WeightKind)continue;
    unsigned at=p.bank*786432+p.offset/4096;std::fill(learned.begin()+at,learned.begin()+at+p.allocated/4096,1);}
   assert(joint->BindCanonical(owner.banks,owner.dram,identity,learned.data(),unsigned(learned.size()),*canonical,ffnBank));
  }else {
   std::vector<PioneerQ38GdnRegion> r;
   for(unsigned layer=0;layer<64;layer++)if(layer%4!=3)for(unsigned slot=0;slot<2;slot++)
    r.push_back({layer,slot,0,48,0,unsigned(r.size())*48*65536});
   assert(joint->Bind(owner.banks,owner.dram,identity,learned.data(),unsigned(learned.size()),r.data(),unsigned(r.size()),1,0));
  }
  savedLearned=learned;
  gdn=joint->Recurrent(identity);ffn=joint->Ffn(identity);assert(gdn && ffn);io.ffnLease=&fl;io.boundLease=&gl;
 }
 bool PrepareFfn(){return PioneerQ38FfnZeroPrepare(io,owner.dram,owner.noc,*ffn,owner.banks,identity,owner.host,owner.store,bulk,transfer,wide,fl);}
 bool PrepareGdn(){return PioneerQ38StateZeroPrepare(io,owner.dram,owner.noc,*gdn,owner.banks,identity,owner.host,owner.store,bulk,transfer,wide,gl);}
 bool RunFfn(){return PioneerQ38FfnZeroRun(io,owner.dram,owner.noc,*ffn,owner.banks,owner.host,owner.store,bulk,transfer,wide,fl,identity,0,0,74,fr);}
 bool RunGdn(){return PioneerQ38StateZeroRun(io,owner.dram,owner.noc,*gdn,owner.banks,owner.host,owner.store,bulk,transfer,wide,gl,identity,0,0,16,gr);}
 bool CloseFfn(){return PioneerQ38FfnZeroClose(io,owner.dram,owner.noc,*ffn,owner.banks,owner.host,owner.store,bulk,transfer,wide,fl,true);}
 bool CloseGdn(){return PioneerQ38StateZeroClose(io,owner.dram,owner.noc,*gdn,owner.banks,owner.host,owner.store,bulk,transfer,wide,gl,true);}
 void Ffn(){assert(PrepareFfn() && RunFfn() && !ffn->InitializationPublished(identity) && CloseFfn() && ffn->InitializationPublished(identity));}
 void Gdn(){assert(PrepareGdn() && RunGdn() && CloseGdn() && !gdn->InitializationPublished(identity));}
 void Restored() {
  assert(io.coreReset==0x47800 && !bulk.dirty && !wide.pending && !io.pciWrites && !joint->Unsafe());
  for(unsigned i=0;i<io.view.size();i++)assert(io.view[i]==0xab860000+i);
  for(unsigned i=0;i<8192;i++)assert(io.l1[i]==0x91730000+i);
  assert(io.entry==0x1250006f);
  assert(learned==savedLearned);
 }
};
int main(int argc,char** argv) {
 assert(argc==2);(void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 unsigned cases=0;
 for(bool ffnFirst:{false,true}) {
  JointZeroFixture f(argv[1]);if(ffnFirst){f.Ffn();f.Restored();f.Gdn();}else{f.Gdn();f.Restored();f.Ffn();}f.Restored();
  unsigned gp=0,fp=0;assert(f.gdn->InitializationProgress(f.identity,gp) && gp==16
   && f.ffn->InitializationProgress(f.identity,fp) && fp==74 && !f.gdn->InitializationPublished(f.identity)
   && f.ffn->InitializationPublished(f.identity)); // FFN seal NEVER publishes GDN
  assert(PioneerReleaseWideOutput(f.io,f.wide,true) && PioneerReleaseOutputBuffer(f.io,f.transfer,true)
   && PioneerResidentLeaseEnd(f.io,f.owner.noc,f.owner.store) && !f.io.ReleaseHostPage());f.io.Safe();cases++;
 }
 for(bool faultFfn:{false,true})for(bool siblingClosed:{false,true}) {
  JointZeroFixture f(argv[1]);if(siblingClosed){if(faultFfn)f.Gdn();else f.Ffn();f.Restored();}
  assert(faultFfn?f.PrepareFfn():f.PrepareGdn());f.io.packetFault=1;
  unsigned stops=f.io.rootStops,holds=f.io.rootHolds;
  assert(!(faultFfn?f.RunFfn():f.RunGdn()) && f.joint->Unsafe() && f.gdn->Unsafe() && f.ffn->Unsafe()
   && f.owner.banks.Quarantined() && f.owner.store.recovery_required && f.bulk.unsafe
   && f.transfer.quarantined && f.wide.quarantined && f.io.rootStops==stops && f.io.rootHolds==holds);
  unsigned before=f.io.touches;assert(!f.PrepareFfn() && !f.PrepareGdn() && !f.RunFfn() && !f.RunGdn()
   && !f.CloseFfn() && !f.CloseGdn() && f.io.touches==before);
  assert(!f.io.wideReleased && !f.io.PageMock::releases && !f.io.OutputBufferMock::releases && !f.io.pciWrites);cases++;
 }
 unsigned canonicalCases=0,canonicalParts=0,canonicalRefusals=0;
 for(unsigned bank=0;bank<7;bank++) {
  JointZeroFixture f(argv[1],true,bank);auto jobs=std::make_unique<PioneerQ38FfnCanonicalJobs>();assert(jobs->Build(*f.canonical,0,123));
  auto preflight=[&](){return f.joint->MatchInitializedFfn(f.identity,*jobs,0,123,f.learned.data(),unsigned(f.learned.size()));};
  assert(!preflight() && f.PrepareFfn() && f.RunFfn() && !preflight());
  // The actual frozen zero image completing is NOT enough:publication
  // waits for known-clean CLOSE and complete restoration of its owner.
  assert(f.CloseFfn() && preflight());f.Restored();canonicalCases++;
  unsigned touches=f.io.touches;
  for(unsigned layer=0;layer<64;layer++) {
   auto all=std::make_unique<PioneerQ38FfnCanonicalJobs>();assert(all->Build(*f.canonical,layer,UINT64_MAX));
   assert(f.joint->MatchInitializedFfn(f.identity,*all,layer,UINT64_MAX,f.learned.data(),unsigned(f.learned.size())));
   for(unsigned role=0;role<3;role++){const auto* j=all->Job(role);assert(j);
    for(unsigned w=0;w<119;w++){PioneerQ38CohortExtent e;
     assert(PioneerQ38CohortAuthorizePart(f.owner.banks,f.owner.dram,f.learned.data(),unsigned(f.learned.size()),12,
      role==2?17408:5120,j->part[w],w+1,e) && e.address==PioneerResidentBanks::Base+j->part[w].offset);canonicalParts++;}}
   assert(!f.joint->MatchInitializedFfn(f.identity,*all,layer^1,UINT64_MAX,f.learned.data(),unsigned(f.learned.size()))
    && !f.joint->MatchInitializedFfn(f.identity,*all,layer,UINT64_MAX-1,f.learned.data(),unsigned(f.learned.size())));canonicalRefusals+=2;
  }
  auto other=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(other->Build());
  auto foreign=std::make_unique<PioneerQ38FfnCanonicalJobs>();assert(foreign->Build(*other,0,123));
  assert(!f.joint->MatchInitializedFfn(f.identity,*foreign,0,123,f.learned.data(),unsigned(f.learned.size())));canonicalRefusals++;
  const auto& p=*f.canonical->FfnStripe(0,0,0);unsigned page=p.bank*786432+p.offset/4096;
  f.learned[page]=0;assert(!preflight());f.learned[page]=1;assert(preflight());canonicalRefusals++;
  auto changed=f.identity;changed.modelEpoch++;assert(!f.joint->MatchInitializedFfn(changed,*jobs,0,123,f.learned.data(),unsigned(f.learned.size())));canonicalRefusals++;
  const auto copied=f.learned;
  assert(!f.joint->MatchInitializedFfn(f.identity,*jobs,0,123,copied.data(),unsigned(copied.size()))
   && !f.joint->MatchInitializedFfn(f.identity,*jobs,0,123,f.learned.data(),unsigned(f.learned.size())-1));canonicalRefusals+=2;
  assert(f.io.touches==touches && f.learned==f.savedLearned && !f.gdn->InitializationPublished(f.identity));
  f.gdn->Fault();assert(f.joint->Unsafe() && !preflight() && f.io.touches==touches);canonicalRefusals++;
 }
 printf("PASS OFFLINE joint mutable+ACTUAL zero instructions:cases%u/both serial role orders/restored code+panel+bulk+boot/separate partialGDN16 and completeFFN74 ledgers+seals/UNKNOWN in EITHER role retains ALL and prevents ANY later IO or pristine+alreadyClosed sibling cleanup grant;SIMULATED_IO_NOT_NATIVE_SCOPE_PRODUCER_OR_SPEED_AUTHORITY\n",cases);
 printf("PASS OFFLINE actual zero-to-canonical FFN preflight:%u banks/%u actual learned-range authorizations/%u inert refusals;ALL64 layers x3 roles x119 workers,NO publication before known-clean zero CLOSE,foreign-but-identical plan/layer/token/epoch/missing learned page/sibling UNKNOWN refuse,NO new IO/learned writes/GDN publication;STILL_NOT_INPUT_PRODUCER_SOURCE_AUTH_OR_NATIVE_EXECUTION\n",canonicalCases,canonicalParts,canonicalRefusals);
}
