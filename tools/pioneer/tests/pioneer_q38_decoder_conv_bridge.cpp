/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_CONV_BATCH_MACHINE_ONLY
#include "pioneer_q38_conv_batch_image_test.cpp"
#include "pioneer_q38_decoder_conv_bridge.h"
#include <set>
struct DecoderBatchMachine:ConvBatchMachine {
 ProjectionExecutionFabric& fabric;DecoderHistoryBytes& producer;std::set<unsigned> dirtied;
 DecoderBatchMachine(const char* image,const PioneerQ38CanonicalModelPlan& plan,unsigned worker,unsigned bank,
  ProjectionExecutionFabric& f,DecoderHistoryBytes& p):ConvBatchMachine(image,plan,f.ReadLocal(worker,TT_Q38_CONV_BATCH_TABLE+5*4),bank,worker,1,0),fabric(f),producer(p){
  for(unsigned i=0;i<384;i++)table[i]=fabric.ReadLocal(worker,TT_Q38_CONV_BATCH_TABLE+i*4);
  count=PioneerQ38ConvBatchCount(worker);assert(PioneerQ38ConvBatchShape(table.data()));
  for(unsigned i=0;i<16;i++)source[i]=fabric.NoC(worker,SourceOffset(i));
  const unsigned offsets[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};
  for(unsigned i=0;i<8;i++)before[i]=counts[i]=fabric.NoC(worker,offsets[i]);
  for(unsigned i=0;i<10;i++)command[i]=fabric.NoC(worker,i*4);
  for(unsigned i=0;i<code.size()/4;i++)l1[Index(0x64000+i*4)]=fabric.ReadLocal(worker,0x64000+i*4);
  auto copy=[&](unsigned at,unsigned bytes){for(unsigned i=0;i<bytes/4;i++)memory[at+i*4]=fabric.ReadLocal(worker,at+i*4);};
  copy(0x6e000,8192);copy(TT_Q38_CONV_BATCH_TABLE,1536);copy(TT_Q38_FFN_RESIDENT_DESCRIPTOR,256);copy(TT_Q38_FFN_RESIDENT_SUMMARY,128);
  copy(0x80000,64);copy(0x80400,256);copy(0x80800,192);copy(0x80c00,192);copy(0x81000,64);copy(0x17ffc0,12);
 }
 void SubmitShared(){
  assert(packets<count*5);unsigned j=packets/5,p=packets%5;packets++;bool write=p>=3;
  const auto* d=table.data()+j*64;auto& g=groups[j];
  const unsigned local[]={0x80000,0x80400,0x80800,0x80c00,0x81000},size[]={64,256,192,192,64};
  const unsigned remote[]={d[4]+d[45],d[12],d[15],d[4]+d[42],d[4]+d[43]},route[]={d[8],d[13],d[41],d[8],d[8]};
  const unsigned expected[]={write?local[p]:remote[p],0,write?d[9]:route[p],write?remote[p]:local[p],0,write?route[p]:d[9],0,write?0x212U:0,size[p],0};
  for(unsigned i=0;i<10;i++)assert(command[i]==expected[i]);
  if(!write){
   reads++;for(unsigned i=0;i<size[p]/4;i++)assert(memory.at(local[p]+i*4)==0x7fc50000+p*1024+i);
   if(!p)for(unsigned i=0;i<16;i++)g.input[i]=fabric.Gddr().at(d[45]/4+i);
   if(p==1 && fabric.LearnedBytes()){
    // ORIGINAL source bytes,not precomputed products. Once attached,this
    // source is mandatory;NEVER retain synthetic coefficients on failure.
    auto* learned=fabric.LearnedBytes();uint8_t bytes[256];
    assert(learned->Ready() && learned->Read(d[13],d[12],bytes,256) && learned->Ready());
    for(unsigned i=0;i<64;i++){const auto* b=bytes+i*4;g.weight[i]=uint32_t(b[0])|(uint32_t(b[1])<<8)|(uint32_t(b[2])<<16)|(uint32_t(b[3])<<24);}
   }
   if(p==2)assert(producer.ReadHistory(d[14],d[15],g.previous.data(),48));
   for(unsigned i=0;i<size[p]/4;i++){memory[local[p]+i*4]=!p?g.input[i]:p==1?g.weight[i]:g.previous[i];dirtied.insert(local[p]+i*4);}
  }else{
   writes++;assert(reads==(j+1)*3 && nextStores==(j+1)*48 && outStores==(j+1)*16);
   for(unsigned i=0;i<size[p]/4;i++){uint32_t value=memory.at(local[p]+i*4);fabric.Gddr().at((p==3?d[42]:d[43])/4+i)=value;
    if(p==3)g.next[i]=value;else g.output[i]=value;}
  }
  unsigned at=write?4:0;counts[at]++;counts[at+1]+=size[p]/64;counts[at+3]++;counts[2]++;counts[6]++;
 }
 void Store(uint32_t a,uint32_t v)override{
  if(a==0xffb20040){assert(v==1 && !pending);SubmitShared();return;}
  ConvMachine::Store(a,v);if(a<0xffb20000)dirtied.insert(a);
 }
 unsigned Execute(){
  while(pc!=0x64038 && pc!=0x64058)StepScalar();
  assert(pc==0x64038 && packets==count*5 && reads==count*3 && writes==count*2);
  for(unsigned i=0;i<32;i++)StepScalar();
  for(unsigned a:dirtied)fabric.WriteLocal(worker,a,memory.at(a));
  fabric.SetControls(worker,command,counts);return pc;
 }
};
unsigned ExecuteDecoderConv(const char* image,const PioneerQ38CanonicalModelPlan& plan,unsigned worker,unsigned bank,
 ProjectionExecutionFabric& fabric,DecoderHistoryBytes& producer){
 (void)&BatchPlans;(void)&Plans;(void)&Sequence;
 DecoderBatchMachine machine(image,plan,worker,bank,fabric,producer);return machine.Execute();
}
