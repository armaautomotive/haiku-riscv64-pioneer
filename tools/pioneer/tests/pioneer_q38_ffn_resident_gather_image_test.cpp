/* SPDX-License-Identifier: MIT */
// Two ACTUAL emitted images;ONLY NoC/GDDR endpoints are simulated.
#define PIONEER_Q38_FFN_RESIDENT_ACTIVATION_MACHINE_ONLY
#include "pioneer_q38_ffn_resident_activation_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38FfnGatherShape.h"
struct GatherMachine:ResidentActivationMachine {
 std::array<uint32_t,8704> nativeOperands={};
 std::array<std::array<uint32_t,16>,68> scaleLines={};
 std::array<uint32_t,80> compact={};
 unsigned rootOperandReceived=0,compactLocalWrites=0;
 GatherMachine(const char* path,unsigned bank=0,bool edge=false,unsigned layer=0):ResidentActivationMachine(path,bank,edge) {
  auto& d=descriptor;d[0]=TT_Q38_FFN_GATHER_MARKER;d[5]=68;d[7]=0;d[9]=129;
  d[10]=239744;d[11]=295360;d[12]=274560;d[13]=34816;d[14]=320;d[42]=layer;
  seals[0]=0x26005;d[16]=seals[0];SetDescriptor();assert(PioneerQ38FfnGatherShape(d.data()));
  for(unsigned i=0;i<8704;i++){nativeOperands[i]=0x40004000|((i%127)<<16)|((i*31)%127);memory[0xa0000+i*4]=0xdeadbeef;}
  for(unsigned i=0;i<80;i++){memory[0xab000+i*4]=0xdeadbeef;compact[i]=0xdeadbeef;}
  for(unsigned block=0;block<68;block++) {
   auto& line=scaleLines[block];line[0]=(block%2?0xbf800000:0x3f800000);line[1]=TT_Q38_FFN_RESIDENT_MARKER;
   line[2]=d[6];line[3]=block;line[4]=d[42];line[5]=d[40];line[6]=d[41];
   for(unsigned i=0;i<8;i++)line[7+i]=d[32+i];
  }
 }
 uint32_t Load(uint32_t a) override {
  if(Range(a,0xa0000,34816) || Range(a,0xab000,320))return memory.at(a);
  return ResidentActivationMachine::Load(a);
 }
 void Store(uint32_t a,uint32_t value) override {
  if(Range(a,0xab000,320)){assert(!(a&3));compactLocalWrites++;memory[a]=value;return;}
  if(a==0xffb20040){assert(value==1 && !pending);SubmitGather();return;}
  // No local gate/up/activation/native-payload stores belong to gather.
  assert(!Range(a,0x90000,3648));ResidentActivationMachine::Store(a,value);
 }
 void SubmitGather() {
  assert(packets<72);unsigned packet=packets++;bool write=packet==71;
  unsigned local,remote,size;
  if(packet<68){local=0x90e00;remote=descriptor[4]+295360+packet*64;size=64;}
  else if(packet<71){unsigned chunk=packet-68;local=0xa0000+chunk*16384;remote=descriptor[4]+239744+chunk*16384;size=chunk==2?2048:16384;}
  else{local=0xab000;remote=descriptor[4]+274560;size=320;}
  const uint32_t expected[]={write?local:remote,0,write?129U:descriptor[8],write?remote:local,0,write?descriptor[8]:129U,0,write?0x212U:0,size,0};
  for(unsigned i=0;i<10;i++)assert(command[i]==expected[i]);
  if(packets==failPacket){pending=true;return;}
  assert(!activatedWrites && !operandWrites && !nativeScaleWrites);
  for(unsigned i=0;i<size/4;i++) {
   if(packet<68)memory[local+i*4]=scaleLines[packet][i];
   else if(packet<71){unsigned word=(packet-68)*4096+i;memory[local+i*4]=nativeOperands[word];rootOperandReceived++;}
   else compact[i]=memory.at(local+i*4);
  }
  unsigned at=write?4:0;counts[at]++;counts[at+1]+=size/64;counts[2]++;counts[6]=counts[2];counts[at+3]++;
  if(packets==badCounter)counts[at+3]++;
  if(packets==foreignCounter)counts[write?0:4]++;
  if(packets==sourceFault)seals[11]^=1;
  if(packets==descriptorFault)memory[TT_Q38_FFN_RESIDENT_DESCRIPTOR+40*4]^=1;
 }
 bool RunGather() {
  while(pc!=0x64038 && pc!=0x64058)StepScalar();
  assert(l1[Index(pc)]==0x0000006f && minStack>=0x6e000);
  if(pc==0x64058) {
   assert(memory.at(0x17ffc8)==TT_Q38_FFN_GATHER_FAULT && !memory.at(0x17ffc4));
   unsigned oldPackets=packets,oldReads=nocReads,oldWrites=commandWrites,oldCompact=compactLocalWrites;
   for(unsigned i=0;i<16;i++)StepScalar();
   assert(packets==oldPackets && nocReads==oldReads && commandWrites==oldWrites && compactLocalWrites==oldCompact);return false;
  }
  assert(memory.at(0x17ffc8)==TT_Q38_FFN_GATHER_MARKER && memory.at(0x17ffc0)==0x6400c);
  return memory.at(0x17ffc4)==68;
 }
 void VerifyGather() {
  assert(packets==72 && !pending && rootOperandReceived==8704 && compactLocalWrites==148);
  assert(!activatedWrites && !operandWrites && !nativeScaleWrites);
  for(unsigned i=0;i<8704;i++)assert(memory.at(0xa0000+i*4)==nativeOperands[i]);
  for(unsigned i=0;i<80;i++)assert(compact[i]==(i<68?scaleLines[i][0]:0) && memory.at(0xab000+i*4)==compact[i]);
  for(unsigned i=0;i<64;i++)assert(memory.at(TT_Q38_FFN_RESIDENT_DESCRIPTOR+i*4)==descriptor[i]);
  uint32_t s[32],before[8],after[8];for(unsigned i=0;i<32;i++)s[i]=memory.at(TT_Q38_FFN_RESIDENT_SUMMARY+i*4);
  for(unsigned i=0;i<8;i++){before[i]=s[8+i];after[i]=s[16+i];}
  assert(PioneerQ38FfnGatherSummary(s,descriptor.data(),before,after));
  for(unsigned i=0;i<32;i++){s[i]^=1;assert(!PioneerQ38FfnGatherSummary(s,descriptor.data(),before,after));s[i]^=1;}
 }
};
int main(int argc,char** argv)
{
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);unsigned producers=0,gathers=0,negative=0,maxSteps=0,minStack=0x70000;
 {
  Fixture fixture;PioneerQwen38Gguf model;assert(model.Open(fixture.file));PioneerQwen38PackedPlan plan;assert(plan.Build(model,8192,true,true));
  for(unsigned bank=0;bank<7;bank++) {
   const unsigned routes[]={786,978,1170,785,977,1169,1361};assert(PioneerQ38SpanRoute(bank)==routes[bank]);
   PioneerQ38FfnScratchLayout layout;assert(PioneerQ38FfnScratchLayout::Describe(model,plan,bank,layout));
   const auto& native=layout.slice[layout.QuantActivated];
   assert(TT_Q38_FFN_COMPACT_SCALE_OFFSET==native.offset+native.activationBytes && native.scaleBytes==320);
   assert(TT_Q38_FFN_COMPACT_SCALE_OFFSET+320==native.offset+native.bytes
    && native.offset+native.bytes==layout.slice[layout.Output].offset);
  }
 }
 for(unsigned bank=0;bank<7;bank++)for(bool edge:{false,true}) {
  unsigned layer=bank*9+unsigned(edge);GatherMachine root(argv[2],bank,edge,layer);std::vector<float> activated(17408);
  for(unsigned block=0;block<68;block++) {
   unsigned physical=block+1;const unsigned columns[]={1,2,3,4,5,6,7,10,11,12,13,14};
   assert(PioneerQ38FfnResidentPeer(block)==(columns[physical/10]|((physical%10+2)<<6)) && PioneerQ38FfnResidentPeer(block)!=129);
   ResidentActivationMachine producer(argv[1],bank,edge,block,(bank+unsigned(edge)+block)%8);
   producer.descriptor[42]=layer;producer.SetDescriptor();assert(producer.RunResident());producer.VerifyResident(activated.data()+block*256);producers++;
   memcpy(root.nativeOperands.data()+block*128,producer.remoteNative.operand,512);memcpy(root.scaleLines[block].data(),producer.remoteNative.scale,64);
  }
  auto operands=root.nativeOperands;auto lines=root.scaleLines;assert(root.RunGather());root.VerifyGather();gathers++;
  assert(root.nativeOperands==operands && root.scaleLines==lines);maxSteps=std::max(maxSteps,root.steps);minStack=std::min(minStack,root.minStack);
  PioneerQ38CohortInput current;assert(PioneerQ38CohortQuantize(12,17408,activated.data(),current));
  for(unsigned i=0;i<8704;i++)assert(root.memory.at(0xa0000+i*4)==current.activation[i]);
  for(unsigned i=0;i<80;i++)assert(root.memory.at(0xab000+i*4)==current.scales[i]);
  printf("OFFLINE ACTUAL peer->GDDR->ROOT0 gather bank%u edge%u ALL68fresh producers/17408lanes/34816operandB/320compactscaleB/exact old matrix-root input PASS\n",bank,unsigned(edge));
 }
 // Every producer/each64B line word:bad scale or stale operation metadata
 // must refuse BEFORE any operand read,compact GDDR write or model dispatch.
 for(unsigned block=0;block<68;block++)for(unsigned word=0;word<16;word++) {
  GatherMachine m(argv[2]);if(!word)m.scaleLines[block][0]=0x7fc00001;else m.scaleLines[block][word]^=1;
  auto before=m.compact;assert(!m.RunGather() && m.packets==block+1 && !m.rootOperandReceived && m.compact==before);negative++;
 }
 for(unsigned packet:{1U,34U,68U,69U,70U,71U,72U})for(unsigned fault=0;fault<5;fault++) {
  GatherMachine m(argv[2]);if(fault==0)m.failPacket=packet;if(fault==1)m.badCounter=packet;
  if(fault==2)m.foreignCounter=packet;if(fault==3)m.sourceFault=packet;if(fault==4)m.descriptorFault=packet;
  assert(!m.RunGather() && m.packets==packet);negative++;
 }
 for(unsigned packet:{1U,34U,68U,69U,70U,71U,72U})for(unsigned word=0;word<10;word++) {
  GatherMachine m(argv[2]);m.badCommandRead=(packet-1)*10+word+1;assert(!m.RunGather() && m.packets==packet-1);negative++;
 }
 for(unsigned source=0;source<16;source++){GatherMachine m(argv[2]);m.seals[source]^=1;assert(!m.RunGather() && !m.packets);negative++;}
 for(unsigned offset:{0x40U,0x840U,0x1040U,0x1840U,0x240U,0x2bcU}) {
  GatherMachine m(argv[2]);m.busyOffset=offset;assert(!m.RunGather() && !m.packets);negative++;
 }
 for(unsigned field:{0U,1U,2U,5U,7U,8U,9U,10U,11U,12U,13U,14U,15U}) {
  GatherMachine m(argv[2]);m.descriptor[field]^=1;m.SetDescriptor();assert(!m.RunGather() && !m.packets && !m.nocReads);negative++;
 }
 for(unsigned field=43;field<64;field++) {
  GatherMachine m(argv[2]);m.descriptor[field]=1;m.SetDescriptor();assert(!m.RunGather() && !m.packets && !m.nocReads);negative++;
 }
 for(unsigned fault=0;fault<12;fault++) {
  GatherMachine m(argv[2]);auto& d=m.descriptor;
  if(fault==0)d[3]=7;if(fault==1)d[4]=0x04000001;if(fault==2)d[4]=0x03fff000;if(fault==3)d[4]=0xc0001000;
  if(fault==4)d[6]=0;if(fault==5)d[6]=0x7fffffff;if(fault==6)d[16]=0;if(fault==7)d[42]=64;
  if(fault>=8)d[32+(fault-8)*2]=d[33+(fault-8)*2]=0;
  m.SetDescriptor();assert(!m.RunGather() && !m.packets && !m.nocReads);negative++;
 }
 printf("PASS OFFLINE TWO ACTUAL IMAGES producers%u ROOTgathers%u refusals%u rootmaxsteps%u rootminstack%08x;ALL7banks/both3GiBedges/ALL68fresh scale tags/ALL4owner epochs+token+layer+nonce/72sealed NoC packets+wrapped counters/old matrix-root L1 input byte-exact/no host PCI/no CPU products/inert fault/no operand fetch before ALLtags;NO_NATIVE_OWNER_SCHEDULER_FULL_FFN_OR_SPEED_AUTHORITY\n",producers,gathers,negative,maxSteps,minStack);
}
