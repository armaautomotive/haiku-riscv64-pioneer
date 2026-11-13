/* SPDX-License-Identifier: MIT */
// Extend the existing single BFP8 worker simulator with raw Conv NoC
// endpoints. RV32 code, registers, tensor state, L1 and counters are shared;
// no device or claimed native scheduler is involved.
#ifndef PIONEER_Q38_BFP_CONV_WORKER_H
#define PIONEER_Q38_BFP_CONV_WORKER_H
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ConvBatchPlan.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentConvServiceShape.h"
struct PioneerQ38BfpConvWorker:DenseProducerImage {
 struct Group {std::array<uint32_t,16> input={},output={};std::array<uint32_t,64> weight={};std::array<uint32_t,48> prior={},next={};};
 std::array<uint32_t,384> table={};std::array<Group,6> group={};
 unsigned packets=0,writes=0,before[8]={};bool conv=false;
 using DenseProducerImage::DenseProducerImage;
 static bool Local(unsigned a){return Range(a,0x80000,0x1100) || Range(a,0x74140,64) || Range(a,TT_Q38_CONV_BATCH_TABLE,1536) || Range(a,TT_Q38_FFN_RESIDENT_SUMMARY,128);}
 uint32_t Load(uint32_t a)override{
  if(conv && Local(a))return WordRead(a);
  if(conv && Range(a,0x74000,96))return Get(a);
  if(conv && a<0x180000 && !CodeAddress(a) && !Range(a,0x6e000,8192) && !Range(a,HW::Boot,512) && a!=HW::Doorbell && !Range(a,HW::Status,64) && !Range(a,0x74000,96) && a!=0x74100 && !Range(a,0x83f00,256) && !Range(a,W::Output,8*32*4))
   std::fprintf(stderr,"unhandled conv worker load %x pc %x\n",a,pc);
  return DenseProducerImage::Load(a);
 }
 void Store(uint32_t a,uint32_t value)override{
  if(conv && Local(a)){WordWrite(a,value);return;}
  if(conv && Range(a,0xffb20000,40)){transfer.command[(a-0xffb20000)/4]=value;return;}
  if(conv && a==0xffb20040){assert(value==1 && transfer.pending.empty());Packet();return;}
  if(conv && a<0x180000 && !Range(a,0x6e000,8192) && !Range(a,HW::Status,64) && !Range(a,0x74000,96) && a!=0x74100 && !Range(a,0x83f00,256) && !Range(a,W::Output,8*32*4))
   std::fprintf(stderr,"unhandled conv worker store %x pc %x\n",a,pc);
  DenseProducerImage::Store(a,value);
 }
 void Packet(){
  unsigned j=packets/5,p=packets%5;assert(j<PioneerQ38ConvBatchCount(boot[3]));packets++;
  const bool write=p>=3;const auto* d=table.data()+j*64;
  const unsigned local[]={0x80000,0x80400,0x80800,0x80c00,0x81000},bytes[]={64,256,192,192,64};
  const unsigned remote[]={d[4]+d[45],d[12],d[15],d[4]+d[42],d[4]+d[43]},route[]={d[8],d[13],d[41],d[8],d[8]};
  const uint32_t command[]={write?local[p]:remote[p],0,write?d[9]:route[p],write?remote[p]:local[p],0,write?route[p]:d[9],0,write?0x212U:0,bytes[p],0};
  for(unsigned i=0;i<10;i++)assert(transfer.command[i]==command[i]);
  auto& g=group[j];for(unsigned i=0;i<bytes[p]/4;i++){
   if(p==0)WordWrite(local[p]+i*4,g.input[i]);else if(p==1)WordWrite(local[p]+i*4,g.weight[i]);
   else if(p==2)WordWrite(local[p]+i*4,g.prior[i]);else if(p==3){g.next[i]=WordRead(local[p]+i*4);writes++;}
   else {g.output[i]=WordRead(local[p]+i*4);writes++;}
  }
  auto& c=transfer.counters;c[4]++;c[14]++;
  if(write){c[1]++;c[8]+=bytes[p]/64;c[10]++;}else{c[2]++;c[3]+=bytes[p]/64;c[5]++;}
 }
 void StageConv(unsigned sequence,const std::vector<uint32_t>& qkv,const std::vector<uint32_t>& learned){
  assert(pc==waitPC && qkv.size()==10240 && learned.size()==41184 && native.Clean() && transfer.pending.empty());
  auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
  auto batch=std::make_unique<PioneerQ38ConvBatchPlan>();PioneerQ38GdnIdentity identity={1,2,3,4};
  assert(batch->Build(*canonical,0,0,0,boot[3],identity,source.data(),2025));
  for(unsigned i=0;i<384;i++){table[i]=batch->Records()[i];WordWrite(TT_Q38_CONV_BATCH_TABLE+i*4,table[i]);}
  for(unsigned j=0;j<batch->Count();j++)for(unsigned c=0;c<16;c++){
   const unsigned row=table[j*64+7]+c;group[j].input[c]=qkv[row];
   for(unsigned t=0;t<4;t++)group[j].weight[c*4+t]=learned[row*4+t];
   for(unsigned t=0;t<3;t++)group[j].prior[c*3+t]=0;
  }
  const unsigned indices[]={2,3,4,5,1,8,4,10};for(unsigned i=0;i<8;i++)before[i]=transfer.counters[indices[i]];
  uint32_t job[64]={};job[0]=HW::Magic;job[1]=sequence;job[2]=7;job[3]=boot[3];job[4]=boot[4];job[5]=0;job[7]=2025;
  for(unsigned i=0;i<8;i++)job[8+i]=boot[8+i];
  job[16]=TT_Q38_RESIDENT_CONV_SERVICE_JOB;job[17]=batch->Count();job[18]=1536;job[19]=2025;
  assert(HW::JobShape(job,boot.data(),sequence-1,0));for(unsigned i=0;i<64;i++)Put(HW::Job+i*4,job[i]);
  conv=true;packets=writes=0;Put(HW::Doorbell,sequence);
 }
 void VerifyConv(unsigned sequence){
  const unsigned n=PioneerQ38ConvBatchCount(boot[3]);assert(packets==n*5 && writes==n*64);
  uint32_t proof[64],job[16],after[8],summary[32];const unsigned indices[]={2,3,4,5,1,8,4,10};
  for(unsigned i=0;i<64;i++)proof[i]=Get(0x83f00+i*4);for(unsigned i=0;i<16;i++)job[i]=Get(0x74000+i*4);
  for(unsigned i=0;i<8;i++)after[i]=transfer.counters[indices[i]];
  for(unsigned i=0;i<32;i++)summary[i]=WordRead(TT_Q38_FFN_RESIDENT_SUMMARY+i*4);
  assert(PioneerQ38ResidentConvServiceProof(proof,job,table.data(),source.data(),boot[3],boot[4],before,after,summary));
  for(unsigned j=0;j<n;j++)for(unsigned c=0;c<16;c++){
   auto& g=group[j];float sum=0;
   for(unsigned t=0;t<4;t++){float x=PioneerAttentionFloat(t==3?g.input[c]:g.prior[c*3+t]);sum+=x*PioneerAttentionFloat(g.weight[c*4+t]);}
   double reference=double(sum)/(1+std::exp(-double(sum))),actual=PioneerAttentionFloat(g.output[c]);
   assert(std::isfinite(actual) && std::fabs(actual-reference)<=3e-6*(1+std::fabs(reference)));
   assert(!g.next[c*3] && !g.next[c*3+1] && g.next[c*3+2]==g.input[c]);
  }
  assert(Get(HW::Doorbell)==sequence && Get(HW::Status+4)==sequence);conv=false;
 }
};
#endif
