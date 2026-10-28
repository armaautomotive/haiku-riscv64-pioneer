/* SPDX-License-Identifier: MIT */
// Execute the NEW returning body. Existing independent numeric observers
// receive outputs only. Do not forge the old standalone JointSummary.
#define PIONEER_Q38_MIXED_RECURRENT_BLOCK_MACHINE_ONLY
#include "pioneer_q38_mixed_recurrent_block_root_test.cpp"
#include "../PioneerQ38LayerReturnComponentLayout.h"
#include "../PioneerQ38RecurrentHistoryLayout.h"
namespace LC=PioneerQ38LayerReturnComponentLayout;
struct ReturningRecurrentMachine:RecurrentBlockMachine {
 std::vector<uint8_t> fault;
 std::array<uint32_t,257> parentBytes={};
 static constexpr unsigned Parent=0x6fb00,Fault=0x60000,Returned=0x60004;
 unsigned historyReads=0,historyWrites=0;
 ReturningRecurrentMachine(const char* legacy,const char* worker,const char* linked):RecurrentBlockMachine(legacy,worker,true,true){
  // Change only the INITIAL test ROM/entry and primitive bootstrap data.
  // No ROOT instructions have run; real peers retain their actual virgin
  // boot state. There is NO subsequent processor/nonce/product rewrite.
  text=Read((std::string(linked)+"/text.bin").c_str());convRom=Read((std::string(linked)+"/conv.bin").c_str());
  fault=Read((std::string(linked)+"/fault.bin").c_str());assert(text.size()<=131072 && convRom.size()<=32768 && fault.size()==8);
  boot[0]=LC::Magic;for(unsigned i=40;i<46;i++)boot[i]=LC::BootWord(i);
  for(unsigned i=0;i<64;i++)memory[RB::Boot+i*4]=boot[i];
  assert(LC::Frame(boot.data(),outer.data(),false));
  for(unsigned i=0;i<16;i++)memory[RB::Status+i*4]=0;
  for(unsigned i=0;i<64;i++)memory[RB::JointSummary+i*4]=0x7fc00001;
  for(unsigned i=0;i<parentBytes.size();i++)memory[Parent+i*4]=parentBytes[i]=0xa55a0000+i;
  memory[RB::Doorbell]=1;pc=0x40000;assert(!steps && !packets && !matrixStarts);
 }
 bool CodeAddress(uint32_t a)const override{return Range(a,Fault,unsigned(fault.size())) || RecurrentBlockMachine::CodeAddress(a);}
 unsigned Index(uint32_t a)override{
  if(Range(a,Fault,unsigned(fault.size()))){assert(!(a&3));l1[8180]=Word(fault,a-Fault);return 8180;}
  return RecurrentBlockMachine::Index(a);
 }
 void Store(uint32_t address,uint32_t value)override{
  assert(!Range(address,Parent,sizeof(parentBytes))); // parent bytes are NEVER component scratch
  RecurrentBlockMachine::Store(address,value);
 }
 void Submit()override{
  namespace H=PioneerQ38RecurrentHistoryLayout;uint32_t history=0;assert(H::Address(outer[9],outer[3],history));
  bool write=command[7]==0x212;unsigned remote=write?command[3]:command[0],route=write?command[5]:command[2],bytes=command[8];
  if(Phase()==14 && route==outer[8]){
   if(write && Range(remote,history,H::Bytes)){assert(remote==history+historyWrites && bytes<=H::Bytes-historyWrites);historyWrites+=bytes;}
   if(!write && Range(remote,outer[9]+TT_Q38_CONV_NEXT,H::Bytes)){assert(remote==outer[9]+TT_Q38_CONV_NEXT+historyReads && bytes<=H::Bytes-historyReads);historyReads+=bytes;}
  }
  RecurrentBlockMachine::Submit();
 }
 bool Call(){
  while(pc!=Fault && pc!=Returned)Step();
  if(pc==Fault)fprintf(stderr,"RETURN_COMPONENT_FAULT phase%u matrices%u jobs%u packets%u SP%x\n",Phase(),matrixStarts,matrixChecks,packets,regs[2]);
  return pc==Returned;
 }
 void VerifyReturn(){
  assert(pc==Returned && regs[10]==5120 && regs[27]==Parent && regs[2]==Parent && minStack>=0x6e000
   && !unknown && !pending && !stopJobs && !stopStarts && matrixStarts==8 && matrixChecks==720 && finalWords==5120
   && initialNorm==1 && postNorm==1 && activationChecks==1 && gatedChecks==1 && numericCohortChecks==1
   && memory.at(RB::Status+4)==1 && !memory.at(RB::Status+16) && memory.at(RB::JointSummary)!=RB::Magic);
  for(unsigned i=0;i<parentBytes.size();i++)assert(memory.at(Parent+i*4)==parentBytes[i]);
  for(unsigned i=0;i<5120;i++)assert(Get(outer[8],outer[9]+RB::FinalOffset+i*4)==Bits(Float(Get(outer[8],outer[9]+PioneerQ38MixedRecurrentOffset(4)+i*4))+Float(memory.at(RB::PostResidual+i*4)))
   && memory.at(RB::Original+i*4)==memory.at(RB::OriginalCopy+i*4));
  namespace H=PioneerQ38RecurrentHistoryLayout;uint32_t history=0;assert(H::Address(outer[9],outer[3],history));
  assert(historyReads==H::Bytes && historyWrites==H::Bytes);
  for(unsigned i=0;i<H::Bytes/4;i++)assert(Get(outer[8],history+i*4)==Get(outer[8],outer[9]+TT_Q38_CONV_NEXT+i*4));
  for(unsigned w=0;w<119;w++){
   const auto& p=*child[w];assert(!p.active && p.GdnFinished(p.sequence)
    && p.sequence==initial[w]+PioneerQ38MixedRecurrentBlockProjectionLayout::RunDelta(w)
    && memory.at(RB::Nonces+w*4)==p.sequence);
   uint32_t actual[8];for(unsigned i=0;i<8;i++)actual[i]=p.Count(i);
   unsigned polls=UINT32_MAX;assert(PioneerQ38RecurrentBlockCounterProof(*block,*canonical,w+1,peerCounterBaseline[w],actual,false,polls)
    && polls==(w<96?p.gdnPolls:0));
  }
  printf("PASS ACTUAL returning recurrent component/8matrices+720jobs+120Conv+96GDN/independent arithmetic/final5120/ALL119 closed chains retained/ConvNEXT122880B staged ON CARD/SP+s11+257parent canaries restored/ROOTstack%u/NO host wait,STOP or fabricated standalone summary;SIMULATED primitive endpoints,NOT original/native/full64/physical/Work/TPS\n",0x70000-minStack);
 }
};
int main(int argc,char** argv){
 assert(argc==4 || (argc==5 && !strcmp(argv[4],"--refusals")));setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;(void)&Refusals;(void)&LateRefusals;
 puts("START ACTUAL QLC1 returning recurrent instructions/SAME119 genuinely virgin peers/raw learned+hidden+zero prior-state endpoints ONLY;NO CPU products or completed-owner facade");
 auto machine=std::make_unique<ReturningRecurrentMachine>(argv[1],argv[2],argv[3]);
 if(argc==5){
  for(unsigned bad=0;bad<12;bad++){
   fflush(nullptr);pid_t child=fork();assert(child>=0);
   if(!child){
    auto& m=*machine;
    if(bad==0)m.memory[RB::Boot]^=1;if(bad==1)m.memory[RB::Boot+40*4]^=1;if(bad==2)m.memory[RB::Boot+63*4]^=1;
    if(bad==3)m.memory[RB::Outer]^=1;if(bad==4)m.memory[RB::Outer+2*4]^=1;if(bad==5)m.memory[RB::Outer+32*4]^=1;
    if(bad==6)m.memory[RB::Outer+48*4]^=1;if(bad==7)m.memory[RB::Outer+69*4]^=4;if(bad==8)m.memory[RB::Outer+70*4]^=1;
    if(bad==9)m.source[0]^=1;if(bad==10)m.source[11]^=1;if(bad==11)m.memory[RB::Boot+20*4]^=1;
    assert(!m.Call() && m.pc==m.Fault && !m.packets && !m.peerWrites && !m.matrixStarts && !m.convStarts && !m.gdnStarts
     && m.memory.at(RB::Status+4)==0 && m.memory.at(RB::JointSummary)!=RB::Magic);
    for(unsigned i=0;i<m.parentBytes.size();i++)assert(m.memory.at(m.Parent+i*4)==m.parentBytes[i]);
    for(unsigned i=0;i<2048;i++)m.Step();
    assert(m.pc==m.Fault && !m.packets && !m.peerWrites);
    for(const auto& p:m.child)assert(!p->active && !p->sequence && p->GdnFinished(0));
    _exit(0);
   }
   int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));
  }
  puts("PASS ACTUAL returning recurrent initial guards/12 reached boot+outer+identity+current physical-source refusals/ZERO NoC or peer work/parent canaries+genuine virgin peers retained/inert fault2048;NO positive RUN/cleanup/native/physical or TPS proof");return 0;
 }
 assert(machine->Call());machine->VerifyReturn();
}
