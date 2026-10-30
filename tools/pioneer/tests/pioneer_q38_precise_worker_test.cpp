/* SPDX-License-Identifier: MIT */
// ACTUAL new worker instructions, synthetic NoC/GDDR endpoints. No ROOT,
// processor-history setter, model expected-product upload or native grant.
#define PIONEER_Q38_GLOBAL_WORKER_INITIAL_MACHINE_ONLY
#include "pioneer_q38_global_worker_initial_test.cpp"
#include "pioneer_q38_sigmoid_precision_host_bridge.h"
#include "pioneer_q38_projection_input_reference.h"
#include "../PioneerQ38PreciseWorkerLayout.h"
namespace P=PioneerQ38PreciseWorkerLayout;
struct PrecisePeer:GlobalInitialPeer {
 std::vector<uint8_t> precise;
 std::array<uint32_t,32> freshLine={};
 unsigned outerJobFaultPacket=0;
 PrecisePeer(const char* old,const char* image,unsigned worker):GlobalInitialPeer(old,image,worker),
  precise(Read((std::string(image)+"/precise.bin").c_str())){
  assert(!steps && pc==0x6000 && !precise.empty() && precise.size()<=0x10000);
  boot[0]=P::Magic;for(unsigned i=40;i<49;i++)boot[i]=P::BootWord(i);
  assert(P::BootShape(boot.data()) && !G::BootShape(boot.data()));
  for(unsigned i=0;i<64;i++)memory[M::Boot+i*4]=boot[i];
 }
 const std::vector<uint8_t>* Rom(unsigned a,unsigned& base)const override{
  if(Range(a,0x30000,unsigned(precise.size()))){base=0x30000;return &precise;}
  return GlobalInitialPeer::Rom(a,base);
 }
 uint32_t Load(uint32_t a)override{
  if(activation && Range(a,0x90000,3712))return memory.at(a);
  return GlobalInitialPeer::Load(a);
 }
 void Store(uint32_t a,uint32_t value)override{
  if(Range(a,M::Status,64)){
   assert(!pending || (a==M::Status+8 && value==P::Fault));memory[a]=value;return;
  }
  if(activation && Range(a,0x90800,1664)){
   assert(!pending && !(a&3));
   if(Range(a,0x90800,1024))activatedWrites++;
   if(Range(a,0x90c00,512))nativeWrites++;
   if(Range(a,0x90e00,128))scaleWrites++;
   memory[a]=value;return;
  }
  GlobalInitialPeer::Store(a,value);
 }
 void StepBlock()override{
  if(!Range(pc,0x30000,unsigned(precise.size())))GlobalInitialPeer::StepBlock();
  else MixedPeer::StepBlock(); // existing F32 engine for every virtual ROM
  if(regs[2]){assert(regs[2]>=0x6e000 && regs[2]<=0x70000);minStack=std::min(minStack,unsigned(regs[2]));}
 }
 void SubmitActivation()override{
  ClearTensorState();assert(activationPackets<4);unsigned p=activationPackets++;bool write=p>=2;
  const unsigned local[]={0x90000,0x90400,0x90c00,0x90e00},bytes[]={1024,1024,512,128};
  unsigned remote=descriptor[4]+descriptor[10+p];
  const uint32_t expected[]={write?local[p]:remote,0,write?descriptor[9]:descriptor[8],write?remote:local[p],0,
   write?descriptor[8]:descriptor[9],0,write?0x212U:0,bytes[p],0};
  for(unsigned i=0;i<10;i++)assert(command[i]==expected[i]);
  if(activationPackets==failPacket){pending=true;return;}
  if(!write)assert(!activatedWrites && !nativeWrites && !scaleWrites);
  else assert(activatedWrites==256 && nativeWrites==128 && scaleWrites==62);
  for(unsigned i=0;i<bytes[p]/4;i++){
   if(p==0)memory[local[p]+i*4]=gate[i];if(p==1)memory[local[p]+i*4]=up[i];
   if(p==2)native[i]=memory.at(local[p]+i*4);if(p==3)freshLine[i]=memory.at(local[p]+i*4);
  }
  unsigned at=write?4:0;countBase[at]++;countBase[at+1]+=bytes[p]/64;countBase[at+3]++;countBase[2]++;countBase[6]++;
  if(activationPackets==badCounterPacket)countBase[at+3]++;
  if(activationPackets==sourcePacket)source[11]^=1;
  if(activationPackets==descriptorPacket)memory[P::Descriptor+40*4]^=1;
  if(activationPackets==outerJobFaultPacket)memory[M::Job+7*4]^=1;
 }
 void BootPrecise(){
  PoisonDst();Poison();while(pc!=waitPC && pc!=faultPC)StepBlock();
  assert(pc==waitPC && !sequence && !requests && !responses && !pending
   && memory.at(M::Status+12)==P::Magic && !memory.at(M::Status+8));
 }
 void StagePrecise(unsigned seq=1){
  assert(pc==waitPC && !pending);activation=true;sequence=seq;
  for(unsigned i=0;i<8;i++)independentBefore[i]=Count(i);
  activationPackets=activatedWrites=nativeWrites=scaleWrites=0;freshLine.fill(0xa5a5a5a5);
  auto& d=descriptor;d={};d[0]=P::ActivationMagic;d[1]=1;d[2]=67108864;d[3]=(logicalWorker-1)%7;
  d[4]=0x04000000;d[5]=logicalWorker-1;d[6]=2025;d[7]=logicalWorker;d[8]=PioneerQ38ProjectionRoute(d[3]);d[9]=selfRoute;
  d[10]=P::GateOffset+d[5]*1024;d[11]=P::UpOffset+d[5]*1024;
  d[12]=P::NativeOffset+d[5]*512;d[13]=P::LinesOffset+d[5]*128;d[14]=12;d[15]=68;
  for(unsigned i=0;i<16;i++)d[16+i]=source[i];for(unsigned i=0;i<8;i++)d[32+i]=boot[8+i];
  d[40]=0;d[41]=248319;d[42]=0;d[43]=4;
  std::array<uint32_t,64> j={};const unsigned head[]={P::Magic,seq,3,logicalWorker,selfRoute,0,0,d[6]};
  for(unsigned i=0;i<8;i++){j[i]=head[i];j[8+i]=boot[8+i];}
  const unsigned capsule[]={P::ActivationMagic,256,576,d[6],12,0,0,0,0,0};
  for(unsigned i=0;i<10;i++)j[16+i]=capsule[i];
  j[27]=d[41];
  if(logicalWorker<=68)assert(P::DescriptorShape(d.data()) && P::JobShape(j.data(),boot.data(),seq-1,0));
  for(unsigned i=0;i<64;i++){memory[P::Descriptor+i*4]=d[i];memory[M::Job+i*4]=j[i];}
  for(unsigned i=0;i<256;i++){
   gate[i]=Bits(float(int((i*17+logicalWorker*37)%4097)-2048)*.00390625f);
   up[i]=Bits(float(int((i*29+logicalWorker*11)%2049)-1024)*.00390625f);
  }
  memory[M::Doorbell]=seq;
 }
 bool FinishPrecise(){
  while(pc!=faultPC && !(pc==waitPC && memory.at(M::Status+4)==sequence))StepBlock();
  if(pc==faultPC)return false;
  assert(!pending && activationPackets==4 && memory.at(M::Status+12)==P::Magic && !memory.at(M::Status+8)
   && minStack>=0x6e000 && !matrixInstructions && !requests && !responses);
  return true;
 }
 void VerifyResult(){
  float wanted[256];for(unsigned i=0;i<256;i++){
   uint32_t sigmoid;assert(ProbeQ38SigmoidPrecision(gate[i],&sigmoid));
   wanted[i]=(Float(gate[i])*Float(sigmoid))*Float(up[i]);
  }
  PioneerQ38ProjectionInput packed,independent;
  assert(PioneerQ38ProjectionQuantize(12,wanted,256,&packed));ProjectionInputReference(12,wanted,independent);
  assert(!memcmp(&packed,&independent,sizeof(packed)) && !memcmp(native.data(),packed.operand,512)
   && freshLine[0]==packed.scale[0] && P::ScaleLine(freshLine.data(),descriptor.data()));
  for(unsigned i=0;i<256;i++)assert(memory.at(0x90800+i*4)==Bits(wanted[i]));
  const unsigned delta[]={2,32,4,2,2,10,4,2};
  for(unsigned i=0;i<8;i++)assert(uint32_t(memory.at(0x83f20+i*4)-memory.at(0x83f00+i*4))==delta[i]);
  assert(memory.at(0x83f4c)==P::ActivationMagic && memory.at(0x83ff8)==P::ActivationMagic && memory.at(0x83ffc)==17408);
  uint32_t proof[64],job[16],after[8];
  for(unsigned i=0;i<64;i++)proof[i]=memory.at(0x83f00+i*4);
  for(unsigned i=0;i<16;i++)job[i]=memory.at(0x74000+i*4);
  for(unsigned i=0;i<8;i++)after[i]=Count(i);
  assert(P::ActivationProof(proof,job,descriptor.data(),source,logicalWorker,selfRoute,independentBefore,after));
  for(unsigned i=0;i<64;i++){
   proof[i]^=1;assert(!P::ActivationProof(proof,job,descriptor.data(),source,logicalWorker,selfRoute,independentBefore,after));proof[i]^=1;
  }
 }
 void FaultInert(uint32_t expectedCompleted=UINT32_MAX){
  while(pc!=faultPC && pc!=stopPC)StepBlock();
  assert(pc==faultPC && memory.at(M::Status+8)==P::Fault
   && memory.at(M::Status+4)==(expectedCompleted==UINT32_MAX?sequence-1:expectedCompleted));
  const auto frozen=memory;unsigned issued=activationPackets;uint32_t saved[8];for(unsigned i=0;i<8;i++)saved[i]=Count(i);
  for(unsigned i=0;i<2048;i++)StepBlock();assert(pc==faultPC && activationPackets==issued && memory==frozen);
  for(unsigned i=0;i<8;i++)assert(Count(i)==saved[i]);
 }
 void StopAfterPrecise(){
  std::array<uint32_t,64> j={};j[0]=P::Magic;j[1]=++sequence;j[2]=P::Stop;j[3]=logicalWorker;j[4]=selfRoute;j[7]=2026;
  for(unsigned i=0;i<8;i++)j[8+i]=boot[8+i];
  assert(P::JobShape(j.data(),boot.data(),sequence-1,0));
  for(unsigned i=0;i<64;i++)memory[M::Job+i*4]=j[i];memory[M::Doorbell]=sequence;
  while(pc!=stopPC && pc!=faultPC)StepBlock();
  assert(pc==stopPC && memory.at(M::Status+4)==sequence && memory.at(M::Status+16)==2 && !pending);
 }
};
int main(int argc,char** argv){
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;
 unsigned maxStack=0,maxSteps=0;
 for(unsigned worker=1;worker<=68;worker++){
  auto p=std::make_unique<PrecisePeer>(argv[1],argv[2],worker);p->BootPrecise();p->StagePrecise();assert(p->FinishPrecise());p->VerifyResult();
  p->StopAfterPrecise();maxStack=std::max(maxStack,0x70000-p->minStack);maxSteps=std::max(maxSteps,p->steps);
 }
 unsigned faults=0;
 for(unsigned worker:{1U,68U})for(unsigned mode=0;mode<5;mode++){
  auto p=std::make_unique<PrecisePeer>(argv[1],argv[2],worker);p->BootPrecise();p->StagePrecise();
  if(mode==0)p->sourcePacket=1;if(mode==1)p->descriptorPacket=2;if(mode==2)p->outerJobFaultPacket=3;
  if(mode==3)p->gate[255]=0x7fc00001;if(mode==4)p->memory[M::Boot+46*4]^=1;
  p->FaultInert();assert(p->activationPackets==(mode==0?1U:mode==1?2U:mode==2?3U:mode==3?2U:0U));faults++;
 }
 for(unsigned worker:{69U,119U}){
  auto p=std::make_unique<PrecisePeer>(argv[1],argv[2],worker);p->BootPrecise();p->StagePrecise();p->FaultInert();assert(!p->activationPackets);faults++;
 }
 for(unsigned mode=0;mode<5;mode++){
  auto p=std::make_unique<PrecisePeer>(argv[1],argv[2],1);
  if(mode==0){
   p->memory[M::Boot]=G::Magic;p->memory[M::Boot+40*4]=G::JobWitness;
   for(unsigned i=46;i<49;i++)p->memory[M::Boot+i*4]=0;
   p->FaultInert(0xdeadbeef);assert(!p->activationPackets);faults++;continue;
  }
  p->BootPrecise();p->StagePrecise();
  if(mode==1)p->memory[M::Job]=G::Magic;
  if(mode==2)p->memory[P::Descriptor]=TT_Q38_FFN_RESIDENT_MARKER;
  if(mode==3)p->memory[M::Job+27*4]^=1; // actual token disagreement, not changed CPU product
  if(mode==4){
   p->memory[M::Job+2*4]=1;p->memory[M::Job+27*4]=0;
   for(unsigned i=0;i<10;i++)p->memory[M::Job+(16+i)*4]=p->matrixJob[i];
   p->memory[M::Job+26*4]=1; // real down geometry, missing private activation
  }
  p->FaultInert();assert(!p->activationPackets && !p->requests);faults++;
 }
 for(unsigned mode=0;mode<2;mode++){
  auto p=std::make_unique<PrecisePeer>(argv[1],argv[2],1);p->BootPrecise();p->StagePrecise();
  if(mode==0)p->failPacket=1;else p->badCounterPacket=4;
  p->FaultInert();assert(p->activationPackets==(mode?4U:1U));faults++;
 }
 // Actual retained private phase rejects a repeated activation, not a
 // host-supplied completion flag or cloned processor history.
 auto repeat=std::make_unique<PrecisePeer>(argv[1],argv[2],1);repeat->BootPrecise();repeat->StagePrecise();assert(repeat->FinishPrecise());
 repeat->StagePrecise(2);repeat->FaultInert();assert(!repeat->activationPackets);faults++;
 printf("PASS ACTUAL QGW2/QPA1/68 independent virgin-worker precise256+packed outputs+fresh128B lines/68 real returning WAIT->STOP histories/faults%u/maxCombinedStack%u/maxInstructions%u/old QGW1 refused;synthetic NoC endpoints,NOT connected ROOT,all64,native,physical,Work or TPS qualification\n",faults,maxStack,maxSteps);
}
