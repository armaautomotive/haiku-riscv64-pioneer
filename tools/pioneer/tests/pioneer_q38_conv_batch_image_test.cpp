/* SPDX-License-Identifier: MIT */
// Execute ACTUAL QCB1 RV32 instructions. Only NoC/GDDR endpoints simulated;
// no host relaunch/callback between any worker's5or6jobs. Oracle OBSERVES.
#define PIONEER_Q38_CONV_MACHINE_ONLY
#include "pioneer_q38_conv_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ConvBatchPlan.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionGraphFootprint.h"
struct ConvBatchMachine:ConvMachine {
 struct Group {
  std::array<uint32_t,16> input,output;
  std::array<uint32_t,64> weight;
  std::array<uint32_t,48> previous,next;
 } groups[6];
 std::array<uint32_t,384> table={};unsigned count=0,worker=0;
 unsigned tableFault=0,tableFaultRecord=5,commandFault=0,betweenAfter=0,aliasReads=0,badAliasRead=0;
 bool injected=false;
 ConvBatchMachine(const char* image,const PioneerQ38CanonicalModelPlan& canonical,unsigned layer,unsigned bank,unsigned selected,unsigned trial=1,unsigned position=8191)
  :ConvMachine(image,canonical,layer,selected*16,bank,selected,trial,position),worker(selected){
  auto plan=std::make_unique<PioneerQ38ConvBatchPlan>();PioneerQ38GdnIdentity id={1,2,3,4};
  assert(plan->Build(canonical,layer,position,bank,worker,id,source,17));count=plan->Count();
  for(unsigned i=0;i<384;i++){table[i]=plan->Records()[i];memory[TT_Q38_CONV_BATCH_TABLE+i*4]=table[i];}
  assert(PioneerQ38ProjectionGraphPrefixBytes(worker)>=code.size());
  for(unsigned a=0x64000;a<0x64000+code.size();a+=4)assert(PioneerQ38ProjectionGraphSnapshotContains(worker,a));
  for(unsigned a=TT_Q38_CONV_BATCH_TABLE;a<TT_Q38_CONV_BATCH_TABLE+1536;a+=4)
   assert(PioneerQ38ProjectionGraphSnapshotContains(worker,a) && !PioneerQ38ProjectionGraphProgramImmutable(worker,a));
  memory[0x747fc]=memory[0x74e00]=0xdecafbad;
  for(unsigned j=0;j<count;j++){
   auto& g=groups[j];unsigned first=table[j*64+7];
   for(unsigned c=0;c<16;c++){
    g.input[c]=PioneerAttentionBits(trial?float(sin((first+c)*.017+trial*.231)):0.f);g.output[c]=0xdeadbeef;
    for(unsigned t=0;t<4;t++)g.weight[c*4+t]=PioneerAttentionBits(float(int(((first+c)*7+t*11)%29)-14)*.03125f);
    for(unsigned t=0;t<3;t++){g.previous[c*3+t]=PioneerAttentionBits(trial?float(cos((first+c)*.013+t*.117+trial*.031)):0.f);g.next[c*3+t]=0xdeadbeef;}
   }
  }
 }
 void SetTable(){for(unsigned i=0;i<384;i++)memory[TT_Q38_CONV_BATCH_TABLE+i*4]=table[i];}
 uint32_t Load(uint32_t a)override{
  if(Range(a,TT_Q38_CONV_BATCH_TABLE,1536)){assert(!(a&3));return memory.at(a);}
  if(a==0xffb20100 && packets==betweenAfter && betweenAfter && !injected){counts[4]++;injected=true;}
  if(a==0xffb20210 && ++aliasReads==badAliasRead){nocReads++;return counts[2]^1;}
  return ConvMachine::Load(a);
 }
 void Submit(){
  assert(packets<count*5);unsigned j=packets/5,p=packets%5;packets++;bool write=p>=3;
  auto& g=groups[j];const auto* d=table.data()+j*64;
  const unsigned local[]={0x80000,0x80400,0x80800,0x80c00,0x81000},size[]={64,256,192,192,64};
  const unsigned remote[]={d[4]+d[45],d[12],d[15],d[4]+d[42],d[4]+d[43]},route[]={d[8],d[13],d[41],d[8],d[8]};
  const uint32_t expected[]={write?local[p]:remote[p],0,write?d[9]:route[p],write?remote[p]:local[p],0,
   write?route[p]:d[9],0,write?0x212U:0,size[p],0};
  for(unsigned i=0;i<10;i++)assert(command[i]==expected[i]);
  if(write){assert(reads==(j+1)*3 && nextStores==(j+1)*48 && outStores==(j+1)*16);writes++;}
  else{reads++;for(unsigned i=0;i<size[p]/4;i++)assert(memory.at(local[p]+i*4)==0x7fc50000+p*1024+i);}
  if(packets==failPacket){pending=true;return;}
  for(unsigned i=0;i<size[p]/4;i++){
   if(!write){if(packets!=poisonFault || i!=size[p]/4-1)memory[local[p]+i*4]=p==0?g.input[i]:p==1?g.weight[i]:g.previous[i];}
   else if(p==3)g.next[i]=memory.at(local[p]+i*4);else g.output[i]=memory.at(local[p]+i*4);
  }
  unsigned at=write?4:0;counts[at]++;counts[at+1]+=size[p]/64;counts[at+3]++;counts[2]++;counts[6]++;
  if(packets==badCounter)counts[at+3]++;if(packets==foreignCounter)counts[write?0:4]++;
  if(packets==sourceFault)source[11]^=1;if(packets==descriptorFault)memory[TT_Q38_CONV_BATCH_TABLE+40*4]^=1;
  if(packets==tableFault)memory[TT_Q38_CONV_BATCH_TABLE+(tableFaultRecord*64+40)*4]^=1;
  if(packets==commandFault)command[1]^=1;
 }
 void Store(uint32_t a,uint32_t value)override{
  if(a==0xffb20040){assert(value==1 && !pending);Submit();return;}
  ConvMachine::Store(a,value);
 }
 bool RunBatch(){
  while(pc!=0x64038 && pc!=0x64058)StepScalar();
  assert(l1[Index(pc)]==0x0000006f && minStack>=0x6e000 && memory.at(0x747fc)==0xdecafbad && memory.at(0x74e00)==0xdecafbad);
  unsigned p=packets,r=nocReads,w=commandWrites,ns=nextStores,os=outStores;
  for(unsigned i=0;i<32;i++)StepScalar();assert(p==packets && r==nocReads && w==commandWrites && ns==nextStores && os==outStores);
  if(pc==0x64058){assert(memory.at(0x17ffc8)==TT_Q38_CONV_BATCH_FAULT && !memory.at(0x17ffc4));return false;}
  assert(memory.at(0x17ffc8)==TT_Q38_CONV_BATCH_MARKER && memory.at(0x17ffc0)==0x6400c);
  return memory.at(0x17ffc4)==count*16;
 }
 void VerifyBatch(){
  assert(packets==count*5 && reads==count*3 && writes==count*2 && nextStores==count*48 && outStores==count*16 && !pending);
  for(unsigned j=0;j<count;j++){
   const auto& g=groups[j];for(unsigned c=0;c<16;c++){
    float sum=0;for(unsigned t=0;t<4;t++)sum+=PioneerAttentionFloat(t==3?g.input[c]:g.previous[c*3+t])*PioneerAttentionFloat(g.weight[c*4+t]);
    double reference=double(sum)/(1+exp(-double(sum))),actual=PioneerAttentionFloat(g.output[c]);
    assert(isfinite(actual) && fabs(actual-reference)<=3e-6*(1+fabs(reference)));
    assert(g.next[c*3]==g.previous[c*3+1] && g.next[c*3+1]==g.previous[c*3+2] && g.next[c*3+2]==g.input[c]);
   }
  }
  for(unsigned i=0;i<384;i++)assert(memory.at(TT_Q38_CONV_BATCH_TABLE+i*4)==table[i]);
  for(unsigned i=0;i<64;i++)assert(memory.at(TT_Q38_FFN_RESIDENT_DESCRIPTOR+i*4)==descriptor[i]);
  uint32_t summary[32];for(unsigned i=0;i<32;i++)summary[i]=memory.at(TT_Q38_FFN_RESIDENT_SUMMARY+i*4);
  assert(PioneerQ38ConvBatchSummary(summary,table.data(),before,counts));
  for(unsigned i=0;i<32;i++){summary[i]^=1;assert(!PioneerQ38ConvBatchSummary(summary,table.data(),before,counts));summary[i]^=1;}
 }
 void NoBatchSink()const{assert(!writes);for(unsigned j=0;j<count;j++){for(auto v:groups[j].next)assert(v==0xdeadbeef);for(auto v:groups[j].output)assert(v==0xdeadbeef);}}
};
static void BatchPlans(){
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();auto other=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build() && other->Build());
 uint32_t source[16]={};for(unsigned i=0;i<16;i++)source[i]=0x16005+i;PioneerQ38GdnIdentity id={1,2,3,4};
 unsigned plans=0,groups=0,refusals=0;std::vector<uint8_t> pages(PioneerResidentBanks::Pages);
 for(unsigned layer=0;layer<64;layer++)if(layer%4!=3){
  unsigned coverage[640]={};
  for(unsigned worker=0;worker<120;worker++){
   auto p=std::make_unique<PioneerQ38ConvBatchPlan>();unsigned bank=(layer+worker)%7;
   assert(p->Build(*canonical,layer,8191,bank,worker,id,source,17) && p->Count()==(worker<40?6U:5U)
    && p->Matches(*canonical,layer,8191,bank,worker,id,source,17) && !p->Matches(*other,layer,8191,bank,worker,id,source,17));refusals++;
   const auto* table=p->Records();assert(!p->LearnedPagesPresent(pages.data(),unsigned(pages.size())));refusals++;
   for(unsigned j=0;j<p->Count();j++){
    const auto* d=table+j*64;assert(++coverage[d[7]/16]==1);groups++;
    unsigned page=d[11]*unsigned(canonical->Capacity/4096)+(d[12]-canonical->Base)/4096;pages[page]=1;
   }
   assert(p->LearnedPagesPresent(pages.data(),unsigned(pages.size())));
   for(unsigned j=0;j<p->Count();j++){
    const auto* d=table+j*64;unsigned page=d[11]*unsigned(canonical->Capacity/4096)+(d[12]-canonical->Base)/4096;
    pages[page]=2;assert(!p->LearnedPagesPresent(pages.data(),unsigned(pages.size())));pages[page]=1;refusals++;
   }
   for(unsigned j=0;j<p->Count();j++){const auto* d=table+j*64;pages[d[11]*unsigned(canonical->Capacity/4096)+(d[12]-canonical->Base)/4096]=0;}
   assert(!p->Build(*canonical,layer,8191,bank,worker,id,source,17));refusals++;plans++;
  }
  for(auto value:coverage)assert(value==1);
 }
 auto p=std::make_unique<PioneerQ38ConvBatchPlan>();assert(p->Build(*canonical,0,8191,0,0,id,source,17));auto* table=const_cast<uint32_t*>(p->Records());
 for(unsigned i=0;i<384;i++){table[i]^=1;assert(!p->Matches(*canonical,0,8191,0,0,id,source,17));table[i]^=1;refusals++;}
 printf("PASS QCB1 canonical DATA plans%u/ALL%ugroups/refusals%u,48layers/120workers,exact640group partition/ALL6records/unused zero/learned pages;NO init/source/native owner grant\n",plans,groups,refusals);
}
#ifndef PIONEER_Q38_CONV_BATCH_MACHINE_ONLY
int main(int argc,char** argv){
 assert(argc==2 || (argc==3 && !strcmp(argv[2],"--faults")));setvbuf(stdout,nullptr,_IOLBF,0);(void)&Plans;(void)&Sequence;
 BatchPlans();auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 unsigned positives=0,faults=0,maxSteps=0,maxStack=0;
 if(argc==2){
  for(unsigned worker=0;worker<120;worker++){
   ConvBatchMachine m(argv[1],*canonical,(worker%48)/3*4+worker%3,worker%7,worker,worker%9);
   assert(m.RunBatch());m.VerifyBatch();positives++;maxSteps=std::max(maxSteps,m.steps);maxStack=std::max(maxStack,0x70000-m.minStack);
  }
 }else{
  for(unsigned worker:{0U,119U}){
   unsigned n=PioneerQ38ConvBatchCount(worker);
   for(unsigned kind=0;kind<7;kind++)for(unsigned packet=1;packet<=n*5;packet++){
    ConvBatchMachine m(argv[1],*canonical,0,0,worker);if(kind==0)m.failPacket=packet;if(kind==1)m.badCounter=packet;
    if(kind==2)m.foreignCounter=packet;if(kind==3)m.sourceFault=packet;if(kind==4)m.descriptorFault=packet;
    if(kind==5)m.tableFault=packet;if(kind==6)m.commandFault=packet;
    assert(!m.RunBatch() && m.packets==packet);if(packet<=3)m.NoBatchSink();faults++;
   }
   for(unsigned j=0;j<n;j++)for(unsigned packet=1;packet<=3;packet++){
    ConvBatchMachine m(argv[1],*canonical,0,0,worker);m.poisonFault=j*5+packet;assert(!m.RunBatch() && m.packets==m.poisonFault);faults++;
   }
   for(unsigned packet:{1U,5U,n*5}){
    ConvBatchMachine m(argv[1],*canonical,0,0,worker);m.betweenAfter=packet;assert(!m.RunBatch() && m.packets==packet && m.injected);faults++;
   }
   for(unsigned alias:{1U,2U,3U,4U}){
    ConvBatchMachine m(argv[1],*canonical,0,0,worker);m.badAliasRead=alias;assert(!m.RunBatch() && !m.packets);m.NoBatchSink();faults++;
   }
  }
  for(unsigned field=0;field<384;field++)for(unsigned bit=0;bit<32;bit++){
   ConvBatchMachine m(argv[1],*canonical,0,0,119);m.table[field]^=1U<<bit;m.SetTable();
   if(!PioneerQ38ConvBatchShape(m.table.data())){assert(!m.RunBatch() && !m.packets && !m.nocReads);m.NoBatchSink();faults++;}
  }
 }
 printf("PASS ACTUAL QCB1 RV32 positive%u/faults%u/maxSteps%u/maxStack%u,5or6jobs/card invocation,ALL640groups partitioned,tracked counters/sealALL6records/fresh poison/private-next,NO host relaunch between jobs;SIMULATED_HARDWARE_NOT_NATIVE_OR_TOKEN_RATE\n",positives,faults,maxSteps,maxStack);
}
#endif
