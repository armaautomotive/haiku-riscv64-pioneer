/* SPDX-License-Identifier: MIT */
// Actual gather instructions, synthetic descriptor/line/wire BYTES only.
// Not a claim that producers/layer/native reservation actually completed.
#define PIONEER_GRAPH_MATH_MACHINE_ONLY
#include "pioneer_graph_math_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentAdvanceShape.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionGatherShape.h"
#include <array>
struct AdvanceGatherMachine:ScalarMachine {
 std::array<uint32_t,64> d={};std::array<std::array<uint32_t,32>,20> lines={};
 std::array<uint32_t,2560> native={},received={};std::array<uint32_t,160> compact={};
 uint32_t source[16]={},command[10]={},counts[8]={},before[8]={};
 unsigned entry=0,packets=0,operandReads=0,compactWrites=0,failPacket=0,rootFault=0,tableFault=0,badCounter=0;
 bool pending=false;
 static constexpr unsigned Return=0x17ff80,Fault=0x72060;
 static unsigned Counter(unsigned i){const unsigned o[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};return o[i];}
 static unsigned Source(unsigned i){return !i?0x100:i<=6?0x118+(i-1)*4:i<=12?0x130+(i-7)*4:i==13?0x150:i==14?0x154:0x16c;}
 AdvanceGatherMachine(const char* image,unsigned trial):ScalarMachine(5120,0){
  code=Read((std::string(image)+"/advance_gather.bin").c_str());assert(!code.empty() && code.size()<=8192);
  std::ifstream symbols(std::string(image)+"/program.symbols");std::string line;
  while(std::getline(symbols,line)){unsigned at=0;char type=0,name[256]={};if(sscanf(line.c_str(),"%x %c %255s",&at,&type,name)==3 && !strcmp(name,"q38_resident_advance_gather_once"))entry=at;}
  assert(CodeAddress(entry));
  d[0]=TT_Q38_PROJECTION_GATHER_MARKER;d[1]=1;d[2]=67108864;d[3]=trial?6:0;d[4]=trial?0xc0000000:0x04000000;d[5]=20;d[6]=37;d[8]=PioneerQ38ProjectionRoute(d[3]);d[9]=129;
  d[10]=TT_Q38_PROJECTION_NATIVE_OFFSET;d[11]=TT_Q38_PROJECTION_LINES_OFFSET;d[12]=TT_Q38_PROJECTION_COMPACT_OFFSET;d[13]=10240;d[14]=640;d[15]=8;
  for(unsigned i=0;i<16;i++)d[16+i]=source[i]=i?0x12340000+i:0x16005;
  for(unsigned i=0;i<4;i++)d[32+i*2]=i+1;
  d[40]=trial?8191:0;d[41]=trial?248319:0;d[42]=trial?62:1;d[43]=5;
  for(unsigned i=0;i<64;i++)memory[0x74200+i*4]=d[i];
  for(unsigned i=0;i<8;i++)counts[i]=before[i]=0xfffffff8;
  for(unsigned chunk=0;chunk<20;chunk++){
   auto& p=lines[chunk];for(unsigned i=0;i<8;i++)p[i]=0x3f800000+chunk*256+i;
   const uint32_t fields[]={TT_Q38_PROJECTION_INPUT_MARKER,d[6],chunk,8,5120,d[42],5,d[40],d[41]};
   for(unsigned i=0;i<9;i++)p[8+i]=fields[i];for(unsigned i=0;i<8;i++)p[17+i]=d[32+i];
   p[25]=20;p[26]=chunk+1;p[27]=PioneerQ38ProjectionPeer(chunk);p[28]=source[0]+(chunk+1)*256;p[31]=1;
   for(unsigned i=0;i<16;i++){
    unsigned word=TT_Q38_COHORT_SOURCE_WORD+(chunk+1)*16+i;
    memory[TT_Q38_COHORT_MANIFEST+word*4]=memory[TT_Q38_COHORT_SHADOW+word*4]=source[i]+(chunk+1)*256;
   }
   assert(!PioneerQ38ProjectionGatherLine(p.data(),d.data(),chunk)); // proves legacy ROOT==peer source assumption refuses
  }
  for(unsigned i=0;i<2560;i++)native[i]=0x3c003c00+i;
  compact.fill(0xdeadbeef);received.fill(0xdeadbeef);
 }
 bool CodeAddress(uint32_t at)const override{return Range(at,0x4e000,unsigned(code.size()));}
 unsigned Index(uint32_t at)override{assert(CodeAddress(at) && !(at&3));l1[1]=Word(code,at-0x4e000);return 1;}
 uint32_t Load(uint32_t at)override{
  assert(!(at&3));if(CodeAddress(at))return Word(code,at-0x4e000);
  if(Range(at,0xffb20000,0x2000)){
   unsigned offset=at-0xffb20000;if(offset<40)return command[offset/4];
   for(unsigned i=0;i<4;i++)if(offset==0x40+i*0x800)return i?0:unsigned(pending);
   if(offset>=0x240 && offset<0x2c0)return 0;
   for(unsigned i=0;i<16;i++)if(offset==Source(i))return source[i];
   for(unsigned i=0;i<8;i++)if(offset==Counter(i))return counts[i];assert(!"unqualified gather CSR");
  }
  assert(Range(at,0x6e000,8192) || Range(at,0x74200,384) || Range(at,0x90e00,128) || Range(at,0xab000,640)
   || Range(at,TT_Q38_COHORT_MANIFEST+TT_Q38_COHORT_SOURCE_WORD*4,120*64)
   || Range(at,TT_Q38_COHORT_SHADOW+TT_Q38_COHORT_SOURCE_WORD*4,120*64));return memory.at(at);
 }
 void Store(uint32_t at,uint32_t value)override{
  assert(!(at&3));if(Range(at,0xffb20000,40)){assert(!pending);command[(at-0xffb20000)/4]=value;return;}
  if(at==0xffb20040){assert(value==1 && !pending);Packet();return;}
  assert(Range(at,0x6e000,8192) || Range(at,0x74300,128) || Range(at,0xab000,640));
  if(Range(at,0x6e000,8192))minStack=std::min(minStack,at);memory[at]=value;
 }
 void Packet(){
  unsigned p=packets++;assert(p<22);bool write=p==21;unsigned local=p<20?0x90e00:write?0xab000:0xa0000,bytes=p<20?128:write?640:10240;
  unsigned remote=d[4]+(p<20?196608+p*128:write?212992:131072);
  const uint32_t expect[]={write?local:remote,0,write?129U:d[8],write?remote:local,0,write?d[8]:129U,0,write?0x212U:0,bytes,0};
  for(unsigned i=0;i<10;i++)assert(command[i]==expect[i]);
  if(packets==failPacket){pending=true;return;}
  if(p<20)for(unsigned i=0;i<32;i++)memory[local+i*4]=lines[p][i];
  else if(p==20){operandReads++;received=native;}
  else{compactWrites++;for(unsigned i=0;i<160;i++)compact[i]=memory.at(local+i*4);}
  unsigned at=write?4:0;counts[at]++;counts[at+1]+=bytes/64;counts[at+3]++;counts[2]++;counts[6]++;
  if(packets==rootFault)source[11]^=1;
  if(packets==tableFault)memory[TT_Q38_COHORT_MANIFEST+(TT_Q38_COHORT_SOURCE_WORD+20*16+11)*4]^=1;
  if(packets==badCounter)counts[at+3]++;
 }
 bool Run(){pc=entry;regs[1]=Return;regs[2]=0x70000;while(pc!=Return && pc!=Fault)StepScalar();assert(minStack>=0x6e000);return pc==Return && regs[10]==20;}
 void Verify(){
  assert(packets==22 && operandReads==1 && compactWrites==1 && native==received);
  for(unsigned c=0;c<20;c++)for(unsigned i=0;i<8;i++)assert(compact[c*8+i]==lines[c][i]);
  std::array<uint32_t,32> summary;for(unsigned i=0;i<32;i++)summary[i]=memory.at(0x74300+i*4);
  assert(PioneerQ38ProjectionGatherSummary(summary.data(),d.data(),before,counts));
 }
};
int main(int argc,char** argv){
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);unsigned faults=0;
 for(unsigned trial=0;trial<2;trial++){AdvanceGatherMachine m(argv[1],trial);assert(m.Run());m.Verify();}
 for(unsigned kind=0;kind<4;kind++)for(unsigned at:{1U,20U,21U,22U}){
  AdvanceGatherMachine m(argv[1],0);if(kind==0)m.failPacket=at;if(kind==1)m.rootFault=at;if(kind==2)m.tableFault=at;if(kind==3)m.badCounter=at;
  assert(!m.Run() && m.pc==m.Fault && m.packets==at);faults++;
 }
 for(unsigned chunk:{0U,19U})for(unsigned word:{9U,13U,17U,27U,28U,31U}){
  AdvanceGatherMachine m(argv[1],0);m.lines[chunk][word]^=1;
  assert(!m.Run() && m.pc==m.Fault && m.packets==chunk+1 && !m.operandReads && !m.compactWrites);faults++;
 }
 printf("PASS ACTUAL advance gather instructions:2boundary positives/ALL20 distinct per-peer sources/160scale words and2560native words BITEXACT copies/legacy ROOT-source predicate provably refuses these valid distinct producers/%u packet-source-table-counter-line failures before later IO;synthetic line DATA only,NO producer/native/full-model/rate grant\n",faults);
}
