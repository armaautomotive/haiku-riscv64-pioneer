/* SPDX-License-Identifier: MIT */
// Execute the NEW returning attention/FFN body. Independent observers never
// supply products. Do not forge an old standalone result/owner receipt.
#define PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_MACHINE_ONLY
#include "pioneer_q38_mixed_live_block_root_test.cpp"
#include "../PioneerQ38LayerReturnComponentLayout.h"
namespace LC=PioneerQ38LayerReturnComponentLayout;
struct ReturningDenseMachine:LiveBlockMachine {
 std::vector<uint8_t> fault,conv;
 std::array<uint32_t,257> parentBytes={};
 static constexpr unsigned Parent=0x6fb00,Fault=0x60000,Returned=0x60004;
 ReturningDenseMachine(const char* legacy,const char* worker,const char* qdp,const char* qda,const char* linked):LiveBlockMachine(legacy,worker,qdp,qda){
  // Only initial command/raw bytes/ROM change. No ROOT instructions have
  // run; all119 peers retain their genuine virgin instruction histories.
  StageLive(1,0,3,false);
  code=Read((std::string(linked)+"/text.bin").c_str());conv=Read((std::string(linked)+"/conv.bin").c_str());
  fault=Read((std::string(linked)+"/fault.bin").c_str());assert(code.size()<=131072 && conv.size()<=32768 && fault.size()==8);
  boot[0]=LC::Magic;for(unsigned i=40;i<46;i++)boot[i]=LC::BootWord(i);
  for(unsigned i=0;i<64;i++)memory[T::Boot+i*4]=boot[i];
  assert(LC::Frame(boot.data(),blockOuter.data(),true));
  for(unsigned i=0;i<16;i++)memory[T::Status+i*4]=0;
  for(unsigned i=0;i<64;i++)memory[T::JointSummary+i*4]=0x7fc00001;
  for(unsigned i=0;i<parentBytes.size();i++)memory[Parent+i*4]=parentBytes[i]=0xa55a0000+i;
  memory[T::Doorbell]=sequence;pc=0x40018;assert(!steps && !packets && !projections);
 }
 bool CodeAddress(uint32_t a)const override{
  return Range(a,Fault,unsigned(fault.size())) || Range(a,0x64000,unsigned(conv.size())) || LiveBlockMachine::CodeAddress(a);
 }
 unsigned Index(uint32_t a)override{
  if(Range(a,Fault,unsigned(fault.size()))){assert(!(a&3));l1[8190]=Word(fault,a-Fault);return 8190;}
  if(Range(a,0x64000,unsigned(conv.size()))){assert(!(a&3));l1[8190]=Word(conv,a-0x64000);return 8190;}
  return LiveBlockMachine::Index(a);
 }
 uint32_t Load(uint32_t a)override{
  if(Range(a,Fault,unsigned(fault.size())) || Range(a,0x64000,unsigned(conv.size())))return l1[Index(a)];
  return LiveBlockMachine::Load(a);
 }
 void Store(uint32_t address,uint32_t value)override{
  assert(!Range(address,Parent,sizeof(parentBytes)));
  LiveBlockMachine::Store(address,value);
 }
 bool Call(){
  while(pc!=Fault && pc!=Returned)StepScalar();
  if(pc==Fault)fprintf(stderr,"RETURN_DENSE_FAULT phase%u projections%u jobs%u packets%u SP%x\n",Phase(),projections,acceptedJobs,packets,regs[2]);
  return pc==Returned;
 }
 void VerifyReturn(){
  assert(pc==Returned && regs[10]==5120 && regs[27]==Parent && regs[2]==Parent && minStack>=0x6e000
   && !pending && !stops && projections==7 && acceptedJobs==723 && prepared==1 && attentionChecks==1 && prepareReads==15
   && scatterWords==59392 && initialNorm==1 && postNorm==1 && activation==1 && finalWords==5120 && quantized==7 && inputObserved
   && memory.at(T::Status+4)==sequence && !memory.at(T::Status+16) && memory.at(T::JointSummary)!=T::Magic);
  for(unsigned i=0;i<parentBytes.size();i++)assert(memory.at(Parent+i*4)==parentBytes[i]);
  for(unsigned i=0;i<5120;i++)assert(memory.at(T::Original+i*4)==wantedInput[i] && memory.at(T::OriginalCopy+i*4)==wantedInput[i]);
  for(unsigned w=0;w<119;w++){
   const auto& p=*child[w];assert((p.pc==p.waitPC || p.pc==p.waitPC+4) && !p.memory.at(M::Status+8)
    && p.memory.at(M::Status+4)==firstNonce[w]+T::RunDelta(w) && memory.at(T::Nonces+w*4)==p.memory.at(M::Status+4));
  }
  printf("PASS ACTUAL returning dense component/on-card GDDR input/RMS+7matrices+723jobs+attention+FFN+residual/final5120/independent numeric observers/ALL119 closed chains retained/SP+s11+257parent canaries restored/ROOTstack%u/NO host wait,STOP or fabricated standalone summary;SIMULATED primitive endpoints,NOT original/native/persistentKV/full64/physical/Work/TPS\n",0x70000-minStack);
 }
};
int main(int argc,char** argv){
 assert(argc==6);setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&BlockInitialRefusals;(void)&BlockLaterRefusals;(void)&JoinedRefusals;(void)&ProjectionRefusals;
 (void)&Refusals;(void)&Group;(void)&Geometry;(void)&Faults;(void)&D::Run;
 puts("START ACTUAL QLC1 returning dense instructions/SAME119 genuinely virgin peers/raw learned+hidden+priorKV endpoints ONLY;NO CPU products or completed-owner facade");
 auto machine=std::make_unique<ReturningDenseMachine>(argv[1],argv[2],argv[3],argv[4],argv[5]);
 assert(machine->Call());machine->VerifyReturn();
}
