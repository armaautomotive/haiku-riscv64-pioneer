/* SPDX-License-Identifier: MIT */
// ACTUAL frozen RV32 zero machine+new512-page owner/controller. Transport/
// endpoints simulated;NO synthetic completion/status/counter fabric allowed.
#define PIONEER_Q38_STATE_ZERO_CONTROLLER_MODEL_ONLY
#include "pioneer_q38_state_zero_controller_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionMutableBanks.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionZeroController.h"
#include <type_traits>
static_assert(!std::is_copy_constructible<PioneerQ38ProjectionZeroLease>::value,"exact operation lease");
struct ProjectionZeroIO:ZeroControlIO {
 using ZeroControlIO::ZeroControlIO;
 PioneerQ38ProjectionZeroLease* projectionLease=nullptr;
 uint32_t workspaceBase=0,workspaceBank=7;
 std::vector<uint32_t> wholeWorkspace=std::vector<uint32_t>(PioneerQ38ProjectionBanks::Bytes/4,0x13579bdf);
 std::vector<unsigned> touchedPages=std::vector<unsigned>(PioneerQ38ProjectionBanks::Pages,0);
 bool ProjectionZeroBeforeCohort(){return StateZeroBeforeCohort();}
 bool ProjectionZeroOtherCoresHeld(){return StateZeroOtherCoresHeld();}
 void RuntimeResetWrite(uint32_t value){
  bool changed=authorityChanged;authorityChanged=false;unsigned starts=rootStarts;
  bool executes=value==0x46800 && !noCompletion;
  unsigned first=executes?(l1[5]-workspaceBase)/4:0,words=executes?l1[4]/4:0;
  if(executes)assert(!publicationCounterFabricOnly && workspaceBank==l1[7] && l1[5]>=workspaceBase && l1[4]<=TT_Q38_ZERO_BYTES
   && first<=wholeWorkspace.size() && words<=wholeWorkspace.size()-first);
  ZeroControlIO::RuntimeResetWrite(value);authorityChanged=changed;
  if(executes && starts!=rootStarts){
   assert(lastDeviceWords.size()==words);
   // Base reset executes the ACTUAL image. Observe its resulting bytes
   // AFTER execution,not by hiding the base's non-virtual ExecuteZero.
   for(unsigned i=0;i<words;i++)wholeWorkspace[first+i]=lastDeviceWords[i];
   for(unsigned p=first/1024;p<(first+words)/1024;p++)touchedPages[p]++;
  }
  if(changed && starts!=rootStarts){assert(projectionLease);projectionLease->identity.modelEpoch++;}
 }
};
struct ProjectionZeroFixture {
 ProjectionZeroIO io;PanelOwners owner;
 PioneerOutputBufferLease transfer;PioneerBulkUploadLease bulk;PioneerWideOutputLease wide;
 PioneerQ38ProjectionZeroLease lease;PioneerQ38ProjectionZeroResult result;
 PioneerQ38ProjectionIdentity id={1,2,3,4};
 std::vector<uint8_t> learned=std::vector<uint8_t>(PioneerResidentBanks::Pages);
 std::unique_ptr<PioneerQ38CanonicalModelPlan> plan=std::make_unique<PioneerQ38CanonicalModelPlan>();
 std::unique_ptr<PioneerQ38ProjectionMutableBanks> graph=std::make_unique<PioneerQ38ProjectionMutableBanks>();
 PioneerQ38ProjectionBanks* state=nullptr;unsigned preparedWideLoads=0;
 ProjectionZeroFixture(const char* image,unsigned bank=0):io(image),owner(io){
  assert(plan->Build() && graph->BindCanonical(owner.banks,owner.dram,id,learned.data(),unsigned(learned.size()),*plan,bank));
  state=graph->Projection(id);assert(state);PioneerQ38ProjectionExtent e;
  assert(state->InitializeSpan(id,0,0,512,e));io.workspaceBase=e.address;io.workspaceBank=e.bank;
  assert(PioneerPrepareOutputBuffer(io,owner.host,transfer) && PioneerBulkUploadPrepare(io,owner.dram,owner.noc,owner.host,owner.store,transfer,bulk)
   && PioneerPrepareWideOutput(io,owner.host,transfer,wide));preparedWideLoads=io.wideLoads;io.projectionLease=&lease;
 }
 bool PrepareZero(){return PioneerQ38ProjectionZeroPrepare(io,owner.dram,owner.noc,*state,owner.banks,id,owner.host,owner.store,bulk,transfer,wide,lease);}
 bool Zero(unsigned first=0,unsigned pages=1){return PioneerQ38ProjectionZeroRun(io,owner.dram,owner.noc,*state,owner.banks,owner.host,
  owner.store,bulk,transfer,wide,lease,id,0,first,pages,result);}
 bool CloseZero(bool knownClean=true){return PioneerQ38ProjectionZeroClose(io,owner.dram,owner.noc,*state,owner.banks,owner.host,
  owner.store,bulk,transfer,wide,lease,knownClean);}
 void Unknown(){
  assert(lease.unsafe && state->Unsafe() && graph->Unsafe() && owner.banks.Quarantined() && owner.store.recovery_required && bulk.unsafe
   && transfer.quarantined && wide.quarantined && io.HostReadMock::quarantined && io.wideQuarantined && !io.pciWrites
   && !io.wideReleased && !io.PageMock::releases && !io.OutputBufferMock::releases);
  unsigned pages=UINT32_MAX;assert(!state->InitializationProgress(id,pages) && pages==UINT32_MAX && !state->InitializationPublished(id)
   && !graph->Projection(id) && !graph->Recurrent(id) && state->OwnsInitializationLease(&lease));
  unsigned before=io.touches;assert(!Zero() && !PrepareZero() && !CloseZero() && !CloseZero(false)
   && !PioneerReleaseWideOutput(io,wide,false) && !PioneerReleaseOutputBuffer(io,transfer,false) && io.touches==before);
 }
 void Finish(){
  assert(CloseZero() && lease.closed && !lease.unsafe && io.coreReset==0x47800 && !bulk.dirty && !wide.pending && !io.pciWrites
   && io.wideLoads==preparedWideLoads && !io.HostReadMock::quarantined);
  for(unsigned i=0;i<io.view.size();i++)assert(io.view[i]==0xab860000+i);
  for(unsigned i=0;i<8192;i++)assert(io.l1[i]==0x91730000+i);assert(io.entry==0x1250006f);
  unsigned before=io.touches;assert(CloseZero() && !PrepareZero() && !Zero() && before==io.touches);
  for(auto v:learned)assert(!v);
 }
};
#ifndef PIONEER_Q38_PROJECTION_ZERO_FIXTURE_ONLY
int main(int argc,char** argv){
 assert(argc==2 || (argc==3 && !strcmp(argv[2],"--faults")));setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 unsigned positives=0,faults=0,refusals=0;
 if(argc==2){
  for(unsigned bank=0;bank<7;bank++){
   ProjectionZeroFixture f(argv[1],bank);assert(f.PrepareZero());unsigned before=f.io.touches;
   PioneerQ38ProjectionZeroLease forged;forged.owner=f.state;forged.banks=&f.owner.banks;forged.proof=&f.owner.dram;
   forged.identity=f.id;forged.saved=forged.ready=true;
   assert(!PioneerQ38ProjectionZeroRun(f.io,f.owner.dram,f.owner.noc,*f.state,f.owner.banks,f.owner.host,f.owner.store,f.bulk,f.transfer,f.wide,
    forged,f.id,0,0,1,f.result) && !PioneerQ38ProjectionZeroClose(f.io,f.owner.dram,f.owner.noc,*f.state,f.owner.banks,f.owner.host,f.owner.store,
    f.bulk,f.transfer,f.wide,forged,true) && f.state->OwnsInitializationLease(&f.lease) && f.io.touches==before);refusals+=2;
   // The frozen ROM max is1MiB. A2MiB request is inert,NOT a runtime fault.
   assert(!f.Zero(0,512) && !f.Zero(0,257) && !f.Zero(511,2) && !f.Zero(0,0) && f.io.touches==before && !f.lease.unsafe);refusals+=4;
   assert(f.Zero(0,256) && f.result.pages==256 && f.result.bytes==1048576 && f.lease.nonce==1);
   unsigned pages=0;assert(f.state->InitializationProgress(f.id,pages) && pages==256 && !f.state->InitializationPublished(f.id));
   before=f.io.touches;assert(!f.Zero(0,1) && !f.Zero(255,2) && f.io.touches==before);refusals+=2;
   for(unsigned i=0;i<f.io.wholeWorkspace.size();i++)assert(f.io.wholeWorkspace[i]==(i<262144?0U:0x13579bdfU));
   assert(f.Zero(256,256) && f.lease.nonce==2 && f.io.rootStarts==2 && f.io.rootHolds==2 && f.io.rootStops==2);
   assert(f.state->InitializationProgress(f.id,pages) && pages==512 && !f.state->InitializationPublished(f.id));
   for(auto value:f.io.wholeWorkspace)assert(!value);for(auto count:f.io.touchedPages)assert(count==1);
   unsigned recurrent=UINT32_MAX;assert(f.graph->Recurrent(f.id)->InitializationProgress(f.id,recurrent) && !recurrent
    && !f.graph->Recurrent(f.id)->InitializationPublished(f.id));
   assert(f.CloseZero() && f.state->InitializationPublished(f.id));
   auto jobs=std::make_unique<PioneerQ38ProjectionJobs>();assert(jobs->Build(*f.plan,0,10,0));
   assert(!f.graph->MatchInitializedProjection(f.id,*jobs,0,10,0,f.learned.data(),unsigned(f.learned.size()))); // zero NEVER marks learned data/input/model ready
   f.Finish();positives++;
  }
  {ProjectionZeroFixture f(argv[1]);assert(f.PrepareZero() && f.Zero(0,1) && f.CloseZero() && !f.state->InitializationPublished(f.id));
   PioneerQ38ProjectionZeroLease next;f.io.projectionLease=&next;PioneerQ38ProjectionZeroResult r;
   assert(PioneerQ38ProjectionZeroPrepare(f.io,f.owner.dram,f.owner.noc,*f.state,f.owner.banks,f.id,f.owner.host,f.owner.store,f.bulk,f.transfer,f.wide,next));
   assert(PioneerQ38ProjectionZeroRun(f.io,f.owner.dram,f.owner.noc,*f.state,f.owner.banks,f.owner.host,f.owner.store,f.bulk,f.transfer,f.wide,next,f.id,0,1,255,r)
    && PioneerQ38ProjectionZeroRun(f.io,f.owner.dram,f.owner.noc,*f.state,f.owner.banks,f.owner.host,f.owner.store,f.bulk,f.transfer,f.wide,next,f.id,0,256,256,r));
   assert(!f.state->InitializationPublished(f.id) && PioneerQ38ProjectionZeroClose(f.io,f.owner.dram,f.owner.noc,*f.state,f.owner.banks,f.owner.host,f.owner.store,
    f.bulk,f.transfer,f.wide,next,true) && f.state->InitializationPublished(f.id));positives++;
  }
 }else{
  for(unsigned word=0;word<32;word++){ProjectionZeroFixture f(argv[1]);assert(f.PrepareZero());f.io.summaryFault=word;assert(!f.Zero() && !f.io.rootStops && !f.io.rootHolds);f.Unknown();faults++;}
  for(unsigned word:{0U,4U,5U,6U,7U,8U,16U,31U,32U,63U}){ProjectionZeroFixture f(argv[1]);assert(f.PrepareZero());f.io.descriptorFault=word;assert(!f.Zero() && !f.io.rootStops && !f.io.rootHolds);f.Unknown();faults++;}
  for(unsigned fault=0;fault<12;fault++){
   ProjectionZeroFixture f(argv[1]);assert(f.PrepareZero() && f.Zero(0,256));
   if(!fault)f.io.packetFault=1;if(fault==1)f.io.packetFault=64;if(fault==2)f.io.packetFault=128;
   // Data injection is a packet DIRECTION (0write/1read),not an ordinal.
   if(fault==3)f.io.deviceDropPacket=0;if(fault==4)f.io.deviceCorruptPacket=1;if(fault==5)f.io.sourceFault=15;
   if(fault==6)f.io.authorityChanged=true;if(fault==7)f.io.lateOtherHeld=true;if(fault==8)f.io.lateCompetingSpan=true;
   if(fault==9)f.io.lateCoreMapLoss=true;if(fault==10)f.io.summaryFault=31;if(fault==11)f.io.commandFault=9;
   bool accepted=f.Zero(256,256);
   if(accepted || f.io.rootStarts!=2 || f.io.rootStops!=1 || f.io.rootHolds!=1)fprintf(stderr,"PROJECTION_ZERO_FAULT case%u accepted%u starts%u stops%u holds%u\n",fault,unsigned(accepted),f.io.rootStarts,f.io.rootStops,f.io.rootHolds);
   assert(!accepted && f.io.rootStarts==2 && f.io.rootStops==1 && f.io.rootHolds==1);f.Unknown();faults++;
  }
 }
 printf("PASS new512-page projection zero controller+ACTUAL frozen RV32 positives%u/faults%u/inertRefusals%u;2x256pages ONE retained lease/all7banks/cumulative2MiB output/ALL512 before published ONLY at CLOSE/partial clean resume/independent learned+GDN ledgers/UNKNOWN retainsALL ZERO later IO;NO_INPUT_PRODUCER_NATIVE_FACTORY_EXECUTION_OR_SPEED\n",positives,faults,refusals);
}
#endif
