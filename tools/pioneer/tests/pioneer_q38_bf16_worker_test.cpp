/* SPDX-License-Identifier: MIT */
// Actual immutable full worker + returning producer. Synthetic NoC byte
// endpoints,NOT a completed ROOT,physical inference or timing benchmark.
#define PIONEER_Q38_GLOBAL_WORKER_INITIAL_MACHINE_ONLY
#include "pioneer_q38_global_worker_initial_test.cpp"
#include "pioneer_q38_sigmoid_precision_host_bridge.h"
#include "pioneer_q38_bf16_worker_probe.h"
#include "../PioneerQ38Bf16ProducerLayout.h"
namespace B=PioneerQ38Bf16WorkerProbe;
namespace BP=PioneerQ38Bf16ProducerLayout;
struct Bf16Peer:GlobalInitialPeer {
 std::vector<uint8_t> producer,fpu;
 std::array<uint32_t,32> freshLine={};unsigned sealed=0,outerJobFaultPacket=0;
 Bf16Peer(const char* old,const char* image,unsigned worker):GlobalInitialPeer(old,image,worker),
  producer(Read((std::string(image)+"/bf16producer.bin").c_str())),fpu(Read((std::string(image)+"/cachedfpu.bin").c_str())){
  assert(!steps && pc==0x6000);boot[0]=B::Magic;for(unsigned i=40;i<49;i++)boot[i]=B::BootWord(i);
  assert(B::BootShape(boot.data()) && !G::BootShape(boot.data()));
  for(unsigned i=0;i<64;i++)memory[M::Boot+i*4]=boot[i];
 }
 const std::vector<uint8_t>* Rom(unsigned a,unsigned& base)const override{
  if(Range(a,0x30000,unsigned(producer.size()))){base=0x30000;return &producer;}
  if(Range(a,0x50000,unsigned(fpu.size()))){base=0x50000;return &fpu;}
  return GlobalInitialPeer::Rom(a,base);
 }
 uint32_t Load(uint32_t a)override{
  if(activation && Range(a,0x90000,3712))return memory.at(a);
  return GlobalInitialPeer::Load(a);
 }
 void Store(uint32_t a,uint32_t value)override{
  if(Range(a,M::Status,64)){assert(!pending || (a==M::Status+8 && value==B::Fault));memory[a]=value;return;}
  if(activation && Range(a,0x90800,1664)){
   assert(!pending && !(a&3));if(Range(a,0x90800,1024))activatedWrites++;
   if(Range(a,0x90c00,512))nativeWrites++;if(Range(a,0x90e00,128))scaleWrites++;memory[a]=value;return;
  }
  GlobalInitialPeer::Store(a,value);
 }
 void StepBlock()override{
  // Existing scalar-FP dispatcher,including fetch from every virtual ROM.
  MixedPeer::StepBlock();
  if(regs[27]){if(!sealed)sealed=regs[27];assert(regs[27]==sealed);}
  if(regs[2]){assert(regs[2]>=0x6e000 && regs[2]<=0x70000 && !(regs[2]&15));minStack=std::min(minStack,unsigned(regs[2]));}
 }
 void SubmitActivation()override{
  const unsigned reads=BP::Reads(descriptor.data()),p=activationPackets++;
  assert(p<reads+2);const bool write=p>=reads,trailer=p==reads+1;
  const unsigned local=write?(trailer?0x90e00:0x90c00):(p?0x90400:0x90000),bytes=write?(trailer?128U:512U):1024U;
  const unsigned offset=write?descriptor[trailer?13:12]:descriptor[10+p];
  const unsigned remote=!write && reads==1?offset:descriptor[4]+offset,sourceRoute=!write && reads==1?unsigned(BP::InputRoot):descriptor[8];
  const unsigned expected[]={write?local:remote,0,write?selfRoute:sourceRoute,write?remote:local,0,
   write?descriptor[8]:selfRoute,0,write?0x212U:0,bytes,0};
  for(unsigned i=0;i<10;i++)assert(command[i]==expected[i]);
  if(activationPackets==failPacket){pending=true;return;}
  if(!write)assert(!activatedWrites && !nativeWrites && !scaleWrites);
  else assert(activatedWrites==(reads==2?256U:0U) && nativeWrites==128 && scaleWrites==56);
  // Only ORIGINAL gate/up bytes are served by the read endpoint. The
  // write endpoint consumes actual guest stores;no expected products enter.
  for(unsigned i=0;i<bytes/4;i++){
   if(!write)memory[local+i*4]=p?up[i]:gate[i];
   else if(trailer)freshLine[i]=memory.at(local+i*4);else native[i]=memory.at(local+i*4);
  }
  const unsigned at=write?4:0;countBase[at]++;countBase[at+1]+=bytes/64;countBase[at+3]++;countBase[2]++;countBase[6]++;
  if(activationPackets==badCounterPacket)countBase[at+3]++;
  if(activationPackets==sourcePacket)source[11]^=1;
  if(activationPackets==descriptorPacket)memory[BP::Descriptor+40*4]^=1;
  if(activationPackets==outerJobFaultPacket)memory[M::Job+7*4]^=1;
 }
 void BootBf16(){
  PoisonDst();Poison();while(pc!=waitPC && pc!=faultPC)StepBlock();
  assert(pc==waitPC && !sequence && !requests && !responses && !pending && sealed
   && memory.at(M::Status+12)==B::Magic && !memory.at(M::Status+8));
 }
 void Stage(unsigned phase,unsigned seq=1){
  assert(pc==waitPC && !pending);activation=true;sequence=seq;const bool gated=phase==4;
  for(unsigned i=0;i<8;i++)independentBefore[i]=Count(i);
  activationPackets=activatedWrites=nativeWrites=scaleWrites=0;freshLine.fill(0xa5a5a5a5);
  auto& d=descriptor;d={};d[0]=BP::Magic;d[1]=1;d[2]=67108864;d[3]=(logicalWorker-1)%7;d[4]=0x04000000;
  d[5]=logicalWorker-1;d[6]=2025;d[7]=logicalWorker;d[8]=PioneerQ38ProjectionRoute(d[3]);d[9]=selfRoute;
  d[10]=(gated?unsigned(BP::GateOffset):unsigned(BP::ProjectionInput))+d[5]*1024;d[11]=gated?BP::UpOffset+d[5]*1024:5120;
  d[12]=BP::NativeOffset+d[5]*512;d[13]=BP::LinesOffset+d[5]*128;d[14]=BP::Format;d[15]=gated?68:20;
  for(unsigned i=0;i<16;i++)d[16+i]=source[i];for(unsigned i=0;i<8;i++)d[32+i]=boot[8+i];
  d[40]=0;d[41]=248319;d[42]=0;d[43]=phase;
  std::array<uint32_t,64> j={};const unsigned h[]={B::Magic,seq,gated?3U:5U,logicalWorker,selfRoute,0,0,d[6]};
  for(unsigned i=0;i<8;i++){j[i]=h[i];j[8+i]=boot[8+i];}
  const unsigned capsule[]={BP::Magic,256,512,d[6],12,0,0,0,0,0};for(unsigned i=0;i<10;i++)j[16+i]=capsule[i];
  j[27]=d[41];j[28]=phase;j[29]=gated?17408:5120;j[30]=BP::Format;j[31]=BP::Witness;
  if(logicalWorker<=(gated?68U:20U))assert(BP::DescriptorShape(d.data()) && B::JobShape(j.data(),boot.data(),seq-1,0));
  for(unsigned i=0;i<64;i++){memory[BP::Descriptor+i*4]=d[i];memory[M::Job+i*4]=j[i];}
  for(unsigned i=0;i<256;i++){
   gate[i]=Bits(float(int((i*17+logicalWorker*37)%4097)-2048)*.00390625f);
   up[i]=Bits(float(int((i*29+logicalWorker*11)%2049)-1024)*.00390625f);
  }
  memory[M::Doorbell]=seq;
 }
 bool Finish(){
  while(pc!=faultPC && !(pc==waitPC && memory.at(M::Status+4)==sequence))StepBlock();
  if(pc==faultPC)return false;
  assert(!pending && activationPackets==BP::Reads(descriptor.data())+2 && !matrixInstructions && !requests && !responses
   && memory.at(M::Status+12)==B::Magic && !memory.at(M::Status+8));return true;
 }
 void Verify(bool print=false){
  for(unsigned i=0;i<256;i++){
   float wanted=Float(gate[i]);if(BP::Reads(descriptor.data())==2){uint32_t sigmoid;assert(ProbeQ38SigmoidPrecision(gate[i],&sigmoid));wanted=(wanted*Float(sigmoid))*Float(up[i]);}
   const unsigned bits=Bits(wanted),low=bits&65535,high=bits>>16;
   const unsigned bf16=high+unsigned(low>32768 || (low==32768 && (high&1)));
   assert(uint16_t(native[i/2]>>((i&1)*16))==bf16);
   if(BP::Reads(descriptor.data())==2)assert(memory.at(0x90800+i*4)==Bits(wanted));
   if(print && i<128)printf("ACTUAL_BF16_PRODUCER bits%08x value%04x\n",Bits(wanted),bf16);
  }
  uint32_t proof[64],job[16],after[8];for(unsigned i=0;i<64;i++)proof[i]=memory.at(0x83f00+i*4);
  for(unsigned i=0;i<16;i++)job[i]=memory.at(0x74000+i*4);for(unsigned i=0;i<8;i++)after[i]=Count(i);
  assert(BP::Line(freshLine.data(),descriptor.data(),sequence) && BP::Proof(proof,job,descriptor.data(),source,logicalWorker,selfRoute,independentBefore,after));
  for(unsigned i=0;i<64;i++){proof[i]^=1;assert(!BP::Proof(proof,job,descriptor.data(),source,logicalWorker,selfRoute,independentBefore,after));proof[i]^=1;}
  for(unsigned i=0;i<32;i++){freshLine[i]^=1;assert(!BP::Line(freshLine.data(),descriptor.data(),sequence));freshLine[i]^=1;}
 }
 void FaultInert(unsigned completed=UINT32_MAX){
  while(pc!=faultPC && pc!=stopPC)StepBlock();assert(pc==faultPC && memory.at(M::Status+8)==B::Fault
   && memory.at(M::Status+4)==(completed==UINT32_MAX?sequence-1:completed));
  const auto frozen=memory;const unsigned issued=activationPackets;uint32_t counts[8];for(unsigned i=0;i<8;i++)counts[i]=Count(i);
  for(unsigned i=0;i<2048;i++)StepBlock();assert(pc==faultPC && activationPackets==issued && memory==frozen);
  for(unsigned i=0;i<8;i++)assert(Count(i)==counts[i]);
 }
 void Stop(){
  std::array<uint32_t,64> j={};j[0]=B::Magic;j[1]=++sequence;j[2]=B::Stop;j[3]=logicalWorker;j[4]=selfRoute;j[7]=2026;
  for(unsigned i=0;i<8;i++)j[8+i]=boot[8+i];assert(B::JobShape(j.data(),boot.data(),sequence-1,0));
  for(unsigned i=0;i<64;i++)memory[M::Job+i*4]=j[i];memory[M::Doorbell]=sequence;
  while(pc!=stopPC && pc!=faultPC)StepBlock();assert(pc==stopPC && memory.at(M::Status+4)==sequence && !pending);
 }
};
int main(int argc,char** argv){
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;
 unsigned jobs=0,words=0,maxStack=0,faults=0;
 for(unsigned phase:{2U,3U,4U})for(unsigned worker=1;worker<=(phase==4?68U:20U);worker++){
  auto p=std::make_unique<Bf16Peer>(argv[1],argv[2],worker);p->BootBf16();p->Stage(phase);assert(p->Finish());p->Verify(phase==4 && worker==1);
  p->Stop();maxStack=std::max(maxStack,0x70000-p->minStack);jobs++;words+=128;
 }
 for(unsigned phase:{2U,4U})for(unsigned mode=0;mode<7;mode++){
  auto p=std::make_unique<Bf16Peer>(argv[1],argv[2],1);p->BootBf16();p->Stage(phase);
  if(mode==0)p->sourcePacket=1;if(mode==1)p->descriptorPacket=1;if(mode==2)p->outerJobFaultPacket=1;
  if(mode==3)p->gate[255]=0x7fc00001;if(mode==4)p->failPacket=1;if(mode==5)p->badCounterPacket=1;
  if(mode==6)p->memory[BP::Descriptor]=TT_Q38_PROJECTION_INPUT_MARKER;
  p->FaultInert();assert(p->activationPackets==(mode==6?0U:mode==3?(phase==4?2U:1U):1U));faults++;
 }
 for(unsigned worker:{21U,69U,119U}){
  auto p=std::make_unique<Bf16Peer>(argv[1],argv[2],worker);p->BootBf16();p->Stage(worker==21?2:4);p->FaultInert();assert(!p->activationPackets);faults++;
 }
 auto warm=std::make_unique<Bf16Peer>(argv[1],argv[2],1);warm->BootBf16();warm->Stage(2);assert(warm->Finish());warm->Verify();
 warm->Stage(3,2);assert(warm->Finish());warm->Verify();warm->Stage(4,3);assert(warm->Finish());warm->Verify();
 warm->Stage(4,4);warm->FaultInert();assert(!warm->activationPackets);faults++;
 printf("BF16_WORKER_OBSERVED jobs%u words%u faults%u stack%u\n",jobs,words,faults,maxStack);
 assert(jobs==108 && words==13824 && faults==18 && maxStack<=8192);
 printf("PASS ACTUAL QGW4 full immutable worker returning BF16 producers jobs%u/packedWords%u/faults%u/maxCombinedStack%u;actual gate/up reads and BF16/trailer writes/private seal/WAIT-to-STOP/repeated projection-to-gated history;NO integer quantization or CPU fallback;Q4 dispatch LINKED not executed,NOT connected ROOT/native/hardware/full-model quality or TPS proof\n",jobs,words,faults,maxStack);
}
