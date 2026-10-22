/* SPDX-License-Identifier: MIT */
// Actual emitted instructions;ONLY NoC/GDDR endpoints are simulated.
#define PIONEER_Q38_FFN_NATIVE_ACTIVATION_MACHINE_ONLY
#include "pioneer_q38_ffn_native_activation_image_test.cpp"
#define PIONEER_Q38_FIXTURE_ONLY
#include "pioneer_qwen38_gguf_test.cpp"
#include "../PioneerQ38FfnScratchLayout.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38FfnResidentShape.h"
#include <array>

struct ResidentActivationMachine:NativeActivationMachine {
 std::array<uint32_t,64> descriptor={};
 std::array<uint32_t,256> remoteGate={},remoteUp={};
 Native remoteNative;
 uint32_t command[10]={},counts[8]={},seals[16]={};
 unsigned packets=0,nocReads=0,commandReads=0,commandWrites=0;
 unsigned failPacket=0,badCounter=0,foreignCounter=0,sourceFault=0,descriptorFault=0,badCommandRead=0,busyOffset=0;
 bool pending=false;
 static unsigned SourceOffset(unsigned i) {
  const unsigned o[]={0x100,0x118,0x11c,0x120,0x124,0x128,0x12c,0x130,0x134,0x138,0x13c,0x140,0x144,0x150,0x154,0x16c};return o[i];
 }
 ResidentActivationMachine(const char* path,unsigned bank=0,bool edge=false,unsigned block=0,unsigned trial=1)
  :NativeActivationMachine(path,trial,block) {
  for(unsigned i=0;i<256;i++) {
   remoteGate[i]=memory.at(0x90000+i*4);remoteUp[i]=memory.at(0x90400+i*4);
   memory[0x90000+i*4]=memory[0x90400+i*4]=0x7fc00001;
  }
  memset(&remoteNative,0xa5,sizeof(remoteNative));for(auto& word:command)word=0xdeadbeef;
  for(auto& value:counts)value=0xfffffff8;
  for(unsigned i=0;i<16;i++)seals[i]=0xa5000000+i*0x13579;seals[0]=0x16005;
  auto& d=descriptor;d[0]=TT_Q38_FFN_RESIDENT_MARKER;d[1]=1;d[2]=67108864;d[3]=bank;d[4]=edge?0xc0000000:0x04000000;
  d[5]=block;d[6]=17;d[7]=block+1;d[8]=PioneerQ38SpanRoute(bank);d[9]=PioneerQ38FfnResidentPeer(block);
  d[10]=30848+block*1024;d[11]=100480+block*1024;d[12]=239744+block*512;d[13]=295360+block*64;d[14]=256;
  for(unsigned i=0;i<16;i++)d[16+i]=seals[i];for(unsigned i=32;i<40;i++)d[i]=0x13570000+i;
  d[40]=123;d[41]=7;d[42]=block%64;
  SetDescriptor();for(unsigned i=0;i<32;i++)memory[TT_Q38_FFN_RESIDENT_SUMMARY+i*4]=0xdeadbeef;
  assert(PioneerQ38FfnResidentShape(d.data()));
 }
 void SetDescriptor(){for(unsigned i=0;i<64;i++)memory[TT_Q38_FFN_RESIDENT_DESCRIPTOR+i*4]=descriptor[i];}
 uint32_t Load(uint32_t at) override {
  if(Range(at,0xffb20000,0x2000)) {
   nocReads++;unsigned offset=at-0xffb20000;
   if(offset<40){uint32_t value=command[offset/4];if(++commandReads==badCommandRead)value^=1;return value;}
   if(busyOffset && offset==busyOffset)return 1;
   for(unsigned i=0;i<4;i++)if(offset==0x40+i*0x800)return i?0:unsigned(pending);
   if(offset>=0x240 && offset<0x2c0)return 0;
   for(unsigned i=0;i<16;i++)if(offset==SourceOffset(i))return seals[i];
   const unsigned offsets[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};
   for(unsigned i=0;i<8;i++)if(offset==offsets[i])return counts[i];
   assert(false && "unqualified resident FFN NoC read");
  }
  if(Range(at,TT_Q38_FFN_RESIDENT_DESCRIPTOR,256) || Range(at,TT_Q38_FFN_RESIDENT_SUMMARY,128))return memory.at(at);
  assert(!Range(at,0x74000,16)); // resident path NEVER reads the local/host payload header
  return NativeActivationMachine::Load(at);
 }
 void Submit() {
  assert(packets<4);unsigned packet=packets++;bool write=packet>=2;
  const unsigned offsets[]={30848+descriptor[5]*1024,100480+descriptor[5]*1024,239744+descriptor[5]*512,295360+descriptor[5]*64};
  const unsigned locals[]={0x90000,0x90400,0x90c00,0x90e00},sizes[]={1024,1024,512,64};
  unsigned remote=descriptor[4]+offsets[packet],local=locals[packet],size=sizes[packet];
  const uint32_t expected[]={write?local:remote,0,write?descriptor[9]:descriptor[8],write?remote:local,0,write?descriptor[8]:descriptor[9],0,write?0x212U:0,size,0};
  for(unsigned i=0;i<10;i++)assert(command[i]==expected[i]);
  if(packets==failPacket){pending=true;return;}
  if(!write)assert(!activatedWrites && !operandWrites && !nativeScaleWrites);
  else assert(activatedWrites==256 && operandWrites==128 && nativeScaleWrites==31);
  for(unsigned i=0;i<size/4;i++) {
   if(packet==0)memory[local+i*4]=remoteGate[i];
   if(packet==1)memory[local+i*4]=remoteUp[i];
   if(packet==2)remoteNative.operand[i]=memory.at(local+i*4);
   if(packet==3)remoteNative.scale[i]=memory.at(local+i*4);
  }
  unsigned at=write?4:0;counts[at]++;counts[at+1]+=size/64;counts[2]++;counts[6]=counts[2];counts[at+3]++;
  if(packets==badCounter)counts[at+3]++;
  if(packets==foreignCounter)counts[write?0:4]++;
  if(packets==sourceFault)seals[11]^=1;
  if(packets==descriptorFault)memory[TT_Q38_FFN_RESIDENT_DESCRIPTOR+40*4]^=1;
 }
 void Store(uint32_t at,uint32_t value) override {
  if(Range(at,0xffb20000,40)){assert(!pending);commandWrites++;command[(at-0xffb20000)/4]=value;return;}
  if(at==0xffb20040){assert(value==1 && !pending);Submit();return;}
  if(Range(at,TT_Q38_FFN_RESIDENT_SUMMARY,128)){memory[at]=value;return;}
  NativeActivationMachine::Store(at,value);
 }
 bool RunResident() {
  while(pc!=0x64038 && pc!=0x64058)StepScalar();
  assert(l1[Index(pc)]==0x0000006f && minStack>=0x6e000);
  for(uint32_t guard:{0x73ffcU,0x74010U,0x8fffcU,0x90e40U})assert(memory.at(guard)==0xdecafbad);
  if(pc==0x64058) {
   assert(memory.at(0x17ffc8)==TT_Q38_FFN_RESIDENT_FAULT && !memory.at(0x17ffc4));
   unsigned oldReads=nocReads,oldPackets=packets,oldWrites=commandWrites,oldMath=activatedWrites;
   for(unsigned i=0;i<16;i++)StepScalar();
   assert(nocReads==oldReads && packets==oldPackets && commandWrites==oldWrites && activatedWrites==oldMath);return false;
  }
  assert(memory.at(0x17ffc8)==TT_Q38_FFN_RESIDENT_MARKER && memory.at(0x17ffc0)==0x6400c);
  return memory.at(0x17ffc4)==256;
 }
 void VerifyResident(float* actual) {
  assert(packets==4 && !pending && activatedWrites==256 && operandWrites==128 && nativeScaleWrites==31);
  uint32_t quantized[73];Native expected;
  for(unsigned i=0;i<256;i++) {
   actual[i]=PioneerAttentionFloat(memory.at(0x90800+i*4));float g=PioneerAttentionFloat(remoteGate[i]),u=PioneerAttentionFloat(remoteUp[i]);
   float reference=(g/(1.0f+expf(-g)))*u;assert(isfinite(actual[i]) && fabsf(actual[i]-reference)<=3e-6f*fabsf(reference));
  }
  Q38ActivationReference(actual,quantized);NativeReference(quantized,expected);
  expected.scale[1]=TT_Q38_FFN_RESIDENT_MARKER;expected.scale[2]=descriptor[6];expected.scale[3]=descriptor[5];expected.scale[4]=descriptor[42];
  expected.scale[5]=descriptor[40];expected.scale[6]=descriptor[41];for(unsigned i=0;i<8;i++)expected.scale[7+i]=descriptor[32+i];
  assert(!memcmp(&expected,&remoteNative,sizeof(expected)) && PioneerQ38FfnResidentScale(remoteNative.scale,descriptor.data(),descriptor[5]));
  for(unsigned i=0;i<128;i++)assert(memory.at(0x90c00+i*4)==expected.operand[i]);
  for(unsigned i=0;i<16;i++)assert(memory.at(0x90e00+i*4)==expected.scale[i]);
  for(unsigned i=0;i<256;i++)assert(memory.at(0x90000+i*4)==remoteGate[i] && memory.at(0x90400+i*4)==remoteUp[i]);
  for(unsigned i=0;i<64;i++)assert(memory.at(TT_Q38_FFN_RESIDENT_DESCRIPTOR+i*4)==descriptor[i]);
  uint32_t s[32],before[8],after[8];for(unsigned i=0;i<32;i++)s[i]=memory.at(TT_Q38_FFN_RESIDENT_SUMMARY+i*4);
  for(unsigned i=0;i<8;i++){before[i]=s[8+i];after[i]=s[16+i];}
  assert(PioneerQ38FfnResidentSummary(s,descriptor.data(),before,after));
  for(unsigned i=0;i<32;i++){s[i]^=1;assert(!PioneerQ38FfnResidentSummary(s,descriptor.data(),before,after));s[i]^=1;}
 }
};

#ifndef PIONEER_Q38_FFN_RESIDENT_ACTIVATION_MACHINE_ONLY
static void Geometry()
{
 Fixture fixture;PioneerQwen38Gguf model;assert(model.Open(fixture.file));PioneerQwen38PackedPlan plan;assert(plan.Build(model,8192,true,true));
 for(unsigned bank=0;bank<7;bank++) {
  const unsigned routes[]={786,978,1170,785,977,1169,1361};assert(PioneerQ38SpanRoute(bank)==routes[bank]);
  PioneerQ38FfnScratchLayout layout;assert(PioneerQ38FfnScratchLayout::Describe(model,plan,bank,layout));
  assert(layout.slice[layout.Gate].offset==TT_Q38_FFN_GATE_OFFSET && layout.slice[layout.Up].offset==TT_Q38_FFN_UP_OFFSET
   && layout.slice[layout.QuantActivated].offset==TT_Q38_FFN_NATIVE_OFFSET && layout.bytes==TT_Q38_FFN_SCALE_LINES_OFFSET);
  auto& workspace=plan.Parts()[layout.workspaceFragment];assert(TT_Q38_FFN_RESIDENT_BYTES<=workspace.bytes);
  assert(TT_Q38_FFN_SCALE_LINES_OFFSET+TT_Q38_FFN_SCALE_LINES_BYTES==TT_Q38_FFN_RESIDENT_BYTES);
  for(unsigned block=0;block<68;block++) {
   unsigned physical=block+1;const unsigned columns[]={1,2,3,4,5,6,7,10,11,12,13,14};
   assert(PioneerQ38FfnResidentPeer(block)==(columns[physical/10]|((physical%10+2)<<6)) && PioneerQ38FfnResidentPeer(block)!=129);
   assert(TT_Q38_FFN_GATE_OFFSET+block*1024+1024<=layout.slice[layout.Gate].offset+layout.slice[layout.Gate].bytes);
   assert(TT_Q38_FFN_UP_OFFSET+block*1024+1024<=layout.slice[layout.Up].offset+layout.slice[layout.Up].bytes);
   assert(TT_Q38_FFN_NATIVE_OFFSET+block*512+512<=layout.slice[layout.QuantActivated].offset+layout.slice[layout.QuantActivated].activationBytes);
   unsigned scale=TT_Q38_FFN_SCALE_LINES_OFFSET+block*64;assert(!(scale&63) && scale>=layout.bytes && scale+64<=workspace.bytes);
   for(unsigned other=0;other<block;other++)assert(TT_Q38_FFN_SCALE_LINES_OFFSET+other*64+64<=scale);
  }
 }
 assert(PioneerQ38FfnResidentPeer(68)==UINT32_MAX);
}
int main(int argc,char** argv)
{
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);Geometry();unsigned positive=0,negative=0,maxSteps=0,minStack=0x70000;
 for(unsigned bank=0;bank<7;bank++)for(bool edge:{false,true}) {
  std::vector<float> activated(17408);PioneerQ38CohortInput assembled;assembled.type=12;assembled.inputs=17408;
  for(unsigned block=0;block<68;block++) {
   ResidentActivationMachine m(argv[1],bank,edge,block,(bank+unsigned(edge)+block)%8);auto g=m.remoteGate,u=m.remoteUp;
   assert(m.RunResident());m.VerifyResident(activated.data()+block*256);positive++;
   assert(m.remoteGate==g && m.remoteUp==u);
   memcpy(assembled.activation+block*128,m.remoteNative.operand,512);assembled.scales[block]=m.remoteNative.scale[0];
   maxSteps=std::max(maxSteps,m.steps);minStack=std::min(minStack,m.minStack);
  }
  PioneerQ38CohortInput current;assert(PioneerQ38CohortQuantize(12,17408,activated.data(),current));
  assert(!memcmp(assembled.activation,current.activation,sizeof(current.activation)) && !memcmp(assembled.scales,current.scales,sizeof(current.scales)));
  printf("OFFLINE ACTUAL resident FFN activation bank%u edge%u ALL68nonroot blocks/17408lanes/GDDRread->nativeGDDRwrite/all8wrapped counters/exact host wire PASS\n",bank,unsigned(edge));
 }
 for(unsigned packet=1;packet<=4;packet++)for(unsigned fault=0;fault<5;fault++) {
  ResidentActivationMachine m(argv[1]);if(fault==0)m.failPacket=packet;if(fault==1)m.badCounter=packet;
  if(fault==2)m.foreignCounter=packet;if(fault==3)m.sourceFault=packet;if(fault==4)m.descriptorFault=packet;
  assert(!m.RunResident() && m.packets==packet);negative++;
 }
 for(unsigned read=1;read<=40;read++){ResidentActivationMachine m(argv[1]);m.badCommandRead=read;assert(!m.RunResident() && m.packets==(read-1)/10);negative++;}
 for(unsigned source=0;source<16;source++){ResidentActivationMachine m(argv[1]);m.seals[source]^=1;assert(!m.RunResident() && !m.packets && !m.activatedWrites);negative++;}
 for(unsigned offset:{0x40U,0x840U,0x1040U,0x1840U,0x240U,0x2bcU}) {
  ResidentActivationMachine m(argv[1]);m.busyOffset=offset;assert(!m.RunResident() && !m.packets && !m.activatedWrites);negative++;
 }
 for(unsigned field:{0U,1U,2U,7U,8U,9U,10U,11U,12U,13U,14U,15U}) {
  ResidentActivationMachine m(argv[1]);m.descriptor[field]^=1;m.SetDescriptor();assert(!m.RunResident() && !m.packets && !m.nocReads);negative++;
 }
 for(unsigned field=43;field<64;field++) {
  ResidentActivationMachine m(argv[1]);m.descriptor[field]=1;m.SetDescriptor();assert(!m.RunResident() && !m.packets && !m.nocReads);negative++;
 }
 for(unsigned invalid=0;invalid<12;invalid++) {
  ResidentActivationMachine m(argv[1]);auto& d=m.descriptor;
  if(invalid==0)d[3]=7;if(invalid==1)d[4]=0x04000001;if(invalid==2)d[4]=0x03fff000;
  if(invalid==3)d[4]=0xc0001000;if(invalid==4)d[5]=68;if(invalid==5)d[6]=0;
  if(invalid==6)d[6]=0x7fffffff;if(invalid==7)d[16]=0;if(invalid>=8)d[32+(invalid-8)*2]=d[33+(invalid-8)*2]=0;
  m.SetDescriptor();assert(!m.RunResident() && !m.packets && !m.nocReads);negative++;
 }
 {ResidentActivationMachine m(argv[1]);m.descriptor[42]=64;m.SetDescriptor();assert(!m.RunResident() && !m.packets && !m.nocReads);negative++;}
 for(unsigned kind=0;kind<4;kind++) {
  ResidentActivationMachine m(argv[1]);
  if(kind==0)m.remoteGate[255]=0x7fc00001;if(kind==1)m.remoteUp[255]=1;
  if(kind>=2)for(unsigned i=0;i<256;i++){m.remoteGate[i]=PioneerAttentionBits(kind==2?32.0f:-32.0f);m.remoteUp[i]=PioneerAttentionBits(kind==2?0x1p20f:0x1p-40f);}
  auto before=m.remoteNative;assert(!m.RunResident() && m.packets==2 && !memcmp(&m.remoteNative,&before,sizeof(before)));negative++;
 }
 printf("PASS OFFLINE ACTUAL resident FFN activation blocks%u faults%u maxsteps%u minstack%08x;ALL7banks/both3GiBedges/ALL68nonroot routes/strict memory/sealed descriptor+sources/4exact bounded NoC packets/2read1024+2write512and64/complete native operands+epoch-tagged scale line/unchanged inputs/no host PCI packet/inert faults;NO_NATIVE_OWNER_ROOT_GATHER_FULL_FFN_OR_SPEED_AUTHORITY\n",positive,negative,maxSteps,minStack);
}
#endif
