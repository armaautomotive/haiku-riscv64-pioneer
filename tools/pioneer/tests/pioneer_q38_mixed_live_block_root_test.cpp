/* SPDX-License-Identifier: MIT */
// Actual fresh boot -> device-input complete blocks -> real119STOP+ROOT.
// The fabric supplies RAW bytes ONLY;never a prior job/proof/nonce/result.
#define PIONEER_Q38_MIXED_BLOCK_ROOT_MACHINE_ONLY
#include "pioneer_q38_mixed_block_root_test.cpp"
#include "../PioneerQ38MixedLiveBlockRootLayout.h"
namespace T=PioneerQ38MixedLiveBlockRootLayout;
struct LiveBlockMachine:BlockRootMachine {
 std::array<uint32_t,5120> wantedInput={};
 bool inputObserved=false;unsigned stops=0,quietBadWorker=0;
 LiveBlockMachine(const char* root,const char* worker,const char* qdp,const char* qda):BlockRootMachine(root,worker,qdp,qda,false){
  boot[0]=T::Magic;memory[T::Boot]=T::Magic;
  for(unsigned w=0;w<119;w++)assert(!child[w]->sequence && !child[w]->memory.at(M::Status+4));
  for(unsigned i=0;i<64;i++)memory[T::StopSummary+i*4]=0x7fc00001;
 }
 static bool QuietRegister(unsigned remote){
  if(Range(remote,0xffb20240,128))return true;
  for(unsigned i=0;i<4;i++)if(remote==0xffb20040+i*0x800)return true;
  return false;
 }
 void PacketFaults(){
  if(packets==counterFaultPacket)counts[3]++;if(packets==sourceFaultPacket)source[11]^=1;
  if(packets==headerFaultPacket)memory[T::Header+48*4]^=1;if(packets==bootFaultPacket)memory[T::Boot+8*4]^=1;
 }
 void SubmitLifecycle(){
  bool write=command[7]==0x212;unsigned local=write?command[0]:command[3],remote=write?command[3]:command[0];
  unsigned route=write?command[5]:command[2],bytes=command[8],w=Worker(route);auto& p=*child[w];
  assert(local==T::Scratch+(remote&31) && bytes && local+bytes<=T::Scratch+16384 && !(bytes&3)
   && !command[1] && !command[4] && !command[6] && !command[9] && (write?command[2]:command[5])==129 && (write || !command[7]));
  packets++;if(packets==failPacket){pending=true;return;}Advance(w);
  if(write){
   assert(Phase()==13 && ((remote==M::Job && bytes==256) || (remote==M::Doorbell && bytes==4)));
   for(unsigned i=0;i<bytes/4;i++)p.memory[remote+i*4]=memory.at(local+i*4);
   if(remote==M::Doorbell){assert(p.memory.at(M::Job+8)==2);stops++;}writes++;
  }else{
   reads++;
   if(QuietRegister(remote)){
    assert(bytes==4);memory[local]=p.Load(remote);if(quietBadWorker==w+1)memory[local]|=1;
   }else if(remote==0xffb121b0){assert(bytes==4);memory[local]=badWorker==w+1 && badKind==1?0x46801:0x46800;resetRead[w]=true;}
   else if(remote==0xffb13140){assert(bytes==4 && resetRead[w]);memory[local]=p.pc;resetRead[w]=false;}
   else if(Range(remote,0xffb20000,0x2000)){
    assert(bytes==4);unsigned offset=remote-0xffb20000;bool found=false;
    for(unsigned i=0;i<16;i++)if(offset==DenseAttentionMachine::SourceOffset(i)){memory[local]=p.source[i];found=true;}
    for(unsigned i=0;i<8;i++)if(offset==FfnPersistentPeer::CounterOffset(i)){memory[local]=p.Count(i);found=true;break;}assert(found);
   }else{
    assert((remote==M::Status && bytes==64) || (remote==M::Boot && bytes==256) || (remote==M::Job && bytes==256) || (remote==M::Doorbell && bytes==4));
    for(unsigned i=0;i<bytes/4;i++)memory[local+i*4]=p.memory.at(remote+i*4);
   }
  }
  counts[write?4:0]++;counts[write?5:1]+=(bytes+63)>>6;counts[write?7:3]++;counts[2]++;counts[6]++;PacketFaults();
 }
 void Store(uint32_t a,uint32_t value)override{
  if(a==0xffb20040){
   assert(value==1 && !pending);bool write=command[7]==0x212;
   if(Phase()==13 || (!write && QuietRegister(command[0]))){SubmitLifecycle();return;}
   if(write && command[3]==M::Job && Phase()==1 && !inputObserved){
    for(unsigned i=0;i<5120;i++)assert(memory.at(T::Original+i*4)==wantedInput[i]);inputObserved=true;
    puts("OBSERVED QBL1 ALL5120 original input words loaded by ACTUAL ROOT GDDR reads;no host L1 input products");
   }
   SubmitBlock();return;
  }
  if(Range(a,T::Original,20480) || Range(a,T::StopSummary,256)){assert(!pending);memory[a]=value;return;}
  BlockRootMachine::Store(a,value);
 }
 void StageLive(unsigned tokens,unsigned trial,unsigned layer,bool chain){
  unsigned inputBank=0,inputAddress=0;
  if(chain){
   inputBank=outer[7];inputAddress=outer[9]+T::FinalOffset;
   for(unsigned i=0;i<5120;i++)wantedInput[i]=WordAt(PioneerQ38ProjectionRoute(inputBank),inputAddress+i*4);
  }
  StageBlock(tokens,trial,layer);inputObserved=false;
  if(!chain){
   inputBank=outer[7];inputAddress=outer[9]+0x480000;
   for(unsigned i=0;i<5120;i++)wantedInput[i]=memory.at(T::Original+i*4);
  }
  for(unsigned i=0;i<5120;i++){
   // Retain actual previous GDDR output bytes across RAW endpoint restaging.
   // Never compute a replacement vector or write processor/private state.
   dram[Key(PioneerQ38ProjectionRoute(inputBank),inputAddress+i*4)]=wantedInput[i];
   memory[T::Original+i*4]=memory[T::OriginalCopy+i*4]=0x7fc00001;
  }
  blockOuter[0]=T::Magic;blockOuter[72]=T::Run;blockOuter[73]=inputBank;blockOuter[74]=inputAddress;blockOuter[75]=5120;
  assert(T::RunShape(blockOuter.data()));for(unsigned i=0;i<128;i++)memory[T::Outer+i*4]=blockOuter[i];
 }
 void StageStop(bool requireValid=true){
  sequence++;blockOuter.fill(0);blockOuter[0]=T::Magic;blockOuter[1]=1;blockOuter[2]=128;blockOuter[5]=129;
  for(unsigned i=0;i<16;i++)blockOuter[16+i]=boot[16+i];for(unsigned i=0;i<8;i++)blockOuter[32+i]=boot[8+i];
  blockOuter[48]=sequence;blockOuter[49]=119;blockOuter[72]=T::Stop;if(requireValid)assert(T::StopShape(blockOuter.data()));
  for(unsigned i=0;i<128;i++)memory[T::Outer+i*4]=blockOuter[i];memory[T::Doorbell]=sequence;
 }
 bool WaitLive(unsigned n,bool stop=false,bool quiet=false){
  while(pc!=0x4009c && !(stop?pc==0x40060:pc==0x4004c && memory.at(T::Status+4)==n))StepScalar();
  if(pc==0x4009c){assert(memory.at(T::Status+8)==T::Fault);if(!quiet)fprintf(stderr,"LIVE_BLOCK_FAULT phase%u projections%u packets%u stops%u\n",Phase(),projections,packets,stops);return false;}
  assert(memory.at(T::Status+12)==T::Magic && !memory.at(T::Status+8) && memory.at(T::Status+4)==n);return true;
 }
 void VerifyLive(){
  assert(projections==7 && acceptedJobs==723 && prepared==1 && attentionChecks==1 && prepareReads==15 && memory.at(T::JointSummary)==T::Magic
   && memory.at(T::JointSummary+4)==sequence && scatterWords==59392 && initialNorm==1 && postNorm==1 && activation==1 && finalWords==5120 && quantized==7 && inputObserved);
  for(unsigned i=0;i<5120;i++)assert(memory.at(T::Original+i*4)==wantedInput[i] && memory.at(T::OriginalCopy+i*4)==wantedInput[i]);
  for(unsigned w=0;w<119;w++)assert(child[w]->memory.at(M::Status+4)==firstNonce[w]+T::RunDelta(w));
 }
 void VerifyStop(){
  assert(pc==0x40060 && stops==119 && memory.at(T::StopSummary)==T::StopMagic && memory.at(T::StopSummary+4)==sequence
   && memory.at(T::StopSummary+8)==sequence-1 && memory.at(T::StopSummary+12)==119 && memory.at(T::Status+16)==2 && Phase()==13);
  for(unsigned i=0;i<8;i++)assert(memory.at(T::StopSummary+(8+i)*4)==counts[i] && memory.at(T::StopSummary+(26+i)*4)==boot[8+i]);
  for(unsigned i=0;i<10;i++)assert(memory.at(T::StopSummary+(16+i)*4)==command[i]);
  for(unsigned i=0;i<16;i++)assert(memory.at(T::StopSummary+(34+i)*4)==source[i]);
  for(unsigned w=0;w<119;w++){
   auto& p=*child[w];assert(p.pc==p.stopPC && p.memory.at(M::Status+16)==2 && !p.memory.at(M::Status+8)
    && p.memory.at(M::Status+4)==firstNonce[w]+T::RunDelta(w)+1 && memory.at(T::Nonces+w*4)==p.memory.at(M::Status+4));
  }
  unsigned before=packets;for(unsigned i=0;i<32;i++)StepScalar();assert(packets==before);
 }
};
template<class Change> static void LiveRefusal(LiveBlockMachine& base,unsigned& refused,Change change,bool stop,bool noIO){
 fflush(nullptr);pid_t pid=fork();assert(pid>=0);
 if(!pid){
  auto& m=base;unsigned start=m.packets;change(m);assert(!m.WaitLive(m.sequence,stop,true));
  assert(m.memory.at(T::StopSummary)!=T::StopMagic && m.memory.at(T::Status+4)!=m.sequence);
  if(noIO)assert(m.packets==start);unsigned packets=m.packets;
  for(unsigned i=0;i<32;i++)m.StepScalar();assert(m.packets==packets);_exit(0);
 }
 int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));refused++;
}
#ifndef PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_MACHINE_ONLY
int main(int argc,char** argv){
 assert(argc==5);(void)&BlockInitialRefusals;(void)&BlockLaterRefusals;(void)&JoinedRefusals;(void)&ProjectionRefusals;
 (void)&Refusals;(void)&Group;(void)&Geometry;(void)&Faults;(void)&D::Run;setvbuf(stdout,nullptr,_IOLBF,0);
 puts("START actual QBL1/SAME freshly booted119/NO fabricated upstream job/GDDR input/2 full blocks/realSTOP;NOT native timing");
 auto m=std::make_unique<LiveBlockMachine>(argv[1],argv[2],argv[3],argv[4]);assert(m->WaitLive(0));unsigned refused=0;
 m->StageStop(false);LiveRefusal(*m,refused,[](LiveBlockMachine&){},false,true);m->sequence=0; // invalid COMMAND staging only;no processor/private state changes
 for(unsigned trial=0;trial<2;trial++){
  m->StageLive(trial?33:1,trial,trial?7:3,bool(trial));
  if(!trial){
   for(unsigned word=0;word<128;word++)LiveRefusal(*m,refused,[&](LiveBlockMachine& b){
    b.memory[T::Outer+word*4]^=(word==13 || word==15 || word==65 || word==67 || word==74)?1:0x80000000U;
   },false,true);
   while(!m->AtSubmitRead(m->blockOuter[74],16384)){assert(m->pc!=0x4009c);m->StepScalar();}
   for(unsigned word=1;word<=10;word++)LiveRefusal(*m,refused,[&](LiveBlockMachine& b){b.badReadback=b.readbacks+word;},false,false);
   LiveRefusal(*m,refused,[](LiveBlockMachine& b){b.failPacket=b.packets+1;},false,false);
   for(unsigned w:{1U,64U,119U})LiveRefusal(*m,refused,[&](LiveBlockMachine& b){b.quietBadWorker=w;},false,false);
  }
  assert(m->WaitLive(m->sequence));m->VerifyLive();
  printf("PASS QBL1 frame%u N%u/actual GDDR original%s/ALL7 matrices+RMS+attention+SiLU+residual/SAME119 stack%u\n",m->sequence,trial?33:1,trial?" from previous actual final output":" RAW input",0x70000-m->minStack);
 }
 m->StageStop();
 for(unsigned word=0;word<128;word++)LiveRefusal(*m,refused,[&](LiveBlockMachine& b){b.memory[T::Outer+word*4]^=0x80000000U;},true,true);
 while(!m->AtSubmitRead(0xffb121b0,4)){assert(m->pc!=0x4009c);m->StepScalar();}
 for(unsigned word=1;word<=10;word++)LiveRefusal(*m,refused,[&](LiveBlockMachine& b){b.badReadback=b.readbacks+word;},true,false);
 LiveRefusal(*m,refused,[](LiveBlockMachine& b){b.failPacket=b.packets+1;},true,false);
 LiveRefusal(*m,refused,[](LiveBlockMachine& b){b.sourceFaultPacket=b.packets+1;},true,false);
 assert(m->WaitLive(m->sequence,true));m->VerifyStop();
 printf("PASS QBL1 lifecycle/refusals%u/fresh admission without proof substitution/actual device output->next input/all119 real STOP+distinct root receipt+terminal;NO native/full64/canonical state commit/speed grant\n",refused);
}
#endif
