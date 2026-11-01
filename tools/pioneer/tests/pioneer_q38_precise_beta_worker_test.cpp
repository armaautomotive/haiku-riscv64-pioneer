/* SPDX-License-Identifier: MIT */
// Actual candidate worker from virgin boot through matrix/GDN/STOP.
// Synthetic raw byte+NoC endpoints, NOT model/native/physical completion.
#define PIONEER_Q38_PUSHED_WORKER_MACHINE_ONLY
#include "pioneer_q38_pushed_worker_test.cpp"
struct PreciseBetaPeer:PushedPeer {
 bool gdnLifetime=false;
 std::array<uint32_t,32> actualPC={};unsigned pcObservations=0;bool faultReported=false;
 using PushedPeer::PushedPeer;
 void StepBlock()override{
  actualPC[pcObservations++%actualPC.size()]=pc;PushedPeer::StepBlock();
  if(pc==faultPC && !faultReported){
   faultReported=true;
   fprintf(stderr,"ACTUAL_BETA_WORKER_FAULT worker%u sequence%u gdnPackets%u phase%u pending%u PCtrail",
    logicalWorker,sequence,gdnPackets,gdnPhase,unsigned(pending));
   for(unsigned i=0;i<actualPC.size();i++)fprintf(stderr," %x",actualPC[(pcObservations+i)%actualPC.size()]);
   fputc('\n',stderr);
  }
 } // HOST observer only; no PC, guest state or completion is supplied
 uint32_t Load(uint32_t address)override{
  // A completed matrix publisher does not turn later GDN reads into
  // payload-ACK reads. Preserve the true existing GDN endpoint semantics.
  return gdnLifetime?GlobalInitialPeer::Load(address):PushedPeer::Load(address);
 }
 void Store(uint32_t address,uint32_t value)override{
  if(Range(address,M::Status,64)){
   assert(!pending || (address==M::Status+8 && value==Push::Fault));memory[address]=value;return;
  }
  if(gdnLifetime)GlobalInitialPeer::Store(address,value);else PushedPeer::Store(address,value);
 }
 void StageInput(unsigned epoch,unsigned trial,uint32_t beta){
  assert(pc==waitPC && !pending && !heldPayload && sequence>=1);
  if(!gdnLifetime){
   std::array<uint32_t,8> physical;for(unsigned i=0;i<8;i++)physical[i]=Count(i);
   for(unsigned i=0;i<8;i++)countBase[i]=physical[i];NextPackets();
   for(unsigned i=0;i<8;i++)assert(Count(i)==physical[i]);
  } // Retire the completed endpoint packet list, NEVER rebase hardware counters or guest state.
  gdnLifetime=true;
  StageGdn(sequence+1,epoch,0,0,0,trial&1,trial);
  raw[321]=beta;learned[1][input[47]]=beta; // raw logit INPUT only,not a supplied gate/result
  std::array<uint32_t,64> job={};const unsigned h[]={Push::Magic,sequence+1,8,logicalWorker,selfRoute,0,0,epoch};
  for(unsigned i=0;i<8;i++){job[i]=h[i];job[8+i]=boot[8+i];}
  for(unsigned i=0;i<10;i++)job[16+i]=memory.at(0x74000+i*4);job[26]=memory.at(0x74038);
  assert(Push::JobShape(job.data(),boot.data(),sequence,0));sequence++;
  for(unsigned i=0;i<64;i++)memory[M::Job+i*4]=job[i];memory[M::Doorbell]=sequence;
 }
 void CheckPreciseBeta(){
  VerifyGdn(sequence);
  double x=Float(raw[321]),e=exp(-fabs(x));unsigned expected=Bits(float(x>=0?1/(1+e):e/(1+e)));
  if(!(expected&0x7f800000))expected=0;
  assert(memory.at(0x8e504)==expected && pushes==2 && !heldPayload);
 }
 void StopLocal(){
  assert(pc==waitPC && !pending);std::array<uint32_t,64> job={};
  const unsigned h[]={Push::Magic,sequence+1,unsigned(Push::Stop),logicalWorker,selfRoute,0,0,9999};
  for(unsigned i=0;i<8;i++){job[i]=h[i];job[8+i]=boot[8+i];}
  assert(Push::JobShape(job.data(),boot.data(),sequence,0));sequence++;
  for(unsigned i=0;i<64;i++)memory[M::Job+i*4]=job[i];memory[M::Doorbell]=sequence;
  while(pc!=stopPC && pc!=faultPC)StepBlock();
  assert(pc==stopPC && memory.at(M::Status+4)==sequence && memory.at(M::Status+16)==2 && !pending);
  unsigned packets=gdnPackets,math=matrixInstructions,writes=gdnNextWrites;
  for(unsigned i=0;i<2048;i++)StepBlock();
  assert(pc==stopPC && gdnPackets==packets && matrixInstructions==math && gdnNextWrites==writes);
 }
};
#ifndef PIONEER_Q38_PRECISE_BETA_WORKER_MACHINE_ONLY
int main(int argc,char** argv){
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);unsigned pairs=0,stack=0;
 for(unsigned head:{0U,31U,47U}){
  auto a=std::make_unique<PreciseBetaPeer>(argv[1],argv[2],head*2+1);
  auto b=std::make_unique<PreciseBetaPeer>(argv[1],argv[2],head*2+2);
  a->peer=b.get();b->peer=a.get();
  for(auto* p:{a.get(),b.get()}){p->BootPushed();p->StagePushed();p->AwaitPayload();p->FinishPushed();p->StageInput(1002,1,0xc09dde93);}
  assert(RunPersistentGdnPair(*a,*b,2));a->CheckPreciseBeta();b->CheckPreciseBeta();
  for(auto* p:{a.get(),b.get()}){p->prior=p->next;p->StageInput(1003,2,0x3f81fec2);}
  assert(RunPersistentGdnPair(*a,*b,3));a->CheckPreciseBeta();b->CheckPreciseBeta();
  for(auto* p:{a.get(),b.get()}){p->StopLocal();stack=std::max(stack,0x70000-p->minStack);}
  printf("PASS_ACTUAL_PRECISE_BETA_WORKER head%u genuineMatrixPush1 GdnSteps2 exactBeta1 retainedNextState1 realStop1;SAME_INSTRUCTION_HISTORY_SYNTHETIC_ENDPOINTS_NOT_ALL64_NATIVE_PHYSICAL_WORK_OR_TPS\n",head);pairs++;
 }
 printf("PASS_ACTUAL_PRECISE_BETA_WORKER_SUMMARY pairs%u virginWorkers%u GdnHalfJobs%u maxCombinedStack%u/8192;NO_NATIVE_MODEL_OR_TPS_GRANT\n",pairs,pairs*2,pairs*4,stack);
}
#endif
