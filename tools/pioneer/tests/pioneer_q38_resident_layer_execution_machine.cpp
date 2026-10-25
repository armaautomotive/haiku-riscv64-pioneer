/* SPDX-License-Identifier: MIT */
// ACTUAL installed full coordinator and ALL119 peers. Byte endpoints only;
// no products, success status or capability is injected into instructions.
#define PIONEER_Q38_RESIDENT_LAYER_MACHINE_ONLY
#include "pioneer_q38_resident_layer_image_test.cpp"
#include "pioneer_q38_resident_layer_execution_machine.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentLayerImages.h"
struct ResidentLayerExecutionMachine::Impl {
 const char* rootImage;const char* peerImage;ResidentLayerExecutionFabric& endpoint;
 std::unique_ptr<LayerFabric> machine;
 bool booted[120]={};unsigned boots=0,stops=0;bool exported=false;
 Impl(const char* r,const char* p,ResidentLayerExecutionFabric& e):rootImage(r),peerImage(p),endpoint(e){(void)&RunPersistentGdnPair;}
 void ImportCode(unsigned w,uint32_t at,std::vector<uint8_t>& code){
  for(unsigned i=0;i<code.size()/4;i++){uint32_t v=endpoint.ReadLocal(w,at+i*4);for(unsigned b=0;b<4;b++)code[i*4+b]=uint8_t(v>>(b*8));}
 }
 void Initialize(){
  assert(!machine);OutputParts parts(0,0);machine.reset(new LayerFabric(rootImage,peerImage,parts));auto& f=*machine;
  // The fixture constructor's unrelated geometry/boot state is never
  // exposed to the controller. Replace EVERY relevant byte and CPU before
  // the first real reset release. Instruction fetch uses installed bytes.
  for(unsigned i=0;i<TT_Q38_COHORT_WORDS;i++)f.manifest[i]=endpoint.ReadLocal(0,TT_Q38_COHORT_MANIFEST+i*4);
  for(unsigned i=0;i<TT_Q38_OUTPUT_COHORT_WORDS;i++)f.post[i]=endpoint.ReadLocal(0,TT_Q38_OUTPUT_COHORT_MANIFEST+i*4);
  assert(PioneerQ38ResidentCohortShape(f.manifest.data()) && PioneerQ38ResidentOutputCohortShape(f.post.data(),f.manifest.data()));
  const unsigned rootAddress[]={0x6000,0x72000,0x64000,0xb8000};
  for(unsigned s=0;s<4;s++)ImportCode(0,rootAddress[s],f.sections[s]);ImportCode(0,0x68000,f.matrixCode);
  ImportCode(0,0x158000,f.extra[0]);ImportCode(0,0x15c000,f.extra[1]);
  std::array<uint32_t,TT_Q38_RESIDENT_LAYER_WORDS> layer;
  for(unsigned i=0;i<layer.size();i++)layer[i]=endpoint.ReadLocal(0,TT_Q38_RESIDENT_LAYER_MANIFEST+i*4);
  const auto* l=layer.data();
  PioneerQ38GdnIdentity identity={uint64_t(l[32])|(uint64_t(l[33])<<32),uint64_t(l[34])|(uint64_t(l[35])<<32),
   uint64_t(l[36])|(uint64_t(l[37])<<32),uint64_t(l[38])|(uint64_t(l[39])<<32)};
  f.plan.reset(new PioneerQ38ResidentLayerPlan());
  assert(f.plan->Build(*f.canonical,l[3],l[4],l[8],f.manifest[14],l[11],l[5],identity,
   f.manifest.data()+TT_Q38_COHORT_SOURCE_WORD,f.manifest.data()+TT_Q38_COHORT_SEQUENCE_WORD));
  for(unsigned i=0;i<layer.size();i++)assert(f.plan->LayerManifest()[i]==layer[i]);
  for(unsigned i=0;i<PioneerQ38ResidentLayerSnapshotWords(0);i++){unsigned a=PioneerQ38ResidentLayerSnapshotAddress(0,i);f.Mem(a)=endpoint.ReadLocal(0,a);}
  for(unsigned i=0;i<16;i++)f.source[i]=endpoint.NoC(0,BlackholeDramSourceOffset(i));
  for(unsigned i=0;i<8;i++)f.initialNoc[i]=f.counts[i]=endpoint.NoC(0,FfnPersistentPeer::CounterOffset(i));
  for(unsigned i=0;i<10;i++)f.command[i]=endpoint.NoC(0,i*4);
  f.pc=0x6000;for(auto& r:f.regs)r=0;f.steps=0;f.l1[0]=endpoint.ReadLocal(0,0x6000);
  for(unsigned w=1;w<120;w++){
   auto& p=*f.peers[w-1];p.pc=0x6000;for(auto& r:p.regs)r=0;p.steps=0;p.active=false;p.launched=0;
   for(auto& entry:p.memory){assert(PioneerQ38ResidentLayerSnapshotContains(w,entry.first));entry.second=endpoint.ReadLocal(w,entry.first);}
   ImportCode(w,0x64000,p.activationCode);ImportCode(w,0x68000,p.service);ImportCode(w,0xb8000,p.gdnCode);
   for(unsigned i=0;i<PioneerQ38ResidentLayerCodeWords(w);i++){
    unsigned a=PioneerQ38ResidentLayerCodeAddress(w,i);p.l1[p.Index(a)]=endpoint.ReadLocal(w,a);
   }
   for(unsigned i=0;i<16;i++)p.source[i]=endpoint.NoC(w,BlackholeDramSourceOffset(i));
   for(unsigned i=0;i<8;i++)f.initialPeerNoc[w-1][i]=p.countBase[i]=endpoint.NoC(w,FfnPersistentPeer::CounterOffset(i));
   for(unsigned i=0;i<10;i++)p.command[i]=endpoint.NoC(w,i*4);
   if(w<=96)for(unsigned i=0;i<64;i++){
    const auto* d=PioneerQ38CohortGdn(f.manifest.data(),w);p.input[i]=d[i];p.state[i]=d[64+i];p.gated[i]=d[128+i];
   }
  }
  // SAME actual QPS1-produced workspace bytes, never a CPU-computed vector.
  const unsigned route=f.manifest[9],base=f.manifest[8];auto& workspace=endpoint.Gddr();
  for(auto& entry:f.gddr){const unsigned r=unsigned(entry.first>>32),a=unsigned(entry.first);
   if(r==route && a>=base && a-base<workspace.size()*4)entry.second=workspace[(a-base)/4];}
  for(unsigned w=0;w<120;w++){
   const auto* table=PioneerQ38CohortConv(f.manifest.data(),w);
   for(unsigned j=0;j<PioneerQ38ConvBatchCount(w);j++){
    const auto* d=table+j*64;uint32_t bytes[48];assert(endpoint.ReadState(d[14],d[15],bytes,48));
    for(unsigned i=0;i<48;i++)f.Put(d[41],d[15]+i*4,bytes[i]);
   }
   if(!w || w>96)continue;const auto* d=PioneerQ38CohortGdn(f.manifest.data(),w)+64;uint32_t bytes[8192];
   for(unsigned side=0;side<2;side++){
    assert(endpoint.ReadState(d[4+side*2],d[5+side*2],bytes,8192));
    for(unsigned i=0;i<8192;i++)f.Put(d[12+side],d[5+side*2]+i*4,bytes[i]);
   }
  }
  for(unsigned i=0;i<5120;i++)f.rawBefore[i]=f.Gddr(route,base+98304+i*4);
  // This endpoint test has synthetic norm/FFN bytes. Importing an original
  // prefix alone would create misleading mixed-model qualification.
  // The future original-model observer must cover the COMPLETE layer.
  assert(!endpoint.LearnedBytes());
 }
 void Sync(unsigned w){
  auto& f=*machine;
  if(w){auto& p=*f.peers[w-1];for(const auto& entry:p.memory)if(PioneerQ38ResidentLayerSnapshotContains(w,entry.first)
    && !PioneerQ38ResidentLayerProgramImmutable(w,entry.first))endpoint.WriteLocal(w,entry.first,entry.second);
   uint32_t counts[8];for(unsigned i=0;i<8;i++)counts[i]=p.Count(i);endpoint.SetControls(w,p.command,counts);
  }else{
   for(unsigned i=0;i<PioneerQ38ResidentLayerSnapshotWords(0);i++){unsigned a=PioneerQ38ResidentLayerSnapshotAddress(0,i);
    if(!PioneerQ38ResidentLayerProgramImmutable(0,a))endpoint.WriteLocal(0,a,f.Mem(a));}
   endpoint.SetControls(0,f.command,f.counts);
  }
 }
 void Export(){
  if(exported)return;auto& f=*machine;const unsigned route=f.manifest[9],base=f.manifest[8];auto& workspace=endpoint.Gddr();
  for(const auto& entry:f.gddr){unsigned r=unsigned(entry.first>>32),a=unsigned(entry.first);
   if(r==route && a>=base && a-base<workspace.size()*4)workspace[(a-base)/4]=entry.second;}
  for(unsigned w=1;w<=96;w++){
   const auto* d=PioneerQ38CohortGdn(f.manifest.data(),w)+64;uint32_t bytes[8192];
   for(unsigned i=0;i<8192;i++)bytes[i]=f.Gddr(d[13],d[7]+i*4);assert(endpoint.WriteState(d[6],d[7],bytes,8192));
  }
  for(unsigned w=0;w<120;w++){
   const auto* table=PioneerQ38CohortConv(f.manifest.data(),w);
   for(unsigned j=0;j<PioneerQ38ConvBatchCount(w);j++){const auto* d=table+j*64;uint32_t bytes[48];
    for(unsigned i=0;i<48;i++)bytes[i]=f.Gddr(d[41],d[15]+i*4);assert(endpoint.WriteState(d[14],d[15],bytes,48));
   }
  } // observe actual emitted convolution history stores too;NEVER reconstruct them from host math
  exported=true;
 }
 void Start(unsigned w){
  assert(w<120 && !booted[w] && endpoint.Reset(w)==0x46800);if(!machine)Initialize();booted[w]=true;
  if(w){boots++;return;}for(unsigned n=1;n<120;n++)assert(booted[n]);
 }
 void Advance(unsigned w){
  assert(w<120 && booted[w] && endpoint.Reset(w)==0x46800);auto& f=*machine;
  if(!w){
   if(f.pc==f.parkPc || (f.unknown && f.l1[f.Index(f.pc)]==0x0000006f))return;
   for(unsigned n=0;n<8192 && f.pc!=f.parkPc && !(f.unknown && f.l1[f.Index(f.pc)]==0x0000006f);n++)f.StepRoot();
   endpoint.SetControls(0,f.command,f.counts);
   // Host polls only the completion trailer while ROOT computes; direct
   // peer reads cannot occur until its genuine final receipt. Publish the
   // actual L1 stores at terminal, not billions of unrelated byte copies
   // after every8192 modeled instructions. No status is invented here.
   if(f.pc==f.parkPc || (f.unknown && f.l1[f.Index(f.pc)]==0x0000006f)){
    Sync(0);for(unsigned peer=1;peer<120;peer++)Sync(peer);if(f.pc==f.parkPc)Export();
   }
   return;
  }
  auto& p=*f.peers[w-1];
  // Host direct STOP writes reach this SAME actual mailbox, not a new VM.
  for(unsigned i=0;i<16;i++)p.memory[0x74000+i*4]=endpoint.ReadLocal(w,0x74000+i*4);
  p.memory[0x74100]=endpoint.ReadLocal(w,0x74100);
  if((p.pc==TT_Q38_PEER_WAIT_LOAD || p.pc==TT_Q38_PEER_STOP) && p.memory.at(0x74100)==p.memory.at(0x74144)){Sync(w);return;}
  bool wasStopped=p.pc==TT_Q38_PEER_STOP;
  for(unsigned n=0;n<8192;n++){
   if(p.GdnFaulted() || ((p.pc==TT_Q38_PEER_WAIT_LOAD || p.pc==TT_Q38_PEER_STOP) && p.memory.at(0x74100)==p.memory.at(0x74144)))break;
   p.StepBlock();
  }
  if(!wasStopped && p.pc==TT_Q38_PEER_STOP)stops++;Sync(w);
 }
};
ResidentLayerExecutionMachine::ResidentLayerExecutionMachine(const char* root,const char* peer,ResidentLayerExecutionFabric& endpoint):implementation(new Impl(root,peer,endpoint)){}
ResidentLayerExecutionMachine::~ResidentLayerExecutionMachine(){delete implementation;}
void ResidentLayerExecutionMachine::Start(unsigned w){implementation->Start(w);}
void ResidentLayerExecutionMachine::Advance(unsigned w){implementation->Advance(w);}
unsigned ResidentLayerExecutionMachine::Pc(unsigned w)const{const auto& p=*implementation;assert(p.booted[w]);return w?p.machine->peers[w-1]->pc:p.machine->pc;}
unsigned ResidentLayerExecutionMachine::Boots()const{return implementation->boots;}
unsigned ResidentLayerExecutionMachine::Stops()const{return implementation->stops;}
void ResidentLayerExecutionMachine::VerifyOutput(){auto& p=*implementation;
 assert(p.exported && (p.stops==0 || p.stops==119));
 assert(!p.stops);p.machine->VerifyLayer();p.machine->Inert();
}
void ResidentLayerExecutionMachine::Diagnostic()const{
 const auto& p=*implementation;if(!p.machine)return;const auto& f=*p.machine;
 fprintf(stderr,"LAYER_CAPTURED pc%08x steps%u packets%u conv%u gdn%u output%u matrix%u\n",f.pc,f.steps,f.packets,f.convLaunches,f.gdnLaunches,f.outputLaunches,f.matrixLaunches);
}
