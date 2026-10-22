/* SPDX-License-Identifier: MIT */
// Execute the EXACT frozen RV32 zero image in the established offline NoC
// machine,not a synthetic marker/counter fabric. No native adapter/card.
#define PIONEER_Q38_STATE_ZERO_CONTROLLER_MODEL_ONLY
#include "pioneer_q38_state_zero_controller_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38FfnZeroController.h"
#include <type_traits>
static_assert(!std::is_copy_constructible<PioneerQ38FfnZeroLease>::value,"exact private lease");
struct FfnZeroControlIO:ZeroControlIO {
 using ZeroControlIO::ZeroControlIO;
 PioneerQ38FfnZeroLease* ffnLease=nullptr;
 void RuntimeResetWrite(uint32_t value) {
  bool change=authorityChanged;authorityChanged=false;unsigned starts=rootStarts;
  ZeroControlIO::RuntimeResetWrite(value);authorityChanged=change;
  if(change && rootStarts!=starts){assert(ffnLease);ffnLease->identity.modelEpoch++;}
 }
};
struct FfnZeroFixture {
 FfnZeroControlIO io;PanelOwners owner;
 PioneerOutputBufferLease transfer;PioneerBulkUploadLease bulk;PioneerWideOutputLease wide;
 PioneerQ38FfnZeroLease lease;PioneerQ38FfnZeroResult result;PioneerQ38FfnIdentity identity={1,2,3,4};
 PioneerQ38FfnBanks state;
 std::vector<uint8_t> learned=std::vector<uint8_t>(PioneerResidentBanks::Pages,0);
 unsigned preparedWideLoads=0;
 explicit FfnZeroFixture(const char* image,unsigned bank=0,uint64_t offset=0x04000000):io(image),owner(io) {
  assert(PioneerPrepareOutputBuffer(io,owner.host,transfer));
  assert(PioneerBulkUploadPrepare(io,owner.dram,owner.noc,owner.host,owner.store,transfer,bulk));
  assert(PioneerPrepareWideOutput(io,owner.host,transfer,wide));preparedWideLoads=io.wideLoads;
  assert(state.Bind(owner.banks,owner.dram,identity,learned.data(),unsigned(learned.size()),bank,offset));io.ffnLease=&lease;
 }
 bool PrepareZero(){return PioneerQ38FfnZeroPrepare(io,owner.dram,owner.noc,state,owner.banks,identity,owner.host,owner.store,bulk,transfer,wide,lease);}
 bool Zero(unsigned region=0,unsigned first=0,unsigned pages=1){return PioneerQ38FfnZeroRun(io,owner.dram,owner.noc,state,owner.banks,
  owner.host,owner.store,bulk,transfer,wide,lease,identity,region,first,pages,result);}
 bool CloseZero(bool clean=true){return PioneerQ38FfnZeroClose(io,owner.dram,owner.noc,state,owner.banks,
  owner.host,owner.store,bulk,transfer,wide,lease,clean);}
 void Unsafe() {
  assert(lease.unsafe && state.Unsafe() && owner.banks.Quarantined() && owner.store.recovery_required && bulk.unsafe
   && transfer.quarantined && wide.quarantined && io.HostReadMock::quarantined && io.wideQuarantined
   && !io.wideReleased && !io.PageMock::releases && !io.OutputBufferMock::releases && !io.pciWrites);
  unsigned progress=UINT32_MAX;assert(!state.InitializationProgress(identity,progress) && progress==UINT32_MAX && !state.InitializationPublished(identity));
  unsigned before=io.touches;
  assert(!Zero() && !PrepareZero() && !CloseZero() && !CloseZero(false));
  assert(!PioneerReleaseWideOutput(io,wide,false) && !PioneerReleaseOutputBuffer(io,transfer,false));
  assert(before==io.touches && !io.wideReleased && !io.PageMock::releases && !io.OutputBufferMock::releases);
  assert(state.OwnsInitializationLease(&lease)); // retain the exact operation on UNKNOWN
 }
 void Finish() {
  assert(CloseZero() && lease.closed && !lease.unsafe && io.coreReset==0x47800 && !wide.pending
   && !bulk.dirty && !io.HostReadMock::quarantined && !io.pciWrites && io.wideLoads==preparedWideLoads);
  for(unsigned w=0;w<io.view.size();w++)assert(io.view[w]==0xab860000+w);
  for(unsigned w=0;w<8192;w++)assert(io.l1[w]==0x91730000+w);
  assert(io.entry==0x1250006f);
  unsigned before=io.touches;assert(CloseZero() && !PrepareZero() && !Zero() && before==io.touches);
  for(auto b:learned)assert(!b);
  assert(PioneerReleaseWideOutput(io,wide,true) && PioneerReleaseOutputBuffer(io,transfer,true));
  assert(PioneerResidentLeaseEnd(io,owner.noc,owner.store) && !io.ReleaseHostPage());io.Safe();
 }
};
int main(int argc,char** argv) {
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 const auto code=Read((std::string(argv[1])+"/text.bin").c_str());
 for(unsigned w=0;w<2048;w++)assert(PioneerQ38StateZeroCodeWord(w)==(w*4<code.size()?Word(code,w*4):0));
 unsigned positive=0,faults=0,refusals=0;
 for(unsigned bank=0;bank<7;bank++)for(uint64_t offset:{UINT64_C(0),PioneerResidentBanks::Capacity-UINT64_C(67108864)}) {
  FfnZeroFixture f(argv[1],bank,offset);assert(f.PrepareZero() && f.Zero(0,0,74));positive++;
  unsigned progress=99;assert(f.state.InitializationProgress(f.identity,progress) && progress==74 && !f.state.InitializationPublished(f.identity));
  assert(f.result.pages==74 && f.result.bytes==303104 && f.result.started && f.result.completed && f.result.held && !f.result.recovery_required);
  assert(f.io.lastDeviceWords.size()==303104/4);for(auto w:f.io.lastDeviceWords)assert(!w);
  assert(f.io.statusOrder==std::vector<unsigned>({2,0,1}));
  assert(f.CloseZero() && f.state.InitializationPublished(f.identity));
  PioneerQ38FfnZeroLease fresh;unsigned before=f.io.touches;
  assert(!PioneerQ38FfnZeroPrepare(f.io,f.owner.dram,f.owner.noc,f.state,f.owner.banks,f.identity,
   f.owner.host,f.owner.store,f.bulk,f.transfer,f.wide,fresh) && before==f.io.touches);
  f.Finish();
 }
 {
  FfnZeroFixture f(argv[1]);assert(f.PrepareZero() && f.Zero());
  for(unsigned i=0;i<2;i++){unsigned before=f.io.touches;assert(!f.Zero(0,0,i?2:1) && !f.lease.unsafe && before==f.io.touches);refusals++;}
  assert(f.CloseZero() && !f.state.InitializationPublished(f.identity));
  // A new lease may finish a known-clean PARTIAL initialization. This is
  // not a retry of an UNKNOWN operation:that state is irreversibly unsafe.
  PioneerQ38FfnZeroLease next;f.io.ffnLease=&next;
  assert(PioneerQ38FfnZeroPrepare(f.io,f.owner.dram,f.owner.noc,f.state,f.owner.banks,f.identity,f.owner.host,
   f.owner.store,f.bulk,f.transfer,f.wide,next));
  assert(PioneerQ38FfnZeroRun(f.io,f.owner.dram,f.owner.noc,f.state,f.owner.banks,f.owner.host,f.owner.store,
   f.bulk,f.transfer,f.wide,next,f.identity,0,1,73,f.result));
  assert(!f.state.InitializationPublished(f.identity));
  assert(PioneerQ38FfnZeroClose(f.io,f.owner.dram,f.owner.noc,f.state,f.owner.banks,f.owner.host,f.owner.store,
   f.bulk,f.transfer,f.wide,next,true) && f.state.InitializationPublished(f.identity));positive++;f.Finish();
 }
 for(unsigned w=0;w<32;w++){FfnZeroFixture f(argv[1]);assert(f.PrepareZero());f.io.summaryFault=w;assert(!f.Zero() && !f.io.rootStops && !f.io.rootHolds);f.Unsafe();faults++;}
 for(unsigned w=0;w<64;w++){FfnZeroFixture f(argv[1]);assert(f.PrepareZero());f.io.descriptorFault=w;assert(!f.Zero() && !f.io.rootStops && !f.io.rootHolds);f.Unsafe();faults++;}
 for(unsigned w=0;w<10;w++){FfnZeroFixture f(argv[1]);assert(f.PrepareZero());f.io.commandFault=w;assert(!f.Zero() && !f.io.rootStops && !f.io.rootHolds);f.Unsafe();faults++;}
 for(unsigned w=0;w<16;w++){FfnZeroFixture f(argv[1]);assert(f.PrepareZero());f.io.sourceFault=w;assert(!f.Zero() && !f.io.rootStops && !f.io.rootHolds);f.Unsafe();faults++;}
 for(unsigned packet=1;packet<=4;packet++){FfnZeroFixture f(argv[1]);assert(f.PrepareZero());f.io.packetFault=packet;
  assert(!f.Zero(0,0,8) && !f.io.rootStops && !f.io.rootHolds && f.io.statusOrder==std::vector<unsigned>{2});f.Unsafe();faults++;}
 // The 74page workspace has a short final packet:exercise EVERY one of
 // its19writes+19readbacks,not only a small aligned prefix.
 for(unsigned packet=1;packet<=38;packet++){FfnZeroFixture f(argv[1]);assert(f.PrepareZero());f.io.packetFault=packet;
  assert(!f.Zero(0,0,74) && !f.io.rootStops && !f.io.rootHolds && f.io.statusOrder==std::vector<unsigned>{2});f.Unsafe();faults++;}
 for(unsigned fault=0;fault<3;fault++) {
  FfnZeroFixture f(argv[1]);assert(f.PrepareZero());if(!fault)f.io.badMarker=1;if(fault==1)f.io.badPhase=1;if(fault==2)f.io.badCount=1;
  std::vector<unsigned> expected={2,0,1};expected.resize(fault+1);assert(!f.Zero() && f.io.statusOrder==expected);f.Unsafe();faults++;
 }
 for(unsigned packet=0;packet<2;packet++)for(bool corrupt:{false,true}) {
  FfnZeroFixture f(argv[1]);assert(f.PrepareZero());if(corrupt)f.io.deviceCorruptPacket=packet;else f.io.deviceDropPacket=packet;
  assert(!f.Zero() && !f.io.rootStops && !f.io.rootHolds);f.Unsafe();faults++;
 }
 for(unsigned fault=0;fault<20;fault++) {
  FfnZeroFixture f(argv[1]);assert(f.PrepareZero());
  if(!fault)f.io.noCompletion=true;if(fault==1)f.io.corruptCode=true;if(fault==2)f.io.corruptCounter=true;
  if(fault==3)f.io.authorityChanged=true;if(fault==4)f.io.stageLost=true;if(fault==5)f.io.wideMapLost=true;
  if(fault==6)f.io.bulkMapLost=true;if(fault==7)f.io.coreMapLost=true;if(fault==8)f.io.othersHeld=false;
  if(fault>=9 && fault<=12)f.io.latePcFault=fault-8;if(fault==13)f.io.lateMapLoss=true;if(fault==14)f.io.lateCohort=true;
  if(fault==15)f.io.lateCoreMapLoss=true;if(fault==16)f.io.lateCompetingSpan=true;if(fault==17)f.io.lateOtherHeld=true;
  if(fault==18)f.io.activeCohort=true;if(fault==19)f.io.competingSpan=true;
  assert(!f.Zero() && !f.io.rootStops && !f.io.rootHolds);f.Unsafe();faults++;
 }
 for(unsigned drop=1;drop<=4;drop++){FfnZeroFixture f(argv[1]);assert(f.PrepareZero());f.io.dropReset=drop;
  assert(!f.Zero() && f.io.resets.size()==drop);f.Unsafe();faults++;}
 for(unsigned fault=0;fault<3;fault++){FfnZeroFixture f(argv[1]);assert(f.PrepareZero());f.io.clock=100000;
  if(!fault)f.io.clock=UINT64_MAX-10;if(fault==1)f.io.backwardsClock=true;if(fault==2){f.io.noCompletion=true;f.io.frozenClock=true;}
  assert(!f.Zero() && !f.io.rootStops && !f.io.rootHolds);f.Unsafe();faults++;}
 for(unsigned fault=0;fault<7;fault++){FfnZeroFixture f(argv[1]);assert(f.PrepareZero());unsigned region=0,first=0,pages=1;
  if(!fault)region=1;if(fault==1)first=74;if(fault==2)pages=0;if(fault==3)pages=75;if(fault==4){first=73;pages=2;}
  if(fault==5)f.identity.modelEpoch++;if(fault==6)f.lease.nonce=0x7ffffffe;
  unsigned before=f.io.touches;assert(!f.Zero(region,first,pages) && !f.lease.unsafe && before==f.io.touches);refusals++;
 }
 // The lease is bound to object identity,not just equal source contents.
 for(unsigned fault=0;fault<3;fault++){FfnZeroFixture f(argv[1]);assert(f.PrepareZero());auto copy=f.owner.dram;
  PioneerResidentBanks foreignBanks;PioneerQ38FfnBanks foreignState;
  auto& proof=fault==0?copy:f.owner.dram;auto& banks=fault==1?foreignBanks:f.owner.banks;auto& state=fault==2?foreignState:f.state;
  unsigned before=f.io.touches;
  assert(!PioneerQ38FfnZeroRun(f.io,proof,f.owner.noc,state,banks,f.owner.host,f.owner.store,f.bulk,f.transfer,
   f.wide,f.lease,f.identity,0,0,1,f.result));
  assert(!PioneerQ38FfnZeroClose(f.io,proof,f.owner.noc,state,banks,f.owner.host,f.owner.store,f.bulk,f.transfer,f.wide,f.lease,false));
  assert(!PioneerQ38FfnZeroClose(f.io,proof,f.owner.noc,state,banks,f.owner.host,f.owner.store,f.bulk,f.transfer,f.wide,f.lease,true));
  assert(before==f.io.touches && !f.lease.unsafe && !f.state.Unsafe());refusals+=3;f.Finish();
 }
 {
  FfnZeroFixture f(argv[1]);assert(f.PrepareZero());PioneerQ38FfnZeroLease competing;
  unsigned before=f.io.touches;
  assert(!PioneerQ38FfnZeroPrepare(f.io,f.owner.dram,f.owner.noc,f.state,f.owner.banks,f.identity,
   f.owner.host,f.owner.store,f.bulk,f.transfer,f.wide,competing));
  // Matching pointers/epochs alone do not own the in-flight saved image.
  competing.owner=&f.state;competing.banks=&f.owner.banks;competing.proof=&f.owner.dram;
  competing.identity=f.identity;competing.ready=true;
  assert(!PioneerQ38FfnZeroRun(f.io,f.owner.dram,f.owner.noc,f.state,f.owner.banks,f.owner.host,f.owner.store,
   f.bulk,f.transfer,f.wide,competing,f.identity,0,0,1,f.result));
  assert(!PioneerQ38FfnZeroClose(f.io,f.owner.dram,f.owner.noc,f.state,f.owner.banks,f.owner.host,f.owner.store,
   f.bulk,f.transfer,f.wide,competing,false));
  assert(before==f.io.touches && !f.lease.unsafe && f.state.OwnsInitializationLease(&f.lease));refusals+=3;f.Finish();
 }
 for(unsigned fault=0;fault<5;fault++) {
  FfnZeroFixture f(argv[1]);if(!fault)f.lease.nonce=1;if(fault==1)f.lease.identity.session=1;
  if(fault==2)f.lease.owner=&f.state;if(fault==3)f.lease.banks=&f.owner.banks;if(fault==4)f.lease.proof=&f.owner.dram;
  unsigned before=f.io.touches;assert(!f.PrepareZero() && !f.lease.unsafe && before==f.io.touches);refusals++;
 }
 for(unsigned fault=0;fault<7;fault++){FfnZeroFixture f(argv[1]);assert(f.PrepareZero() && f.Zero(0,0,74));
  assert(!f.state.InitializationPublished(f.identity));
  if(!fault)f.io.view[(0x72000-TT_Q38_SPAN_VIEW_BASE)/4]^=1;
  if(fault==1)f.io.dropWrite=f.io.panelWrites+(0x72000-TT_Q38_SPAN_VIEW_BASE)/4+1;
  if(fault==2){f.io.l1[0]^=1;f.io.dropBulk=f.io.bulkWrites+1;}
  if(fault==3)f.io.dropEntry=f.io.entryWrites+1;if(fault==4)f.io.othersHeld=false;if(fault==5)f.io.source[11]^=1;
  assert(!f.CloseZero(fault!=6));f.Unsafe();faults++;
 }
 for(unsigned fault=0;fault<3;fault++){FfnZeroFixture f(argv[1]);if(!fault)f.io.dropWrite=1;if(fault==1)f.io.dropEntry=1;if(fault==2)f.io.othersHeld=false;
  assert(!f.PrepareZero() && !f.io.rootStarts);f.Unsafe();faults++;}
 {FfnZeroFixture f(argv[1]);assert(f.PrepareZero());f.io.dropBulk=f.io.bulkWrites+1;assert(!f.Zero() && !f.io.rootStarts);f.Unsafe();faults++;}
 {FfnZeroFixture f(argv[1]);f.io.capacity=false;unsigned before=f.io.touches;
  assert(!f.PrepareZero() && !f.lease.unsafe && before==f.io.touches && !f.CloseZero(false));
  assert(f.CloseZero() && f.lease.closed && !f.PrepareZero() && before==f.io.touches);refusals+=2;}
 {FfnZeroFixture f(argv[1]);assert(f.PrepareZero());f.lease.running=true;unsigned before=f.io.touches;
  assert(!f.CloseZero() && before==f.io.touches);f.Unsafe();faults++;}
 // Mutation of a bound source is uncertainty,not an inert foreign request.
 {FfnZeroFixture f(argv[1]);assert(f.PrepareZero());f.owner.dram.source[1]^=1;
  unsigned before=f.io.touches;assert(!f.Zero() && before==f.io.touches);f.Unsafe();faults++;}
 printf("PASS OFFLINE FFN zero controller+EXACT RV32 image:positive%u/faults%u/inert-refusals%u;74page303104B fullfootprint/all7banks/2edges;fullledgerNOTready,sealONLYafterknowncleanCLOSE;ALLdata-summary-descriptor-command-source-counter-PCproofsBEFOREhold;UNKNOWNretainsALLowners/ZEROlaterIO;NO_NATIVE_INTEGRATION\n",positive,faults,refusals);
}
