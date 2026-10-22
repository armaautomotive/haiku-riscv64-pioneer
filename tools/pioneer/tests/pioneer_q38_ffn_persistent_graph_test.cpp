/* SPDX-License-Identifier: MIT */
// SAME persistent owner + ACTUAL installed QFG2 ROOT and ALL119 QFP2
// RV32/FPU/Tensix machines. Fabric only copies bytes. SYNTHETIC learned
// bytes/endpoints,NOT native factory/authenticated model/speed evidence.
#define PIONEER_Q38_TEST_QUALIFIED_LEGACY
#define PIONEER_Q38_FFN_CODE_OWNER_MOCK_ONLY
#include "pioneer_q38_ffn_graph_code_owner_test.cpp"
#define PIONEER_Q38_FFN_GRAPH_COOPERATIVE_FIXTURE_ONLY
#include "pioneer_q38_ffn_graph_cooperative_test.cpp"
#define PIONEER_Q38_FFN_ZERO_CONTROLLER_MODEL_ONLY
#define PIONEER_Q38_SPAN_HAS_BLOCK_MACHINE
#include "pioneer_q38_ffn_zero_controller_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38FfnPersistentGraphOwner.h"
#ifndef B_DEVICE_OP_CODES_END
#define B_DEVICE_OP_CODES_END 0x1000
#endif
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38DispatchClose.h"
#include <csignal>
static unsigned LastLoad=0,LastPc=0,LastWorker=0;
struct InstalledPeer:GraphArithmeticPeer {
 using GraphArithmeticPeer::GraphArithmeticPeer;
 uint32_t Load(uint32_t a)override{LastLoad=a;LastPc=pc;LastWorker=logicalWorker;return GraphArithmeticPeer::Load(a);}
};
struct PersistentGraphIO;
struct PersistentRoot:GraphRootMachine {
 PersistentGraphIO& io;
 PersistentRoot(const char* path,PersistentGraphIO& endpoint);
 void Packet()override;
};
static const char* GraphImage=nullptr;
static const char* PeerImage=nullptr;
struct PersistentGraphIO:CodeIO {
 const PioneerQ38FfnPersistentGraphResult* result=nullptr;
 const PioneerQ38GraphMutableBanks* expectedGraph=nullptr;const PioneerQ38CanonicalModelPlan* expectedPlan=nullptr;
 PioneerQ38FfnIdentity expectedIdentity={};const uint8_t* expectedLedger=nullptr;unsigned expectedPages=0;
 std::vector<uint32_t> gddr=std::vector<uint32_t>(303104/4,0x7fc00001);
 std::array<std::unique_ptr<InstalledPeer>,120> peer;
 std::unique_ptr<PersistentRoot> root;
 std::array<std::vector<uint32_t>,3> numeric;
 unsigned bootCount=0,rootCount=0,holdCount=0,matrixCount=0,producerCount=0,unknownAt=0;
 unsigned graphFault=0,faultWorker=1,faultWord=0,faultPhase=2,packetFault=0,packetCount=0;
 unsigned loggedPhase=0;
 unsigned workManifestPostReads=0,stopManifestPostReads=0,inputStageReads=0,manifestStageReads=0;
 bool scope=true,retained=false,rootTerminal=false;
 bool FfnGraphScope(const PioneerQ38FfnPersistentGraphOwner&,const PioneerQ38GraphMutableBanks& g,
  const PioneerQ38CanonicalModelPlan& p,const PioneerQ38FfnIdentity& id,const uint8_t* ledger,unsigned pages) {
  if(result && result->phase!=loggedPhase){loggedPhase=result->phase;printf("PERSISTENT_PHASE%u runs%u rootStarts%u;NOT_NATIVE_TIME\n",loggedPhase,result->runs,result->rootStarts);}
  return scope && &g==expectedGraph && &p==expectedPlan && id==expectedIdentity && ledger==expectedLedger && pages==expectedPages;
 }
 void FfnGraphRetainHostOwners(){retained=true;}
 bool AtFault()const{return result && result->phase==faultPhase && current==faultWorker;}
 bool FfnGraphCodeMappingStable(unsigned w){Touch();assert(w==current);
  if(graphFault==3 && AtFault()){observed=touches;return false;}return true;}
 uint32_t ResetRead(){Touch();uint32_t v=BootstrapIO::ResetRead();
  if(graphFault==4 && AtFault()){observed=touches;return v^1;}return v;}
 uint32_t SourceRead(unsigned i){Touch();uint32_t v=BootstrapIO::SourceRead(i);
  if(graphFault==5 && AtFault()){observed=touches;return v^1;}return v;}
 uint32_t ResidentNocRead(uint32_t a){Touch();uint32_t v=BootstrapIO::ResidentNocRead(a);
  if(graphFault==6 && AtFault() && a==0x130){observed=touches;return v^1;}return v;}
 uint32_t HostPciRead(unsigned p,uint16_t a,uint8_t b){Touch();uint32_t v=BootstrapIO::HostPciRead(p,a,b);
  if(graphFault==7 && AtFault() && a==4){observed=touches;return v^1;}return v;}
 static void Counts(PanelIO& c,const uint32_t* n){c.counters[0]=n[0];c.counters[1]=n[1];c.accepted=n[2];c.counters[3]=n[3];
  c.writeCounters[0]=n[4];c.writeCounters[1]=n[5];c.writeCounters[3]=n[7];}
 void SyncPeer(unsigned w){auto& p=*peer[w];auto& c=*core[w];
  for(unsigned a=0x70000;a<0x74000;a+=4){if(p.CodeAddress(a))c.view[(a-TT_Q38_PANEL_VIEW_BASE)/4]=p.l1[p.Index(a)];
   else if(p.memory.count(a))c.view[(a-TT_Q38_PANEL_VIEW_BASE)/4]=p.memory.at(a);}
  for(unsigned a=0x74000;a<0x74180;a+=4)c.view[(a-TT_Q38_PANEL_VIEW_BASE)/4]=p.memory.at(a);
  uint32_t count[8];for(unsigned i=0;i<8;i++)count[i]=p.Count(i);Counts(c,count);
  for(unsigned i=0;i<10;i++)c.command[i]=p.command[i];
 }
 void AdvancePeer(unsigned w){if(!peer[w] || core[w]->coreReset!=0x46800)return;
  auto& p=*peer[w];
  if(!p.prepared){for(unsigned i=0;i<256 && p.pc!=TT_Q38_PEER_WAIT_LOAD && p.pc!=TT_Q38_PEER_FAULT_PARK;i++)p.StepBlock();}
  else p.Advance();
  SyncPeer(w);
 }
 void AdvanceRoot(){if(!root || core[0]->coreReset!=0x46800)return;
  auto& p=*root;
  for(unsigned n=0;n<8192;n++){if(p.pc==TT_Q38_DISPATCH_PARK || p.pc==TT_Q38_DISPATCH_FAULT_PARK)break;p.StepBlock();}
  auto& c=*core[0];Counts(c,p.counts);for(unsigned i=0;i<10;i++)c.command[i]=p.command[i];
  for(unsigned i=0;i<3;i++)c.destination[2032+i]=p.l1[p.Index(0x17ffc0+i*4)];
  if(p.pc==TT_Q38_DISPATCH_PARK || p.pc==TT_Q38_DISPATCH_FAULT_PARK){
   for(const auto& v:p.memory)if((v.first>=0x6e000 && v.first<0x70000) || (v.first>=0x74000 && v.first<0xb0000))
    c.view[(v.first-TT_Q38_PANEL_VIEW_BASE)/4]=v.second;
   rootTerminal=p.pc==TT_Q38_DISPATCH_PARK;
  }
 }
 bool FfnGraphProgramWrite(unsigned w,uint32_t a,uint32_t v){Touch();writes++;
  assert(result && result->savedWords==867448 && (result->phase==2 || result->phase==9));
  if(graphFault==1 && result->phase==2 && w==faultWorker && a==0x6000){observed=touches;return false;}
  RawWrite(w,a,v);return true;
 }
 uint32_t Q38PanelRead(uint32_t a){if(track && current && peer[current])AdvancePeer(current);
  Touch();uint32_t v=BootstrapIO::Q38PanelRead(a);
  if(track && !current && result){
   if(result->phase==6){
    if(a>=0x80000 && a<0x85000)inputStageReads++;
    if(a>=TT_Q38_FFN_GRAPH_MANIFEST && a<TT_Q38_FFN_GRAPH_SHADOW)manifestStageReads++;
   }
   if(result->phase==7 && rootTerminal && root && a>=TT_Q38_FFN_GRAPH_MANIFEST
    && a<TT_Q38_FFN_GRAPH_COUNTERS){
    if(root->fixture.manifest[5]==1)workManifestPostReads++;else stopManifestPostReads++;
   }
  }
  if(graphFault==2 && rootTerminal && !current && a==TT_Q38_FFN_GRAPH_STATS+faultWord*4){observed=touches;return v^0x80000000U;}
  return v;
 }
 void Q38PanelWrite(uint32_t a,uint32_t v){Touch();BootstrapIO::Q38PanelWrite(a,v);}
 uint32_t RuntimeStatusRead(unsigned i){if(track && !current)AdvanceRoot();Touch();return BootstrapIO::RuntimeStatusRead(i);}
 void RuntimeStatusWrite(unsigned i,uint32_t v){Touch();BootstrapIO::RuntimeStatusWrite(i,v);}
 uint32_t RuntimePcRead(){Touch();if(current){AdvancePeer(current);return peer[current]->pc;}AdvanceRoot();return root->pc;}
 void ResidentNocWrite(uint32_t a,uint32_t v){Touch();if(track)assert(a<40 && !(a&3));BootstrapIO::ResidentNocWrite(a,v);}
 void RuntimeResetWrite(uint32_t v){
  if(!track){BootstrapIO::RuntimeResetWrite(v);return;}
  Touch();assert(result);
  if(v==0x87ffff && result->phase==7 && rootTerminal && root && root->fixture.manifest[5]==2){
   // Actual ROOT+ALL119 physical terminal evidence BEFORE first hold.
   assert(!current && root->pc==TT_Q38_DISPATCH_PARK && !root->pending && !root->unknown);
   for(unsigned w=1;w<120;w++)assert(peer[w] && peer[w]->pc==TT_Q38_PEER_STOP && peer[w]->prepared==peer[w]->verified);
  }
  C().coreReset=v;
  if(v==0x47800){holdCount++;return;}
  if(v!=0x46800)return;
  if(current){assert(!peer[current]);auto p=std::make_unique<InstalledPeer>(PeerImage,current,gddr,false);
   p->memory.clear();for(unsigned i=0;i<PioneerQ38FfnGraphCodeWords(current);i++){
    uint32_t a=PioneerQ38FfnGraphCodeAddress(current,i),value=RawRead(current,a);
    if(a==0x6000 || p->CodeAddress(a))p->l1[p->Index(a)]=value;else p->memory[a]=value;
   }
   for(unsigned a=0x74000;a<0x74180;a+=4)p->memory[a]=BootstrapIO::Q38PanelRead(a);
   for(unsigned i=0;i<16;i++)p->source[i]=BootstrapIO::ResidentNocRead(BlackholeDramSourceOffset(i));
   for(unsigned i=0;i<8;i++)p->countBase[i]=BootstrapIO::ResidentNocRead(PioneerQ38PanelCounter(i));
   for(unsigned i=0;i<10;i++)p->command[i]=BootstrapIO::ResidentNocRead(i*4);
   peer[current]=std::move(p);bootCount++;
  }else{rootTerminal=false;root=std::make_unique<PersistentRoot>(GraphImage,*this);rootCount++;}
 }
 void Pause(){now++;}
};
PersistentRoot::PersistentRoot(const char* path,PersistentGraphIO& endpoint):GraphRootMachine(path),io(endpoint){
 memory.clear();gddr=io.gddr;
 for(unsigned a=0x6e000;a<0x70000;a+=4)memory[a]=io.BootstrapIO::Q38PanelRead(a);
 for(unsigned a=0x74000;a<0xb0000;a+=4)memory[a]=io.BootstrapIO::Q38PanelRead(a);
 for(unsigned i=0;i<PioneerQ38FfnGraphCodeWords(0);i++){
  uint32_t a=PioneerQ38FfnGraphCodeAddress(0,i),v=io.RawRead(0,a);
  if(Range(a,0x68000,unsigned(graph.size())))for(unsigned b=0;b<4;b++)graph[a-0x68000+b]=uint8_t(v>>(b*8));
  else if(Range(a,0x64000,unsigned(prefix.size())))for(unsigned b=0;b<4;b++)prefix[a-0x64000+b]=uint8_t(v>>(b*8));
  if(a==0x6000 || CodeAddress(a))l1[Index(a)]=v;
 }
 for(unsigned i=0;i<TT_Q38_FFN_GRAPH_WORDS;i++)fixture.manifest[i]=memory.at(TT_Q38_FFN_GRAPH_MANIFEST+i*4);
 for(unsigned w=0;w<119;w++)for(unsigned i=0;i<16;i++)fixture.source[w*16+i]=memory.at(0x7d800+w*64+i*4);
 for(unsigned i=0;i<16;i++)rootSource[i]=io.BootstrapIO::ResidentNocRead(BlackholeDramSourceOffset(i));
 for(unsigned i=0;i<8;i++)counts[i]=io.BootstrapIO::ResidentNocRead(PioneerQ38PanelCounter(i));
 for(unsigned i=0;i<10;i++)command[i]=io.BootstrapIO::ResidentNocRead(i*4);
 for(unsigned i=0;i<3;i++)l1[Index(0x17ffc0+i*4)]=io.BootstrapIO::RuntimeStatusRead(i);
 assert(PioneerQ38FfnGraphManifest(fixture.manifest.data(),fixture.source.data()));
}
void PersistentRoot::Packet(){
 bool write=command[7]==0x212;unsigned route=write?command[5]:command[2];
 io.packetCount++;
 if(route==fixture.manifest[8]){gddr=io.gddr;GraphRootMachine::Packet();io.gddr=gddr;return;}
 assert(pending && !unknown && route!=1555 && !command[1] && !command[4] && !command[6] && !command[9]);
 assert((write?command[2]:command[5])==129 && (write || !command[7]));
 unsigned w=Worker(route),local=write?command[0]:command[3],remote=write?command[3]:command[0],bytes=command[8];
 assert(w && w<120 && local>=0x75000 && local+bytes<=0x75800 && !((local^remote)&31));auto& p=*io.peer[w];
 if(write){assert(p.pc==TT_Q38_PEER_WAIT_LOAD && p.prepared==p.verified);
  for(unsigned i=0;i<bytes/4;i++)p.memory[remote+i*4]=memory.at(local+i*4);
  if(remote==0x74100){unsigned role=memory.count(0x7f914)?memory.at(0x7f914):0;p.Prepare(memory,role,fixture);stages[p.operation]++;}
 }else if(remote==0x74140){assert(bytes==64);io.AdvancePeer(w);
  for(unsigned i=0;i<16;i++)memory[local+i*4]=p.memory.at(remote+i*4);
 }else if(remote==0xffb13140){assert(bytes==4);memory[local]=p.pc;}
 else if(remote==0xffb121b0){assert(bytes==4);memory[local]=io.core[w]->coreReset;}
 else if(remote==0x83f00){assert(p.prepared==p.verified && bytes==(p.operation==1?256+p.groups*64:256));
  for(unsigned i=0;i<64;i++)memory[local+i*4]=p.memory.at(remote+i*4);
  if(p.operation==1){auto* m=fixture.manifest.data()+PioneerQ38FfnGraphRoleOffset(p.role);unsigned first=m[16+119*8+w-1];
   for(unsigned i=0;i<p.groups*16;i++){memory[local+256+i*4]=p.buffers[4096+i];io.numeric[p.role].at(first+i)=p.buffers[4096+i];}
   io.matrixCount++;
  }else if(p.operation==3 || p.operation==4)io.producerCount++;
 }else{
  assert(bytes==4);bool matched=false;
  for(unsigned i=0;i<16;i++)if(remote==0xffb20000+BlackholeDramSourceOffset(i)){memory[local]=p.source[i];matched=true;}
  for(unsigned i=0;i<8;i++)if(remote==0xffb20000+PioneerQ38PanelCounter(i)){memory[local]=p.Count(i);matched=true;}
  assert(matched);
 }
 counts[write?4:0]++;counts[write?5:1]+=(bytes+63)/64;counts[write?7:3]++;counts[2]++;responses++;pending=false;
}
template<class Endpoint=PersistentGraphIO> struct PersistentFixtureT {
 Endpoint io;
 std::unique_ptr<PioneerQ38DispatchLease> cohort{new PioneerQ38DispatchLease};
 std::unique_ptr<PioneerQ38FfnGraphCodeOwner> code{new PioneerQ38FfnGraphCodeOwner};
 std::unique_ptr<PioneerQ38FfnPersistentGraphOwner> graphOwner{new PioneerQ38FfnPersistentGraphOwner};
 std::unique_ptr<PioneerQ38GraphMutableBanks> mutableOwner{new PioneerQ38GraphMutableBanks};
 std::unique_ptr<PioneerQ38CanonicalModelPlan> plan{new PioneerQ38CanonicalModelPlan};
 std::unique_ptr<PioneerQ38FfnCanonicalJobs> jobs{new PioneerQ38FfnCanonicalJobs};
 std::vector<std::unique_ptr<PanelOwners>> owners;
 tt_dram_samples_result proof;PioneerResidentBanks banks;
 std::vector<uint8_t> learned=std::vector<uint8_t>(PioneerResidentBanks::Pages,0);
 PioneerQ38FfnIdentity id={1,2,3,4};PioneerQ38FfnPersistentGraphResult result;
 PioneerQ38FfnZeroLease zeroLease;
 PioneerOutputBufferLease zeroTransfer;PioneerBulkUploadLease zeroBulk;PioneerWideOutputLease zeroWide;
 tt_host_page_result zeroHost={};
 uint32_t values[5120]={},output[5120]={};
 PersistentFixtureT(const char* zeroImage){
  puts("PERSISTENT_TEST_CONSTRUCT:real canonical joint owner then frozen zero");
  proof=BankProof(*io.core[0]);assert(banks.Bind(proof) && plan->Build());
  for(unsigned layer=0;layer<2;layer++)for(unsigned role=0;role<3;role++)for(unsigned stripe=0;stripe<7;stripe++){
   const auto& p=*plan->FfnStripe(layer,role,stripe);unsigned at=p.bank*unsigned(PioneerResidentBanks::Capacity/4096)+p.offset/4096;
   for(unsigned page=0;page<p.allocated/4096;page++)learned[at+page]=1;
  }
  assert(mutableOwner->BindCanonical(banks,proof,id,learned.data(),unsigned(learned.size()),*plan,0));
  // Initialization seal is minted ONLY by the SAME actual frozen zero
  // controller+machine,not set by a Boolean/test-only private friend.
  FfnZeroControlIO zero(zeroImage);PanelOwners z(zero);auto& transfer=zeroTransfer;
  auto& bulk=zeroBulk;auto& wide=zeroWide;auto& lease=zeroLease;PioneerQ38FfnZeroResult r;
  assert(PioneerPrepareOutputBuffer(zero,z.host,transfer) && PioneerBulkUploadPrepare(zero,proof,z.noc,z.host,z.store,transfer,bulk)
   && PioneerPrepareWideOutput(zero,z.host,transfer,wide));auto* state=mutableOwner->Ffn(id);assert(state);zero.ffnLease=&lease;
  assert(PioneerQ38FfnZeroPrepare(zero,proof,z.noc,*state,banks,id,z.host,z.store,bulk,transfer,wide,lease)
   && PioneerQ38FfnZeroRun(zero,proof,z.noc,*state,banks,z.host,z.store,bulk,transfer,wide,lease,id,0,0,74,r)
   && PioneerQ38FfnZeroClose(zero,proof,z.noc,*state,banks,z.host,z.store,bulk,transfer,wide,lease,true));
  assert(state->InitializationPublished(id));zeroHost=z.host;io.gddr=zero.lastDeviceWords;
  puts("PERSISTENT_TEST_ZERO_CLOSED:prepare actual original120 snapshots");
  for(unsigned w=0;w<120;w++){owners.emplace_back(new PanelOwners(*io.core[w]));auto& n=cohort->worker[w];n.master=&owners[w]->store;
   n.noc=owners[w]->noc;if(w)n.noc.version=PioneerWorkerVersion(w);assert(PioneerQ38DispatchPrepareRole(io,proof,banks,*cohort,w));}
  assert(jobs->Build(*plan,0,0));
  io.expectedGraph=mutableOwner.get();io.expectedPlan=plan.get();io.expectedIdentity=id;io.expectedLedger=learned.data();io.expectedPages=unsigned(learned.size());
  io.result=&result;io.state=nullptr;io.track=true;
  for(unsigned role=0;role<3;role++)io.numeric[role].assign(role==2?5120:17408,0x7fc00001);
  for(unsigned i=0;i<5120;i++)values[i]=Bits(1.0f+float((i*17)%127)/256);
 }
 bool Prepare(){return graphOwner->Prepare(io,proof,banks,*cohort,*code,*mutableOwner,*plan,id,learned.data(),unsigned(learned.size()),*jobs,0,0,result);}
 bool Run(unsigned layer=0,uint64_t token=0){return graphOwner->Run(io,*jobs,layer,token,values,result);}
 bool Close(){return graphOwner->Close(io,result);}
 bool NativeMatches()const{return graphOwner->MatchesNativeOwner(proof,banks,*cohort,*code,*mutableOwner,*plan,id,learned.data(),unsigned(learned.size()));}
};
using PersistentFixture=PersistentFixtureT<>;
template<class Endpoint> [[maybe_unused]] static void PersistentOuterEnd(PersistentFixtureT<Endpoint>& f){
 assert(f.graphOwner->Closed() && f.code->Closed());
 f.io.track=false;assert(PioneerQ38DispatchClose(f.io,f.proof,f.banks,*f.cohort,true));
 assert(PioneerQ38DispatchPeerLeaseEnd(f.io,f.cohort->worker[0].noc,f.cohort->worker[0].Store()));
 for(unsigned w=0;w<120;w++){
  auto& c=*f.io.core[w];auto& n=f.cohort->worker[w];
  assert(c.coreReset==0x47800 && !n.image.running && !n.image.ready && !n.image.unsafe && !n.image.dirty
   && !n.Store().ready && !n.Store().recovery_required && c.entry==c.saved[0]);
  for(unsigned i=0;i<TT_Q38_PANEL_VIEW_WORDS;i++)assert(c.view[i]==c.saved[1+i]);
 }
}
#ifndef PIONEER_Q38_PERSISTENT_GRAPH_FIXTURE_ONLY
int main(int argc,char** argv){
 assert(argc==4);GraphImage=argv[1];PeerImage=argv[2];setvbuf(stdout,nullptr,_IOLBF,0);
 std::signal(SIGABRT,[](int){fprintf(stderr,"LAST_PEER_LOAD worker%u PC%08x address%08x\n",LastWorker,LastPc,LastLoad);std::_Exit(134);});
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;(void)&FreshInputs;
 PersistentFixture f(argv[3]);puts("PERSISTENT_TEST_PREPARE");assert(f.Prepare());unsigned installed=f.result.installedWords;
 std::array<uint32_t,5120> firstOutput={};
 assert(f.io.bootCount==119 && installed==867448 && !f.result.restoredWords && f.graphOwner->Ready() && f.NativeMatches());
 for(unsigned pass=0;pass<2;pass++){
  if(pass){f.jobs.reset(new PioneerQ38FfnCanonicalJobs);assert(f.jobs->Build(*f.plan,1,1));
   for(unsigned i=0;i<5120;i++)f.values[i]=Bits(.5f+float((i*31+17)%127)/512);
   for(unsigned role=0;role<3;role++)std::fill(f.io.numeric[role].begin(),f.io.numeric[role].end(),0x7fc00001);
  }
  const unsigned inputReadsBefore=f.io.inputStageReads,manifestReadsBefore=f.io.manifestStageReads;
  bool okay=f.Run(pass,pass);if(!okay)fprintf(stderr,"PERSISTENT_GRAPH_FAIL pass%u phase%u worker%u word%u rootPC%08x marker%08x statsR%u/W%u polls%u/%u;NO_POSTFAULT_IO\n",
   pass,f.result.phase,f.result.worker,f.result.word,f.io.root?f.io.root->pc:0,f.io.core[0]->destination[2034],
   f.io.root?f.io.root->memory.at(TT_Q38_FFN_GRAPH_STATS+24):0,f.io.root?f.io.root->memory.at(TT_Q38_FFN_GRAPH_STATS+28):0,
   f.io.root?f.io.root->memory.at(TT_Q38_FFN_GRAPH_STATS+40):0,f.io.root?f.io.root->memory.at(TT_Q38_FFN_GRAPH_STATS+44):0);
  assert(okay && f.graphOwner->CopyOutput(f.io,f.output,f.result));
  for(unsigned i=0;i<5120;i++)assert(f.output[i]==f.io.numeric[2][i] && isfinite(Float(f.output[i])) && Float(f.output[i])>0);
  if(!pass){for(unsigned i=0;i<5120;i++)firstOutput[i]=f.output[i];}
  else assert(memcmp(firstOutput.data(),f.output,sizeof(f.output))!=0);
  assert(f.io.bootCount==119 && f.result.installedWords==installed && !f.result.restoredWords && f.result.runs==pass+1 && f.NativeMatches());
  assert(f.io.inputStageReads-inputReadsBefore==5120 && f.io.manifestStageReads-manifestReadsBefore==TT_Q38_FFN_GRAPH_WORDS);
#ifdef PIONEER_Q38_FFN_SEALED_PROOF
  assert(!f.io.workManifestPostReads);
  printf("PASS sealed work proof pass%u:ZERO duplicate CPU manifest post-reads;initial5120input+3456manifest readbacks retained;NO_NATIVE_SPEED_CLAIM\n",pass);
#else
  assert(f.io.workManifestPostReads==(pass+1)*2*TT_Q38_FFN_GRAPH_WORDS);
#endif
#ifdef PIONEER_Q38_FFN_HOST_PROFILE
  assert(f.result.hostProfile.Complete() && f.result.hostProfile.polls && !f.result.hostProfile.invalid);
  printf("PASS CPU phase attribution pass%u synthetic_total%llu polls%u;FIXTURE_CLOCK_NOT_NATIVE_MICROSECONDS\n",pass,
   (unsigned long long)f.result.hostProfile.Total(),f.result.hostProfile.polls);
#endif
  printf("PASS ACTUAL persistent owner FFN pass%u rootR%u/W%u flits%u/%u,119 peers kept running;NOT_NATIVE_SPEED\n",pass,f.result.rootReads,f.result.rootWrites,f.result.rootReadFlits,f.result.rootWriteFlits);
 }
#ifdef PIONEER_Q38_FFN_HOST_PROFILE
 auto profileBeforeStop=f.result.hostProfile;
#endif
 assert(f.Close() && f.graphOwner->Closed() && f.code->Closed() && !f.result.unsafe && f.result.closed
  && f.io.bootCount==119 && f.io.rootCount==3 && f.result.restoredWords==867448 && f.result.stopped==119 && f.result.held==119);
 assert(f.io.stopManifestPostReads==2*TT_Q38_FFN_GRAPH_WORDS);// NEVER elide STOP/CLOSE proof
#ifdef PIONEER_Q38_FFN_HOST_PROFILE
 assert(memcmp(&profileBeforeStop,&f.result.hostProfile,sizeof(profileBeforeStop))==0);// STOP must not corrupt the last FFN attribution
#endif
 PersistentOuterEnd(f);
 puts("PASS ACTUAL persistent owner+QFG2ROOT+ALL119QFP2 TWO fullFFNs:one867448word install/119boots,changed input/layer/token,176real producers/79872rows,intermediates stay GDDR,STOP+ALL physical terminals BEFORE FIRSThold,exact restoration only at CLOSE,original parentCLOSE+root residentEND restore ALL120outer snapshots;SYNTHETIC_WEIGHTS_AND_ENDPOINTS_NOT_NATIVE_AUTHORITY_OR_MODEL_SPEED");
}
#endif
