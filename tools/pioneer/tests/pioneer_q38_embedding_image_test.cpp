/* SPDX-License-Identifier: MIT */
// Actual emitted RV32IMF embedding instructions. ONLY NoC/GDDR transport
// is simulated;independent TEST references NEVER seed the card output.
#ifndef PIONEER_Q38_GDN_MUTABLE_STATE
#define PIONEER_Q38_GDN_MUTABLE_STATE
#endif
#ifndef PIONEER_Q38_EMBEDDING_REUSE_SCALAR
#define PIONEER_GRAPH_MATH_MACHINE_ONLY
#include "pioneer_graph_math_image_test.cpp"
#endif
#include "../TenstorrentQ4K.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38EmbeddingPlan.h"
#include <array>
#include <memory>
struct EmbeddingMachine:ScalarMachine {
 std::array<uint32_t,64> descriptor={};
 std::array<uint8_t,46080> packed={};
 std::array<uint32_t,5120> resident;
 uint32_t command[10]={},counts[8]={},seals[16]={},before[8]={};
 unsigned packets=0,readPackets=0,writePackets=0,nocReads=0,commandReads=0,commandWrites=0,scratchWrites=0;
 unsigned failPacket=0,badCounter=0,foreignCounter=0,sourceFault=0,descriptorFault=0,badCommandRead=0,busyOffset=0;
 unsigned aliasReads=0,badAliasRead=0;
 bool pending=false;
 static unsigned SourceOffset(unsigned i){
  const unsigned o[]={0x100,0x118,0x11c,0x120,0x124,0x128,0x12c,0x130,0x134,0x138,0x13c,0x140,0x144,0x150,0x154,0x16c};return o[i];
 }
 EmbeddingMachine(const char* path,const PioneerQ38CanonicalModelPlan& canonical,unsigned token,unsigned position,unsigned bank,unsigned trial=0)
  :ScalarMachine(path,5120,0){
  memory.clear();memory[0x17ffc0]=memory[0x17ffc4]=memory[0x17ffc8]=0;
  for(unsigned i=0;i<16;i++)seals[i]=i?0x13570000+i*0x12345:0x16005;
  PioneerQ38GdnIdentity id={1,2,3,4};PioneerQ38EmbeddingPlan plan;
  assert(plan.Build(canonical,token,position,bank,id,seals,17));
  for(unsigned i=0;i<64;i++)descriptor[i]=plan.Descriptor()[i];SetDescriptor();
  for(unsigned i=0;i<8;i++)before[i]=counts[i]=0xfffffff8;
  for(auto& word:command)word=0xdeadbeef;
  resident.fill(0xdeadbeef);
  for(unsigned i=0;i<5120;i++)memory[0x80000+i*4]=0xa5a5a5a5;
  for(unsigned i=0;i<576;i++)memory[0x90000+i*4]=0xa5a5a5a5;
  for(unsigned i=0;i<32;i++)memory[TT_Q38_FFN_RESIDENT_SUMMARY+i*4]=0xdeadbeef;
  for(unsigned at:{0x741fcU,0x74380U,0x7fffcU,0x85000U,0x8fffcU,0x90900U,0x17ffbcU,0x17ffccU})memory[at]=0xdecafbad;
  // All16 rows DIFFER,all scale/min high bits and both quant planes vary.
  // Include both signed zeros,subnormal/normal/max finite FP16 and signs.
  const uint16_t halves[]={0,0x8000,1,0x8001,0x3c00,0xbc00,0x7bff,0xfbff,0x0400,0x8400,0x3555,0xb555};
  for(unsigned block=0;block<20;block++)for(unsigned lane=0;lane<16;lane++){
   for(unsigned byte=0;byte<144;byte++)packed[block*2304+byte*16+lane]=uint8_t(byte*37+lane*71+block*19+trial*107);
   uint16_t d=halves[(block+lane+trial)%12],m=halves[(block*7+lane*3+trial)%12];
   Put(block,lane,0,uint8_t(d));Put(block,lane,1,uint8_t(d>>8));Put(block,lane,2,uint8_t(m));Put(block,lane,3,uint8_t(m>>8));
  }
 }
 void Put(unsigned block,unsigned lane,unsigned byte,uint8_t value){packed[block*2304+byte*16+lane]=value;}
 void SetDescriptor(){for(unsigned i=0;i<64;i++)memory[TT_Q38_FFN_RESIDENT_DESCRIPTOR+i*4]=descriptor[i];}
 uint32_t Load(uint32_t at)override{
  assert(!(at&3));
  if(Range(at,0xffb20000,0x2000)){
   nocReads++;unsigned offset=at-0xffb20000;
   if(offset<40){uint32_t value=command[offset/4];if(++commandReads==badCommandRead)value^=1;return value;}
   if(busyOffset && offset==busyOffset)return 1;
   for(unsigned i=0;i<4;i++)if(offset==0x40+i*0x800)return i?0:unsigned(pending);
   if(offset>=0x240 && offset<0x2c0)return 0;
   for(unsigned i=0;i<16;i++)if(offset==SourceOffset(i))return seals[i];
   if(offset==0x210 && ++aliasReads==badAliasRead)return counts[2]^1;
   const unsigned offsets[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};
   for(unsigned i=0;i<8;i++)if(offset==offsets[i])return counts[i];
   assert(false && "unqualified embedding NoC read");
  }
  if(Range(at,0x64000,unsigned(code.size())))return l1[Index(at)];
  assert(Range(at,0x6e000,8192) || Range(at,TT_Q38_FFN_RESIDENT_DESCRIPTOR,256)
   || Range(at,TT_Q38_FFN_RESIDENT_SUMMARY,128) || Range(at,0x80000,20480) || Range(at,0x90000,2304) || Range(at,0x17ffc0,12));
  assert(memory.count(at));return memory.at(at);
 }
 void Store(uint32_t at,uint32_t value)override{
  assert(!(at&3));
  if(Range(at,0xffb20000,40)){assert(!pending);commandWrites++;command[(at-0xffb20000)/4]=value;return;}
  if(at==0xffb20040){assert(value==1 && !pending);Submit();return;}
  assert(Range(at,0x6e000,8192) || Range(at,TT_Q38_FFN_RESIDENT_SUMMARY,128) || Range(at,0x80000,20480) || Range(at,0x17ffc0,12));
  if(Range(at,0x6e000,8192))minStack=std::min(minStack,unsigned(at));
  if(Range(at,0x80000,20480))scratchWrites++;
  memory[at]=value;
 }
 void Submit(){
  assert(packets<22);unsigned p=packets++;bool write=p>=20;
  unsigned offset=write?(p-20)*16384:p*2304,local=write?0x80000+offset:0x90000;
  unsigned remote=write?descriptor[4]+offset:descriptor[11]+offset,route=write?descriptor[8]:descriptor[12];
  unsigned bytes=write?(p==20?16384:4096):2304;
  const uint32_t expected[]={write?local:remote,0,write?129U:route,write?remote:local,0,write?route:129U,0,write?0x212U:0,bytes,0};
  for(unsigned i=0;i<10;i++)assert(command[i]==expected[i]);
  if(write){assert(readPackets==20 && scratchWrites==5120);writePackets++;}else readPackets++;
  if(packets==failPacket){pending=true;return;}
  for(unsigned i=0;i<bytes/4;i++){
   if(!write){uint32_t word=0;for(unsigned b=0;b<4;b++)word|=uint32_t(packed[offset+i*4+b])<<(b*8);memory[local+i*4]=word;}
   else resident[offset/4+i]=memory.at(local+i*4);
  }
  unsigned at=write?4:0;counts[at]++;counts[at+1]+=bytes/64;counts[at+3]++;counts[2]++;counts[6]++;
  if(packets==badCounter)counts[at+3]++;
  if(packets==foreignCounter)counts[write?0:4]++;
  if(packets==sourceFault)seals[11]^=1;
  if(packets==descriptorFault)memory[TT_Q38_FFN_RESIDENT_DESCRIPTOR+40*4]^=1;
 }
 bool Run(){
  while(pc!=0x64038 && pc!=0x64058)StepScalar();
  assert(l1[Index(pc)]==0x0000006f && minStack>=0x6e000);
  for(unsigned at:{0x741fcU,0x74380U,0x7fffcU,0x85000U,0x8fffcU,0x90900U,0x17ffbcU,0x17ffccU})assert(memory.at(at)==0xdecafbad);
  unsigned p=packets,r=nocReads,w=commandWrites,s=scratchWrites;
  for(unsigned i=0;i<32;i++)StepScalar();assert(p==packets && r==nocReads && w==commandWrites && s==scratchWrites);
  if(pc==0x64058){assert(memory.at(0x17ffc8)==TT_Q38_EMBEDDING_FAULT && !memory.at(0x17ffc4));return false;}
  assert(memory.at(0x17ffc8)==TT_Q38_EMBEDDING_MARKER && memory.at(0x17ffc0)==0x6400c);
  return memory.at(0x17ffc4)==5120;
 }
 void Verify(){
  assert(packets==22 && !pending && readPackets==20 && writePackets==2 && scratchWrites==5120);
  uint8_t original[144];float reference[256];
  for(unsigned block=0;block<20;block++){
   for(unsigned byte=0;byte<144;byte++)original[byte]=packed[block*2304+byte*16+descriptor[13]];
   assert(PioneerDecodeQ4K(original,reference));
   for(unsigned i=0;i<256;i++)assert(resident[block*256+i]==PioneerAttentionBits(reference[i]) && resident[block*256+i]==memory.at(0x80000+(block*256+i)*4));
  }
  for(unsigned i=0;i<64;i++)assert(memory.at(TT_Q38_FFN_RESIDENT_DESCRIPTOR+i*4)==descriptor[i]);
  uint32_t s[32];for(unsigned i=0;i<32;i++)s[i]=memory.at(TT_Q38_FFN_RESIDENT_SUMMARY+i*4);
  assert(PioneerQ38EmbeddingSummary(s,descriptor.data(),before,counts));
  for(unsigned i=0;i<32;i++){s[i]^=1;assert(!PioneerQ38EmbeddingSummary(s,descriptor.data(),before,counts));s[i]^=1;}
 }
 void NoSink()const{assert(!writePackets);for(auto value:resident)assert(value==0xdeadbeef);}
};

static unsigned PlanTests(const PioneerQ38CanonicalModelPlan& canonical){
 auto other=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(other->Build());
 std::vector<uint8_t> ledger(PioneerResidentBanks::Pages,0);PioneerQ38GdnIdentity id={1,2,3,4};
 uint32_t source[16];for(unsigned i=0;i<16;i++)source[i]=i+1;unsigned refusals=0;
 for(unsigned bank=0;bank<7;bank++)for(unsigned token:{PioneerQ38EmbeddingFirst(bank),PioneerQ38EmbeddingFirst(bank)+15,
   PioneerQ38EmbeddingFirst(bank)+16,PioneerQ38EmbeddingFirst(bank)+PioneerQ38EmbeddingRows(bank)-1}){
  for(unsigned destination=0;destination<7;destination++){
   PioneerQ38EmbeddingPlan plan;assert(!plan.Descriptor() && !plan.LearnedPagesPresent(ledger.data(),unsigned(ledger.size())));
   assert(plan.Build(canonical,token,8191,destination,id,source,17));auto* d=plan.Descriptor();
   assert(plan.Matches(canonical,token,8191,destination,id,source,17));
   assert(!plan.Matches(*other,token,8191,destination,id,source,17));refusals++;
   assert(!plan.Build(canonical,token,8191,destination,id,source,17));refusals++;
   assert(!plan.Matches(canonical,token,8190,destination,id,source,17));refusals++;
   assert(!plan.Matches(canonical,token^1,8191,destination,id,source,17));refusals++;
   for(unsigned i=0;i<16;i++){source[i]^=0x10;assert(!plan.Matches(canonical,token,8191,destination,id,source,17));source[i]^=0x10;refusals++;}
   unsigned offset=d[11]-canonical.Base,first=d[10]*unsigned(canonical.Capacity/4096)+offset/4096,last=first+((offset%4096)+46080-1)/4096;
   for(unsigned i=first;i<=last;i++)ledger[i]=1;
   assert(plan.LearnedPagesPresent(ledger.data(),unsigned(ledger.size())));
   auto* mutableData=const_cast<uint32_t*>(d);
   for(unsigned word=0;word<64;word++){
    mutableData[word]^=1;
    assert(!plan.Matches(canonical,token,8191,destination,id,source,17));
    if(!PioneerQ38EmbeddingShape(mutableData))assert(!plan.LearnedPagesPresent(ledger.data(),unsigned(ledger.size())));
    mutableData[word]^=1;refusals++;
   }
   for(unsigned i=first;i<=last;i++)for(unsigned bad:{0U,2U,255U}){
    ledger[i]=uint8_t(bad);assert(!plan.LearnedPagesPresent(ledger.data(),unsigned(ledger.size())));ledger[i]=1;refusals++;
   }
   assert(!plan.LearnedPagesPresent(nullptr,unsigned(ledger.size())) && !plan.LearnedPagesPresent(ledger.data(),unsigned(ledger.size())-1));refusals+=2;
   for(unsigned i=first;i<=last;i++)ledger[i]=0;
  }
 }
 for(unsigned invalid=0;invalid<9;invalid++){
  PioneerQ38EmbeddingPlan plan;auto identity=id;unsigned token=0,position=0,bank=0,phase=17;const uint32_t* sources=source;
  if(invalid==0)token=248320;if(invalid==1)position=8192;if(invalid==2)bank=7;if(invalid==3)phase=0;
  if(invalid==4)phase=0x7fffffff;if(invalid==5)identity.session=0;if(invalid==6)sources=nullptr;
  if(invalid==7)source[0]=0;
  auto unbuilt=std::make_unique<PioneerQ38CanonicalModelPlan>();
  assert(!plan.Build(invalid==8?*unbuilt:canonical,token,position,bank,identity,sources,phase) && !plan.Descriptor());
  assert(!plan.Build(canonical,0,0,0,id,source,17));source[0]=1;refusals+=2;
 }
 for(auto value:ledger)assert(!value);
 return refusals;
}
#ifndef PIONEER_Q38_EMBEDDING_MACHINE_ONLY
int main(int argc,char** argv){
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 unsigned planRefusals=PlanTests(*canonical),positive=0,faults=0,maxSteps=0,minStack=0x70000;
 for(unsigned bank=0;bank<7;bank++){
  for(unsigned lane=0;lane<16;lane++){
   unsigned token=PioneerQ38EmbeddingFirst(bank)+(lane&1?PioneerQ38EmbeddingRows(bank)-16:0)+lane;
   EmbeddingMachine m(argv[1],*canonical,token,lane?8191:0,(bank+lane)%7,lane);auto packed=m.packed;
   assert(m.Run());m.Verify();assert(m.packed==packed);positive++;maxSteps=std::max(maxSteps,m.steps);minStack=std::min(minStack,m.minStack);
  }
  printf("PASS actual QPE1 embedding sourcebank%u all16 distinct rowlanes/first+last groups/cross-bank output,all5120 values bit-exact independent reference\n",bank);
 }
 // Every packet/command word;first/middle/final source failures cannot
 // proceed to a sink. Late output UNKNOWN cannot publish success.
 for(unsigned packet=1;packet<=22;packet++)for(unsigned fault=0;fault<5;fault++){
  EmbeddingMachine m(argv[1],*canonical,35519,0,6);
  if(!fault)m.failPacket=packet;if(fault==1)m.badCounter=packet;if(fault==2)m.foreignCounter=packet;
  if(fault==3)m.sourceFault=packet;if(fault==4)m.descriptorFault=packet;
  assert(!m.Run() && m.packets==packet);if(packet<=20)m.NoSink();faults++;
 }
 for(unsigned packet=1;packet<=22;packet++)for(unsigned word=0;word<10;word++){
  EmbeddingMachine m(argv[1],*canonical,35520,0,0);m.badCommandRead=(packet-1)*10+word+1;
  assert(!m.Run() && m.packets==packet-1);if(packet<=21)m.NoSink();faults++;
 }
 // All20 blocks,both d/dmin,nonfinite half variants;even a FINAL invalid
 // scale must leave the complete GDDR resident sink unchanged.
 for(unsigned block=0;block<20;block++)for(unsigned field:{0U,2U})for(unsigned half:{0x7c00U,0xfc00U,0x7e01U}){
  EmbeddingMachine m(argv[1],*canonical,15,0,0);m.Put(block,15,field,uint8_t(half));m.Put(block,15,field+1,uint8_t(half>>8));
  assert(!m.Run() && m.packets==block+1);m.NoSink();faults++;
 }
 for(unsigned word=0;word<16;word++){
  EmbeddingMachine m(argv[1],*canonical,0,0,0);m.seals[word]^=1;assert(!m.Run() && !m.packets);m.NoSink();faults++;
 }
 for(unsigned offset:{0x40U,0x840U,0x1040U,0x1840U,0x240U,0x2bcU}){
  EmbeddingMachine m(argv[1],*canonical,0,0,0);m.busyOffset=offset;assert(!m.Run() && !m.packets);m.NoSink();faults++;
 }
 for(unsigned alias:{2U,4U}){
  EmbeddingMachine m(argv[1],*canonical,0,0,0);m.badAliasRead=alias;assert(!m.Run() && !m.packets);m.NoSink();faults++;
 }
 for(unsigned word=0;word<64;word++){
  if(word==6 || (word>=16 && word<40) || word==40)continue;
  EmbeddingMachine m(argv[1],*canonical,0,0,0);m.descriptor[word]^=1;m.SetDescriptor();
  assert(!m.Run() && !m.packets && !m.nocReads);m.NoSink();faults++;
 }
 for(unsigned invalid=0;invalid<14;invalid++){
  EmbeddingMachine m(argv[1],*canonical,0,0,0);auto& d=m.descriptor;
  if(invalid<4)d[32+invalid*2]=d[33+invalid*2]=0;
  if(invalid==4)d[16]=0;if(invalid==5)d[6]=0;if(invalid==6)d[6]=0x7fffffff;
  if(invalid==7)d[5]=248320;if(invalid==8)d[40]=8192;if(invalid==9)d[4]=0x03fff000;
  if(invalid==10)d[4]=0xc0001000;if(invalid==11)d[4]=0x04000000;
  if(invalid==12)d[11]+=46080;if(invalid==13)d[10]=7;
  m.SetDescriptor();assert(!m.Run() && !m.packets && !m.nocReads);m.NoSink();faults++;
 }
 // Valid alternate identities/position/phase remain DATA,never producer
 // authorization. Device tests cannot certify their native lifetime.
 printf("PASS OFFLINE QPE1 actual RV32 positives%u/faults%u/canonical-plan-refusals%u/maxsteps%u/maxstack%u;22 bounded packets/20tiles/5120 F32,unchanged tiled source,atomic pre-sink validation,inert terminal;NO_NATIVE_PRODUCER_AUTHORITY_OR_MODEL_SPEED_EVIDENCE\n",
  positive,faults,planRefusals,maxSteps,0x70000-minStack);
}
#endif
