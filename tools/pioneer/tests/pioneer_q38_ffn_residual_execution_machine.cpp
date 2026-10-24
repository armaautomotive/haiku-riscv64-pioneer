/* SPDX-License-Identifier: MIT */
// TEST actual installed QFG3/QFP2 instructions. Kernel Scope/claim/holds/
// restore authority is supplied by the ACTUAL native factory,not this VM.
#undef PIONEER_Q38_FFN_GRAPH_INPUT_ROOT
#undef PIONEER_Q38_PROJECTION_PERSISTENT
#define PIONEER_Q38_FFN_GRAPH_RESIDUAL_ROOT 1
#define PIONEER_Q38_FFN_GRAPH_COOPERATIVE_FIXTURE_ONLY
#include "pioneer_q38_ffn_graph_cooperative_test.cpp"
#include "pioneer_q38_ffn_residual_execution_machine.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionGraphFootprint.h"
struct FfnResidualExecutionMachine::Impl {
 struct Root;
 struct Peer:GraphArithmeticPeer {
  Impl& device;
  Peer(const char* image,unsigned w,Impl& d):GraphArithmeticPeer(image,w,d.fabric.Gddr(),false),device(d){}
  bool ExternalLearnedBytes()const override{return device.fabric.LearnedBytes()!=nullptr;}
  bool ReadLearnedBytes(unsigned route,uint32_t address,uint8_t* out,unsigned bytes)override{
   auto* s=device.fabric.LearnedBytes();if(!s || !s->Ready() || !s->Read(route,address,out,bytes) || !s->Ready())return false;
   if(device.faultTile && !device.faultInjected){assert(bytes==2304);out[0]=0;out[16]=0x7e;device.faultInjected=true;}
   return true; // mutated byte endpoint only;original file/cache/reference remains untouched
  }
 };
 const char* rootImage;const char* peerImage;ProjectionExecutionFabric& fabric;
 std::array<std::unique_ptr<Peer>,120> peer;
 std::unique_ptr<Root> root;
 unsigned matrices=0,producers=0,stops=0;
 bool bootObserved[120]={};
 bool faultTile=false,faultInjected=false;
 std::vector<uint32_t> lastDown,lastRaw;
 Impl(const char* r,const char* p,ProjectionExecutionFabric& f):rootImage(r),peerImage(p),fabric(f){}
 template<class VM> void Sync(unsigned w,VM& vm){
  for(unsigned i=0;i<PioneerQ38ProjectionGraphSnapshotWords(w);i++){
   unsigned a=PioneerQ38ProjectionGraphSnapshotAddress(w,i);
   if(PioneerQ38ProjectionGraphProgramImmutable(w,a))continue;
   if(a>=0x17ffc0 && a<=0x17ffc8){fabric.WriteLocal(w,a,vm.l1[vm.Index(a)]);continue;}
   auto value=vm.memory.find(a);if(value!=vm.memory.end())fabric.WriteLocal(w,a,value->second);
  }
 }
 void AdvancePeer(unsigned w){
  auto& p=*peer[w];
  if(p.pc==TT_Q38_PEER_FAULT_PARK)return;
  const unsigned steps=p.steps,verified=p.verified;
  if(p.prepared!=p.verified)p.Advance(); // actual matrix/quantizer/SiLU instructions + observers
  else if(p.pc!=TT_Q38_PEER_WAIT_LOAD && p.pc!=TT_Q38_PEER_STOP){
   for(unsigned n=0;n<8192 && p.pc!=TT_Q38_PEER_WAIT_LOAD && p.pc!=TT_Q38_PEER_STOP && p.pc!=TT_Q38_PEER_FAULT_PARK;n++)p.StepBlock();
  }
  // An idle WAIT is not an instruction or a new endpoint write. Besides
  // avoiding O(ALL119 snapshots) on every native poll,this leaves external
  // byte faults observable instead of silently rewriting them from the VM.
  const bool changed=p.steps!=steps || p.verified!=verified;
  if(changed && (p.pc==TT_Q38_PEER_WAIT_LOAD || p.pc==TT_Q38_PEER_STOP || p.pc==TT_Q38_PEER_FAULT_PARK))Sync(w,p);
  if(!bootObserved[w] && p.pc==TT_Q38_PEER_WAIT_LOAD && !p.prepared && !p.verified
   && !p.memory.at(0x74140) && !p.memory.at(0x74144) && p.memory.at(0x7414c)==TT_Q38_FFN_PERSISTENT_MARKER){
   bootObserved[w]=true;if(w==1 || w==64 || w==119)printf("NATIVE57_ACTUAL_BOOT peer%u/119 emitted marker+WAIT/private sequence0;NOT_CARD_TIME\n",w);
  }
  if(changed){uint32_t counts[8];for(unsigned i=0;i<8;i++)counts[i]=p.Count(i);fabric.SetControls(w,p.command,counts);}
 }
 void Start(unsigned);void AdvanceRoot();
};
struct FfnResidualExecutionMachine::Impl::Root:GraphArithmeticRoot {
 Impl& device;std::vector<uint32_t> raw;
 Root(Impl& d):GraphArithmeticRoot(d.rootImage,d.peerImage,true,0,false,0,0,false),device(d){
  memory.clear();
  for(unsigned i=0;i<PioneerQ38ProjectionGraphSnapshotWords(0);i++){
   unsigned a=PioneerQ38ProjectionGraphSnapshotAddress(0,i),v=d.fabric.ReadLocal(0,a);memory[a]=v;
   if(Range(a,0x68000,unsigned(graph.size())))for(unsigned b=0;b<4;b++)graph[a-0x68000+b]=uint8_t(v>>(b*8));
   if(Range(a,0x64000,unsigned(prefix.size())))for(unsigned b=0;b<4;b++)prefix[a-0x64000+b]=uint8_t(v>>(b*8));
   if(Range(a,0x72000,unsigned(code.size())))for(unsigned b=0;b<4;b++)code[a-0x72000+b]=uint8_t(v>>(b*8));
   if(a==0x6000 || (CodeAddress(a) && !Range(a,0x68000,unsigned(graph.size()))) || (a>=0x17ffc0 && a<=0x17ffc8))l1[Index(a)]=v;
  }
  for(unsigned i=0;i<TT_Q38_FFN_GRAPH_WORDS;i++)fixture.manifest[i]=memory.at(TT_Q38_FFN_GRAPH_MANIFEST+i*4);
  for(unsigned i=0;i<119*16;i++)fixture.source[i]=memory.at(0x7d800+i*4);
  for(unsigned i=0;i<16;i++)rootSource[i]=d.fabric.NoC(0,BlackholeDramSourceOffset(i));
  assert(PioneerQ38FfnGraphManifest(fixture.manifest.data(),fixture.source.data()));
  for(unsigned w=1;w<120;w++){
   assert(d.peer[w]);auto& p=*d.peer[w];auto& n=node[w-1];
   n.requested=p.memory.at(0x74140);n.sequence=p.memory.at(0x74144);n.polls=p.verified;n.stopped=p.pc==TT_Q38_PEER_STOP;
   assert(n.sequence==fixture.manifest[64+w-1] && p.prepared==p.verified);
   for(unsigned i=0;i<16;i++)n.src[i]=p.source[i];for(unsigned i=0;i<8;i++)n.counter[i]=p.Count(i);
  }
  for(unsigned role=0;role<3;role++)expected[role].assign(role==2?5120:17408,0x7fc00001);
  gddr=d.fabric.Gddr();if(fixture.manifest[5]==1)raw.assign(gddr.begin()+0x18000/4,gddr.begin()+0x18000/4+5120);
  for(unsigned i=0;i<8;i++)counts[i]=d.fabric.NoC(0,PioneerQ38PanelCounter(i));
  for(unsigned i=0;i<10;i++)command[i]=d.fabric.NoC(0,i*4);
 }
 uint32_t Load(uint32_t a)override{
  if(Range(a,0x94000,40960))return memory.at(a);
  for(unsigned i=0;i<16;i++)if(a==0xffb20000+BlackholeDramSourceOffset(i))return device.fabric.NoC(0,BlackholeDramSourceOffset(i));
  return GraphArithmeticRoot::Load(a);
 }
 void Store(uint32_t a,uint32_t v)override{
  if(Range(a,0x99000,20480)){assert(!pending && !unknown && finishedMatrices==357 && finishedProducers==88);memory[a]=v;return;}
  GraphArithmeticRoot::Store(a,v);
 }
 void RecordPeer(unsigned w){
  auto& p=*device.peer[w+1];auto& n=node[w];
  n.requested=p.memory.at(0x74140);n.sequence=p.memory.at(0x74144);n.stopped=p.pc==TT_Q38_PEER_STOP;
  if(n.sequence!=p.prepared || p.verified!=p.prepared || n.polls==p.verified)return;
  n.polls=p.verified;
  if(p.operation==1){const auto* m=fixture.manifest.data()+PioneerQ38FfnGraphRoleOffset(p.role);unsigned first=m[16+119*8+w];
   for(unsigned row=0;row<p.groups*16;row++)expected[p.role].at(first+row)=p.buffers[4096+row];
   assembled[p.role]+=p.groups*16;finishedMatrices++;device.matrices++;
   if(!w || (w+1)%16==0 || w==118)printf("NATIVE57_ACTUAL_MATRIX role%u peer%u/119 retainedSeq%u;NOT_CARD_TIME\n",p.role,w+1,p.verified);
  }else if(p.operation==3 || p.operation==4){finishedProducers++;device.producers++;}
  else{assert(p.operation==2 && n.stopped);device.stops++;}
 }
 uint32_t PeerRegister(unsigned w,unsigned a){
  auto& p=*device.peer[w+1];
  if(a==0xffb13140)return p.pc;if(a==0xffb121b0)return device.fabric.Reset(w+1);
  for(unsigned i=0;i<16;i++)if(a==0xffb20000+BlackholeDramSourceOffset(i))return device.fabric.NoC(w+1,BlackholeDramSourceOffset(i));
  for(unsigned i=0;i<8;i++)if(a==0xffb20000+p.CounterOffset(i))return p.Count(i);
  assert(false && "unqualified/reserved peer CSR endpoint");return 0;
 }
 void Packet()override{
  bool write=command[7]==0x212;unsigned route=write?command[5]:command[2];
  if(route==fixture.manifest[8]){gddr=device.fabric.Gddr();GraphRootMachine::Packet();device.fabric.Gddr()=gddr;return;}
  unsigned w=Worker(route)-1,local=write?command[0]:command[3],remote=write?command[3]:command[0],bytes=command[8];
  assert(w<119 && pending && !unknown && !command[1] && !command[4] && !command[6] && !command[9]);
  assert((write?command[2]:command[5])==129 && bytes && local>=0x75000 && local+bytes<=0x75800 && !((local^remote)&31));
  auto& p=*device.peer[w+1];
  if(write){
   assert(p.pc==TT_Q38_PEER_WAIT_LOAD && p.prepared==p.verified);
   assert((remote==0x74000 && bytes==64) || (remote==0x74200 && bytes==256) || (remote==0x74100 && bytes==4));
   for(unsigned i=0;i<bytes/4;i++)p.memory[remote+i*4]=memory.at(local+i*4);
   if(remote==0x74100){unsigned role=memory.count(0x7f914)?memory.at(0x7f914):0;p.Prepare(memory,role,fixture);stages[p.operation]++;}
  }else if(remote==0x74140){assert(bytes==64);device.AdvancePeer(w+1);RecordPeer(w);
   for(unsigned i=0;i<16;i++)memory[local+i*4]=p.memory.at(remote+i*4);
  }else if(remote==0x83f00){assert(p.prepared==p.verified && bytes==(p.operation==1?256+p.groups*64:256));
   for(unsigned i=0;i<64;i++)memory[local+i*4]=p.memory.at(remote+i*4);
   if(p.operation==1)for(unsigned i=0;i<p.groups*16;i++)memory[local+256+i*4]=p.buffers[4096+i];
  }else{assert(bytes==4);memory[local]=PeerRegister(w,remote);}
  counts[write?4:0]++;counts[write?5:1]+=(bytes+63)/64;counts[write?7:3]++;counts[2]++;responses++;pending=false;
 }
};
void FfnResidualExecutionMachine::Impl::Start(unsigned w){
 if(!w){root.reset(new Root(*this));return;}
 assert(w<120 && !peer[w]);auto p=std::make_unique<Peer>(peerImage,w,*this);
 // Unknown simulated Tensix Dst/caches are poisoned,not assumed zero or
 // initialized by a forged completion. ALL boot/frame/code/NoC history
 // comes from the actual native byte fabric BEFORE emitted execution.
 p->PoisonDst();p->Poison();p->memory.clear();
 for(unsigned i=0;i<PioneerQ38ProjectionGraphSnapshotWords(w);i++){
  unsigned a=PioneerQ38ProjectionGraphSnapshotAddress(w,i),v=fabric.ReadLocal(w,a);p->memory[a]=v;
  if(p->Range(a,0x64000,unsigned(p->activationCode.size())))for(unsigned b=0;b<4;b++)p->activationCode[a-0x64000+b]=uint8_t(v>>(b*8));
#ifdef PIONEER_Q38_TEST_INT_ACCUMULATOR
  if(p->Range(a,0x70b00,unsigned(p->affineCode.size())))for(unsigned b=0;b<4;b++)p->affineCode[a-0x70b00+b]=uint8_t(v>>(b*8));
#endif
  if(p->Range(a,0x72000,unsigned(p->code.size())))for(unsigned b=0;b<4;b++)p->code[a-0x72000+b]=uint8_t(v>>(b*8));
  if(p->CodeAddress(a))p->l1[p->Index(a)]=v;
 }
 for(unsigned i=0;i<16;i++)p->source[i]=fabric.NoC(w,BlackholeDramSourceOffset(i));
 for(unsigned i=0;i<8;i++)p->countBase[i]=p->independentBefore[i]=fabric.NoC(w,PioneerQ38PanelCounter(i));
 for(unsigned i=0;i<10;i++)p->command[i]=fabric.NoC(w,i*4);
 peer[w]=std::move(p);
}
void FfnResidualExecutionMachine::Impl::AdvanceRoot(){
 auto& p=*root;if(p.pc==TT_Q38_DISPATCH_PARK || p.pc==TT_Q38_DISPATCH_FAULT_PARK)return;
 for(unsigned i=0;i<8192 && p.pc!=TT_Q38_DISPATCH_PARK && p.pc!=TT_Q38_DISPATCH_FAULT_PARK;i++)p.StepBlock();
 fabric.SetControls(0,p.command,p.counts);
 if(p.pc==TT_Q38_DISPATCH_PARK || p.pc==TT_Q38_DISPATCH_FAULT_PARK){Sync(0,p);
  if(p.pc==TT_Q38_DISPATCH_PARK && p.fixture.manifest[5]==1){
   assert(p.finishedMatrices==357 && p.finishedProducers==88 && p.assembled[0]==17408 && p.assembled[1]==17408 && p.assembled[2]==5120);
   lastDown=p.expected[2];lastRaw=p.raw;
  }
 }
}
FfnResidualExecutionMachine::FfnResidualExecutionMachine(const char* r,const char* p,ProjectionExecutionFabric& f):implementation(new Impl(r,p,f)){}
FfnResidualExecutionMachine::~FfnResidualExecutionMachine(){delete implementation;}
void FfnResidualExecutionMachine::Start(unsigned w){implementation->Start(w);}
void FfnResidualExecutionMachine::Advance(unsigned w){if(w)implementation->AdvancePeer(w);else implementation->AdvanceRoot();}
unsigned FfnResidualExecutionMachine::Pc(unsigned w)const{return w?implementation->peer[w]->pc:implementation->root->pc;}
unsigned FfnResidualExecutionMachine::Matrices()const{return implementation->matrices;}
unsigned FfnResidualExecutionMachine::Producers()const{return implementation->producers;}
unsigned FfnResidualExecutionMachine::Stops()const{return implementation->stops;}
void FfnResidualExecutionMachine::FaultLearnedTile(){assert(!implementation->faultTile && !implementation->matrices);implementation->faultTile=true;}
bool FfnResidualExecutionMachine::FaultInjected()const{return implementation->faultInjected;}
void FfnResidualExecutionMachine::VerifyOutput(){
 auto& d=*implementation;assert(d.root && d.root->pc==TT_Q38_DISPATCH_PARK && d.lastDown.size()==5120 && d.lastRaw.size()==5120);
 assert(d.matrices==357 && d.producers==88 && d.stops==119);
 for(unsigned w=1;w<120;w++)assert(d.peer[w]->pc==TT_Q38_PEER_STOP && !d.peer[w]->dotProbeReads);
 for(unsigned i=0;i<5120;i++)assert(d.fabric.Gddr().at(274880/4+i)==d.lastDown[i]
  && d.fabric.Gddr().at(i)==Bits(Float(d.lastRaw[i])+Float(d.lastDown[i])));
}
void FfnResidualExecutionMachine::Diagnostic()const{
 const auto& d=*implementation;if(!d.root)return;const auto& p=*d.root;
 fprintf(stderr,"CAPTURED_FFN_ROOT pc%08x matrices%u producers%u stops%u packets%u\n",p.pc,d.matrices,d.producers,d.stops,p.packets);
}
