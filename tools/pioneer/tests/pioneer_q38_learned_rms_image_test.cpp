/* SPDX-License-Identifier: MIT */
// ACTUAL emitted QRN1 scalar instructions,ONLY transport/GDDR simulated.
// Independent double/sqrtf references OBSERVE output,never populate it.
#ifndef PIONEER_Q38_GDN_MUTABLE_STATE
#define PIONEER_Q38_GDN_MUTABLE_STATE
#endif
#ifndef PIONEER_Q38_RMS_REUSE_SCALAR
#define PIONEER_GRAPH_MATH_MACHINE_ONLY
#include "pioneer_graph_math_image_test.cpp"
#endif
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38LearnedRmsPlan.h"
#include <array>
#include <memory>
struct LearnedRmsMachine:ScalarMachine {
 std::array<uint32_t,64> descriptor={};
 std::array<uint32_t,5120> input,weight,resident,backup;
 uint32_t command[10]={},counts[8]={},before[8]={},source[16]={};
 unsigned packets=0,readPackets=0,writePackets=0,commandReads=0,commandWrites=0,nocReads=0;
 unsigned failPacket=0,badCounter=0,foreignCounter=0,sourceFault=0,descriptorFault=0,badCommandRead=0,busyOffset=0;
 unsigned aliasReads=0,badAliasRead=0;
 bool pending=false;
 LearnedRmsMachine(const char* image,const PioneerQ38CanonicalModelPlan& canonical,unsigned layer,unsigned role,unsigned bank,unsigned trial=0)
  :ScalarMachine(image,5120,0){
  memory.clear();memory[0x17ffc0]=memory[0x17ffc4]=memory[0x17ffc8]=0;
  for(unsigned i=0;i<16;i++)source[i]=i?0x13570000+i*0x12345:0x16005;
  PioneerQ38GdnIdentity id={1,2,3,4};PioneerQ38LearnedRmsPlan plan;
  assert(plan.Build(canonical,layer,role,8191,bank,id,source,17));
  for(unsigned i=0;i<64;i++)descriptor[i]=plan.Descriptor()[i];SetDescriptor();
  for(unsigned i=0;i<8;i++)before[i]=counts[i]=0xfffffff8;
  for(auto& value:command)value=0xdeadbeef;
  for(unsigned i=0;i<5120;i++){
   float x=trial%5==1?65504.f:trial%5==2?PioneerAttentionFloat(0x26800000):trial%5==3?-0.f:float(int((i*47+trial*97)%511)-255)*.03125f;
   if(trial==5){uint32_t h=(i+1)*2654435761U;x=PioneerAttentionFloat((h&0x807fffff)|((77+(i*17)%65)<<23));}
   float w=trial%5==4?65504.f:float(int((i*71+trial*13)%127)-63)*.0234375f;
   input[i]=PioneerAttentionBits(x);weight[i]=PioneerAttentionBits(w);resident[i]=input[i];backup[i]=0xdeadbeef;
   memory[0x80000+i*4]=memory[0x88000+i*4]=memory[0x90000+i*4]=0xa5a5a5a5;
  }
  for(unsigned i=0;i<32;i++)memory[TT_Q38_FFN_RESIDENT_SUMMARY+i*4]=0xdeadbeef;
  for(unsigned a:{0x741fcU,0x74380U,0x7fffcU,0x85000U,0x87ffcU,0x8d000U,0x8fffcU,0x95000U,0x17ffbcU,0x17ffccU})memory[a]=0xdecafbad;
 }
 void SetDescriptor(){for(unsigned i=0;i<64;i++)memory[TT_Q38_FFN_RESIDENT_DESCRIPTOR+i*4]=descriptor[i];}
 static unsigned SourceOffset(unsigned i){const unsigned o[]={0x100,0x118,0x11c,0x120,0x124,0x128,0x12c,0x130,0x134,0x138,0x13c,0x140,0x144,0x150,0x154,0x16c};return o[i];}
 uint32_t Load(uint32_t a)override{
  assert(!(a&3));
  if(Range(a,0xffb20000,0x2000)){
   nocReads++;unsigned offset=a-0xffb20000;
   if(offset<40){uint32_t v=command[offset/4];if(++commandReads==badCommandRead)v^=1;return v;}
   if(busyOffset && offset==busyOffset)return 1;
   for(unsigned i=0;i<4;i++)if(offset==0x40+i*0x800)return i?0:unsigned(pending);
   if(offset>=0x240 && offset<0x2c0)return 0;
   for(unsigned i=0;i<16;i++)if(offset==SourceOffset(i))return source[i];
   if(offset==0x210 && ++aliasReads==badAliasRead)return counts[2]^1;
   const unsigned o[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};
   for(unsigned i=0;i<8;i++)if(offset==o[i])return counts[i];
   assert(!"unqualified RMS NoC read");
  }
  if(Range(a,0x64000,unsigned(code.size())))return l1[Index(a)];
  assert(Range(a,0x6e000,8192) || Range(a,TT_Q38_FFN_RESIDENT_DESCRIPTOR,256) || Range(a,TT_Q38_FFN_RESIDENT_SUMMARY,128)
   || Range(a,0x80000,20480) || Range(a,0x88000,20480) || Range(a,0x90000,20480) || Range(a,0x17ffc0,12));
  assert(memory.count(a));return memory.at(a);
 }
 void Store(uint32_t a,uint32_t value)override{
  assert(!(a&3));
  if(Range(a,0xffb20000,40)){assert(!pending);commandWrites++;command[(a-0xffb20000)/4]=value;return;}
  if(a==0xffb20040){assert(value==1 && !pending);Submit();return;}
  assert(Range(a,0x6e000,8192) || Range(a,TT_Q38_FFN_RESIDENT_SUMMARY,128) || Range(a,0x90000,20480) || Range(a,0x17ffc0,12));
  if(Range(a,0x6e000,8192))minStack=std::min(minStack,unsigned(a));
  if(Range(a,0x90000,20480))outputWrites++;
  memory[a]=value;
 }
 virtual void CopyPacket(bool write,bool learned,bool normalized,unsigned offset,unsigned local,unsigned bytes){
  for(unsigned i=0;i<bytes/4;i++){
   if(!write)memory[local+i*4]=(learned?weight:input)[offset/4+i];
   else (normalized?resident:backup)[offset/4+i]=memory.at(local+i*4);
  }
 }
 void Submit(){
  assert(packets<8);unsigned p=packets++;bool write=p>=4,learned=p>=2 && p<4,normalized=p>=6;
  unsigned offset=(p&1)*16384,bytes=p&1?4096:16384;
  unsigned local=(write?(normalized?0x90000:0x80000):learned?0x88000:0x80000)+offset;
  unsigned remote=(learned?descriptor[11]:descriptor[4])+offset+(write && !normalized?TT_Q38_RMS_BACKUP:0);
  unsigned route=learned?descriptor[12]:descriptor[8];
  const uint32_t expected[]={write?local:remote,0,write?129U:route,write?remote:local,0,write?route:129U,0,write?0x212U:0,bytes,0};
  for(unsigned i=0;i<10;i++)assert(command[i]==expected[i]);
  if(write){assert(readPackets==4 && outputWrites==5120);writePackets++;}else readPackets++;
  if(packets==failPacket){pending=true;return;}
  CopyPacket(write,learned,normalized,offset,local,bytes);
  unsigned at=write?4:0;counts[at]++;counts[at+1]+=bytes/64;counts[at+3]++;counts[2]++;counts[6]++;
  if(packets==badCounter)counts[at+3]++;
  if(packets==foreignCounter)counts[write?0:4]++;
  if(packets==sourceFault)source[11]^=1;
  if(packets==descriptorFault)memory[TT_Q38_FFN_RESIDENT_DESCRIPTOR+40*4]^=1;
 }
 bool Run(){
  while(pc!=0x64038 && pc!=0x64058)StepScalar();
  assert(l1[Index(pc)]==0x0000006f && minStack>=0x6e000);
  for(unsigned a:{0x741fcU,0x74380U,0x7fffcU,0x85000U,0x87ffcU,0x8d000U,0x8fffcU,0x95000U,0x17ffbcU,0x17ffccU})assert(memory.at(a)==0xdecafbad);
  unsigned p=packets,r=nocReads,w=commandWrites,ow=outputWrites;
  for(unsigned i=0;i<32;i++)StepScalar();assert(p==packets && r==nocReads && w==commandWrites && ow==outputWrites);
  if(pc==0x64058){assert(memory.at(0x17ffc8)==TT_Q38_RMS_FAULT && !memory.at(0x17ffc4));return false;}
  assert(memory.at(0x17ffc8)==TT_Q38_RMS_MARKER && memory.at(0x17ffc0)==0x6400c);return memory.at(0x17ffc4)==5120;
 }
 void Verify(){
  assert(packets==8 && readPackets==4 && writePackets==4 && outputWrites==5120 && !pending);
  double sum=0;for(auto bits:input){float x=PioneerAttentionFloat(bits);sum+=double(x*x);}
  float scale=1.f/sqrtf(float(sum/5120)+1e-6f);
  for(unsigned i=0;i<5120;i++){
   float expected=(PioneerAttentionFloat(input[i])*scale)*PioneerAttentionFloat(weight[i]);
   assert(resident[i]==PioneerAttentionBits(expected) && backup[i]==input[i] && memory.at(0x80000+i*4)==input[i]
    && memory.at(0x88000+i*4)==weight[i] && memory.at(0x90000+i*4)==resident[i]);
  }
  for(unsigned i=0;i<64;i++)assert(memory.at(TT_Q38_FFN_RESIDENT_DESCRIPTOR+i*4)==descriptor[i]);
  uint32_t s[32];for(unsigned i=0;i<32;i++)s[i]=memory.at(TT_Q38_FFN_RESIDENT_SUMMARY+i*4);
  assert(PioneerQ38LearnedRmsSummary(s,descriptor.data(),before,counts));
  for(unsigned i=0;i<32;i++){s[i]^=1;assert(!PioneerQ38LearnedRmsSummary(s,descriptor.data(),before,counts));s[i]^=1;}
 }
 void NoSink()const{assert(!writePackets && resident==input);for(auto value:backup)assert(value==0xdeadbeef);}
};
static void RmsPlanTests(){
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();auto other=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build() && other->Build());
 uint32_t source[16]={};for(unsigned i=0;i<16;i++)source[i]=0x16005+i;
 PioneerQ38GdnIdentity id={1,2,3,4};std::vector<uint8_t> pages(PioneerResidentBanks::Pages);unsigned plans=0,refusals=0;
 for(unsigned layer=0;layer<=64;layer++)for(unsigned r=0;r<(layer==64?1U:2U);r++)for(unsigned bank=0;bank<7;bank++){
  unsigned role=layer==64?2:r;PioneerQ38LearnedRmsPlan plan;assert(plan.Build(*canonical,layer,role,8191,bank,id,source,17)
   && plan.Matches(*canonical,layer,role,8191,bank,id,source,17) && !plan.Matches(*other,layer,role,8191,bank,id,source,17));refusals++;
  const auto* d=plan.Descriptor();unsigned first=d[10]*unsigned(canonical->Capacity/4096)+(d[11]-canonical->Base)/4096;
  assert(!plan.LearnedPagesPresent(pages.data(),unsigned(pages.size())));refusals++;
  for(unsigned p=0;p<5;p++)pages[first+p]=1;
  assert(plan.LearnedPagesPresent(pages.data(),unsigned(pages.size())));
  for(unsigned p=0;p<5;p++){pages[first+p]=2;assert(!plan.LearnedPagesPresent(pages.data(),unsigned(pages.size())));pages[first+p]=1;refusals++;}
  for(unsigned i=0;i<64;i++){auto* word=const_cast<uint32_t*>(d)+i;*word^=1;
   assert(!plan.Matches(*canonical,layer,role,8191,bank,id,source,17));*word^=1;refusals++;}
  for(unsigned p=0;p<5;p++)pages[first+p]=0;
  assert(!plan.Build(*canonical,layer,role,8191,bank,id,source,17));refusals++;plans++;
 }
 printf("PASS canonical learned RMS DATA plans%u/refusals%u ALL129norms x7workspaces,exact5learned pages/pointer+identity+descriptor,NO native producer authority\n",plans,refusals);
}
#ifndef PIONEER_Q38_RMS_MACHINE_ONLY
int main(int argc,char** argv){
 assert(argc==2 || argc==3);setvbuf(stdout,nullptr,_IOLBF,0);RmsPlanTests();unsigned positives=0,faults=0,maxSteps=0,maxStack=0;
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 if(argc==2){
  for(unsigned bank=0;bank<7;bank++){
   LearnedRmsMachine m(argv[1],*canonical,bank*9,bank%2,bank,bank);assert(m.Run());m.Verify();positives++;
   maxSteps=std::max(maxSteps,m.steps);maxStack=std::max(maxStack,0x70000-m.minStack);
  }
  for(unsigned layer:{63U,64U}){LearnedRmsMachine m(argv[1],*canonical,layer,layer==64?2:1,6);assert(m.Run());m.Verify();positives++;
   maxSteps=std::max(maxSteps,m.steps);maxStack=std::max(maxStack,0x70000-m.minStack);}
 }else{
  for(unsigned kind=0;kind<5;kind++)for(unsigned packet=1;packet<=8;packet++){
   LearnedRmsMachine m(argv[1],*canonical,0,0,0);if(kind==0)m.failPacket=packet;if(kind==1)m.badCounter=packet;
   if(kind==2)m.foreignCounter=packet;if(kind==3)m.sourceFault=packet;if(kind==4)m.descriptorFault=packet;
   assert(!m.Run() && m.packets==packet);if(packet<=4)m.NoSink();faults++;
  }
  for(bool learned:{false,true})for(unsigned i:{0U,5119U})for(uint32_t bits:{1U,0x267fffffU,0x47800000U,0x7f800000U,0x7fc00000U}){
   LearnedRmsMachine m(argv[1],*canonical,0,0,0);(learned?m.weight:m.input)[i]=bits;m.resident=m.input;
   assert(!m.Run() && m.packets==4);m.NoSink();faults++;
  }
  for(unsigned field=0;field<64;field++){
   LearnedRmsMachine m(argv[1],*canonical,0,0,0);m.descriptor[field]^=1;m.SetDescriptor();
   if(!PioneerQ38LearnedRmsShape(m.descriptor.data())){assert(!m.Run() && !m.packets);m.NoSink();faults++;}
  }
  for(unsigned read:{1U,10U,40U,41U,80U}){LearnedRmsMachine m(argv[1],*canonical,0,0,0);m.badCommandRead=read;assert(!m.Run());faults++;}
  for(unsigned slot=0;slot<36;slot++){
   LearnedRmsMachine m(argv[1],*canonical,0,0,0);m.busyOffset=slot<4?0x40+slot*0x800:0x240+(slot-4)*4;
   assert(!m.Run() && !m.packets);m.NoSink();faults++;
  }
  for(unsigned i=0;i<16;i++){LearnedRmsMachine m(argv[1],*canonical,0,0,0);m.source[i]^=1;assert(!m.Run() && !m.packets);m.NoSink();faults++;}
  for(unsigned read=1;read<=4;read++){LearnedRmsMachine m(argv[1],*canonical,0,0,0);m.badAliasRead=read;assert(!m.Run() && !m.packets);m.NoSink();faults++;}
 }
 printf("PASS ACTUAL learned RMS RV32:positive%u/faults%u/maxSteps%u/maxStack%u,5120independent CPU-double-reduction bits+raw residual backup,all math BEFORE GDDR writes,8exactpackets,NO host vector/old op5 format change/native qualification\n",positives,faults,maxSteps,maxStack);
}
#endif
