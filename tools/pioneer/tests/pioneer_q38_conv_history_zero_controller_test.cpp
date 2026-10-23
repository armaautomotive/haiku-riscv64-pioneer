/* SPDX-License-Identifier: MIT */
// Actual frozen zero RV32 + actual production joint/history zero controller.
// Hardware endpoints are simulated. Synthetic completion fabric is FORBIDDEN.
#define PIONEER_Q38_DECODER_RUNTIME 1
#define PIONEER_Q38_STATE_ZERO_CONTROLLER_MODEL_ONLY
#include "pioneer_q38_state_zero_controller_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionMutableBanks.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ConvHistoryZeroController.h"
#include <type_traits>
static_assert(!std::is_copy_constructible<PioneerQ38ConvHistoryZeroLease>::value
 && !std::is_move_constructible<PioneerQ38ConvHistoryZeroLease>::value,"exact operation lease");
struct ConvHistoryZeroIO:ZeroControlIO {
 using ZeroControlIO::ZeroControlIO;
 PioneerQ38ConvHistoryZeroLease* historyLease=nullptr;
 PioneerQ38ConvHistoryExtent extents[336]={};
 std::vector<uint32_t> wholeHistory=std::vector<uint32_t>(1680*1024,0x13579bdf);
 std::vector<unsigned> touchedPages=std::vector<unsigned>(1680,0);
 bool projectionMode=false;uint32_t projectionBase=0;unsigned projectionBank=7;
 bool ConvHistoryZeroBeforeCohort(){return StateZeroBeforeCohort() && !projectionMode;}
 bool ConvHistoryZeroOtherCoresHeld(){return StateZeroOtherCoresHeld();}
 bool ProjectionZeroBeforeCohort(){return StateZeroBeforeCohort() && projectionMode;}
 bool ProjectionZeroOtherCoresHeld(){return StateZeroOtherCoresHeld();}
 void RuntimeResetWrite(uint32_t value){
  bool changed=authorityChanged;authorityChanged=false;unsigned starts=rootStarts,region=336,first=0,words=0;
  bool executes=value==0x46800 && !noCompletion;
  if(executes){
   assert(!publicationCounterFabricOnly);
   if(projectionMode){
    assert(projectionBank==l1[7] && l1[5]>=projectionBase && l1[5]-projectionBase<2097152);
    first=(l1[5]-projectionBase)/4;words=l1[4]/4;assert(words && words<=524288-first);
   }else{
    for(unsigned i=0;i<336;i++)if(extents[i].bank==l1[7] && l1[5]>=extents[i].address && l1[5]-extents[i].address<20480){
     assert(region==336);region=i;first=(l1[5]-extents[i].address)/4;words=l1[4]/4;
    }
    assert(region<336 && first<5120 && words && words<=5120-first);
   }
  }
  ZeroControlIO::RuntimeResetWrite(value);authorityChanged=changed;
  if(executes && starts!=rootStarts && !projectionMode){
   assert(lastDeviceWords.size()==words);
   // Observe ONLY bytes produced by the actual instruction machine. Never
   // manufacture a completed counter/zero span or substitute a CPU memset.
   for(unsigned i=0;i<words;i++)wholeHistory[region*5120+first+i]=lastDeviceWords[i];
   for(unsigned p=first/1024;p<(first+words)/1024;p++)touchedPages[region*5+p]++;
  }
  if(changed && starts!=rootStarts){assert(historyLease);historyLease->identity.modelEpoch++;}
 }
};
struct ConvHistoryZeroFixture {
 ConvHistoryZeroIO io;PanelOwners owner;
 PioneerOutputBufferLease transfer;PioneerBulkUploadLease bulk;PioneerWideOutputLease wide;
 PioneerQ38ConvHistoryZeroLease lease;PioneerQ38ConvHistoryZeroLease* active=&lease;PioneerQ38ConvHistoryZeroResult result;
 PioneerQ38GdnIdentity id={1,2,3,4};
 std::vector<uint8_t> learned=std::vector<uint8_t>(PioneerResidentBanks::Pages);
 std::unique_ptr<PioneerQ38CanonicalModelPlan> plan=std::make_unique<PioneerQ38CanonicalModelPlan>();
 std::unique_ptr<PioneerQ38ProjectionMutableBanks> graph=std::make_unique<PioneerQ38ProjectionMutableBanks>();
 PioneerQ38ConvHistoryBanks* state=nullptr;unsigned preparedWideLoads=0;
 explicit ConvHistoryZeroFixture(const char* image):io(image),owner(io){
  assert(plan->Build() && graph->BindCanonical(owner.banks,owner.dram,id,learned.data(),unsigned(learned.size()),*plan,0));
  state=graph->ConvHistory(id);assert(state);
  for(unsigned i=0;i<336;i++)assert(state->InitializeSpan(id,i,0,5,io.extents[i]));
  assert(PioneerPrepareOutputBuffer(io,owner.host,transfer) && PioneerBulkUploadPrepare(io,owner.dram,owner.noc,owner.host,owner.store,transfer,bulk)
   && PioneerPrepareWideOutput(io,owner.host,transfer,wide));preparedWideLoads=io.wideLoads;io.historyLease=active;
 }
 void UseLease(PioneerQ38ConvHistoryZeroLease& next){active=&next;io.historyLease=active;}
 bool PrepareZero(){return PioneerQ38ConvHistoryZeroPrepare(io,owner.dram,owner.noc,*state,owner.banks,id,owner.host,owner.store,bulk,transfer,wide,*active);}
 bool Zero(unsigned region=0,unsigned first=0,unsigned pages=5){return PioneerQ38ConvHistoryZeroRun(io,owner.dram,owner.noc,*state,owner.banks,owner.host,
  owner.store,bulk,transfer,wide,*active,id,region,first,pages,result);}
 bool CloseZero(bool clean=true){return PioneerQ38ConvHistoryZeroClose(io,owner.dram,owner.noc,*state,owner.banks,owner.host,
  owner.store,bulk,transfer,wide,*active,clean);}
 void Unknown(){
  assert(active->unsafe && state->Unsafe() && graph->Unsafe() && owner.banks.Quarantined() && owner.store.recovery_required && bulk.unsafe
   && transfer.quarantined && wide.quarantined && io.HostReadMock::quarantined && io.wideQuarantined && !io.pciWrites
   && !io.wideReleased && !io.PageMock::releases && !io.OutputBufferMock::releases);
  unsigned pages=~0U;assert(!state->InitializationProgress(id,pages) && pages==~0U && !state->InitializationPublished(id)
   && !graph->ConvHistory(id) && !graph->Projection(id) && !graph->Recurrent(id) && state->OwnsInitializationLease(active));
  unsigned before=io.touches;assert(!Zero() && !PrepareZero() && !CloseZero() && !CloseZero(false)
   && !PioneerReleaseWideOutput(io,wide,false) && !PioneerReleaseOutputBuffer(io,transfer,false) && io.touches==before);
 }
 void Finish(){
  assert(CloseZero() && active->closed && !active->unsafe && io.coreReset==0x47800 && !bulk.dirty && !wide.pending && !io.pciWrites
   && io.wideLoads==preparedWideLoads && !io.HostReadMock::quarantined && !state->OwnsInitializationLease(active));
  for(unsigned i=0;i<io.view.size();i++)assert(io.view[i]==0xab860000+i);
  for(unsigned i=0;i<8192;i++)assert(io.l1[i]==0x91730000+i);assert(io.entry==0x1250006f);
  unsigned before=io.touches;assert(CloseZero() && !PrepareZero() && !Zero() && before==io.touches);
  for(auto v:learned)assert(!v);
 }
};
#ifndef PIONEER_Q38_CONV_HISTORY_ZERO_FIXTURE_ONLY
int main(int argc,char** argv){
 assert(argc==2 || (argc==3 && !strcmp(argv[2],"--faults")));setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 unsigned positives=0,faults=0,refusals=0;
 if(argc==2){
  {ConvHistoryZeroFixture f(argv[1]);assert(f.PrepareZero());unsigned before=f.io.touches;
   PioneerQ38ConvHistoryZeroLease forged;forged.owner=f.state;forged.banks=&f.owner.banks;forged.proof=&f.owner.dram;
   forged.identity=f.id;forged.saved=forged.ready=true;
   assert(!PioneerQ38ConvHistoryZeroRun(f.io,f.owner.dram,f.owner.noc,*f.state,f.owner.banks,f.owner.host,f.owner.store,f.bulk,f.transfer,f.wide,
    forged,f.id,0,0,5,f.result) && !PioneerQ38ConvHistoryZeroClose(f.io,f.owner.dram,f.owner.noc,*f.state,f.owner.banks,f.owner.host,f.owner.store,
    f.bulk,f.transfer,f.wide,forged,true) && f.state->OwnsInitializationLease(&f.lease) && f.io.touches==before);refusals+=2;
   assert(!f.Zero(336,0,1) && !f.Zero(0,5,1) && !f.Zero(0,0,0) && !f.Zero(0,0,6) && !f.Zero(0,4,2) && f.io.touches==before);refusals+=5;
   assert(f.Zero(0,0,2) && f.CloseZero());unsigned progress=0;
   assert(f.state->InitializationProgress(f.id,progress) && progress==2 && !f.state->InitializationPublished(f.id));
   for(unsigned word=0;word<f.io.wholeHistory.size();word++)assert(f.io.wholeHistory[word]==(word<2048?0U:0x13579bdfU));
   PioneerQ38ConvHistoryZeroLease next;f.UseLease(next);assert(f.PrepareZero());before=f.io.touches;
   assert(!f.Zero(0,0,1) && !f.Zero(0,1,2) && f.io.touches==before && !next.unsafe);refusals+=2;
   assert(f.Zero(0,2,3));
   // ALL336 actual canonical stripes/all7physical routes,nonlinear order.
   for(unsigned ordinal=1;ordinal<336;ordinal++){
    unsigned region=(ordinal*97)%336;assert(region && f.Zero(region));
    assert(f.result.pages==5 && f.result.bytes==20480 && f.result.completed && f.result.held && !f.result.recovery_required);
    assert(f.state->InitializationProgress(f.id,progress) && progress==(ordinal+1)*5 && !f.state->InitializationPublished(f.id));
    if(!(ordinal%48))printf("PASS actual history zero regions%u/pages%u (not yet CLOSE-published)\n",ordinal+1,progress);
   }
   assert(f.io.rootStarts==337 && f.io.rootStops==337 && f.io.rootHolds==337);
   for(auto v:f.io.wholeHistory)assert(!v);for(auto count:f.io.touchedPages)assert(count==1);
   assert(f.graph->Recurrent(f.id)->InitializationProgress(f.id,progress) && !progress && !f.graph->Recurrent(f.id)->InitializationPublished(f.id));
   assert(f.graph->Projection(f.id)->InitializationProgress(f.id,progress) && !progress && !f.graph->Projection(f.id)->InitializationPublished(f.id));
   assert(f.CloseZero() && f.state->InitializationPublished(f.id));f.Finish();positives++;
  }
  for(unsigned pages=1;pages<=5;pages++){
   ConvHistoryZeroFixture f(argv[1]);assert(f.PrepareZero() && f.Zero(335,5-pages,pages));unsigned progress=0;
   assert(f.state->InitializationProgress(f.id,progress) && progress==pages && !f.state->InitializationPublished(f.id));f.Finish();positives++;
  }
 }else{
  for(unsigned word=0;word<32;word++){ConvHistoryZeroFixture f(argv[1]);assert(f.PrepareZero());f.io.summaryFault=word;assert(!f.Zero() && !f.io.rootStops && !f.io.rootHolds);f.Unknown();faults++;}
  for(unsigned word=0;word<64;word++){ConvHistoryZeroFixture f(argv[1]);assert(f.PrepareZero());f.io.descriptorFault=word;assert(!f.Zero() && !f.io.rootStops && !f.io.rootHolds);f.Unknown();faults++;}
  for(unsigned word=0;word<10;word++){ConvHistoryZeroFixture f(argv[1]);assert(f.PrepareZero());f.io.commandFault=word;assert(!f.Zero() && !f.io.rootStops && !f.io.rootHolds);f.Unknown();faults++;}
  for(unsigned word=0;word<16;word++){ConvHistoryZeroFixture f(argv[1]);assert(f.PrepareZero());f.io.sourceFault=word;assert(!f.Zero() && !f.io.rootStops && !f.io.rootHolds);f.Unknown();faults++;}
  for(unsigned packet=1;packet<=4;packet++){ConvHistoryZeroFixture f(argv[1]);assert(f.PrepareZero());f.io.packetFault=packet;assert(!f.Zero() && !f.io.rootStops && !f.io.rootHolds);f.Unknown();faults++;}
  for(unsigned direction=0;direction<2;direction++)for(bool corrupt:{false,true}){
   ConvHistoryZeroFixture f(argv[1]);assert(f.PrepareZero());if(corrupt)f.io.deviceCorruptPacket=direction;else f.io.deviceDropPacket=direction;
   assert(!f.Zero() && !f.io.rootStops && !f.io.rootHolds);f.Unknown();faults++;
  }
  for(unsigned fault=0;fault<23;fault++){
   ConvHistoryZeroFixture f(argv[1]);assert(f.PrepareZero());
   if(!fault)f.io.noCompletion=true;if(fault==1)f.io.corruptCode=true;if(fault==2)f.io.corruptCounter=true;if(fault==3)f.io.authorityChanged=true;
   if(fault==4)f.io.stageLost=true;if(fault==5)f.io.wideMapLost=true;if(fault==6)f.io.bulkMapLost=true;if(fault==7)f.io.coreMapLost=true;
   if(fault==8)f.io.othersHeld=false;if(fault>=9 && fault<=12)f.io.latePcFault=fault-8;
   if(fault==13)f.io.lateMapLoss=true;if(fault==14)f.io.lateCohort=true;if(fault==15)f.io.lateCoreMapLoss=true;
   if(fault==16)f.io.lateCompetingSpan=true;if(fault==17)f.io.lateOtherHeld=true;if(fault==18)f.io.activeCohort=true;if(fault==19)f.io.competingSpan=true;
   if(fault==20)f.io.badMarker=1;if(fault==21)f.io.badPhase=1;if(fault==22)f.io.badCount=1;
   assert(!f.Zero() && !f.io.rootStops && !f.io.rootHolds);f.Unknown();faults++;
  }
  for(unsigned drop=1;drop<=4;drop++){ConvHistoryZeroFixture f(argv[1]);assert(f.PrepareZero());f.io.dropReset=drop;
   assert(!f.Zero() && f.io.resets.size()==drop);f.Unknown();faults++;
  }
  for(unsigned fault=0;fault<3;fault++){ConvHistoryZeroFixture f(argv[1]);assert(f.PrepareZero());f.io.clock=100000;
   if(!fault)f.io.clock=UINT64_MAX-10;if(fault==1)f.io.backwardsClock=true;if(fault==2){f.io.noCompletion=true;f.io.frozenClock=true;}
   assert(!f.Zero() && !f.io.rootStops && !f.io.rootHolds);f.Unknown();faults++;
  }
  for(unsigned fault=0;fault<7;fault++){ConvHistoryZeroFixture f(argv[1]);assert(f.PrepareZero() && f.Zero());
   if(!fault)f.io.view[(0x72000-TT_Q38_SPAN_VIEW_BASE)/4]^=1;
   if(fault==1)f.io.dropWrite=f.io.panelWrites+(0x72000-TT_Q38_SPAN_VIEW_BASE)/4+1;
   if(fault==2){f.io.l1[0]^=1;f.io.dropBulk=f.io.bulkWrites+1;}if(fault==3)f.io.dropEntry=f.io.entryWrites+1;
   if(fault==4)f.io.othersHeld=false;if(fault==5)f.io.source[11]^=1;
   assert(!f.CloseZero(fault!=6));f.Unknown();faults++;
  }
  for(unsigned fault=0;fault<3;fault++){ConvHistoryZeroFixture f(argv[1]);if(!fault)f.io.dropWrite=1;if(fault==1)f.io.dropEntry=1;if(fault==2)f.io.source[5]^=1;
   assert(!f.PrepareZero() && !f.io.rootStarts);f.Unknown();faults++;
  }
  {ConvHistoryZeroFixture f(argv[1]);assert(f.PrepareZero() && f.Zero());unsigned before=f.io.touches;
   PioneerQ38CanonicalPart* part=nullptr;
   for(unsigned i=0;i<f.plan->Count();i++)if(f.plan->Part(i)->kind==f.plan->ConvKind)part=const_cast<PioneerQ38CanonicalPart*>(f.plan->Part(i));
   assert(part);
   part->offset+=4096;assert(!f.Zero(335) && f.io.touches==before);f.Unknown();faults++;
  }
 }
 printf("PASS convolution history zero ACTUAL frozen RV32 positives%u/faults%u/refusals%u;ALL336stripes/1680pages/real padding+fresh readback/partial clean resume/EMPTY learned+GDN+projection ledgers/full count NOT published until physical CLOSE/UNKNOWN retainsALL ZERO later IO;NO_NATIVE_FACTORY_CURRENT_HISTORY_PRODUCER_COMPUTE_OR_SPEED_GRANT\n",positives,faults,refusals);
}
#endif
