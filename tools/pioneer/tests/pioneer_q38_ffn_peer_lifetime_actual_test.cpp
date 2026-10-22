/* SPDX-License-Identifier: MIT */
// Same ownership controller driven by ALL119 ACTUAL QFP2 interpreters.
// Only MMIO/L1/PCI endpoints are simulated. Never synthesize ready/STOP.
#define PIONEER_Q38_TEST_QUALIFIED_LEGACY
#define PIONEER_Q38_FFN_PEER_LIFETIME_FIXTURE_ONLY
#include "pioneer_q38_ffn_peer_lifetime_test.cpp"
#define PIONEER_Q38_FFN_PERSISTENT_MACHINE_ONLY
#include "pioneer_q38_ffn_persistent_image_test.cpp"
static const char* PeerLifetimeImagePath=nullptr;
static unsigned PeerLifetimeAdvanceBatch=64;
struct ActualPeerLifetimeIO:PeerLifetimeIO {
 std::array<std::unique_ptr<FfnPersistentPeer>,120> peer;
 unsigned observedStarts=0,observedStops=0;
 void Advance() {
  if(!track || !current || !peer[current] || C().coreReset!=0x46800)return;
  auto& p=*peer[current];
  for(unsigned i=0;i<PeerLifetimeAdvanceBatch;i++) {
   if(p.pc==TT_Q38_PEER_STOP || p.pc==TT_Q38_PEER_FAULT_PARK)break;
   if(p.pc==TT_Q38_PEER_WAIT_LOAD && !p.memory.at(0x74100))break;
   p.StepBlock();
  }
  assert(!p.requests && !p.responses && !p.pending && !p.matrixInstructions && !p.activationPackets);
  // Device-written scratch comes ONLY from actual instructions,not test
  // status fields. Code bytes remain the independently installed image.
  for(unsigned a=0x70000;a<0x70b00;a+=4)if(p.memory.count(a))C().view[(a-TT_Q38_PANEL_VIEW_BASE)/4]=p.memory.at(a);
  for(unsigned a=0x74000;a<0x74180;a+=4)C().view[(a-TT_Q38_PANEL_VIEW_BASE)/4]=p.memory.at(a);
 }
 void Released()override {
  assert(current && !peer[current] && PeerLifetimeImagePath && life && life->armed==119);
  auto p=std::make_unique<FfnPersistentPeer>(PeerLifetimeImagePath,12,5120,1,current);
  // Fetch the image ACTUALLY installed by the controller,not merely the
  // matching artifact constructor. Host-staged frames override ALL old
  // fixture defaults. Do NOT call BootNew/Job/Stop test staging helpers.
  for(unsigned i=0;i<PioneerQ38FfnGraphCodeWords(current);i++) {
   uint32_t a=PioneerQ38FfnGraphCodeAddress(current,i),v=RawRead(current,a);
   if(a==0x6000 || p->CodeAddress(a))p->l1[p->Index(a)]=v;else p->memory[a]=v;
  }
  for(unsigned a=0x74000;a<0x74180;a+=4)p->memory[a]=BootstrapIO::Q38PanelRead(a);
  for(unsigned i=0;i<16;i++)p->source[i]=BootstrapIO::ResidentNocRead(BlackholeDramSourceOffset(i));
  for(unsigned i=0;i<8;i++)p->countBase[i]=BootstrapIO::ResidentNocRead(PioneerQ38PanelCounter(i));
  for(unsigned i=0;i<10;i++)p->command[i]=BootstrapIO::ResidentNocRead(i*4);
  peer[current]=std::move(p);observedStarts++;
 }
 void Stopped()override{} // NEVER fabricate status;actual worker must run
 void Pause(){now++;} // synthetic monotonic clock,NEVER native timing evidence
 uint32_t CurrentPc()override {Advance();assert(peer[current]);return peer[current]->pc;}
 uint32_t Q38PanelRead(uint32_t a){Advance();return PeerLifetimeIO::Q38PanelRead(a);}
 void Q38PanelWrite(uint32_t a,uint32_t v){PeerLifetimeIO::Q38PanelWrite(a,v);
  if(track && life && (life->phase==6 || life->phase==8) && current && peer[current]){
   assert(!peer[current]->pending);peer[current]->memory[a]=v;}}
 void RuntimeResetWrite(uint32_t v) {
  if(track && life && life->phase==7 && v==0x87ffff) {
   // Independent ALL119 actual terminal PCs BEFORE the FIRST hold,not
   // controller result counts or synthetic ready/stopped flags.
   for(unsigned w=1;w<120;w++)assert(peer[w] && peer[w]->pc==TT_Q38_PEER_STOP
    && peer[w]->memory.at(0x74140)==1 && peer[w]->memory.at(0x74144)==1 && peer[w]->memory.at(0x74150)==2);
   observedStops++;
  }
  PeerLifetimeIO::RuntimeResetWrite(v);
 }
};
int main(int argc,char** argv) {
 assert(argc==2 || argc==3);PeerLifetimeImagePath=argv[1];
 if(argc==3)PeerLifetimeAdvanceBatch=unsigned(strtoul(argv[2],nullptr,10));assert(PeerLifetimeAdvanceBatch && PeerLifetimeAdvanceBatch<=512);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;(void)&FreshInputs;
 PeerLifetimeFixture<ActualPeerLifetimeIO> f;auto prefix=f.io.prefix;
 // Nonzero/wrapped private MASTER baselines are copied into actual peers.
 for(unsigned w=0;w<120;w++){auto& c=*f.io.core[w];c.accepted=UINT32_MAX-w;c.counters[0]=UINT32_MAX-2-w;c.counters[1]=0xfff00000+w;}
 bool okay=f.Run();if(!okay) {
  const auto& r=f.result;const auto* p=f.io.peer[r.worker].get();
  fprintf(stderr,"ACTUAL_LIFETIME_FAILURE phase%u worker%u word%u arm%u release%u ready%u stop%u stopped%u held%u pc%08x steps%u status[%08x,%08x,%08x,%08x,%08x] reset%08x;HOST_MEMORY_ONLY_NO_POSTFAULT_IO\n",
   r.phase,r.worker,r.word,r.armed,r.released,r.ready,r.stopIssued,r.stopped,r.held,p?p->pc:0,p?p->steps:0,
   p?p->memory.at(0x74140):0,p?p->memory.at(0x74144):0,p?p->memory.at(0x74148):0,p?p->memory.at(0x7414c):0,p?p->memory.at(0x74150):0,f.io.core[r.worker]->coreReset);
 }
 assert(okay && f.result.passed && !f.result.unsafe && f.lifetime->Closed() && f.code->Closed()
  && f.io.observedStarts==119 && f.io.observedStops==119 && f.io.prefix==prefix);
 uint64_t steps=0;unsigned minStack=0x70000;
 for(unsigned w=1;w<120;w++) {
  const auto& p=*f.io.peer[w];assert(p.pc==TT_Q38_PEER_STOP && !p.requests && !p.responses && !p.pending && !p.matrixInstructions
   && !p.activationPackets && f.io.core[w]->coreReset==0x47800 && !f.cohort->worker[w].image.running);
  steps+=p.steps;minStack=std::min(minStack,p.minStack);
 }
 assert(f.io.core[0]->coreReset==0x47800 && !f.cohort->worker[0].image.running && f.result.held==119);
 printf("PASS OFFLINE actual ownership+ALL119 QFP2 instruction workers:interleave%u/boot+seq1STOP/current physical terminal audits BEFORE first hold/867448program+11424frame words snapshot-install-restore/root NEVER started/workerVM%llu maxStack%u/no NoC packets,GDDR,learned inputs,CPU products or model computation;SIMULATED_ENDPOINTS_NOT_NATIVE_FACTORY_OR_SPEED_QUALIFICATION\n",PeerLifetimeAdvanceBatch,(unsigned long long)steps,0x70000-minStack);
}
