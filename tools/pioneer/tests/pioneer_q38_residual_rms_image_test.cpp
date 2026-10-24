/* SPDX-License-Identifier: MIT */
// Actual emitted QRR1 scalar instructions. Only NoC/GDDR are byte mocks;
// independent F32 add/double reduction/sqrtf reference OBSERVES outputs.
#define PIONEER_Q38_GDN_MUTABLE_STATE
#ifndef PIONEER_Q38_RESIDUAL_RMS_REUSE_SCALAR
#define PIONEER_GRAPH_MATH_MACHINE_ONLY
#include "pioneer_graph_math_image_test.cpp"
#endif
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidualRmsPlan.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionGraphFootprint.h"
#include <array>
#include <memory>
struct ResidualRmsMachine:ScalarMachine {
 std::array<uint32_t,64> descriptor={};
 std::array<uint32_t,5120> projected,raw,weight,resident,backup,initial;
 uint32_t command[10]={},counts[8]={},before[8]={},source[16]={};
 unsigned packets=0,readPackets=0,writePackets=0,commandReads=0,commandWrites=0,nocReads=0,sumWrites=0,normWrites=0;
 unsigned failPacket=0,badCounter=0,foreignCounter=0,sourceFault=0,descriptorFault=0,badCommandRead=0,busyOffset=0;
 unsigned aliasReads=0,badAliasRead=0,partialPacket=0;
 bool pending=false;
 ResidualRmsMachine(const char* image,const PioneerQ38CanonicalModelPlan& canonical,unsigned token,unsigned bank,unsigned trial=0)
  :ScalarMachine(image,5120,0){
  memory.clear();memory[0x17ffc0]=memory[0x17ffc4]=memory[0x17ffc8]=0;
  for(unsigned i=0;i<16;i++)source[i]=i?0x13570000+i*0x12345:0x16005;
  PioneerQ38GdnIdentity id={1,2,3,4};PioneerQ38ResidualRmsPlan plan;
  assert(plan.Build(canonical,token,bank,id,source,17));
  for(unsigned i=0;i<64;i++)descriptor[i]=plan.Descriptor()[i];SetDescriptor();
  for(unsigned i=0;i<8;i++)before[i]=counts[i]=0xfffffff8;
  for(auto& value:command)value=0xdeadbeef;
  for(unsigned i=0;i<5120;i++){
   float x=float(int((i*47+trial*97)%511)-255)*.03125f,y=float(int((i*31+trial*13)%255)-127)*.015625f;
   if(trial==1){x=65504.f;y=-32752.f;}
   if(trial==2)x=y=PioneerAttentionFloat(0x26800000);
   if(trial==3)x=y=-0.f;
   if(trial==5){uint32_t h=(i+1)*2654435761U;x=PioneerAttentionFloat((h&0x807fffff)|((79+(i*17)%63)<<23));y=x*.25f;}
   if(trial==6){x=1.f;y=-1.f;}
   if(trial==7){x=PioneerAttentionFloat(0x3f800001);y=-1.f;}
   if(trial==8){x=65504.f;y=-65504.f;}
   if(trial==9){x=PioneerAttentionFloat(0x26800000);y=i&1?-x:x;}
   float w=trial==4?65504.f:float(int((i*71+trial*13)%127)-63)*.0234375f;
   raw[i]=PioneerAttentionBits(x);projected[i]=PioneerAttentionBits(y);weight[i]=PioneerAttentionBits(w);
   initial[i]=resident[i]=0xdeadbeef;backup[i]=raw[i];
   for(unsigned a:{0x80000U,0x88000U,0x90000U,0x98000U,0xa0000U})memory[a+i*4]=0xa5a5a5a5;
  }
  for(unsigned i=0;i<32;i++)memory[TT_Q38_FFN_RESIDENT_SUMMARY+i*4]=0xdeadbeef;
  for(unsigned a:{0x741fcU,0x74380U,0x7fffcU,0x85000U,0x87ffcU,0x8d000U,0x8fffcU,0x95000U,0x97ffcU,0x9d000U,0x9fffcU,0xa5000U,0x17ffbcU,0x17ffccU})memory[a]=0xdecafbad;
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
   assert(!"unqualified QRR1 NoC read");
  }
  if(Range(a,0x64000,unsigned(code.size())))return l1[Index(a)];
  assert(Range(a,0x6e000,8192) || Range(a,TT_Q38_FFN_RESIDENT_DESCRIPTOR,256) || Range(a,TT_Q38_FFN_RESIDENT_SUMMARY,128)
   || Range(a,0x80000,20480) || Range(a,0x88000,20480) || Range(a,0x90000,20480)
   || Range(a,0x98000,20480) || Range(a,0xa0000,20480) || Range(a,0x17ffc0,12));
  assert(memory.count(a));return memory.at(a);
 }
 void Store(uint32_t a,uint32_t value)override{
  assert(!(a&3));
  if(Range(a,0xffb20000,40)){assert(!pending);commandWrites++;command[(a-0xffb20000)/4]=value;return;}
  if(a==0xffb20040){assert(value==1 && !pending);Submit();return;}
  assert(PioneerQ38ProjectionGraphSnapshotContains(0,a)); // NO new L1 lease window
  assert(Range(a,0x6e000,8192) || Range(a,TT_Q38_FFN_RESIDENT_SUMMARY,128) || Range(a,0x98000,20480)
   || Range(a,0xa0000,20480) || Range(a,0x17ffc0,12)); // inputs/descriptors are read-only
  if(Range(a,0x6e000,8192))minStack=std::min(minStack,unsigned(a));
  if(Range(a,0x98000,20480))sumWrites++;
  if(Range(a,0xa0000,20480))normWrites++;
  memory[a]=value;
 }
 virtual void CopyPacket(bool write,bool learned,bool rawRead,bool normalized,unsigned offset,unsigned local,unsigned bytes){
  for(unsigned i=0;i<bytes/4;i++){
   if(!write)memory[local+i*4]=(learned?weight:rawRead?raw:projected)[offset/4+i];
   else (normalized?resident:backup)[offset/4+i]=memory.at(local+i*4);
  }
 }
 void Submit(){
  assert(packets<10);unsigned p=packets++;bool write=p>=6,learned=p>=4 && p<6,rawRead=p>=2 && p<4,normalized=p>=8;
  unsigned offset=(p&1)*16384,bytes=p&1?4096:16384;
  unsigned local=(write?(normalized?0xa0000:0x98000):learned?0x90000:rawRead?0x88000:0x80000)+offset;
  unsigned remote=(learned?descriptor[11]:descriptor[4])+(learned?0U:write?(normalized?0U:unsigned(TT_Q38_RMS_BACKUP)):rawRead?unsigned(TT_Q38_RMS_BACKUP):unsigned(TT_Q38_GDN_OUTPUT_RESULT))+offset;
  unsigned route=learned?descriptor[12]:descriptor[8];
  const uint32_t expected[]={write?local:remote,0,write?129U:route,write?remote:local,0,write?route:129U,0,write?0x212U:0,bytes,0};
  for(unsigned i=0;i<10;i++)assert(command[i]==expected[i]);
  if(write){assert(readPackets==6 && sumWrites==5120 && normWrites==5120);writePackets++;}else readPackets++;
  if(packets==failPacket){pending=true;return;}
  CopyPacket(write,learned,rawRead,normalized,offset,local,packets==partialPacket?bytes-64:bytes);
  if(packets==partialPacket){pending=true;return;} // no fabricated completion for partial transfer
  unsigned at=write?4:0;counts[at]++;counts[at+1]+=bytes/64;counts[at+3]++;counts[2]++;counts[6]++;
  if(packets==badCounter)counts[at+3]++;
  if(packets==foreignCounter)counts[write?0:4]++;
  if(packets==sourceFault)source[11]^=1;
  if(packets==descriptorFault)memory[TT_Q38_FFN_RESIDENT_DESCRIPTOR+41*4]^=1;
 }
 bool Run(){
  while(pc!=0x64038 && pc!=0x64058){StepScalar();if(regs[2]){assert(regs[2]>=0x6e000 && regs[2]<=0x70000);minStack=std::min(minStack,unsigned(regs[2]));}}
  assert(l1[Index(pc)]==0x0000006f && minStack>=0x6e000);
  for(unsigned a:{0x741fcU,0x74380U,0x7fffcU,0x85000U,0x87ffcU,0x8d000U,0x8fffcU,0x95000U,0x97ffcU,0x9d000U,0x9fffcU,0xa5000U,0x17ffbcU,0x17ffccU})assert(memory.at(a)==0xdecafbad);
  unsigned p=packets,r=nocReads,w=commandWrites,sw=sumWrites,nw=normWrites;
  for(unsigned i=0;i<32;i++)StepScalar();assert(p==packets && r==nocReads && w==commandWrites && sw==sumWrites && nw==normWrites);
  if(pc==0x64058){assert(memory.at(0x17ffc8)==TT_Q38_RESIDUAL_RMS_FAULT && !memory.at(0x17ffc4));return false;}
  assert(memory.at(0x17ffc8)==TT_Q38_RESIDUAL_RMS_MARKER && memory.at(0x17ffc0)==0x6400c);return memory.at(0x17ffc4)==5120;
 }
 void Verify(){
  assert(packets==10 && readPackets==6 && writePackets==4 && sumWrites==5120 && normWrites==5120 && !pending);
  std::array<uint32_t,5120> sum;double squares=0;
  for(unsigned i=0;i<5120;i++){float x=PioneerAttentionFloat(raw[i])+PioneerAttentionFloat(projected[i]);sum[i]=PioneerAttentionBits(x);squares+=double(x*x);}
  float scale=1.f/sqrtf(float(squares/5120)+1e-6f);
  for(unsigned i=0;i<5120;i++){
   float expected=(PioneerAttentionFloat(sum[i])*scale)*PioneerAttentionFloat(weight[i]);
   assert(resident[i]==PioneerAttentionBits(expected) && backup[i]==sum[i] && memory.at(0x98000+i*4)==sum[i]
    && memory.at(0x80000+i*4)==projected[i] && memory.at(0x88000+i*4)==raw[i] && memory.at(0x90000+i*4)==weight[i]
    && memory.at(0xa0000+i*4)==resident[i]);
  }
  for(unsigned i=0;i<64;i++)assert(memory.at(TT_Q38_FFN_RESIDENT_DESCRIPTOR+i*4)==descriptor[i]);
  uint32_t s[32];for(unsigned i=0;i<32;i++)s[i]=memory.at(TT_Q38_FFN_RESIDENT_SUMMARY+i*4);
  assert(PioneerQ38ResidualRmsSummary(s,descriptor.data(),before,counts) && !PioneerQ38LearnedRmsShape(descriptor.data()));
  for(unsigned i=0;i<32;i++){s[i]^=1;assert(!PioneerQ38ResidualRmsSummary(s,descriptor.data(),before,counts));s[i]^=1;}
 }
 void NoSink()const{assert(!writePackets && resident==initial && backup==raw);}
};
static void ResidualRmsPlanTests(){
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();auto other=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build() && other->Build());
 uint32_t source[16]={};for(unsigned i=0;i<16;i++)source[i]=0x16005+i;
 PioneerQ38GdnIdentity id={1,2,3,4};std::vector<uint8_t> pages(PioneerResidentBanks::Pages);unsigned plans=0,refusals=0;
 for(unsigned bank=0;bank<7;bank++)for(unsigned token:{0U,248319U})for(unsigned phase:{2U,0x7ffffffeU}){
  PioneerQ38ResidualRmsPlan plan;assert(plan.Build(*canonical,token,bank,id,source,phase)
   && plan.Matches(*canonical,token,bank,id,source,phase) && !plan.Matches(*other,token,bank,id,source,phase));refusals++;
  const auto* d=plan.Descriptor();unsigned first=d[10]*unsigned(canonical->Capacity/4096)+(d[11]-canonical->Base)/4096;
  assert(!plan.LearnedPagesPresent(pages.data(),unsigned(pages.size())));refusals++;
  for(unsigned p=0;p<5;p++)pages[first+p]=1;
  assert(plan.LearnedPagesPresent(pages.data(),unsigned(pages.size())));
  for(unsigned p=0;p<5;p++){pages[first+p]=2;assert(!plan.LearnedPagesPresent(pages.data(),unsigned(pages.size())));pages[first+p]=1;refusals++;}
  for(unsigned i=0;i<64;i++){auto* word=const_cast<uint32_t*>(d)+i;*word^=1;
   assert(!plan.Matches(*canonical,token,bank,id,source,phase));*word^=1;refusals++;}
  for(unsigned p=0;p<5;p++)pages[first+p]=0;
  assert(!plan.Build(*canonical,token,bank,id,source,phase));refusals++;plans++;
 }
 for(unsigned kind=0;kind<7;kind++){
  PioneerQ38ResidualRmsPlan plan;auto bad=id;uint32_t copy[16];memcpy(copy,source,sizeof(copy));
  if(kind==4)bad.sequence=0;if(kind==5)copy[0]=0;
  assert(!plan.Build(*canonical,kind==0?248320:0,kind==1?7:0,bad,kind==6?nullptr:copy,kind==2?1:kind==3?0x7fffffff:17));
  assert(!plan.Descriptor() && !plan.Build(*canonical,0,0,id,source,17));refusals++;
 }
 printf("PASS QRR1 canonical DATA plans%u/refusals%u/7banks/token+phase edges/ALL64descriptor words/exact5norm pages;NO producer/launch authority\n",plans,refusals);
}
#ifndef PIONEER_Q38_RESIDUAL_RMS_MACHINE_ONLY
int main(int argc,char** argv){
 assert(argc==2 || (argc==3 && !strcmp(argv[2],"--faults")));setvbuf(stdout,nullptr,_IOLBF,0);ResidualRmsPlanTests();
 unsigned positives=0,faults=0,maxSteps=0,maxStack=0;auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 if(argc==2){
  for(unsigned trial=0;trial<10;trial++){
   ResidualRmsMachine m(argv[1],*canonical,trial&1?248319:0,trial%7,trial);assert(m.Run());m.Verify();positives++;
   maxSteps=std::max(maxSteps,m.steps);maxStack=std::max(maxStack,0x70000-m.minStack);
  }
 }else{
  for(unsigned kind=0;kind<6;kind++)for(unsigned packet=1;packet<=10;packet++){
   ResidualRmsMachine m(argv[1],*canonical,0,0);if(kind==0)m.failPacket=packet;if(kind==1)m.badCounter=packet;
   if(kind==2)m.foreignCounter=packet;if(kind==3)m.sourceFault=packet;if(kind==4)m.descriptorFault=packet;if(kind==5)m.partialPacket=packet;
   assert(!m.Run() && m.packets==packet);if(packet<=6)m.NoSink();faults++;
  }
  for(unsigned input=0;input<3;input++)for(unsigned i:{0U,5119U})for(uint32_t bits:{1U,0x267fffffU,0x47800000U,0x7f800000U,0x7fc00000U}){
   ResidualRmsMachine m(argv[1],*canonical,0,0);(input==0?m.raw:input==1?m.projected:m.weight)[i]=bits;m.backup=m.raw;
   assert(!m.Run() && m.packets==6);m.NoSink();faults++;
  }
  for(unsigned i:{0U,5119U}){ResidualRmsMachine m(argv[1],*canonical,0,0);m.raw[i]=m.projected[i]=0x477fe000;m.backup=m.raw;
   assert(!m.Run() && m.packets==6);m.NoSink();faults++;}
  for(unsigned i:{0U,5119U}){ResidualRmsMachine m(argv[1],*canonical,0,0);m.raw[i]=0x26800001;m.projected[i]=0xa6800000;m.backup=m.raw;
   assert(!m.Run() && m.packets==6);m.NoSink();faults++;} // normal inputs,cancellation result below qualified bound
  for(unsigned field=0;field<64;field++){
   ResidualRmsMachine m(argv[1],*canonical,0,0);m.descriptor[field]^=1;m.SetDescriptor();
   if(!PioneerQ38ResidualRmsShape(m.descriptor.data())){assert(!m.Run() && !m.packets);m.NoSink();faults++;}
  }
  for(unsigned read:{1U,10U,60U,61U,100U}){ResidualRmsMachine m(argv[1],*canonical,0,0);m.badCommandRead=read;assert(!m.Run());faults++;}
  for(unsigned slot=0;slot<36;slot++){
   ResidualRmsMachine m(argv[1],*canonical,0,0);m.busyOffset=slot<4?0x40+slot*0x800:0x240+(slot-4)*4;
   assert(!m.Run() && !m.packets);m.NoSink();faults++;
  }
  for(unsigned i=0;i<16;i++){ResidualRmsMachine m(argv[1],*canonical,0,0);m.source[i]^=1;assert(!m.Run() && !m.packets);m.NoSink();faults++;}
  for(unsigned read=1;read<=4;read++){ResidualRmsMachine m(argv[1],*canonical,0,0);m.badAliasRead=read;assert(!m.Run() && !m.packets);m.NoSink();faults++;}
 }
 printf("PASS ACTUAL QRR1 residual+post-attention learned RMS RV32 positives%u/faults%u/maxSteps%u/maxStack%u/ALL5120 F32-add+CPU-double-reduction BIT-EXACT/10exactpackets/math BEFORE GDDR writes/all stores within OLD snapshot;NO host interstage vector/native qualification/TPS\n",positives,faults,maxSteps,maxStack);
}
#endif
