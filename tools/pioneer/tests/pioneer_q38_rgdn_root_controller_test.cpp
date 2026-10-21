/* SPDX-License-Identifier: MIT */
// Real controller+RV32/SFPU image;NoC/GDDR/physical mapping are simulated.
// Initial ALL-page ledger uses EXPLICIT COUNTER FABRIC,NOT zero-data/native
// initialization. Native zero qualification is separately recorded.
#define PIONEER_Q38_RGDN_ROOT_QUALIFICATION
#define PIONEER_Q38_RESIDENT_GDN_IMAGE_MODEL_ONLY
#include "pioneer_q38_resident_gdn_image_test.cpp"
#define PIONEER_Q38_STATE_ZERO_CONTROLLER_MODEL_ONLY
#include "pioneer_q38_state_zero_controller_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38InitializeOwnedState.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentGdnRootController.h"
#include "../PioneerQ38DeltaProbeDebug.h"
struct RgdnControlIO:ZeroControlIO {
 using ZeroControlIO::ZeroControlIO;
 const char* residentImage=nullptr;
 bool residentMode=false,rootOwner=true,lateCounterAfterExport=false;
 unsigned exportedWords=0,vmPackets=0,vmFaultPacket=0,vmSourceFault=0,vmBadCounter=0,vmCorruptWrite=UINT32_MAX;
 std::map<std::pair<unsigned,unsigned>,std::array<uint32_t,8192>> stateData;
 bool ResidentGdnBeforeRoot() { return residentMode && rootOwner && !activeCohort && !competingSpan; }
 bool ResidentGdnOtherCoresHeld() { Touch(); return othersHeld; } // EXPLICIT physical audit stand-in
 uint32_t Q38PanelRead(uint32_t at) {
  if (residentMode && rootStarts && coreReset==0x47800 && at>=0x98000 && at<0xa0000) {
   exportedWords++;
   if (exportedWords==8192 && lateCounterAfterExport) counters[0]++;
  }
  return ZeroControlIO::Q38PanelRead(at);
 }
 void ExecuteResident() {
  ResidentGdnImage m(residentImage);
  for (unsigned w=0;w<view.size();w++) {
   uint32_t at=TT_Q38_SPAN_VIEW_BASE+w*4; m.memory[at]=view[w];
   if (m.CodeAddress(at)) m.l1[m.Index(at)]=view[w];
  }
  m.l1[m.Index(0x6000)]=entry;
  for (unsigned w=0;w<64;w++) m.descriptor[w]=PanelIO::Q38PanelRead(TT_Q38_RGDN_DESCRIPTOR+w*4);
  auto read=std::make_pair(m.descriptor[4],m.descriptor[5]),write=std::make_pair(m.descriptor[6],m.descriptor[7]);
  assert(stateData.count(read) && stateData.count(write) && read!=write);
  m.prior=stateData.at(read); m.next=stateData.at(write);
  for (unsigned w=0;w<16;w++) m.seals[w]=w?source[w]:config;
  for (unsigned w=0;w<8;w++) m.counts[w]=PanelIO::ResidentNocRead(PioneerQ38SpanCounter(w));
  for (unsigned w=0;w<10;w++) m.command[w]=command[w];
  m.failPacket=vmFaultPacket; m.sourceFault=vmSourceFault; m.badCounter=vmBadCounter; m.corruptWriteAt=vmCorruptWrite;
  (void)m.RunResident(); vmPackets=m.packets; stateData.at(write)=m.next;
  for (unsigned w=0;w<view.size();w++) view[w]=m.memory.at(TT_Q38_SPAN_VIEW_BASE+w*4);
  for (unsigned w=0;w<3;w++) destination[2032+w]=m.memory.at(0x17ffc0+w*4);
  counters[0]=m.counts[0]; counters[1]=m.counts[1]; accepted=m.counts[2]; counters[3]=m.counts[3];
  writeCounters[0]=m.counts[4]; writeCounters[1]=m.counts[5]; writeCounters[3]=m.counts[7];
  for (unsigned w=0;w<10;w++) command[w]=m.command[w];
  if (summaryFault<32) view[(TT_Q38_RGDN_SUMMARY-TT_Q38_SPAN_VIEW_BASE)/4+summaryFault]^=1;
  if (descriptorFault<64) view[(TT_Q38_RGDN_DESCRIPTOR-TT_Q38_SPAN_VIEW_BASE)/4+descriptorFault]^=1;
  if (commandFault<10) command[commandFault]^=1;
  if (sourceFault<16) { if (sourceFault) source[sourceFault]^=1; else config^=1; }
  if (corruptCode) view[(0x72000-TT_Q38_SPAN_VIEW_BASE)/4]^=1;
  if (corruptCounter) counters[0]++;
  if (lateMapLoss) wideMapLost=true;
  if (badMarker) destination[2034]^=1;
  if (badPhase) destination[2032]^=1;
  if (badCount) destination[2033]^=1;
 }
 void RuntimeResetWrite(uint32_t value) {
  if (!residentMode) { ZeroControlIO::RuntimeResetWrite(value); return; }
  Touch(); resets.push_back(value); if (resets.size()==dropReset) return;
  if (value==0x87ffff) { if (coreReset==0x46800) rootStops++; coreReset=value; return; }
  if (value==0x47800) { rootHolds++; coreReset=value; return; }
  assert(value==0x46800 && coreReset==0x87ffff && othersHeld && !activeCohort && !competingSpan
   && pciCommand[0]==0x402 && pciCommand[1]==0x402 && !wideDirty);
  coreReset=value; rootStarts++; if (!noCompletion) ExecuteResident();
 }
};
struct RgdnFixture {
 RgdnControlIO io;
 PanelOwners owner;
 PioneerOutputBufferLease transfer; PioneerBulkUploadLease bulk; PioneerWideOutputLease wide;
 PioneerQ38StateZeroLease zero;
 PioneerQ38ResidentGdnRootLease lease;
 PioneerQ38GdnIdentity identity={1,2,3,4};
 std::unique_ptr<PioneerQ38GdnBanks> state=std::make_unique<PioneerQ38GdnBanks>();
 std::vector<uint8_t> initialized=std::vector<uint8_t>(PioneerResidentBanks::Pages,0);
 std::vector<PioneerQ38GdnRegion> regions;
 std::unique_ptr<tt_q38_rgdn_root_request> job=std::make_unique<tt_q38_rgdn_root_request>();
 std::array<uint32_t,8192> cpuPrior={};
 RgdnFixture(const char* zeroImage,const char* rootImage):io(zeroImage),owner(io) {
  assert(PioneerPrepareOutputBuffer(io,owner.host,transfer));
  assert(PioneerBulkUploadPrepare(io,owner.dram,owner.noc,owner.host,owner.store,transfer,bulk));
  assert(PioneerPrepareWideOutput(io,owner.host,transfer,wide));
  for (unsigned layer=0;layer<64;layer++) if (layer%4!=3) for (unsigned slot=0;slot<2;slot++) {
   unsigned index=unsigned(regions.size()); regions.push_back({layer,slot,0,48,index%7,0x04000000+(index/7)*48*65536});
  }
  assert(state->Bind(owner.banks,owner.dram,identity,initialized.data(),unsigned(initialized.size()),regions.data(),unsigned(regions.size())));
  io.publicationCounterFabricOnly=true; PioneerQ38InitializeOwnedResult r;
  assert(PioneerQ38InitializeOwnedState(io,owner.dram,owner.noc,*state,owner.banks,identity,owner.host,owner.store,bulk,transfer,wide,zero,r)
   && r.published && zero.closed); // EXPLICIT synthetic counter fabric,NOT data initialization
  for (unsigned slot=0;slot<2;slot++) {
   PioneerQ38GdnBankExtent e; assert(state->InitializeHalf(identity,0,slot,0,e)); io.stateData[{e.bank,e.address}]={};
  }
  io.residentMode=true; io.residentImage=rootImage; io.rootStarts=io.rootStops=io.rootHolds=0; io.pcSamples=0; io.resets.clear();
 }
 bool PrepareRoot() { return PioneerQ38RgdnRootPrepare(io,owner.dram,owner.noc,*state,owner.banks,identity,owner.host,owner.store,bulk,transfer,wide,lease); }
 void Input(unsigned trial=4) {
  tt_q38_delta_probe fixture={}; PioneerQ38DeltaDebugFixture(fixture,trial);
  *job={}; job->version=1; job->decay=fixture.decay; job->beta=fixture.beta;
  std::copy(std::begin(fixture.query),std::end(fixture.query),job->query);
  std::copy(std::begin(fixture.key),std::end(fixture.key),job->key);
  std::copy(std::begin(fixture.value),std::end(fixture.value),job->value);
 }
 bool RunRoot() { return PioneerQ38RgdnRootRun(io,owner.dram,owner.noc,*state,owner.banks,owner.host,owner.store,bulk,transfer,wide,lease,*job); }
 bool CloseRoot(bool clean=true) { return PioneerQ38RgdnRootClose(io,owner.dram,owner.noc,*state,owner.banks,owner.host,owner.store,bulk,transfer,wide,lease,clean); }
 bool Accept() { tt_q38_rgdn_root_accept a={1,lease.nonce,{0,0}}; return PioneerQ38RgdnRootAccept(lease,a); }
 void Verify(unsigned position) {
  auto reference=std::make_unique<tt_q38_delta_probe>(); reference->version=TT_Q38_DELTA_PROBE_VERSION;
  reference->decay=job->decay; reference->beta=job->beta;
  std::copy(cpuPrior.begin(),cpuPrior.end(),reference->prior);
  std::copy(std::begin(job->query),std::end(job->query),reference->query);
  std::copy(std::begin(job->key),std::end(job->key),reference->key);
  std::copy(std::begin(job->value),std::end(job->value),reference->value);
  std::copy(std::begin(job->next),std::end(job->next),reference->next);
  std::copy(std::begin(job->output),std::end(job->output),reference->output);
  assert(!PioneerQ38DeltaDebugErrors(*reference));
  assert(job->result.state==TT_RUNTIME_PASSED && job->result.started && job->result.completed && job->result.held
   && !job->result.recovery_required && job->result.awaiting && job->result.position==position && job->result.nonce==position+1
   && job->result.read_slot==position%2 && job->result.write_slot==(position%2^1));
  std::copy(std::begin(job->next),std::end(job->next),cpuPrior.begin());
 }
 void Unsafe() {
  assert(lease.unsafe && state->Unsafe() && owner.banks.Quarantined() && owner.store.recovery_required && bulk.unsafe
   && transfer.quarantined && wide.quarantined && io.HostReadMock::quarantined && io.wideQuarantined
   && !io.pciWrites && !io.wideReleased && !io.PageMock::releases && !io.OutputBufferMock::releases);
  unsigned touches=io.touches;
  assert(!RunRoot() && !CloseRoot() && !CloseRoot(false) && !PrepareRoot() && !Accept() && io.touches==touches);
 }
};
int main(int argc,char** argv) {
 assert(argc==3); setvbuf(stdout,nullptr,_IOLBF,0); (void)&TestWorker;
 (void)&Input; (void)&Prepare; (void)&Upload; (void)&Close; (void)&PreparePanel; (void)&Run; (void)&Job; (void)&ClosePanel; (void)&Finish;
 {
  RgdnFixture f(argv[1],argv[2]); assert(f.PrepareRoot());
  for (unsigned position=0;position<16;position++) {
   f.Input(4+position); assert(f.RunRoot()); f.Verify(position);
   unsigned touches=f.io.touches;
   assert(!f.RunRoot() && !f.CloseRoot() && f.io.touches==touches && f.lease.awaiting);
   for (unsigned bad=0;bad<5;bad++) {
    tt_q38_rgdn_root_accept a={1,f.lease.nonce,{0,0}};
    if (!bad) a.version=2; if (bad==1) a.nonce=0; if (bad==2) a.nonce++;
    if (bad==3) a.reserved[0]=1; if (bad==4) a.reserved[1]=1;
    assert(!PioneerQ38RgdnRootAccept(f.lease,a) && f.lease.awaiting && f.lease.position==position && f.io.touches==touches);
   }
   assert(f.Accept() && !f.Accept());
  }
  assert(f.CloseRoot() && f.lease.closed && !f.lease.unsafe && f.io.coreReset==0x47800);
  unsigned touches=f.io.touches; assert(f.CloseRoot() && !f.PrepareRoot() && !f.RunRoot() && f.io.touches==touches);
  for (auto bit:f.initialized) assert(!bit);
 }
 puts("PASS OFFLINE ACTUAL root GDN controller+RV32/SFPU:16 NONRESETTING GDDR-slot transitions,ALL8256 bits match independent FP32 debug reference,explicit ACCEPT-only private half slot flips,unaccepted Run/Close inert,known-clean Close;initial ALL-page zero ledger EXPLICIT counter fabric,NO native/model readiness");
 for (unsigned field=0;field<32;field++) {
  RgdnFixture f(argv[1],argv[2]); assert(f.PrepareRoot()); f.Input(); f.io.summaryFault=field;
  assert(!f.RunRoot() && !f.io.rootStops && !f.io.rootHolds); f.Unsafe();
 }
 for (unsigned packet=1;packet<=6;packet++) {
  RgdnFixture f(argv[1],argv[2]); assert(f.PrepareRoot()); f.Input(); f.io.vmFaultPacket=packet;
  assert(!f.RunRoot() && f.io.vmPackets==packet && !f.io.rootStops && !f.io.rootHolds); f.Unsafe();
 }
 for (unsigned fault=0;fault<11;fault++) {
  RgdnFixture f(argv[1],argv[2]); assert(f.PrepareRoot()); f.Input();
  if (!fault) f.io.corruptCode=true; if (fault==1) f.io.corruptCounter=true;
  if (fault==2) f.io.sourceFault=11; if (fault==3) f.io.descriptorFault=40;
  if (fault==4) f.io.commandFault=0; if (fault==5) f.io.badMarker=1; if (fault==6) f.io.badPhase=1; if (fault==7) f.io.badCount=1;
  if (fault==8) f.io.vmCorruptWrite=8191; if (fault==9) f.io.latePcFault=3; if (fault==10) f.io.lateCounterAfterExport=true;
  assert(!f.RunRoot()); if (fault!=10) assert(!f.io.rootStops && !f.io.rootHolds); f.Unsafe();
 }
 puts("PASS OFFLINE ACTUAL root GDN controller faults:32 summary fields,ALL6 packet timeouts,late code/counter/source/descriptor/command/marker/phase/count/writeback/PC/export-counter faults quarantine ALL owners,zero output authority,no later IO/hold/reset/cleanup after detection;NO native/PCI/BME/model performance claim");
 for (unsigned bad=0;bad<9;bad++) {
  RgdnFixture f(argv[1],argv[2]); assert(f.PrepareRoot()); f.Input();
  if (!bad) f.job->version=2; if (bad>=1 && bad<=3) f.job->reserved[bad-1]=1;
  if (bad==4) f.job->decay=0x80000000; if (bad==5) f.job->beta=1; if (bad==6) f.job->query[127]=0x7fc00000;
  if (bad==7) f.job->key[0]=1; if (bad==8) f.job->value[63]=0x7f800000;
  unsigned touches=f.io.touches;
  assert(!f.RunRoot() && f.io.touches==touches && !f.lease.unsafe && !f.io.rootStarts && f.CloseRoot());
 }
 for (unsigned bad=0;bad<9;bad++) {
  RgdnFixture f(argv[1],argv[2]);
  if (!bad) f.io.rootOwner=false; if (bad==1) f.io.capacity=false; if (bad==2) f.lease.identity.session=1;
  if (bad==3) f.lease.readSlot=1; if (bad==4) f.lease.nonce=1; if (bad==5) f.lease.position=1;
  if (bad==6) f.lease.owner=f.state.get(); if (bad==7) f.lease.proof=&f.owner.dram;
  if (bad==8) f.identity.driverGeneration++;
  unsigned touches=f.io.touches;
  assert(!f.PrepareRoot() && f.io.touches==touches && !f.lease.unsafe && !f.io.rootStarts);
 }
 for (unsigned bad=0;bad<4;bad++) {
  RgdnFixture f(argv[1],argv[2]); assert(f.PrepareRoot()); f.Input(); assert(f.RunRoot()); f.Verify(0); assert(f.Accept());
  if (!bad) f.io.view[(0x72000-TT_Q38_SPAN_VIEW_BASE)/4]^=1;
  if (bad==1) f.io.dropWrite=f.io.panelWrites+(0x72000-TT_Q38_SPAN_VIEW_BASE)/4+1;
  if (bad==2) f.io.dropEntry=f.io.entryWrites+1;
  assert(!f.CloseRoot(bad!=3)); f.Unsafe();
 }
 puts("PASS OFFLINE root GDN CPU gates:invalid DATA/pristine identity/role/capacity/ref-owner refusals are inert,five wrong ACCEPT fields never flip slots,code/restoration/entry/unclean CLOSE faults retain whole owner;NO native integration claim");
}
