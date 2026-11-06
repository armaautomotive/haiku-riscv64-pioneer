/* SPDX-License-Identifier: MIT */
// Whole immutable ROOT reset prefix continues through real BF16 producers.
// The SAME119 workers retain real Q8/Conv/GDN histories on one byte fabric.
// Observation stops BEFORE the first BF16 matrix job submission. NO native,
// final matrix/ledger closure, model-quality, card timing or TPS proof.
#define PIONEER_Q38_TOKEN_FLOW_MACHINE_ONLY
#include "pioneer_q38_token_flow_test.cpp"
namespace BP=PioneerQ38Bf16ProducerLayout;
struct ConnectedPrepareMachine:TokenFlowMachine {
 bool deferred,entered=false;unsigned ledger=0,producerRequests=0,cycleReads=0;
 std::vector<unsigned> prepareEntries;
 std::array<unsigned,119> idleRequests={};
 ConnectedPrepareMachine(const char* image,const char* old,const char* worker,bool candidate)
  :TokenFlowMachine(image,old,worker),deferred(candidate){
  std::ifstream symbols(std::string(image)+"/program.symbols");std::string address,kind,name;
  while(symbols>>address>>kind>>name)if(name.find("_ZN10ProjectionL9BfPrepareE")==0)
   prepareEntries.push_back(unsigned(std::stoul(address,nullptr,16)));
  assert(prepareEntries.size()==2 && !steps);
 }
 uint32_t ReadCycleCsr(unsigned csr)override{
  // The exact timing BUILDs read ONLY these two CSRs in this immutable
  // clock helper. Ordinary harnesses still reject CSR access. This is the
  // existing synthetic scheduler clock, NEVER physical cycles or speed.
  assert((csr==0xb00 || csr==0xb80) && Range(pc,0x20d30,0x50));cycleReads++;
  return csr==0xb00?uint32_t(Clock()):uint32_t(Clock()>>32);
 }
 unsigned Index(unsigned a)override{
  if(deferred && !ledger && std::find(prepareEntries.begin(),prepareEntries.end(),a)!=prepareEntries.end()){
   ledger=regs[11];assert(ledger>=0x6e000 && ledger<=0x70000-52);
  }
  return TokenFlowMachine::Index(a);
 }
 bool AtSubmit(unsigned remote,unsigned route,unsigned bytes)const{
  if(!releases || Phase()!=10 || memory.at(RB::Header+43*4)!=2 || command[3]!=remote
   || command[5]!=route || command[7]!=0x212 || command[8]!=bytes)return false;
  const unsigned i=l1[const_cast<ConnectedPrepareMachine*>(this)->Index(pc)],op=i&127,d=(i>>7)&31,s=(i>>15)&31,t=(i>>20)&31;
  return op==0x23 && ((i>>12)&7)==2 && regs[s]+Sign(((i>>25)<<5)|d,12)==0xffb20040 && regs[t]==1;
 }
 bool AtFirstMatrix()const{return AtSubmit(M::Job,G::Peer(1),256) && memory.at(RB::Scratch+16*4)==0x314d4351;}
 bool AtFirstLine()const{
  if(!entered || command[0]!=outer[9]+BP::LinesOffset || command[2]!=outer[8]
   || command[3]!=unsigned(RB::Scratch) || command[7] || command[8]!=128)return false;
  const unsigned i=l1[const_cast<ConnectedPrepareMachine*>(this)->Index(pc)],d=(i>>7)&31,s=(i>>15)&31,t=(i>>20)&31;
  return (i&0x707f)==0x2023 && regs[s]+Sign(((i>>25)<<5)|d,12)==0xffb20040 && regs[t]==1;
 }
 void Store(unsigned a,unsigned value)override{
  if(a==0xffb20040 && releases && Phase()==10 && memory.at(RB::Header+43*4)==2){
   if(command[3]==unsigned(BP::Descriptor) && command[7]==0x212){entered=true;producerRequests++;}
   if(entered){
    unsigned route=command[7]==0x212?command[5]:command[2];
    for(unsigned w=20;w<119;w++)if(route==G::Peer(w+1))idleRequests[w]++;
   }
  }
  TokenFlowMachine::Store(a,value);
 }
 void Tick(){cycles+=19;StepToken();if(!(steps&255))for(unsigned w=0;w<119;w++)Advance(w,64);}
 void RunToFirstLine(){
  unsigned previous=0;
  while(!AtFirstLine() && pc!=0x60000 && pc!=0x60004){
   previous=pc;Tick();
   if(!(steps%10000000))printf("OBSERVE compiled prepare %s steps%u phase%u Q8%u BF16producers%u PC%x\n",deferred?"deferred":"control",steps,Phase(),matrixJobs,bf16ProducerJobs,pc);
  }
  if(pc==0x60000)PioneerQ38InstructionFault::Dump(*this,previous,RB::Projected,RB::PostResidual,RB::Final,RB::Original);
  assert(AtFirstLine() && producerRequests==20 && bf16ProducerJobs==20);
  // ROOT is paused at its genuinely reached read. Let the already released
  // processors finish by executing their own code; no history transplantation.
  for(unsigned w=0;w<20;w++){while(child[w]->active)Advance(w);assert(child[w]->GdnFinished(child[w]->sequence));}
 }
 bool FinishPrepare(){
  unsigned previous=0;
  while(!AtFirstMatrix() && pc!=0x60000 && pc!=0x60004){previous=pc;Tick();}
  if(pc==0x60000)printf("OBSERVE guest fault afterPC%x;no BF16 matrix submitted\n",previous);
  return AtFirstMatrix();
 }
 void Verify(){
  assert(AtFirstMatrix() && !position && !layer && !completed && !committed && !stopJobs
   && releases==1 && matrices==5 && convJobs==119 && gdnJobs==96 && matrixJobs==363 && gdnChecks==1
   && bf16ProducerJobs==20 && producerRequests==20 && cycleReads);
  unsigned total=0;
  for(unsigned w=0;w<119;w++){
   auto& p=*child[w];Advance(w);assert(!p.active && p.GdnFinished(p.sequence));
   unsigned expected=1+unsigned(w<96)+unsigned(w<20);
   for(unsigned slot=0;slot<5;slot++)expected+=unsigned(w<recurrent[0]->Recipe(slot)[7]);
   assert(p.sequence==expected && memory.at(RB::Nonces+w*4)==expected);
   assert(idleRequests[w]==(w>=20 && !deferred?15U:0U));total+=idleRequests[w];
   if(w<20)assert(p.bf16ProducerPackets==3);
  }
  for(unsigned i=0;i<5120;i++){
   const unsigned fp32=memory.at(BP::ProjectionInput+i*4),lo=fp32&65535,hi=fp32>>16;
   const unsigned expected=hi+unsigned(lo>32768 || (lo==32768 && (hi&1)));
   const unsigned packed=Get(outer[8],outer[9]+BP::NativeOffset+(i/2)*4);
   assert(uint16_t(packed>>((i&1)*16))==expected);
   assert(memory.at(RB::Native+(i/2)*4)==packed && memory.at(RB::NativeShadow+(i/2)*4)==packed);
  }
  if(deferred){
   assert(ledger && memory.at(ledger+32)==5120 && memory.at(ledger+40)==memory.at(RB::Header+7*4)
    && memory.at(ledger+48)==0x31414442);
   for(unsigned i=0;i<4;i++)assert(!memory.at(ledger+i*4) && memory.at(ledger+16+i*4)==UINT32_MAX);
  }
  printf("PASS COMPILED_CONNECTED_PREPARE %s SAME119 reset/Q8jobs363/Conv119/GDN96 -> real BF16producers20/60packets/5120values -> real ROOT proof+line+native reads/shadows/idleControlRequests%u/stack%u;BEFORE matrix submission,NOT final119closure/native/hardware/quality/TPS\n",deferred?"deferred":"control",total,0x70000-minSp);
 }
 void Faults(){
  assert(AtFirstLine());fflush(nullptr);
  for(unsigned fault=1;fault<=6;fault++){
   const pid_t pid=fork();assert(pid>=0);
   if(!pid){
    if(fault==1)source[11]^=1;
    if(fault==2)counts[7]++;
    if(fault==3){unsigned at=outer[9]+BP::LinesOffset+8*4;Put(outer[8],at,Get(outer[8],at)^1);}
    if(fault==4)child[1]->memory[0x83f00+19*4]^=1;
    if(fault==5)child[1]->countBase[7]++;
    if(fault==6)memory[RB::Header+40*4]^=1;
    assert(!FinishPrepare() && pc==0x60000 && matrices==5 && matrixJobs==363 && !committed && !stopJobs);
    const auto commands=command;const auto physical=counts;const unsigned issued=packets,writes=peerWrites;
    for(unsigned i=0;i<2048;i++)Tick();
    assert(pc==0x60000 && command==commands && counts==physical && packets==issued && peerWrites==writes);
    printf("PASS COMPILED_PREPARE_FAULT %s fault%u actual scalar park/no matrix/no publication/inert2048\n",deferred?"deferred":"control",fault);fflush(nullptr);_exit(0);
   }
   int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && WEXITSTATUS(status)==0);
  }
 }
};
int main(int argc,char** argv){
 assert(argc==5 && (!strcmp(argv[4],"control") || !strcmp(argv[4],"deferred")));setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&RunPersistentGdnPair;(void)&Refusals;(void)&LateRefusals;
 auto m=std::make_unique<ConnectedPrepareMachine>(argv[1],argv[2],argv[3],!strcmp(argv[4],"deferred"));
 printf("START compiled connected prepare %s;RAW synthetic inputs,actual reset history,NO processor/product setters\n",argv[4]);
 m->RunToFirstLine();m->Faults();assert(m->FinishPrepare());m->Verify();
}
