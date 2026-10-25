/* SPDX-License-Identifier: MIT */
// ACTUAL installed full coordinator and ALL119 peers. Byte endpoints only;
// no products, success status or capability is injected into instructions.
#define PIONEER_Q38_RESIDENT_OUTPUT_COHORT_MACHINE_ONLY
#include "pioneer_q38_resident_output_cohort_image_test.cpp"
#include "pioneer_q38_resident_output_execution_machine.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentOutputImages.h"
struct ResidentOutputExecutionMachine::Impl {
 const char* rootImage;const char* peerImage;ResidentOutputExecutionFabric& endpoint;
 std::unique_ptr<OutputCohortFabric> machine;
 bool booted[120]={};unsigned boots=0,stops=0;bool exported=false;
 Impl(const char* r,const char* p,ResidentOutputExecutionFabric& e):rootImage(r),peerImage(p),endpoint(e){(void)&RunPersistentGdnPair;}
 void ImportCode(unsigned w,uint32_t at,std::vector<uint8_t>& code){
  for(unsigned i=0;i<code.size()/4;i++){uint32_t v=endpoint.ReadLocal(w,at+i*4);for(unsigned b=0;b<4;b++)code[i*4+b]=uint8_t(v>>(b*8));}
 }
 void Initialize(){
  assert(!machine);OutputParts parts(0,0);machine.reset(new OutputCohortFabric(rootImage,peerImage,parts));auto& f=*machine;
  // The fixture constructor's unrelated geometry/boot state is never
  // exposed to the controller. Replace EVERY relevant byte and CPU before
  // the first real reset release. Instruction fetch uses installed bytes.
  for(unsigned i=0;i<TT_Q38_COHORT_WORDS;i++)f.manifest[i]=endpoint.ReadLocal(0,TT_Q38_COHORT_MANIFEST+i*4);
  for(unsigned i=0;i<TT_Q38_OUTPUT_COHORT_WORDS;i++)f.post[i]=endpoint.ReadLocal(0,TT_Q38_OUTPUT_COHORT_MANIFEST+i*4);
  assert(PioneerQ38ResidentCohortShape(f.manifest.data()) && PioneerQ38ResidentOutputCohortShape(f.post.data(),f.manifest.data()));
  const unsigned rootAddress[]={0x6000,0x72000,0x64000,0xb8000};
  for(unsigned s=0;s<4;s++)ImportCode(0,rootAddress[s],f.sections[s]);ImportCode(0,0x68000,f.matrixCode);
  for(unsigned i=0;i<PioneerQ38ResidentOutputSnapshotWords(0);i++){unsigned a=PioneerQ38ResidentOutputSnapshotAddress(0,i);f.Mem(a)=endpoint.ReadLocal(0,a);}
  for(unsigned i=0;i<16;i++)f.source[i]=endpoint.NoC(0,BlackholeDramSourceOffset(i));
  for(unsigned i=0;i<8;i++)f.counts[i]=endpoint.NoC(0,FfnPersistentPeer::CounterOffset(i));
  for(unsigned i=0;i<10;i++)f.command[i]=endpoint.NoC(0,i*4);
  f.pc=0x6000;for(auto& r:f.regs)r=0;f.steps=0;f.l1[0]=endpoint.ReadLocal(0,0x6000);
  for(unsigned w=1;w<120;w++){
   auto& p=*f.peers[w-1];p.pc=0x6000;for(auto& r:p.regs)r=0;p.steps=0;p.active=false;p.launched=0;
   for(auto& entry:p.memory){assert(PioneerQ38ResidentOutputSnapshotContains(w,entry.first));entry.second=endpoint.ReadLocal(w,entry.first);}
   ImportCode(w,0x64000,p.activationCode);ImportCode(w,0x68000,p.service);ImportCode(w,0xb8000,p.gdnCode);
   for(unsigned i=0;i<PioneerQ38ResidentOutputCodeWords(w);i++){
    unsigned a=PioneerQ38ResidentOutputCodeAddress(w,i);p.l1[p.Index(a)]=endpoint.ReadLocal(w,a);
   }
   for(unsigned i=0;i<16;i++)p.source[i]=endpoint.NoC(w,BlackholeDramSourceOffset(i));
   for(unsigned i=0;i<8;i++)p.countBase[i]=endpoint.NoC(w,FfnPersistentPeer::CounterOffset(i));
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
 }
 void Sync(unsigned w){
  auto& f=*machine;
  if(w){auto& p=*f.peers[w-1];for(const auto& entry:p.memory)if(PioneerQ38ResidentOutputSnapshotContains(w,entry.first)
    && !PioneerQ38ResidentOutputProgramImmutable(w,entry.first))endpoint.WriteLocal(w,entry.first,entry.second);
   uint32_t counts[8];for(unsigned i=0;i<8;i++)counts[i]=p.Count(i);endpoint.SetControls(w,p.command,counts);
  }else{
   for(unsigned i=0;i<PioneerQ38ResidentOutputSnapshotWords(0);i++){unsigned a=PioneerQ38ResidentOutputSnapshotAddress(0,i);
    if(!PioneerQ38ResidentOutputProgramImmutable(0,a))endpoint.WriteLocal(0,a,f.Mem(a));}
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
ResidentOutputExecutionMachine::ResidentOutputExecutionMachine(const char* root,const char* peer,ResidentOutputExecutionFabric& endpoint):implementation(new Impl(root,peer,endpoint)){}
ResidentOutputExecutionMachine::~ResidentOutputExecutionMachine(){delete implementation;}
void ResidentOutputExecutionMachine::Start(unsigned w){implementation->Start(w);}
void ResidentOutputExecutionMachine::Advance(unsigned w){implementation->Advance(w);}
unsigned ResidentOutputExecutionMachine::Pc(unsigned w)const{const auto& p=*implementation;assert(p.booted[w]);return w?p.machine->peers[w-1]->pc:p.machine->pc;}
unsigned ResidentOutputExecutionMachine::Boots()const{return implementation->boots;}
unsigned ResidentOutputExecutionMachine::Stops()const{return implementation->stops;}
void ResidentOutputExecutionMachine::VerifyOutput(){auto& p=*implementation;assert(p.exported);p.machine->Verify();p.machine->Inert();}
void ResidentOutputExecutionMachine::Diagnostic()const{
 const auto& p=*implementation;if(!p.machine)return;const auto& f=*p.machine;
 fprintf(stderr,"OUTPUT_CAPTURED pc%08x steps%u packets%u conv%u gdn%u output%u matrix%u\n",f.pc,f.steps,f.packets,f.convLaunches,f.gdnLaunches,f.outputLaunches,f.matrixLaunches);
}
