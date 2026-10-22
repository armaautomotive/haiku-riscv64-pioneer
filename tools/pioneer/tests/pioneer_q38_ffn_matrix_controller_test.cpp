/* SPDX-License-Identifier: MIT */
// Controller+ACTUAL zero/root instructions. Global lifetime/input-producer
// capability and peer arithmetic are explicitly SIMULATED;NO native grant.
#define PIONEER_Q38_FFN_ZERO_CONTROLLER_MODEL_ONLY
#include "pioneer_q38_ffn_zero_controller_test.cpp"
#define PIONEER_Q38_DISPATCH_BOOTSTRAP_MOCK_ONLY
#include "pioneer_q38_dispatch_bootstrap_test.cpp"
#define PIONEER_Q38_DISPATCH_BASE_ALREADY_INCLUDED
#define PIONEER_Q38_FFN_MATRIX_MACHINE_ONLY
#include "pioneer_q38_ffn_matrix_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38FfnMatrixController.h"
struct MatrixControlMachine:FfnMatrixMachine {
 uint32_t physical[16]={};
 using FfnMatrixMachine::FfnMatrixMachine;
 uint32_t Load(uint32_t at)override {
  for(unsigned i=0;i<16;i++)if(at==0xffb20000+BlackholeDramSourceOffset(i))return physical[i];
  return FfnMatrixMachine::Load(at);
 }
};
struct MatrixControlIO:BootstrapIO {
 FfnZeroFixture& zero;PioneerQ38DispatchLease* parent=nullptr;PioneerQ38FfnMatrixLease* binding=nullptr;PioneerQ38FfnMatrixJob* liveJob=nullptr;
 const char* image;std::vector<uint32_t> prefix=std::vector<uint32_t>(4096),lastOutput;
 unsigned touches=0,rootRuns=0,rootStops=0,rootHolds=0,pcSamples=0,prefixWrites=0,dropPrefix=0,numericReads=0;
 unsigned badStats=~0U,badCommand=~0U,badPc=0,dropReset=0,rootResetWrites=0;
 bool scopeReady=true,inputReady=true,prefixStable=true,noCompletion=false,lateScope=false,lateInput=false;
 bool lateCode=false,lateSource=false,lateFrame=false,lateCounter=false,frozenClock=false,backwardClock=false,lateNonce=false,lateJob=false;
 uint64_t frozenAt=0;
 std::vector<unsigned> statusOrder;
 MatrixControlIO(FfnZeroFixture& f,const char* path):zero(f),image(path) {
  // Borrow the EXACT physical root/NIU/store used for actual workspace
  // zero initialization,not a cloned baseline or second mutable claimant.
  core[0].reset();core[0].reset(&f.io);
  for(unsigned i=0;i<4096;i++)prefix[i]=0x8c4e0000+i;
 }
 ~MatrixControlIO(){core[0].release();} // borrowed root is owned by zero fixture
 void Touch(){assert(!quarantined);touches++;}
 PanelIO& Core(){return *core[current];}
 bool SelectRuntimeCore(unsigned w){Touch();return BootstrapIO::SelectRuntimeCore(w);}
 bool FfnMatrixScope(const PioneerQ38FfnMatrixLease& l,const PioneerQ38FfnBanks& s,const PioneerQ38DispatchLease& c,const PioneerQ38FfnIdentity& id) {
  return scopeReady && &l==binding && &s==&zero.state && &c==parent && id==zero.identity;
 } // SIMULATED kernel context seal,not a native implementation
 bool FfnMatrixInputReady(const PioneerQ38FfnMatrixLease&,const PioneerQ38FfnMatrixJob&){return inputReady;}
 void FfnMatrixRetainHostOwners(){zero.io.QuarantineHostPage();PioneerQuarantineOutputBuffer(zero.io,zero.transfer);PioneerQuarantineWideOutput(zero.io,zero.wide);}
 bool FfnMatrixPrefixStable(){Touch();return !current && prefixStable;}
 uint32_t FfnMatrixPrefixRead(unsigned i){Touch();assert(!current && i<4096);return prefix[i];}
 void FfnMatrixPrefixWrite(unsigned i,uint32_t value){Touch();assert(!current && i<4096 && Core().coreReset==0x47800);if(++prefixWrites!=dropPrefix)prefix[i]=value;}
 bool Q38PanelMappingStable(){Touch();return Core().mapping;}
 uint32_t Q38PanelRead(uint32_t a){Touch();if(!current && binding && binding->ready
  && ((a>=0x80000 && a<0x9dc00) || (a>=0xa0000 && a<0xab880)))numericReads++;return Core().Q38PanelRead(a);}
 void Q38PanelWrite(uint32_t a,uint32_t value){Touch();Core().Q38PanelWrite(a,value);}
 uint32_t RuntimeCodeRead(unsigned i){Touch();return Core().RuntimeCodeRead(i);}
 uint32_t ResetRead(){Touch();return Core().ResetRead();}
 uint32_t SourceRead(unsigned i){Touch();return Core().SourceRead(i);}
 uint32_t HostPciRead(unsigned p,uint16_t o,uint8_t s){Touch();return Core().HostPciRead(p,o,s);}
 uint32_t ResidentNocRead(uint32_t a){Touch();return Core().ResidentNocRead(a);}
 uint32_t RuntimeStatusRead(unsigned i){Touch();if(!current && rootRuns && Core().coreReset==0x46800)statusOrder.push_back(i);return Core().RuntimeStatusRead(i);}
 void RuntimeStatusWrite(unsigned i,uint32_t v){Touch();Core().RuntimeStatusWrite(i,v);}
 uint32_t RuntimePcRead(){Touch();if(current)return TT_Q38_PEER_WAIT_LOAD;return ++pcSamples==badPc?0x72004:TT_Q38_DISPATCH_PARK;}
 void Pause(){Touch();now+=10000;}
 uint64_t Q38CohortNow(){return rootRuns && backwardClock?0:rootRuns && frozenClock?frozenAt:now;}
 void Execute() {
  auto& c=Core();unsigned base=(0x74000-TT_Q38_PANEL_VIEW_BASE)/4;
  unsigned n=c.view[base+5],type=c.view[base+3],role=c.view[(TT_Q38_FFN_MATRIX_FRAME+20-TT_Q38_PANEL_VIEW_BASE)/4];
  unsigned bank=c.view[(TT_Q38_FFN_MATRIX_FRAME+12-TT_Q38_PANEL_VIEW_BASE)/4];
  MatrixControlMachine m(image,role,bank,false,0,type,n);
  for(unsigned i=0;i<c.view.size();i++)m.memory[TT_Q38_PANEL_VIEW_BASE+i*4]=c.view[i];
  uint32_t guard=m.memory.at(0x7fbfc);m.memory[0x7fbfc]=0xdecafbad; // VM's untouched guard oracle
  for(unsigned i=0;i<4096;i++)m.l1[m.Index(0x64000+i*4)]=prefix[i];
  for(unsigned i=0;i<2048;i++)m.l1[m.Index(0x72000+i*4)]=c.view[(0x72000-TT_Q38_PANEL_VIEW_BASE)/4+i];
  for(unsigned i=0;i<64;i++)m.frame[i]=c.view[(TT_Q38_FFN_MATRIX_FRAME-TT_Q38_PANEL_VIEW_BASE)/4+i];
  for(unsigned i=0;i<119;i++)m.first[i]=c.view[(TT_Q38_FFN_MATRIX_ROWS-TT_Q38_PANEL_VIEW_BASE)/4+i];
  m.order=PioneerQ38FfnMatrixOrdered(binding->authority.task,m.first.data(),n)?0:1;
  for(unsigned i=0;i<16;i++)m.physical[i]=i?c.source[i]:c.config;
  for(unsigned i=0;i<8;i++)m.counts[i]=c.ResidentNocRead(PioneerQ38PanelCounter(i));
  for(unsigned w=0;w<119;w++)m.peers[w].requested=m.peers[w].finished=parent->peerNonce[w];
  assert(m.RunFfn());lastOutput=m.remoteOutput;m.memory[0x7fbfc]=guard;
  for(unsigned i=0;i<c.view.size();i++)c.view[i]=m.memory.at(TT_Q38_PANEL_VIEW_BASE+i*4);
  for(unsigned w=0;w<n;w++) {
   auto& p=*core[w+1];unsigned at=(0x74140-TT_Q38_PANEL_VIEW_BASE)/4;
   for(unsigned i=0;i<16;i++)p.view[at+i]=0;
   p.view[at]=p.view[at+1]=m.peers[w].finished;p.view[at+3]=TT_Q38_PEER_MARKER;
  }
  c.counters[0]=m.counts[0];c.counters[1]=m.counts[1];c.accepted=m.counts[2];c.counters[3]=m.counts[3];
  c.writeCounters[0]=m.counts[4];c.writeCounters[1]=m.counts[5];c.writeCounters[3]=m.counts[7];
  for(unsigned i=0;i<10;i++)c.command[i]=m.command[i];
  c.destination[2032]=0x7200c;c.destination[2033]=n;c.destination[2034]=TT_Q38_FFN_MATRIX_MARKER;
  if(badStats<64)c.view[(0x7f800-TT_Q38_PANEL_VIEW_BASE)/4+badStats]^=1;
  if(badCommand<10)c.command[badCommand]^=1;
  if(lateScope)scopeReady=false;if(lateInput)inputReady=false;if(lateCode)prefix[0]^=1;
  if(lateFrame)c.view[(TT_Q38_FFN_MATRIX_FRAME-TT_Q38_PANEL_VIEW_BASE)/4+32]^=1;
  if(lateSource)c.config^=1;if(lateCounter)c.counters[0]++;
  if(lateNonce)parent->rootNonce++;
  if(lateJob){assert(liveJob);liveJob->layer++;}
 }
 void RuntimeResetWrite(uint32_t value){Touch();if(current){BootstrapIO::RuntimeResetWrite(value);return;}
  auto& c=Core();c.resets.push_back(value);if(++rootResetWrites==dropReset)return;
  if(value==0x87ffff){if(c.coreReset==0x46800)rootStops++;c.coreReset=value;return;}
  if(value==0x47800){rootHolds++;c.coreReset=value;return;}
  assert(value==0x46800 && c.coreReset==0x87ffff);c.coreReset=value;rootRuns++;frozenAt=now;if(!noCompletion)Execute();
 }
};
struct MatrixControlFixture {
 FfnZeroFixture zero;MatrixControlIO io;std::unique_ptr<PioneerQ38DispatchLease> cohort{new PioneerQ38DispatchLease};
 std::unique_ptr<PioneerQ38FfnMatrixLease> lease{new PioneerQ38FfnMatrixLease};
 std::vector<std::unique_ptr<PanelOwners>> owners;PioneerQ38FfnMatrixJob job;PioneerQ38FfnMatrixResult result;
 MatrixControlFixture(const char* zeroImage,const char* matrixImage,unsigned role=0,bool initialize=true):zero(zeroImage),io(zero,matrixImage) {
  if(initialize)assert(zero.PrepareZero() && zero.Zero(0,0,74) && zero.CloseZero() && zero.state.InitializationPublished(zero.identity));
  std::fill(zero.learned.begin(),zero.learned.end(),1);for(unsigned i=0;i<74;i++)zero.learned[0x04000000/4096+i]=0;
  cohort->worker[0].master=&zero.owner.store;cohort->worker[0].noc=zero.owner.noc;
  for(unsigned w=1;w<120;w++){owners.emplace_back(new PanelOwners(*io.core[w]));auto& n=cohort->worker[w];n.master=&owners.back()->store;n.noc=owners.back()->noc;n.noc.version=PioneerWorkerVersion(w);}
  for(unsigned w=0;w<120;w++)assert(PioneerQ38DispatchPrepareRole(io,zero.owner.dram,zero.owner.banks,*cohort,w));
  assert(PioneerQ38DispatchBootstrap(io,zero.owner.dram,zero.owner.banks,*cohort));
  io.parent=cohort.get();io.binding=lease.get();job.role=role;job.type=12;job.peers=role==2?20:68;
  for(unsigned w=0;w<job.peers;w++){job.part[w]={w%7,16,UINT64_C(0x0c000040)+(w/7)*0x00800000};job.first[w]=w*256;}
  io.liveJob=&job;
 }
 bool Prepare(){return PioneerQ38FfnMatrixPrepare(io,zero.owner.dram,zero.owner.banks,zero.state,*cohort,zero.identity,*lease);}
 bool Run(){return PioneerQ38FfnMatrixRun(io,zero.owner.dram,zero.owner.banks,zero.state,*cohort,*lease,zero.learned.data(),unsigned(zero.learned.size()),job,result);}
 bool Close(bool clean=true){return PioneerQ38FfnMatrixClose(io,zero.owner.dram,zero.owner.banks,zero.state,*cohort,*lease,clean);}
 void Unsafe(){assert(lease->unsafe && zero.state.Unsafe() && zero.owner.banks.Quarantined() && cohort->unsafe
   && zero.state.OwnsMatrixLease(lease.get()) && io.quarantined && zero.transfer.quarantined && zero.wide.quarantined);
  unsigned before=io.touches;assert(!Prepare() && !Run() && !Close() && !Close(false) && before==io.touches);
 }
};
int main(int argc,char** argv){assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 for(bool prefix:{false,true}) {
  const auto code=Read((std::string(argv[2])+(prefix?"/prefix.bin":"/text.bin")).c_str());
  unsigned words=prefix?4096:2048;
  assert(!code.empty() && !(code.size()&3) && code.size()<=words*4);
  for(unsigned w=0;w<words;w++)assert(PioneerQ38FfnMatrixCodeWord(prefix,w)==(w*4<code.size()?Word(code,w*4):0));
  assert(PioneerQ38FfnMatrixCodeWord(prefix,words)==UINT32_MAX && PioneerQ38FfnMatrixCodeWord(prefix,UINT32_MAX)==UINT32_MAX);
 }
 puts("PASS full frozen root+prefix pins and padded tails equal independently built images");
 unsigned positives=0,faults=0,refusals=0;
 for(unsigned role:{0U,2U}){MatrixControlFixture f(argv[1],argv[2],role);assert(f.Prepare() && f.Run() && f.result.held && f.Close());positives++;
  assert(!f.zero.state.OwnsMatrixLease(f.lease.get()) && !f.cohort->unsafe && !f.cohort->rootRunning && f.cohort->rootNonce==1);
  for(unsigned i=0;i<4096;i++)assert(f.io.prefix[i]==0x8c4e0000+i);
  assert(f.result.state==TT_RUNTIME_PASSED && !f.result.recovery_required && !f.io.numericReads);
  unsigned before=f.io.touches;assert(f.Close() && !f.Run() && !f.Prepare() && before==f.io.touches);}
 puts("PASS controller clean source/sink/restore");
 {MatrixControlFixture f(argv[1],argv[2]);assert(f.Prepare());const auto gate=f.job;
  for(unsigned role:{0U,1U,2U,0U}) {
   f.job=gate;f.job.role=role;f.job.position=UINT64_C(0x100000002);f.job.layer=7;
   if(role==2){f.job.peers=20;for(unsigned w=20;w<119;w++){f.job.part[w]={};f.job.first[w]=0;}}
   assert(f.Run() && f.result.held && !f.result.recovery_required);
  }
  assert(f.cohort->rootNonce==4 && f.io.rootRuns==4 && !f.io.numericReads);
  for(unsigned w=0;w<119;w++)assert(f.cohort->peerNonce[w]==(w<20?4U:w<68?3U:0U));
  assert(f.Close() && !f.io.numericReads);positives++;
 }
 puts("PASS repeated gate/up/down/gate metadata-only dispatch with independent persistent peer sequences");
 for(unsigned word=0;word<57;word++){MatrixControlFixture f(argv[1],argv[2]);assert(f.Prepare());f.io.badStats=word;
  assert(!f.Run() && !f.io.rootStops && !f.io.rootHolds && !f.cohort->rootNonce);f.Unsafe();faults++;}
 for(unsigned word=0;word<10;word++){MatrixControlFixture f(argv[1],argv[2]);assert(f.Prepare());f.io.badCommand=word;
  assert(!f.Run() && !f.io.rootStops && !f.io.rootHolds);f.Unsafe();faults++;}
 puts("PASS controller ALL57 safety-stat/10command corruptions retained before hold");
 for(unsigned fault=0;fault<13;fault++){MatrixControlFixture f(argv[1],argv[2]);assert(f.Prepare());
  if(!fault)f.io.lateScope=true;if(fault==1)f.io.lateInput=true;if(fault==2)f.io.lateCode=true;if(fault==3)f.io.lateSource=true;
  if(fault==4)f.io.lateFrame=true;if(fault==5)f.io.lateCounter=true;if(fault>=6 && fault<=9)f.io.badPc=fault-5;
  if(fault==10)f.io.noCompletion=true;if(fault==11)f.io.lateNonce=true;if(fault==12)f.io.lateJob=true;
  assert(!f.Run() && !f.io.rootStops && !f.io.rootHolds);f.Unsafe();faults++;}
 for(unsigned drop=1;drop<=4;drop++){MatrixControlFixture f(argv[1],argv[2]);assert(f.Prepare());f.io.dropReset=drop;
  assert(!f.Run() && f.io.rootResetWrites==drop);f.Unsafe();faults++;}
 for(unsigned fault=0;fault<3;fault++){MatrixControlFixture f(argv[1],argv[2]);assert(f.Prepare());
  if(!fault)f.io.now=UINT64_MAX-10;if(fault==1)f.io.backwardClock=true;if(fault==2){f.io.frozenClock=true;f.io.noCompletion=true;}
  assert(!f.Run() && !f.io.rootStops && !f.io.rootHolds);f.Unsafe();faults++;}
 puts("PASS controller scope/producer/code/source/frame/counter/nonce/PC/reset/clock failures retained");
 for(unsigned fault=0;fault<9;fault++){MatrixControlFixture f(argv[1],argv[2]);assert(f.Prepare());
  if(!fault)f.job.role=3;if(fault==1)f.job.layer=64;if(fault==2)f.job.type=8;if(fault==3)f.job.peers=0;
  if(fault==4)f.job.first[0]=16;if(fault==5)f.job.part[0].offset++;
  if(fault==6){f.job.part[0].offset=0x04000000;std::fill(f.zero.learned.begin(),f.zero.learned.end(),1);}
  if(fault==7)f.zero.learned[uint32_t(f.job.part[0].offset/4096)]=0;if(fault==8)f.io.inputReady=false;
  unsigned before=f.io.touches;assert(!f.Run() && !f.lease->unsafe && before==f.io.touches && !f.cohort->rootNonce);refusals++;}
 for(unsigned fault=0;fault<3;fault++){MatrixControlFixture f(argv[1],argv[2]);assert(f.Prepare() && f.Run());
  if(!fault)f.io.dropPrefix=f.io.prefixWrites+1;
  if(fault==1)f.io.core[1]->view[(0x74140-TT_Q38_PANEL_VIEW_BASE)/4]^=1;
  assert(!f.Close(fault!=2));f.Unsafe();faults++;}
 {MatrixControlFixture f(argv[1],argv[2]);assert(f.Prepare() && f.Run());unsigned different=0;
  while(different<2048 && f.lease->savedMain[different]==PioneerQ38FfnMatrixCodeWord(false,different))different++;
  assert(different<2048);f.io.core[0]->dropWrite=f.io.core[0]->panelWrites+different+1;
  assert(!f.Close());f.Unsafe();faults++;}
 {MatrixControlFixture f(argv[1],argv[2]);f.io.dropPrefix=1;assert(!f.Prepare());f.Unsafe();faults++;}
 for(unsigned fault=0;fault<3;fault++){MatrixControlFixture f(argv[1],argv[2]);assert(f.Prepare());
  if(!fault)f.cohort->rootNonce=0x7ffffffd;
  if(fault==1){f.cohort->rootNonce=0x7ffffffc;f.cohort->peerNonce[0]=0x7ffffffd;}
  if(fault==2)f.lease->identity.driverGeneration++;
  unsigned before=f.io.touches;assert(!f.Run() && before==f.io.touches && !f.lease->unsafe);refusals++;}
 {MatrixControlFixture f(argv[1],argv[2]);assert(f.Prepare());f.cohort->rootNonce=0x7ffffffc;
  assert(f.Run() && f.cohort->rootNonce==0x7ffffffd && f.Close());positives++;}
 {MatrixControlFixture f(argv[1],argv[2],0,false);unsigned before=f.io.touches;
  assert(!f.Prepare() && before==f.io.touches && !f.zero.state.OwnsMatrixLease(f.lease.get()));refusals++;}
 {MatrixControlFixture f(argv[1],argv[2]);assert(f.Prepare());PioneerQ38FfnMatrixLease competing;f.io.binding=&competing;
  unsigned before=f.io.touches;assert(!PioneerQ38FfnMatrixPrepare(f.io,f.zero.owner.dram,f.zero.owner.banks,f.zero.state,*f.cohort,f.zero.identity,competing)
   && before==f.io.touches && f.zero.state.OwnsMatrixLease(f.lease.get()));f.io.binding=f.lease.get();assert(f.Close());refusals++;}
 for(unsigned fault=0;fault<3;fault++){MatrixControlFixture f(argv[1],argv[2]);assert(f.Prepare());auto copied=f.zero.owner.dram;
  PioneerResidentBanks otherBanks;PioneerQ38DispatchLease otherCohort;
  auto& d=fault==0?copied:f.zero.owner.dram;auto& b=fault==1?otherBanks:f.zero.owner.banks;auto& c=fault==2?otherCohort:*f.cohort;
  unsigned before=f.io.touches;assert(!PioneerQ38FfnMatrixRun(f.io,d,b,f.zero.state,c,*f.lease,f.zero.learned.data(),unsigned(f.zero.learned.size()),f.job,f.result)
   && !PioneerQ38FfnMatrixClose(f.io,d,b,f.zero.state,c,*f.lease,false) && before==f.io.touches && !f.lease->unsafe);refusals+=2;assert(f.Close());}
 printf("PASS OFFLINE private FFN controller+ACTUAL zero/root:positive%u/faults%u/inertrefusals%u;initialization-beforecohort/exactworkspace+cohortclaim/noCPUintermediates/allphysicalproofsBEFOREhold/originalprefix+mainrestore/UNKNOWNretainsALLownersZEROlaterIO;SIMULATED_PRODUCER_AND_NATIVE_SCOPE_NOT_QUALIFIED\n",positives,faults,refusals);
}
