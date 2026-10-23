/* SPDX-License-Identifier: MIT */
// Actual emitted QGI1 NoC->prep instructions. ONLY physical byte endpoints
// simulated. No CPU-prepared q/k/gates supplied to the worker.
#ifndef PIONEER_Q38_GDN_MUTABLE_STATE
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#endif
#define PIONEER_Q38_GDN_PREPARE_MACHINE_ONLY
#include "pioneer_q38_gdn_prepare_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnInputPlan.h"
#include <memory>
struct GdnInputMachine:PrepareMachine {
 std::array<uint32_t,64> descriptor={};std::array<std::array<uint32_t,16>,4> blocks={};
 std::vector<uint32_t> received[7];uint32_t source[16]={},counts[8]={},before[8]={},command[10]={};
 unsigned packets=0,nocReads=0,commandWrites=0,commandReads=0,rawWrites=0,headerWrites=0;
 unsigned failPacket=0,badCounter=0,foreignCounter=0,sourceFault=0,descriptorFault=0,poisonFault=0,busyOffset=0,badCommandRead=0;
 bool pending=false;
 GdnInputMachine(const char* image,const PioneerQ38CanonicalModelPlan& canonical,unsigned layer,unsigned half,unsigned bank,unsigned trial=0)
  :PrepareMachine(image,half,trial){
  for(unsigned i=0;i<16;i++)source[i]=0x16005+i*0x12345;
  PioneerQ38GdnIdentity id={1,2,3,4};PioneerQ38GdnInputPlan plan;assert(plan.Build(canonical,layer,half,8191,bank,id,source,17));
  for(unsigned i=0;i<64;i++)descriptor[i]=plan.Descriptor()[i];SetDescriptor();
  for(unsigned i=0;i<8;i++)before[i]=counts[i]=0xfffffff8;
  for(auto& v:command)v=0xdeadbeef;
  for(unsigned p=0;p<4;p++)for(unsigned i=0;i<16;i++)blocks[p][i]=PioneerAttentionBits(p==2?-float(i+1)*.25f:float(i+1)*.01f);
  for(unsigned p=0;p<4;p++)blocks[p][descriptor[47]]=input[320+p];
  for(unsigned i=0;i<32;i++)memory[TT_Q38_GDN_INPUT_SUMMARY+i*4]=0xdeadbeef;
  for(unsigned a:{0x747fcU,0x74980U,0x807fcU,0x80900U})memory[a]=0xdecafbad;
 }
 static unsigned SourceOffset(unsigned i){const unsigned o[]={0x100,0x118,0x11c,0x120,0x124,0x128,0x12c,0x130,0x134,0x138,0x13c,0x140,0x144,0x150,0x154,0x16c};return o[i];}
 void SetDescriptor(){for(unsigned i=0;i<64;i++)memory[TT_Q38_GDN_INPUT_DESCRIPTOR+i*4]=descriptor[i];}
 uint32_t Load(uint32_t a)override{
  assert(!(a&3));
  if(Range(a,0xffb20000,0x2000)){
   nocReads++;unsigned at=a-0xffb20000;
   if(at<40){uint32_t v=command[at/4];if(++commandReads==badCommandRead)v^=1;return v;}
   if(busyOffset && at==busyOffset)return 1;
   for(unsigned i=0;i<4;i++)if(at==0x40+i*0x800)return i?0:unsigned(pending);
   if(at>=0x240 && at<0x2c0)return 0;
   for(unsigned i=0;i<16;i++)if(at==SourceOffset(i))return source[i];
   const unsigned o[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};
   for(unsigned i=0;i<8;i++)if(at==o[i])return counts[i];
   assert(!"unqualified QGI1 NoC read");
  }
  if(Range(a,TT_Q38_GDN_INPUT_DESCRIPTOR,256) || Range(a,TT_Q38_GDN_INPUT_SUMMARY,128) || Range(a,0x80800,256)){
   assert(memory.count(a));return memory.at(a);
  }
  return PrepareMachine::Load(a);
 }
 void Store(uint32_t a,uint32_t v)override{
  assert(!(a&3));
  if(Range(a,0xffb20000,40)){assert(!pending);commandWrites++;command[(a-0xffb20000)/4]=v;return;}
  if(a==0xffb20040){assert(v==1 && !pending);Submit();return;}
  if(Range(a,0x80000,1296) || Range(a,0x80800,256)){rawWrites++;memory[a]=v;return;}
  if(Range(a,0x74600,128)){headerWrites++;memory[a]=v;return;}
  if(Range(a,TT_Q38_GDN_INPUT_SUMMARY,128)){memory[a]=v;return;}
  PrepareMachine::Store(a,v); // fixed local outputs/stack/status ONLY
 }
 virtual std::vector<uint32_t> Payload(unsigned p){
  std::vector<uint32_t> bytes(p<2?128:p==2?64:16);
  for(unsigned i=0;i<bytes.size();i++)bytes[i]=p<3?input[(p==0?0:p==1?128:256)+i]:blocks[p-3][i];
  return bytes;
 }
 void Submit(){
  assert(packets<7);unsigned p=packets++;
  const unsigned local[]={0x80000,0x80200,0x80400,0x80800,0x80840,0x80880,0x808c0},size[]={512,512,256,64,64,64,64};
  const unsigned remote[]={descriptor[4]+descriptor[42],descriptor[4]+descriptor[43],descriptor[4]+descriptor[44],
   descriptor[4]+descriptor[45],descriptor[4]+descriptor[46],descriptor[12],descriptor[15]};
  const unsigned route[]={descriptor[8],descriptor[8],descriptor[8],descriptor[8],descriptor[8],descriptor[13],descriptor[41]};
  const unsigned expected[]={remote[p],0,route[p],local[p],0,descriptor[9],0,0,size[p],0};
  for(unsigned i=0;i<10;i++)assert(command[i]==expected[i]); // READ ONLY;NO GDDR writes
  for(unsigned i=0;i<size[p]/4;i++)assert(memory.at(local[p]+i*4)==0x7fc60000+p*1024+i);
  if(packets==failPacket){pending=true;return;}
  received[p]=Payload(p);assert(received[p].size()==size[p]/4);
  for(unsigned i=0;i<size[p]/4;i++)if(packets!=poisonFault || i!=size[p]/4-1)memory[local[p]+i*4]=received[p][i];
  counts[0]++;counts[1]+=size[p]/64;counts[2]++;counts[3]++;counts[6]++;
  if(packets==badCounter)counts[3]++;if(packets==foreignCounter)counts[4]++;
  if(packets==sourceFault)source[11]^=1;if(packets==descriptorFault)memory[TT_Q38_GDN_INPUT_DESCRIPTOR+40*4]^=1;
 }
 bool Run(){
  while(pc!=0x64038 && pc!=0x64058)StepScalar();
  assert(l1[Index(pc)]==0x0000006f && minStack>=0x6e000);
  for(unsigned a:{0x7fffcU,0x80510U,0x8dffcU,0x8e508U,0x745fcU,0x74680U,0x73ffcU,0x74010U,0x747fcU,0x74980U,0x807fcU,0x80900U})assert(memory.at(a)==0xdecafbad);
  unsigned p=packets,r=nocReads,w=commandWrites,raw=rawWrites,local=localWrites,heads=headerWrites;
  for(unsigned i=0;i<32;i++)StepScalar();
  assert(p==packets && r==nocReads && w==commandWrites && raw==rawWrites && local==localWrites && heads==headerWrites);
  if(pc==0x64058){assert(memory.at(0x17ffc8)==TT_Q38_GDN_INPUT_FAULT && !memory.at(0x17ffc4));return false;}
  assert(memory.at(0x17ffc8)==TT_Q38_GDN_INPUT_MARKER && memory.at(0x17ffc0)==0x6400c);
  return memory.at(0x17ffc4)==64;
 }
 std::array<uint32_t,322> Values()const{
  std::array<uint32_t,322> out={};for(unsigned i=0;i<322;i++)out[i]=memory.at(GL::Query+i*4);return out;
 }
 void VerifyTransport(){
  assert(packets==7 && !pending && localWrites==322 && descriptorWrites==5 && headerWrites==32);
  const unsigned local[]={0x80000,0x80200,0x80400,0x80800,0x80840,0x80880,0x808c0};
  for(unsigned p=0;p<7;p++)for(unsigned i=0;i<received[p].size();i++)assert(memory.at(local[p]+i*4)==received[p][i]);
  for(unsigned i=0;i<4;i++)assert(memory.at(0x80500+i*4)==received[3+i][descriptor[47]]);
  for(unsigned i=0;i<64;i++)assert(memory.at(GL::Value+i*4)==received[2][i]);
  uint32_t summary[32];for(unsigned i=0;i<32;i++)summary[i]=memory.at(TT_Q38_GDN_INPUT_SUMMARY+i*4);
  assert(PioneerQ38GdnInputSummary(summary,descriptor.data(),before,counts));
  for(unsigned i=0;i<32;i++){summary[i]^=1;assert(!PioneerQ38GdnInputSummary(summary,descriptor.data(),before,counts));summary[i]^=1;}
 }
};
static void GdnInputPlans(){
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>(),other=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build() && other->Build());
 uint32_t source[16]={};for(unsigned i=0;i<16;i++)source[i]=0x16005+i;PioneerQ38GdnIdentity id={1,2,3,4};
 std::vector<uint8_t> pages(PioneerResidentBanks::Pages);unsigned total=0;
 for(unsigned layer=0;layer<64;layer++)if(layer%4!=3)for(unsigned half=0;half<96;half++){
  unsigned bank=(half+layer)%7;PioneerQ38GdnInputPlan p;
  assert(p.Build(*canonical,layer,half,8191,bank,id,source,17) && p.Matches(*canonical,layer,half,8191,bank,id,source,17)
   && !p.Matches(*other,layer,half,8191,bank,id,source,17) && !p.LearnedPagesPresent(pages.data(),unsigned(pages.size())));
  const auto* d=p.Descriptor();unsigned a=d[11]*unsigned(canonical->Capacity/4096)+(d[12]-canonical->Base)/4096;
  unsigned b=d[14]*unsigned(canonical->Capacity/4096)+(d[15]-canonical->Base)/4096;
  pages[a]=pages[b]=1;assert(p.LearnedPagesPresent(pages.data(),unsigned(pages.size())));
  pages[b]=2;assert(!p.LearnedPagesPresent(pages.data(),unsigned(pages.size())));pages[a]=pages[b]=0;total++;
 }
 for(unsigned i=0;i<64;i++){
  PioneerQ38GdnInputPlan p;assert(p.Build(*canonical,0,0,0,0,id,source,17));const_cast<uint32_t*>(p.Descriptor())[i]^=1;
  assert(!p.Matches(*canonical,0,0,0,0,id,source,17));
 }
 printf("PASS QGI1 canonical DATA plans%u/all48layers/96halves/7banks/A-dtpage-ledger/64fieldmutations;NOT native or producer authority\n",total);
}
#ifndef PIONEER_Q38_GDN_INPUT_MACHINE_ONLY
int main(int argc,char** argv){
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);GdnInputPlans();
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 unsigned positive=0,negative=0,maxSteps=0,maxStack=0;
 for(unsigned half=0;half<96;half++){
  GdnInputMachine m(argv[1],*canonical,(half%48)/3*4+half%3,half,half%7);assert(m.Run());m.VerifyTransport();m.Verify();
  positive++;maxSteps=std::max(maxSteps,m.steps);maxStack=std::max(maxStack,0x70000-m.minStack);
 }
 for(unsigned kind=0;kind<6;kind++)for(unsigned p=1;p<=7;p++){
  GdnInputMachine m(argv[1],*canonical,0,95,6);
  if(kind==0)m.failPacket=p;if(kind==1)m.badCounter=p;if(kind==2)m.foreignCounter=p;
  if(kind==3)m.sourceFault=p;if(kind==4)m.descriptorFault=p;if(kind==5)m.poisonFault=p;
  assert(!m.Run() && m.packets==p && !m.localWrites);negative++;
 }
 for(unsigned i=0;i<64;i++)for(unsigned bit=0;bit<32;bit++){
  GdnInputMachine m(argv[1],*canonical,0,0,0);m.descriptor[i]^=1U<<bit;
  if(PioneerQ38GdnInputShape(m.descriptor.data()))continue;
  m.SetDescriptor();assert(!m.Run() && !m.packets && !m.nocReads && !m.localWrites);negative++;
 }
 for(unsigned slot=0;slot<36;slot++){
  GdnInputMachine m(argv[1],*canonical,0,0,0);m.busyOffset=slot<4?0x40+slot*0x800:0x240+(slot-4)*4;
  assert(!m.Run() && !m.packets && !m.localWrites);negative++;
 }
 for(unsigned i=0;i<16;i++){
  GdnInputMachine m(argv[1],*canonical,0,0,0);m.source[i]^=1;assert(!m.Run() && !m.packets && !m.localWrites);negative++;
 }
 for(unsigned i:{1U,10U,11U,30U,140U}){
  GdnInputMachine m(argv[1],*canonical,0,0,0);m.badCommandRead=i;assert(!m.Run() && !m.localWrites);negative++;
 }
 printf("PASS ACTUAL QGI1 resident7reads/24flits->LOCALQK/gates positive%u/faults%u/maxSteps%u/maxStack%u;ALLrawinputs/readpoison/source-command-counter-guards/inertfault;NO hostnumeric-input/NoCwrite/nativeownership/GDNstate/card/speedclaim\n",positive,negative,maxSteps,maxStack);
}
#endif
