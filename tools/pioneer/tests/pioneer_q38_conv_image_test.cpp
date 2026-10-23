/* SPDX-License-Identifier: MIT */
// Execute ACTUAL RV32 QCV1 bytes,not the host C++ implementation. Only
// physical NoC/GDDR endpoints are simulated. Numerical oracle OBSERVES.
#define PIONEER_Q38_GDN_MUTABLE_STATE
#define PIONEER_GRAPH_MATH_MACHINE_ONLY
#include "pioneer_graph_math_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ConvPlan.h"
#include <array>
#include <memory>
struct ConvMachine:ScalarMachine {
 std::array<uint32_t,64> descriptor={};
 std::array<uint32_t,16> input,output;std::array<uint32_t,64> weight;std::array<uint32_t,48> previous,next;
 uint32_t command[10]={},counts[8]={},before[8]={},source[16]={};
 unsigned packets=0,reads=0,writes=0,nocReads=0,commandReads=0,commandWrites=0,nextStores=0,outStores=0;
 unsigned failPacket=0,badCounter=0,foreignCounter=0,sourceFault=0,descriptorFault=0,poisonFault=0,busyOffset=0,badCommandRead=0;
 bool pending=false;
 ConvMachine(const char* image,const PioneerQ38CanonicalModelPlan& canonical,unsigned layer,unsigned first,unsigned bank,unsigned worker,unsigned trial=0,unsigned position=8191)
  :ScalarMachine(image,16,0){
  memory.clear();memory[0x17ffc0]=memory[0x17ffc4]=memory[0x17ffc8]=0;
  for(unsigned i=0;i<16;i++)source[i]=0x16005+i*0x12345;
  PioneerQ38GdnIdentity id={1,2,3,4};PioneerQ38ConvPlan plan;
  assert(plan.Build(canonical,layer,first,position,bank,worker,id,source,17));
  for(unsigned i=0;i<64;i++)descriptor[i]=plan.Descriptor()[i];SetDescriptor();
  for(unsigned i=0;i<8;i++)before[i]=counts[i]=0xfffffff8;
  for(auto& v:command)v=0xdeadbeef;
  for(unsigned c=0;c<16;c++){
   input[c]=PioneerAttentionBits(trial==0?0.f:float(sin((first+c)*.017+trial*.231)));
   output[c]=0xdeadbeef;
   for(unsigned t=0;t<4;t++)weight[c*4+t]=PioneerAttentionBits(float(int(((first+c)*7+t*11+trial)%29)-14)*.03125f);
   for(unsigned t=0;t<3;t++){previous[c*3+t]=PioneerAttentionBits(trial==0?0.f:float(cos((first+c)*.013+t*.117+trial*.031)));next[c*3+t]=0xdeadbeef;}
  }
  if(trial==9 || trial==10 || trial==11)for(unsigned c=0;c<16;c++){
   float value=trial==9?65504.f:trial==10?PioneerAttentionFloat(0x26800000):0.f;
   input[c]=PioneerAttentionBits(c&1?-value:value);
   for(unsigned t=0;t<4;t++)weight[c*4+t]=PioneerAttentionBits(value);
   for(unsigned t=0;t<3;t++)previous[c*3+t]=PioneerAttentionBits(c&1?-value:value);
  }
  for(unsigned at:{0x80000U,0x80400U,0x80800U,0x80c00U,0x81000U})for(unsigned i=0;i<64;i++)memory[at+i*4]=0xa5a5a5a5;
  for(unsigned i=0;i<32;i++)memory[TT_Q38_FFN_RESIDENT_SUMMARY+i*4]=0xdeadbeef;
  for(unsigned a:{0x741fcU,0x74380U,0x7fffcU,0x80040U,0x803fcU,0x80500U,0x807fcU,0x808c0U,0x80bfcU,0x80cc0U,0x80ffcU,0x81040U,0x17ffbcU,0x17ffccU})memory[a]=0xdecafbad;
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
   const unsigned o[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};
   for(unsigned i=0;i<8;i++)if(offset==o[i])return counts[i];
   assert(!"unqualified QCV1 NoC read");
  }
  if(Range(a,0x64000,unsigned(code.size())))return l1[Index(a)];
  assert(Range(a,0x6e000,8192) || Range(a,TT_Q38_FFN_RESIDENT_DESCRIPTOR,256) || Range(a,TT_Q38_FFN_RESIDENT_SUMMARY,128)
   || Range(a,0x80000,64) || Range(a,0x80400,256) || Range(a,0x80800,192) || Range(a,0x80c00,192)
   || Range(a,0x81000,64) || Range(a,0x17ffc0,12));
  assert(memory.count(a));return memory.at(a);
 }
 void Store(uint32_t a,uint32_t value)override{
  assert(!(a&3));
  if(Range(a,0xffb20000,40)){assert(!pending);commandWrites++;command[(a-0xffb20000)/4]=value;return;}
  if(a==0xffb20040){assert(value==1 && !pending);Submit();return;}
  assert(Range(a,0x6e000,8192) || Range(a,TT_Q38_FFN_RESIDENT_SUMMARY,128) || Range(a,0x80000,64)
   || Range(a,0x80400,256) || Range(a,0x80800,192) || Range(a,0x80c00,192) || Range(a,0x81000,64) || Range(a,0x17ffc0,12));
  if(Range(a,0x6e000,8192))minStack=std::min(minStack,unsigned(a));
  if(Range(a,0x80c00,192))nextStores++;if(Range(a,0x81000,64))outStores++;
  memory[a]=value;
 }
 void Submit(){
  assert(packets<5);unsigned p=packets++;bool write=p>=3;
  const unsigned local[]={0x80000,0x80400,0x80800,0x80c00,0x81000},size[]={64,256,192,192,64};
  const unsigned remote[]={descriptor[4]+descriptor[45],descriptor[12],descriptor[15],descriptor[4]+descriptor[42],descriptor[4]+descriptor[43]};
  const unsigned route[]={descriptor[8],descriptor[13],descriptor[41],descriptor[8],descriptor[8]};
  const uint32_t expected[]={write?local[p]:remote[p],0,write?descriptor[9]:route[p],write?remote[p]:local[p],0,
   write?route[p]:descriptor[9],0,write?0x212U:0,size[p],0};
  for(unsigned i=0;i<10;i++)assert(command[i]==expected[i]);
  if(write){assert(reads==3 && nextStores==48 && outStores==16);writes++;}else{
   reads++;for(unsigned i=0;i<size[p]/4;i++)assert(memory.at(local[p]+i*4)==0x7fc50000+p*1024+i);
  }
  if(packets==failPacket){pending=true;return;}
  for(unsigned i=0;i<size[p]/4;i++){
   if(!write){if(packets!=poisonFault || i!=size[p]/4-1)memory[local[p]+i*4]=p==0?input[i]:p==1?weight[i]:previous[i];}
   else if(p==3)next[i]=memory.at(local[p]+i*4);else output[i]=memory.at(local[p]+i*4);
  }
  unsigned at=write?4:0;counts[at]++;counts[at+1]+=size[p]/64;counts[at+3]++;counts[2]++;counts[6]++;
  if(packets==badCounter)counts[at+3]++;if(packets==foreignCounter)counts[write?0:4]++;
  if(packets==sourceFault)source[11]^=1;if(packets==descriptorFault)memory[TT_Q38_FFN_RESIDENT_DESCRIPTOR+40*4]^=1;
 }
 bool Run(){
  while(pc!=0x64038 && pc!=0x64058)StepScalar();
  assert(l1[Index(pc)]==0x0000006f && minStack>=0x6e000);
  for(unsigned a:{0x741fcU,0x74380U,0x7fffcU,0x80040U,0x803fcU,0x80500U,0x807fcU,0x808c0U,0x80bfcU,0x80cc0U,0x80ffcU,0x81040U,0x17ffbcU,0x17ffccU})assert(memory.at(a)==0xdecafbad);
  unsigned p=packets,r=nocReads,w=commandWrites,ns=nextStores,os=outStores;
  for(unsigned i=0;i<32;i++)StepScalar();assert(p==packets && r==nocReads && w==commandWrites && ns==nextStores && os==outStores);
  if(pc==0x64058){assert(memory.at(0x17ffc8)==TT_Q38_CONV_FAULT && !memory.at(0x17ffc4));return false;}
  assert(memory.at(0x17ffc8)==TT_Q38_CONV_MARKER && memory.at(0x17ffc0)==0x6400c);return memory.at(0x17ffc4)==16;
 }
 void Verify(){
  assert(packets==5 && reads==3 && writes==2 && nextStores==48 && outStores==16 && !pending);
  for(unsigned c=0;c<16;c++){
   float sum=0;double exact=0;
   for(unsigned t=0;t<4;t++){
    float x=PioneerAttentionFloat(t==3?input[c]:previous[c*3+t]),w=PioneerAttentionFloat(weight[c*4+t]);sum+=x*w;exact+=double(x)*w;
   }
   double reference=double(sum)/(1+exp(-double(sum)));float actual=PioneerAttentionFloat(output[c]);
   assert(isfinite(actual) && fabs(actual-reference)<=3e-6*(1+fabs(reference))
    && fabs(sum-exact)<=1e-6*(1+fabs(exact)));
   assert(next[c*3]==previous[c*3+1] && next[c*3+1]==previous[c*3+2] && next[c*3+2]==input[c]);
  }
  for(unsigned i=0;i<16;i++)assert(memory.at(0x80000+i*4)==input[i]);
  for(unsigned i=0;i<64;i++)assert(memory.at(0x80400+i*4)==weight[i]);
  for(unsigned i=0;i<48;i++)assert(memory.at(0x80800+i*4)==previous[i]);
  uint32_t summary[32];for(unsigned i=0;i<32;i++)summary[i]=memory.at(TT_Q38_FFN_RESIDENT_SUMMARY+i*4);
  assert(PioneerQ38ConvSummary(summary,descriptor.data(),before,counts));
  for(unsigned i=0;i<32;i++){summary[i]^=1;assert(!PioneerQ38ConvSummary(summary,descriptor.data(),before,counts));summary[i]^=1;}
 }
 void NoSink()const{assert(!writes);for(auto v:next)assert(v==0xdeadbeef);for(auto v:output)assert(v==0xdeadbeef);}
};
static void Plans(){
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();auto other=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build() && other->Build());
 uint32_t source[16]={};for(unsigned i=0;i<16;i++)source[i]=0x16005+i;PioneerQ38GdnIdentity id={1,2,3,4};
 std::vector<uint8_t> pages(PioneerResidentBanks::Pages);unsigned plans=0,refusals=0;
 for(unsigned layer=0;layer<64;layer++)if(layer%4!=3)for(unsigned first=0;first<10240;first+=16){
  unsigned bank=(first/16+layer)%7,worker=(first/16)%120;PioneerQ38ConvPlan p;
  assert(p.Build(*canonical,layer,first,8191,bank,worker,id,source,17)
   && p.Matches(*canonical,layer,first,8191,bank,worker,id,source,17)
   && !p.Matches(*other,layer,first,8191,bank,worker,id,source,17));refusals++;
  const auto* d=p.Descriptor();unsigned at=d[11]*unsigned(canonical->Capacity/4096)+(d[12]-canonical->Base)/4096;
  assert((d[12]&4095)<=3840 && !p.LearnedPagesPresent(pages.data(),unsigned(pages.size())));refusals++;
  pages[at]=1;assert(p.LearnedPagesPresent(pages.data(),unsigned(pages.size())));pages[at]=2;
  assert(!p.LearnedPagesPresent(pages.data(),unsigned(pages.size())));pages[at]=0;refusals++;plans++;
 }
 for(unsigned field=0;field<64;field++){
  PioneerQ38ConvPlan p;assert(p.Build(*canonical,0,0,8191,0,0,id,source,17));auto* d=const_cast<uint32_t*>(p.Descriptor());d[field]^=1;
  assert(!p.Matches(*canonical,0,0,8191,0,0,id,source,17));refusals++;
 }
 for(unsigned kind=0;kind<12;kind++){
  PioneerQ38ConvPlan p;unsigned layer=0,first=0,position=8191,bank=0,worker=0,phase=17;auto owner=id;const uint32_t* local=source;
  if(kind==0)layer=3;if(kind==1)layer=64;if(kind==2)first=1;if(kind==3)first=10240;if(kind==4)position=8192;
  if(kind==5)bank=7;if(kind==6)worker=120;if(kind==7)phase=0;if(kind==8)phase=0x7fffffff;
  if(kind==9)owner.modelEpoch=0;if(kind==10)local=nullptr;if(kind==11)source[0]=0;
  assert(!p.Build(*canonical,layer,first,position,bank,worker,owner,local,phase) && !p.Descriptor()
   && !p.Build(*canonical,0,0,8191,0,0,id,source,17));source[0]=0x16005;refusals++;
 }
 printf("PASS canonical QCV1 DATA plans%u/refusals%u,ALL48layers/640groups/stripe seams/current-next nonalias,NOT initialized-region/producer/native ownership\n",plans,refusals);
}
static void Sequence(const char* image,const PioneerQ38CanonicalModelPlan& canonical){
 std::vector<uint32_t> current(30720,0),next(30720,0),history(30720,0);unsigned jobs=0;
 for(unsigned step=0;step<4;step++){
  for(unsigned first=0;first<10240;first+=16){
   unsigned worker=(first/16)%120;ConvMachine m(image,canonical,0,first,(first/16)%7,worker,step+1,step);
   for(unsigned i=0;i<48;i++)m.previous[i]=current[first*3+i];
   auto before=m.previous;assert(m.Run());m.Verify();assert(m.previous==before);
   for(unsigned c=0;c<16;c++){
    unsigned channel=first+c;float sum=0;
    for(unsigned t=0;t<3;t++)sum+=PioneerAttentionFloat(history[channel*3+t])*PioneerAttentionFloat(m.weight[c*4+t]);
    sum+=PioneerAttentionFloat(m.input[c])*PioneerAttentionFloat(m.weight[c*4+3]);
    double reference=double(sum)/(1+exp(-double(sum)));double actual=PioneerAttentionFloat(m.output[c]);
    assert(fabs(actual-reference)<=3e-6*(1+fabs(reference)));
    for(unsigned t=0;t<3;t++)next[channel*3+t]=m.next[c*3+t];
   }
   // Independent shift-only history is NEVER the device's next-state
   // input. The subsequent step reads the ACTUAL emitted write results.
   for(unsigned c=0;c<16;c++){
    unsigned channel=first+c;history[channel*3]=history[channel*3+1];history[channel*3+1]=history[channel*3+2];history[channel*3+2]=m.input[c];
   }jobs++;
  }
  assert(next==history);current.swap(next);
  printf("PASS QCV1 actual RV32 sequence step%u/ALL10240channels/30720private next words/640jobs\n",step);
 }
 printf("PASS QCV1 actual instruction state chain ALL%ujobs/40960outputs/122880next-state words,ALL120worker coordinates;NO native coordinator or inference speed claim\n",jobs);
}
#ifndef PIONEER_Q38_CONV_MACHINE_ONLY
int main(int argc,char** argv){
 assert(argc==2 || (argc==3 && (!strcmp(argv[2],"--faults") || !strcmp(argv[2],"--sequence"))));
 setvbuf(stdout,nullptr,_IOLBF,0);Plans();unsigned positives=0,faults=0,maxSteps=0,maxStack=0;
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 if(argc==3 && !strcmp(argv[2],"--sequence")){Sequence(argv[1],*canonical);return 0;}
 if(argc==2){
  for(unsigned worker=0;worker<120;worker++){
   ConvMachine m(argv[1],*canonical,(worker%48)/3*4+(worker%3),((worker*79)%640)*16,worker%7,worker,worker%9);
   assert(m.Run());m.Verify();positives++;maxSteps=std::max(maxSteps,m.steps);maxStack=std::max(maxStack,0x70000-m.minStack);
  }
  for(unsigned trial=9;trial<12;trial++){
   ConvMachine m(argv[1],*canonical,62,10224,6,119,trial);assert(m.Run());m.Verify();positives++;
   maxSteps=std::max(maxSteps,m.steps);maxStack=std::max(maxStack,0x70000-m.minStack);
  }
 }else{
  for(unsigned kind=0;kind<5;kind++)for(unsigned packet=1;packet<=5;packet++){
   ConvMachine m(argv[1],*canonical,0,10224,6,119,1);if(kind==0)m.failPacket=packet;if(kind==1)m.badCounter=packet;
   if(kind==2)m.foreignCounter=packet;if(kind==3)m.sourceFault=packet;if(kind==4)m.descriptorFault=packet;
   assert(!m.Run() && m.packets==packet);if(packet<=3)m.NoSink();faults++;
  }
  for(unsigned packet=1;packet<=3;packet++){
   ConvMachine m(argv[1],*canonical,0,0,0,0,1);m.poisonFault=packet;assert(!m.Run() && m.packets==packet);m.NoSink();faults++;
  }
  for(unsigned part=0;part<3;part++)for(unsigned at:{0U,part==0?15U:part==1?63U:47U})for(uint32_t bits:{1U,0x267fffffU,0x47800000U,0x7f800000U,0x7fc00000U}){
   ConvMachine m(argv[1],*canonical,0,0,0,0,1);(part==0?m.input[at]:part==1?m.weight[at]:m.previous[at])=bits;
   assert(!m.Run() && m.packets==part+1);m.NoSink();faults++;
  }
  for(unsigned field=0;field<64;field++)for(unsigned bit=0;bit<32;bit++){
   ConvMachine m(argv[1],*canonical,0,0,0,0,1);m.descriptor[field]^=1U<<bit;m.SetDescriptor();
   if(!PioneerQ38ConvShape(m.descriptor.data())){assert(!m.Run() && !m.packets && !m.nocReads);m.NoSink();faults++;}
  }
  for(unsigned slot=0;slot<36;slot++){
   ConvMachine m(argv[1],*canonical,0,0,0,0,1);m.busyOffset=slot<4?0x40+slot*0x800:0x240+(slot-4)*4;
   assert(!m.Run() && !m.packets);m.NoSink();faults++;
  }
  for(unsigned i=0;i<16;i++){
   ConvMachine m(argv[1],*canonical,0,0,0,0,1);m.source[i]^=1;assert(!m.Run() && !m.packets);m.NoSink();faults++;
  }
  for(unsigned at:{1U,10U,11U,30U,50U}){
   ConvMachine m(argv[1],*canonical,0,0,0,0,1);m.badCommandRead=at;assert(!m.Run());faults++;
  }
 }
 printf("PASS ACTUAL QCV1 RV32 positive%u/faults%u/maxSteps%u/maxStack%u,Conv4+SiLU+immutable prior-state/private-next/3reads2writes/fresh poison/strict whitelist/inert first fault;SIMULATED_HARDWARE_NOT_NATIVE_SELECTION_OR_SPEED\n",positives,faults,maxSteps,maxStack);
}
#endif
