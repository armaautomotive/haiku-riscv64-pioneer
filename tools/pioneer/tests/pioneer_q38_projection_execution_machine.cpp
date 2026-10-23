/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_PROJECTION_COOPERATIVE_FIXTURE_ONLY
#include "pioneer_q38_projection_graph_cooperative_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionGraphFootprint.h"
#include "pioneer_q38_projection_execution_machine.h"
struct ProjectionExecutionMachine::Impl {
 struct Root;
 const char* rootImage;const char* peerImage;ProjectionExecutionFabric& fabric;
 std::array<std::unique_ptr<ParentArithmeticPeer>,120> peer;
 std::unique_ptr<Root> root;
 unsigned matrices=0,producers=0,stops=0;
 Impl(const char* r,const char* p,ProjectionExecutionFabric& f):rootImage(r),peerImage(p),fabric(f){
  (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&BankProof;
 }
 template<class MachineType> void Sync(unsigned w,MachineType& p){
  for(unsigned i=0;i<PioneerQ38ProjectionGraphSnapshotWords(w);i++){
   uint32_t a=PioneerQ38ProjectionGraphSnapshotAddress(w,i);
   if(PioneerQ38ProjectionGraphProgramImmutable(w,a))continue;
   if(a>=0x17ffc0 && a<=0x17ffc8){fabric.WriteLocal(w,a,p.l1[p.Index(a)]);continue;}
   auto value=p.memory.find(a);if(value!=p.memory.end())fabric.WriteLocal(w,a,value->second);
  }
 }
 void AdvancePeer(unsigned w){
  auto& p=*peer[w];
  if((p.pc==TT_Q38_PEER_WAIT_LOAD || p.pc==TT_Q38_PEER_STOP) && p.memory.at(0x74100)==p.memory.at(0x74144))return;
  for(unsigned n=0;n<8192;n++){
   if(p.pc==TT_Q38_PEER_FAULT_PARK || ((p.pc==TT_Q38_PEER_WAIT_LOAD || p.pc==TT_Q38_PEER_STOP)
    && p.memory.at(0x74100)==p.memory.at(0x74144)))break;
   p.StepBlock();
  }
  assert(p.pc!=TT_Q38_PEER_FAULT_PARK);
  if((p.pc==TT_Q38_PEER_WAIT_LOAD || p.pc==TT_Q38_PEER_STOP) && p.memory.at(0x74100)==p.memory.at(0x74144))Sync(w,p);
  uint32_t c[8];for(unsigned i=0;i<8;i++)c[i]=p.Count(i);fabric.SetControls(w,p.command,c);
 }
 void Start(unsigned w);void AdvanceRoot();
};
struct ProjectionExecutionMachine::Impl::Root:ProjectionParentMachine {
 Impl& device;std::vector<uint32_t> expected;
 Root(Impl& d):ProjectionParentMachine(d.rootImage,d.fabric.ReadLocal(0,TT_Q38_PROJECTION_GRAPH_MANIFEST+42*4),
  d.fabric.ReadLocal(0,TT_Q38_PROJECTION_GRAPH_MANIFEST+43*4),d.fabric.ReadLocal(0,TT_Q38_PROJECTION_GRAPH_MANIFEST+3*4),
  d.fabric.ReadLocal(0,TT_Q38_PROJECTION_GRAPH_MANIFEST+5*4)==2),device(d){
  memory.clear();
  for(unsigned i=0;i<PioneerQ38ProjectionGraphSnapshotWords(0);i++){
   unsigned a=PioneerQ38ProjectionGraphSnapshotAddress(0,i),v=d.fabric.ReadLocal(0,a);memory[a]=v;
   if(Range(a,0x68000,unsigned(graph.size())))for(unsigned b=0;b<4;b++)graph[a-0x68000+b]=uint8_t(v>>(b*8));
   if(Range(a,0x64000,unsigned(prefix.size())))for(unsigned b=0;b<4;b++)prefix[a-0x64000+b]=uint8_t(v>>(b*8));
   if(a==0x6000 || (CodeAddress(a) && !Range(a,0x68000,unsigned(graph.size()))) || (a>=0x17ffc0 && a<=0x17ffc8))l1[Index(a)]=v;
  }
  const auto* m=&memory.at(TT_Q38_PROJECTION_GRAPH_MANIFEST);(void)m;
  // Rebuild the independent observer's DATA geometry from the installed
  // frame. This does not alter any actual installed instruction/descriptor.
  auto word=[&](unsigned i){return memory.at(TT_Q38_PROJECTION_GRAPH_MANIFEST+i*4);};
  fixture.token=uint64_t(word(40))|(uint64_t(word(41))<<32);fixture.phase=word(6);fixture.input=uint64_t(word(46))|(uint64_t(word(47))<<32);
  uint64_t* fields[]={&fixture.id.session,&fixture.id.sequence,&fixture.id.modelEpoch,&fixture.id.driverGeneration};
  for(unsigned i=0;i<4;i++)*fields[i]=uint64_t(word(32+i*2))|(uint64_t(word(33+i*2))<<32);
  for(unsigned i=0;i<16;i++)fixture.root[i]=d.fabric.NoC(0,BlackholeDramSourceOffset(i));
  for(unsigned w=1;w<120;w++){
   auto& n=node[w-1];auto& p=*d.peer[w];n.sequence=n.requested=fixture.sequence[w-1]=word(64+w-1);n.verified=true;n.op=0;n.stopped=false;
   for(unsigned i=0;i<16;i++)fixture.sources[w-1][i]=n.src[i]=p.source[i];
   for(unsigned i=0;i<8;i++)n.count[i]=p.Count(i);
  }
  fixture.jobs.reset(new PioneerQ38ProjectionJobs);assert(fixture.jobs->Build(*fixture.canonical,fixture.layer,fixture.role,fixture.token));
  fixture.graph.reset(new PioneerQ38ProjectionGraphPlan);assert(fixture.Build());
  for(unsigned i=0;i<TT_Q38_PROJECTION_GRAPH_WORDS;i++)assert(word(i)==fixture.graph->Manifest()[i]);
  expected.assign(fixture.jobs->Outputs(),0x7fc00001);gddr=d.fabric.Gddr();
  for(unsigned i=0;i<8;i++)counts[i]=d.fabric.NoC(0,PioneerQ38PanelCounter(i));
  for(unsigned i=0;i<10;i++)command[i]=d.fabric.NoC(0,i*4);
 }
 uint32_t Load(uint32_t a)override{
  for(unsigned i=0;i<16;i++)if(a==0xffb20000+BlackholeDramSourceOffset(i))return device.fabric.NoC(0,BlackholeDramSourceOffset(i));
  return ProjectionParentMachine::Load(a);
 }
 void Advance(unsigned w)override{
  auto& n=node[w];if(n.verified)return;device.AdvancePeer(w+1);auto& p=*device.peer[w+1];
  n.requested=p.memory.at(0x74140);n.sequence=p.memory.at(0x74144);for(unsigned i=0;i<8;i++)n.count[i]=p.Count(i);
  if(n.sequence!=n.requested || (p.pc!=TT_Q38_PEER_WAIT_LOAD && p.pc!=TT_Q38_PEER_STOP))return;
  p.ClearTensorState();assert(!p.pending && !p.memory.at(0x74148));n.verified=true;n.stopped=p.pc==TT_Q38_PEER_STOP;
  if(n.op==1){assert(p.CorrectRows() && !p.dotProbeReads && p.requests==p.packets.size());device.matrices++;
   unsigned first=memory.at(TT_Q38_PROJECTION_MATRIX_ROWS+w*4);for(unsigned row=0;row<p.groups*16;row++)expected.at(first+row)=p.buffers[4096+row];}
  else if(n.op==5){p.Verify();device.producers++;}else{assert(n.op==2 && n.stopped);device.stops++;}
 }
 uint32_t PhysicalRegister(unsigned w,unsigned a)override{
  auto& p=*device.peer[w+1];if(a==0xffb13140)return p.pc;if(a==0xffb121b0)return device.fabric.Reset(w+1);
  for(unsigned i=0;i<16;i++)if(a==0xffb20000+BlackholeDramSourceOffset(i))return p.source[i];
  for(unsigned i=0;i<8;i++)if(a==0xffb20000+p.CounterOffset(i))return p.Count(i);
  assert(false);return 0;
 }
 uint32_t RowOutput(unsigned w,unsigned row)const override{return device.peer[w+1]->buffers[4096+row];}
 void PeerPacket()override{
  bool write=command[7]==0x212;unsigned w=Worker(write?command[5]:command[2])-1;auto& n=node[w];auto& p=*device.peer[w+1];
  unsigned local=write?command[0]:command[3],remote=write?command[3]:command[0],bytes=command[8];
  assert(w<119 && (write?command[2]:command[5])==129 && bytes && local>=0x75000 && local+bytes<=0x75800 && !((local^remote)&31));
  if(write){
   if(remote==0x74200 || remote==0x74000){
    assert(bytes==(remote==0x74200?256U:64U) && p.pc==TT_Q38_PEER_WAIT_LOAD);
    for(unsigned i=0;i<bytes/4;i++)p.memory[remote+i*4]=memory.at(local+i*4);
    if(remote==0x74200)for(unsigned i=0;i<64;i++)n.frame[i]=p.memory.at(remote+i*4);
    else for(unsigned i=0;i<16;i++)n.job[i]=p.memory.at(remote+i*4);
   }else{
    assert(remote==0x74100 && bytes==4 && memory.at(local)==p.memory.at(0x74144)+1);
    p.Prepare(memory);BeforeDoorbell(w);p.memory[remote]=memory.at(local);
   }
  }else if(remote==0x74140){if(n.requested!=n.sequence || !n.verified)Advance(w);
   for(unsigned i=0;i<16;i++)memory[local+i*4]=p.memory.at(remote+i*4);
  }else if(remote==0x83f00){assert(n.verified && n.sequence==n.requested && bytes==(n.op==1?256+p.groups*64:256));
   for(unsigned i=0;i<64;i++)memory[local+i*4]=p.memory.at(remote+i*4);
   if(bytes>256)for(unsigned row=0;row<p.groups*16;row++)memory[local+256+row*4]=p.buffers[4096+row];
  }else{assert(bytes==4);memory[local]=PhysicalRegister(w,remote);}
  Retire(bytes,write);
 }
 void Packet()override{
  bool write=command[7]==0x212;unsigned route=write?command[5]:command[2];
  if(route==fixture.graph->Manifest()[8]){gddr=device.fabric.Gddr();DramPacket();device.fabric.Gddr()=gddr;}else PeerPacket();
 }
};
void ProjectionExecutionMachine::Impl::Start(unsigned w){
 if(!w){root.reset(new Root(*this));return;}
 assert(!peer[w]);auto p=std::make_unique<ParentArithmeticPeer>(peerImage,w,fabric.Gddr(),false);
 p->memory.clear();for(unsigned i=0;i<PioneerQ38ProjectionGraphSnapshotWords(w);i++){
  unsigned a=PioneerQ38ProjectionGraphSnapshotAddress(w,i),v=fabric.ReadLocal(w,a);p->memory[a]=v;
  // The16KiB helper's VM fetch slot reloads from activationCode. Populate
  // that ROM view from ACTUAL installed bytes too,not from the input file.
  if(p->Range(a,0x64000,unsigned(p->activationCode.size())))for(unsigned b=0;b<4;b++)p->activationCode[a-0x64000+b]=uint8_t(v>>(b*8));
  if(p->CodeAddress(a))p->l1[p->Index(a)]=v;
 }
 for(unsigned i=0;i<16;i++)p->source[i]=fabric.NoC(w,BlackholeDramSourceOffset(i));
 for(unsigned i=0;i<8;i++)p->countBase[i]=p->independentBefore[i]=fabric.NoC(w,PioneerQ38PanelCounter(i));
 for(unsigned i=0;i<10;i++)p->command[i]=fabric.NoC(w,i*4);
 peer[w]=std::move(p);
}
void ProjectionExecutionMachine::Impl::AdvanceRoot(){
 auto& p=*root;if(p.pc==TT_Q38_DISPATCH_PARK || p.pc==TT_Q38_DISPATCH_FAULT_PARK)return; // terminal hardware never rewrites a later fault
 for(unsigned i=0;i<8192 && p.pc!=TT_Q38_DISPATCH_PARK && p.pc!=TT_Q38_DISPATCH_FAULT_PARK;i++)p.StepBlock();
 assert(p.pc!=TT_Q38_DISPATCH_FAULT_PARK);fabric.SetControls(0,p.command,p.counts);
 if(p.pc==TT_Q38_DISPATCH_PARK)Sync(0,p);
}
ProjectionExecutionMachine::ProjectionExecutionMachine(const char* root,const char* peer,ProjectionExecutionFabric& f):implementation(new Impl(root,peer,f)){}
ProjectionExecutionMachine::~ProjectionExecutionMachine(){delete implementation;}
void ProjectionExecutionMachine::Start(unsigned w){implementation->Start(w);}
void ProjectionExecutionMachine::Advance(unsigned w){if(w)implementation->AdvancePeer(w);else implementation->AdvanceRoot();}
unsigned ProjectionExecutionMachine::Pc(unsigned w)const{return w?implementation->peer[w]->pc:implementation->root->pc;}
unsigned ProjectionExecutionMachine::Matrices()const{return implementation->matrices;}
unsigned ProjectionExecutionMachine::Producers()const{return implementation->producers;}
unsigned ProjectionExecutionMachine::Stops()const{return implementation->stops;}
void ProjectionExecutionMachine::VerifyOutput(){
 auto& d=*implementation;assert(d.root->pc==TT_Q38_DISPATCH_PARK);const auto* m=d.root->fixture.graph->Manifest();
 if(m[5]==2){for(unsigned w=1;w<120;w++)assert(d.peer[w]->pc==TT_Q38_PEER_STOP);return;}
 for(unsigned row=0;row<m[45];row++)assert(d.fabric.Gddr().at(TT_Q38_PROJECTION_OUTPUT_OFFSET/4+row)==d.root->expected[row]);
}
void ProjectionExecutionMachine::Diagnostic()const{
 const auto& p=*implementation->root;
 fprintf(stderr,"CAPTURED_ROOT pc%08x stats",p.pc);
 for(unsigned i=0;i<64;i++)fprintf(stderr," %u:%u",i,p.memory.at(TT_Q38_PROJECTION_GRAPH_STATS+i*4));
 fprintf(stderr," COUNTS");for(unsigned i=0;i<8;i++)fprintf(stderr," %u",p.counts[i]);
 fprintf(stderr," COMMAND");for(unsigned i=0;i<10;i++)fprintf(stderr," %08x",p.command[i]);fprintf(stderr,"\n");
}
