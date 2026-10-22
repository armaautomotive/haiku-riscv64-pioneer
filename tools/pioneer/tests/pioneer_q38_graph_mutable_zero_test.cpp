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
 std::vector<uint8_t> learned=std::vector<uint8_t>(PioneerResidentBanks::Pages,0);
 PioneerQ38GdnBanks* gdn=nullptr;PioneerQ38FfnBanks* ffn=nullptr;
 explicit JointZeroFixture(const char* image):io(image),owner(io) {
  assert(PioneerPrepareOutputBuffer(io,owner.host,transfer)
   && PioneerBulkUploadPrepare(io,owner.dram,owner.noc,owner.host,owner.store,transfer,bulk)
   && PioneerPrepareWideOutput(io,owner.host,transfer,wide));
  std::vector<PioneerQ38GdnRegion> r;
  for(unsigned layer=0;layer<64;layer++)if(layer%4!=3)for(unsigned slot=0;slot<2;slot++)
   r.push_back({layer,slot,0,48,0,unsigned(r.size())*48*65536});
  assert(joint->Bind(owner.banks,owner.dram,identity,learned.data(),unsigned(learned.size()),r.data(),unsigned(r.size()),1,0));
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
  for(auto byte:learned)assert(!byte);
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
 printf("PASS OFFLINE joint mutable+ACTUAL zero instructions:cases%u/both serial role orders/restored code+panel+bulk+boot/separate partialGDN16 and completeFFN74 ledgers+seals/UNKNOWN in EITHER role retains ALL and prevents ANY later IO or pristine+alreadyClosed sibling cleanup grant;SIMULATED_IO_NOT_NATIVE_SCOPE_PRODUCER_OR_SPEED_AUTHORITY\n",cases);
}
