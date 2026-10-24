/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_GATED_RESIDENT_MACHINE_ONLY
#include "pioneer_q38_gdn_gated_resident_image_test.cpp"
#include "pioneer_q38_gdn_gated_native_bridge.h"
struct NativeGatedMachine:GatedResidentMachine {
 ProjectionExecutionFabric& fabric;DecoderHistoryBytes& state;unsigned worker;
 std::array<uint32_t,16> learnedBlock[2];bool learnedRead[2]={},normRead=false;
 NativeGatedMachine(const char* image,const PioneerQ38CanonicalModelPlan& p,unsigned w,unsigned bank,ProjectionExecutionFabric& f,DecoderHistoryBytes& s)
  :GatedResidentMachine(image,p,0,w-1,bank),fabric(f),state(s),worker(w){
  assert(w && w<=96 && fabric.LearnedBytes() && fabric.LearnedBytes()->Ready() && fabric.Reset(w)==0x46800);
  for(unsigned i=0;i<64;i++){
   inputDescriptor[i]=fabric.ReadLocal(w,TT_Q38_GDN_INPUT_DESCRIPTOR+i*4);
   descriptor[i]=fabric.ReadLocal(w,TT_Q38_RGDN_DESCRIPTOR+i*4);
   gatedDescriptor[i]=fabric.ReadLocal(w,TT_Q38_GDN_GATED_DESCRIPTOR+i*4);
  }
  assert(PioneerQ38GdnGatedShape(gatedDescriptor.data(),inputDescriptor.data(),descriptor.data()) && descriptor[9]==w-1);
  SetDescriptors();SetGatedDescriptor();
  assert(fabric.ReadLocal(w,0x6000)==0x0005e06f);
  for(unsigned i=0;i<code.size()/4;i++){
   uint32_t value=fabric.ReadLocal(w,0x64000+i*4);assert(value==Word(code,i*4));l1[Index(0x64000+i*4)]=value;
  }
  for(unsigned i=0;i<16;i++)seals[i]=fabric.NoC(w,SourceOffset(i));
  const unsigned offsets[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};
  for(unsigned i=0;i<8;i++)counts[i]=initialCounts[i]=fabric.NoC(w,offsets[i]);
  for(unsigned i=0;i<10;i++)command[i]=fabric.NoC(w,i*4);
  auto copy=[&](unsigned at,unsigned bytes){for(unsigned i=0;i<bytes/4;i++)memory[at+i*4]=fabric.ReadLocal(w,at+i*4);};
  copy(0x6e000,8192);copy(0x80000,1536);copy(0x80800,256);copy(0x8e000,1288);copy(0x8e800,256);copy(0x8ea00,256);
  copy(0x90000,32768);copy(0xa0000,32768);copy(0x74000,16);copy(0x74600,128);copy(0x17ffc0,12);
  copy(TT_Q38_GDN_INPUT_SUMMARY,128);copy(TT_Q38_RGDN_SUMMARY,128);copy(TT_Q38_GDN_GATED_SUMMARY,128);copy(TT_Q38_GDN_GATED_READY,128);
 }
 uint32_t Load(uint32_t at)override{
  if(Range(at,0xffb20000,0x2000)){
   unsigned off=at-0xffb20000;
   for(unsigned i=0;i<16;i++)if(off==SourceOffset(i))return fabric.NoC(worker,off);
   const unsigned offsets[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};
   for(unsigned o:offsets)if(off==o)return fabric.NoC(worker,off);
  }
  return GatedResidentMachine::Load(at);
 }
 uint32_t RawPayload(unsigned p,unsigned i)override{
  if(p<5){unsigned offset=inputDescriptor[p==0?42:p==1?43:p==2?44:p==3?45:46];return fabric.Gddr().at(offset/4+i);}
  auto* original=fabric.LearnedBytes();assert(p<7 && original && original->Ready());
  if(!learnedRead[p-5]){
   uint8_t bytes[64];assert(!i && original->Read(inputDescriptor[p==5?13:41],inputDescriptor[p==5?12:15],bytes,64));
   for(unsigned j=0;j<16;j++)learnedBlock[p-5][j]=uint32_t(bytes[j*4])|(uint32_t(bytes[j*4+1])<<8)|(uint32_t(bytes[j*4+2])<<16)|(uint32_t(bytes[j*4+3])<<24);
   learnedRead[p-5]=true;
  }
  return learnedBlock[p-5][i];
 }
 bool StateTransfer(bool write,unsigned piece,unsigned local)override{
  uint32_t values[4096];
  if(write){
   for(unsigned i=0;i<4096;i++)values[i]=memory.at(local+i*4);
   if(!state.WriteGdnState(descriptor[6],descriptor[7]+piece*16384,values,4096))return false;
   for(unsigned i=0;i<4096;i++)next[piece*4096+i]=values[i];
  }else{
   if(!state.ReadGdnState(descriptor[4],descriptor[5]+piece*16384,values,4096))return false;
   for(unsigned i=0;i<4096;i++){memory[local+i*4]=prior[piece*4096+i]=values[i];fabric.WriteLocal(worker,local+i*4,values[i]);}
  }
  return true;
 }
 bool HasPeerBytes()const override{return gatedDescriptor[14]>0 && gatedDescriptor[14]<=96;}
 uint32_t GatedPayload(unsigned p,unsigned i)override{
  const auto& d=gatedDescriptor;
  if(!p)return fabric.ReadLocal(d[14],d[44]+i*4); // actual zero until that real peer publishes
  if(p==1)return fabric.ReadLocal(d[14],d[45]+i*4);
  if(p==3)return fabric.Gddr().at(d[42]/4+i);
  assert(p==2);auto* original=fabric.LearnedBytes();assert(original && original->Ready());
  if(!normRead){
   uint8_t bytes[512];assert(!i && original->Read(d[13],d[12],bytes,512) && original->Ready());
   for(unsigned j=0;j<128;j++)norm[j]=uint32_t(bytes[j*4])|(uint32_t(bytes[j*4+1])<<8)|(uint32_t(bytes[j*4+2])<<16)|(uint32_t(bytes[j*4+3])<<24);
   normRead=true;
  }
  return norm[i];
 }
 void Store(uint32_t at,uint32_t value)override{
  unsigned before=packets,oldPhase=phase;GatedResidentMachine::Store(at,value);
  if(at<0x180000)fabric.WriteLocal(worker,at,value);
  if(at>=0xffb20000 && at<0xffb20028)fabric.SetControls(worker,command,counts);
  if(at==0xffb20040){
   fabric.SetControls(worker,command,counts);
   if(pending)return;
   unsigned local=0,bytes=0;
   if(before<7){
    const unsigned l[]={0x80000,0x80200,0x80400,0x80800,0x80840,0x80880,0x808c0},n[]={512,512,256,64,64,64,64};local=l[before];bytes=n[before];
   }else if(before>=11 && oldPhase<4){
    const unsigned l[]={TT_Q38_GDN_GATED_PEER_READY,0x80000+((gatedDescriptor[7]^1)&1)*256,0x80200,0x80400},n[]={64,256,512,512};
    local=l[oldPhase];bytes=n[oldPhase];
   }else if(before>=11 && oldPhase==4){
    for(unsigned i=0;i<64;i++)fabric.Gddr().at(gatedDescriptor[43]/4+i)=gatedOutput[i];
   }
   for(unsigned i=0;i<bytes/4;i++)fabric.WriteLocal(worker,local+i*4,memory.at(local+i*4));
  }
 }
 void StepNative(){
  uint32_t instruction=l1[Index(pc)],at=0;
  bool byte=(instruction&0x707f)==0x23;
  if(byte)at=(regs[(instruction>>15)&31]+Sign(((instruction>>25)<<5)|((instruction>>7)&31),12))&~3U;
  StepCombined();if(byte)fabric.WriteLocal(worker,at,memory.at(at));
 }
};
struct GatedNativeCohort::Impl {
 const char* image;const PioneerQ38CanonicalModelPlan& plan;unsigned bank;ProjectionExecutionFabric& fabric;DecoderHistoryBytes& state;
 std::unique_ptr<NativeGatedMachine> machine[96];
 Impl(const char* i,const PioneerQ38CanonicalModelPlan& p,unsigned b,ProjectionExecutionFabric& f,DecoderHistoryBytes& s):image(i),plan(p),bank(b),fabric(f),state(s){}
};
GatedNativeCohort::GatedNativeCohort(const char* i,const PioneerQ38CanonicalModelPlan& p,unsigned b,ProjectionExecutionFabric& f,DecoderHistoryBytes& s):impl(new Impl(i,p,b,f,s)){(void)&TestWorker;(void)&RunGatedPair;}
GatedNativeCohort::~GatedNativeCohort(){delete impl;}
void GatedNativeCohort::Start(unsigned w){assert(w && w<=96 && !impl->machine[w-1]);impl->machine[w-1].reset(new NativeGatedMachine(impl->image,impl->plan,w,impl->bank,impl->fabric,impl->state));}
void GatedNativeCohort::Advance(unsigned quantum){
 assert(quantum && quantum<=4096);
 for(auto& m:impl->machine)if(m && !m->Terminal()){
  assert(impl->fabric.Reset(m->worker)==0x46800);
  for(unsigned i=0;i<quantum && !m->Terminal();i++)m->StepNative();
 }
}
unsigned GatedNativeCohort::Pc(unsigned w)const{assert(w && w<=96 && impl->machine[w-1]);return impl->machine[w-1]->pc;}
