/* SPDX-License-Identifier: MIT */
// SAME controller + actual installed QFG2 ROOT and ALL119 QFP2 instruction
// machines. Only PCI/MMIO/L1 fabric is simulated. No manufactured status,
// proof,sequence,root stats or completion. STOP-only;NOT model/speed/native.
#define PIONEER_Q38_FFN_ROOT_LIFETIME_FIXTURE_ONLY
#include "pioneer_q38_ffn_root_lifetime_test.cpp"
#define PIONEER_Q38_FFN_PERSISTENT_MACHINE_ONLY
#include "pioneer_q38_ffn_persistent_image_test.cpp"
#define PIONEER_Q38_DISPATCH_BASE_ALREADY_INCLUDED
#define PIONEER_Q38_FFN_GRAPH_MACHINE_ONLY
#include "pioneer_q38_ffn_graph_image_test.cpp"
struct ActualRootLifetimeIO;
struct LifetimeRootMachine:GraphRootMachine {
 ActualRootLifetimeIO& io;
 LifetimeRootMachine(const char* path,ActualRootLifetimeIO& endpoint);
 void Packet()override;
};
static const char* RootLifetimeImagePath=nullptr;
static const char* RootLifetimePeerPath=nullptr;
static unsigned RootLifetimeAdvanceBatch=4096,RootLifetimePeerBatch=64;
static unsigned RootLifetimeCounterFaultPacket=0;
struct ActualRootLifetimeIO:RootLifetimeIO {
 std::array<std::unique_ptr<FfnPersistentPeer>,120> peer;
 std::unique_ptr<LifetimeRootMachine> root;
 unsigned actualPeerStarts=0,actualRootStarts=0,actualHolds=0;
 static void Counters(PanelIO& c,const uint32_t* count) {
  c.counters[0]=count[0];c.counters[1]=count[1];c.accepted=count[2];c.counters[3]=count[3];
  c.writeCounters[0]=count[4];c.writeCounters[1]=count[5];c.writeCounters[3]=count[7];
 }
 void SyncPeer(unsigned w) {
  auto& p=*peer[w];auto& c=*core[w];
  for(unsigned a=0x70000;a<0x70b00;a+=4)if(p.memory.count(a))c.view[(a-TT_Q38_PANEL_VIEW_BASE)/4]=p.memory.at(a);
  for(unsigned a=0x74000;a<0x74180;a+=4)c.view[(a-TT_Q38_PANEL_VIEW_BASE)/4]=p.memory.at(a);
  uint32_t count[8];for(unsigned i=0;i<8;i++)count[i]=p.Count(i);assert(count[2]==count[6]);Counters(c,count);
  for(unsigned i=0;i<10;i++)c.command[i]=p.command[i];
 }
 void AdvancePeer(unsigned w) {
  if(!track || !w || !peer[w] || core[w]->coreReset!=0x46800)return;
  auto& p=*peer[w];
  for(unsigned i=0;i<RootLifetimePeerBatch;i++) {
   if(p.pc==TT_Q38_PEER_STOP || p.pc==TT_Q38_PEER_FAULT_PARK)break;
   if(p.pc==TT_Q38_PEER_WAIT_LOAD && !p.memory.at(0x74100))break;
   p.StepBlock();
  }
  assert(!p.requests && !p.responses && !p.pending && !p.matrixInstructions && !p.activationPackets);SyncPeer(w);
 }
 void AdvanceRoot() {
  if(!track || !root || core[0]->coreReset!=0x46800)return;
  auto& p=*root;
  for(unsigned i=0;i<RootLifetimeAdvanceBatch;i++) {
   if(p.pc==TT_Q38_DISPATCH_PARK || p.pc==TT_Q38_DISPATCH_FAULT_PARK)break;
   p.StepBlock();
  }
  auto& c=*core[0];Counters(c,p.counts);for(unsigned i=0;i<10;i++)c.command[i]=p.command[i];
  for(unsigned i=0;i<3;i++)c.destination[2032+i]=p.l1[p.Index(0x17ffc0+i*4)];
  if(p.pc==TT_Q38_DISPATCH_PARK || p.pc==TT_Q38_DISPATCH_FAULT_PARK) {
   for(const auto& v:p.memory)if((v.first>=0x6e000 && v.first<0x70000) || (v.first>=0x74000 && v.first<0x80000))
    c.view[(v.first-TT_Q38_PANEL_VIEW_BASE)/4]=v.second;
   rootDone=p.pc==TT_Q38_DISPATCH_PARK;
  }
 }
 void ReleasedPeer()override {
  assert(current && !peer[current] && life && life->armed==119 && RootLifetimePeerPath);
  auto p=std::make_unique<FfnPersistentPeer>(RootLifetimePeerPath,12,5120,1,current);
  for(unsigned i=0;i<PioneerQ38FfnGraphCodeWords(current);i++) {
   uint32_t a=PioneerQ38FfnGraphCodeAddress(current,i),v=RawRead(current,a);
   if(a==0x6000 || p->CodeAddress(a))p->l1[p->Index(a)]=v;else p->memory[a]=v;
  }
  for(unsigned a=0x74000;a<0x74180;a+=4)p->memory[a]=BootstrapIO::Q38PanelRead(a);
  for(unsigned i=0;i<16;i++)p->source[i]=BootstrapIO::ResidentNocRead(BlackholeDramSourceOffset(i));
  for(unsigned i=0;i<8;i++)p->countBase[i]=BootstrapIO::ResidentNocRead(PioneerQ38PanelCounter(i));
  for(unsigned i=0;i<10;i++)p->command[i]=BootstrapIO::ResidentNocRead(i*4);
  peer[current]=std::move(p);actualPeerStarts++;
 }
 void ReleasedRoot()override {
  assert(!current && !root && life && life->ready==119 && life->rootStarted && RootLifetimeImagePath);
  root=std::make_unique<LifetimeRootMachine>(RootLifetimeImagePath,*this);actualRootStarts++;
 }
 void Pause(){now++;} // Simulated monotonic clock,NOT device time.
 uint32_t CurrentPc()override {if(current){AdvancePeer(current);return peer[current]->pc;}AdvanceRoot();return root->pc;}
 uint32_t RuntimePcRead(){uint32_t pc=RootLifetimeIO::RuntimePcRead();
  if(RootLifetimeCounterFaultPacket && !current && pc>=0x72060 && pc<0x72080)observed=touches;
  return pc;}
 uint32_t Q38PanelRead(uint32_t a){if(current)AdvancePeer(current);return RootLifetimeIO::Q38PanelRead(a);}
 uint32_t RuntimeStatusRead(unsigned i){if(!current)AdvanceRoot();uint32_t value=RootLifetimeIO::RuntimeStatusRead(i);
  if(RootLifetimeCounterFaultPacket && !current && i==2 && value && value!=TT_Q38_FFN_GRAPH_MARKER)observed=touches;
  return value;}
 void RuntimeResetWrite(uint32_t value) {
  if(track && life && life->phase==8 && value==0x87ffff) {
   // Independent actual ROOT+ALL119 PCs/status BEFORE the FIRST hold,
   // not the controller's reported rootCompleted/stopped booleans.
   assert(root && root->pc==TT_Q38_DISPATCH_PARK && !root->pending && !root->unknown);
   for(unsigned w=1;w<120;w++)assert(peer[w] && peer[w]->pc==TT_Q38_PEER_STOP
    && peer[w]->memory.at(0x74140)==1 && peer[w]->memory.at(0x74144)==1 && peer[w]->memory.at(0x74150)==2);
   actualHolds++;
  }
  RootLifetimeIO::RuntimeResetWrite(value);
 }
};
LifetimeRootMachine::LifetimeRootMachine(const char* path,ActualRootLifetimeIO& endpoint):GraphRootMachine(path),io(endpoint) {
 assert(!io.current);memory.clear(); // NO artifact constructor's model/job defaults survive.
 for(unsigned a=0x6e000;a<0x70000;a+=4)memory[a]=io.BootstrapIO::Q38PanelRead(a);
 for(unsigned a=0x74000;a<0x80000;a+=4)memory[a]=io.BootstrapIO::Q38PanelRead(a);
 // Fetch EXACT installed bytes. The graph interpreter's extended ROM is
 // backed separately,so update that storage too rather than artifact data.
 for(unsigned i=0;i<PioneerQ38FfnGraphCodeWords(0);i++) {
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
 assert(fixture.manifest[5]==2 && PioneerQ38FfnGraphManifest(fixture.manifest.data(),fixture.source.data()));
}
void LifetimeRootMachine::Packet() {
 assert(pending && !unknown && !command[1] && !command[4] && !command[6] && !command[9]);
 bool write=command[7]==0x212;unsigned route=write?command[5]:command[2];
 assert(route!=fixture.manifest[8] && route!=1555 && (write?command[2]:command[5])==129 && (write || !command[7]));
 unsigned w=Worker(route),local=write?command[0]:command[3],remote=write?command[3]:command[0],bytes=command[8];
 assert(w && w<120 && local>=0x75000 && local+bytes<=0x75800 && !((local^remote)&31));
 auto& p=*io.peer[w];
 if(write) {
  assert(p.pc==TT_Q38_PEER_WAIT_LOAD && ((remote==0x74000 && bytes==64) || (remote==0x74100 && bytes==4)));
  for(unsigned i=0;i<bytes/4;i++)p.memory[remote+i*4]=memory.at(local+i*4);
 }else if(remote==0x74140) {
  assert(bytes==64);io.AdvancePeer(w);
  for(unsigned i=0;i<16;i++)memory[local+i*4]=p.memory.at(remote+i*4);
 }else if(remote==0xffb13140) {assert(bytes==4);memory[local]=p.pc;}
 else if(remote==0xffb121b0) {assert(bytes==4);memory[local]=io.core[w]->coreReset;}
 else {
  assert(bytes==4);bool matched=false;
  for(unsigned i=0;i<16;i++)if(remote==0xffb20000+BlackholeDramSourceOffset(i)){assert(!matched);memory[local]=p.source[i];matched=true;}
  for(unsigned i=0;i<8;i++)if(remote==0xffb20000+PioneerQ38PanelCounter(i)){assert(!matched || i==6);memory[local]=p.Count(i);matched=true;}
  assert(matched && "NO synthesized proof/GDDR/matrix/numeric data reads in STOP-only fabric");
 }
 counts[write?4:0]++;counts[write?5:1]+=(bytes+63)/64;counts[write?7:3]++;counts[2]++;responses++;pending=false;
 if(packets==RootLifetimeCounterFaultPacket)counts[0]++; // Real packet + unrelated physical counter traffic.
}
int main(int argc,char** argv) {
 assert(argc>=3 && argc<=5);RootLifetimeImagePath=argv[1];RootLifetimePeerPath=argv[2];
 if(argc>=4){RootLifetimeAdvanceBatch=unsigned(strtoul(argv[3],nullptr,10));RootLifetimePeerBatch=17;}
 if(argc==5)RootLifetimeCounterFaultPacket=unsigned(strtoul(argv[4],nullptr,10));
 assert(RootLifetimeAdvanceBatch && RootLifetimeAdvanceBatch<=8192);setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;(void)&FreshInputs;
 RootLifetimeFixture<ActualRootLifetimeIO> f;
 for(unsigned w=0;w<120;w++){auto& c=*f.io.core[w];c.accepted=UINT32_MAX-w;c.counters[0]=UINT32_MAX-2-w;c.counters[1]=0xfff00000+w;}
 bool okay=f.Run();if(RootLifetimeCounterFaultPacket) {
  assert(!okay && f.io.root && f.io.root->unknown && f.io.root->pc==TT_Q38_DISPATCH_FAULT_PARK
   && f.io.root->packets==RootLifetimeCounterFaultPacket && f.io.root->responses==RootLifetimeCounterFaultPacket
   && !f.io.actualHolds && f.io.resetWrites==240 && !f.result.rootCompleted && !f.result.stopped);
  f.Unknown();unsigned packets=f.io.root->packets,reads=f.io.root->nocReads;f.io.root->Inert();
  assert(packets==f.io.root->packets && reads==f.io.root->nocReads);
  printf("PASS OFFLINE actual ROOT+ALL119 counter-fault packet%u:devicefault park/no following packet,controller memory-only quarantine/no hold/restore/END/retry,all120 retained;SIMULATED_ENDPOINTS_NOT_NATIVE\n",RootLifetimeCounterFaultPacket);return 0;
 }
 if(!okay) {
  const auto& r=f.result;const auto* root=f.io.root.get();
  fprintf(stderr,"ACTUAL_ROOT_LIFETIME_FAILURE phase%u worker%u word%u ready%u root%u/%u stop%u held%u rootPC%08x rootSteps%u statsR%u/W%u;NO_POSTFAULT_IO\n",
   r.phase,r.worker,r.word,r.ready,r.rootStarted,r.rootCompleted,r.stopped,r.held,root?root->pc:0,root?root->steps:0,
   root?root->memory.at(TT_Q38_FFN_GRAPH_STATS+6*4):0,root?root->memory.at(TT_Q38_FFN_GRAPH_STATS+7*4):0);
 }
 assert(okay && f.result.passed && f.lifetime->Closed() && f.code->Closed() && !f.result.unsafe
  && f.io.actualPeerStarts==119 && f.io.actualRootStarts==1 && f.io.actualHolds==120 && f.io.resetWrites==480);
 auto& root=*f.io.root;assert(root.pc==TT_Q38_DISPATCH_PARK && !root.pending && !root.unknown && root.stages[1]==0 && !root.nativeLoads);
 uint64_t steps=0;unsigned minStack=root.minStack;
 for(unsigned w=1;w<120;w++){const auto& p=*f.io.peer[w];assert(p.pc==TT_Q38_PEER_STOP && !p.requests && !p.responses && !p.pending
  && !p.matrixInstructions && !p.activationPackets && f.io.core[w]->coreReset==0x47800 && !f.cohort->worker[w].image.running);
  steps+=p.steps;minStack=std::min(minStack,p.minStack);}
 assert(f.io.core[0]->coreReset==0x47800 && !f.cohort->rootRunning);
 printf("PASS OFFLINE actual QFG2ROOT+ALL119QFP2 lifetime:interleave%u/%u rootVM%u peerVM%llu maxStack%u observedRootReads%u/Writes%u;actual installed bytes/boot+STOP/current ROOT+ALL119physical terminals BEFORE FIRSThold/exact867448program+11424peer+12288rootmeta+2048rootstack restore/wrapped counters preserved NOT restored;NO_GDDR_MODEL_HOSTDMA_MATRIX_NATIVE_FACTORY_OR_SPEED_AUTHORITY\n",
  RootLifetimeAdvanceBatch,RootLifetimePeerBatch,root.steps,(unsigned long long)steps,0x70000-minStack,f.result.rootReads,f.result.rootWrites);
}
