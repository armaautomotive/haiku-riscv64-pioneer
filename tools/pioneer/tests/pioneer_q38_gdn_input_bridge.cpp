/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_INPUT_MACHINE_ONLY
#include "pioneer_q38_gdn_input_image_test.cpp"
#include "pioneer_q38_gdn_input_bridge.h"
#include <set>
struct OriginalGdnInputMachine:GdnInputMachine {
 const std::vector<uint32_t>& resident;ProjectionExecutionLearnedBytes& original;
 OriginalGdnInputMachine(const char* image,const PioneerQ38CanonicalModelPlan& p,unsigned half,unsigned bank,
  const std::vector<uint32_t>& workspace,ProjectionExecutionLearnedBytes& source)
  :GdnInputMachine(image,p,0,half,bank),resident(workspace),original(source){}
 std::vector<uint32_t> Payload(unsigned p)override{
  assert(p<7 && original.Ready());std::vector<uint32_t> out(p<2?128:p==2?64:16);
  if(p<5){
   unsigned offset=descriptor[p==0?42:p==1?43:p==2?44:p==3?45:46];
   assert(resident.size()==524288 && !(offset&63) && offset/4+out.size()<=resident.size());
   for(unsigned i=0;i<out.size();i++)out[i]=resident[offset/4+i];
  }else{
   uint8_t bytes[64];assert(original.Read(descriptor[p==5?13:41],descriptor[p==5?12:15],bytes,64) && original.Ready());
   for(unsigned i=0;i<16;i++){const auto* b=bytes+i*4;out[i]=uint32_t(b[0])|(uint32_t(b[1])<<8)|(uint32_t(b[2])<<16)|(uint32_t(b[3])<<24);}
  }
  return out; // RAW existing device products/ORIGINAL weights,NO host q/k/gates
 }
};
bool ExecuteGdnInputResidentBytes(const char* image,const PioneerQ38CanonicalModelPlan& p,unsigned half,unsigned bank,
 const std::vector<uint32_t>& workspace,ProjectionExecutionLearnedBytes& original,GdnInputEmulationResult& result){
 (void)&GdnInputPlans;result={};OriginalGdnInputMachine machine(image,p,half,bank,workspace,original);
 if(!machine.Run())return false;machine.VerifyTransport();
 result.values=machine.Values();result.packets=machine.packets;result.steps=machine.steps;result.stack=0x70000-machine.minStack;return true;
}
struct NativeGdnInputMachine:GdnInputMachine {
 ProjectionExecutionFabric& fabric;unsigned worker;std::set<unsigned> dirtied;
 NativeGdnInputMachine(const char* image,const PioneerQ38CanonicalModelPlan& p,unsigned w,unsigned bank,ProjectionExecutionFabric& f)
  :GdnInputMachine(image,p,0,w-1,bank),fabric(f),worker(w){
  assert(w && w<=96 && fabric.LearnedBytes() && fabric.LearnedBytes()->Ready());
  assert(fabric.ReadLocal(w,0x6000)==0x0005e06f);
  for(unsigned i=0;i<64;i++)descriptor[i]=fabric.ReadLocal(w,TT_Q38_GDN_INPUT_DESCRIPTOR+i*4);
  assert(PioneerQ38GdnInputShape(descriptor.data()) && descriptor[7]==w-1 && descriptor[10]==w);SetDescriptor();
  for(unsigned i=0;i<code.size()/4;i++)l1[Index(0x64000+i*4)]=fabric.ReadLocal(w,0x64000+i*4);
  for(unsigned i=0;i<16;i++)source[i]=fabric.NoC(w,SourceOffset(i));
  const unsigned offsets[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};
  for(unsigned i=0;i<8;i++)before[i]=counts[i]=fabric.NoC(w,offsets[i]);
  for(unsigned i=0;i<10;i++)command[i]=fabric.NoC(w,i*4);
  auto copy=[&](unsigned at,unsigned bytes){for(unsigned i=0;i<bytes/4;i++)memory[at+i*4]=fabric.ReadLocal(w,at+i*4);};
  copy(0x6e000,8192);copy(0x80000,1296);copy(0x80800,256);copy(0x8e000,1288);
  copy(0x74000,16);copy(0x74600,128);copy(TT_Q38_GDN_INPUT_SUMMARY,128);copy(0x17ffc0,12);
 }
 std::vector<uint32_t> Payload(unsigned p)override{
  assert(p<7);std::vector<uint32_t> out(p<2?128:p==2?64:16);
  if(p<5){
   unsigned offset=descriptor[p==0?42:p==1?43:p==2?44:p==3?45:46];
   for(unsigned i=0;i<out.size();i++)out[i]=fabric.Gddr().at(offset/4+i);
  }else{
   auto* original=fabric.LearnedBytes();uint8_t bytes[64];
   assert(original && original->Ready() && original->Read(descriptor[p==5?13:41],descriptor[p==5?12:15],bytes,64) && original->Ready());
   for(unsigned i=0;i<16;i++){const auto* b=bytes+i*4;out[i]=uint32_t(b[0])|(uint32_t(b[1])<<8)|(uint32_t(b[2])<<16)|(uint32_t(b[3])<<24);}
  }
  return out; // actual resident products+original learned bytes ONLY
 }
 void Store(uint32_t a,uint32_t value)override{
  GdnInputMachine::Store(a,value);if(a<0xffb20000)dirtied.insert(a);
 }
 unsigned Execute(){
  while(pc!=0x64038 && pc!=0x64058)StepScalar();
  assert(pc==0x64038 && memory.at(0x17ffc4)==64);VerifyTransport();
  for(unsigned i=0;i<32;i++)StepScalar();
  const unsigned local[]={0x80000,0x80200,0x80400,0x80800,0x80840,0x80880,0x808c0};
  for(unsigned p=0;p<7;p++)for(unsigned i=0;i<received[p].size();i++)dirtied.insert(local[p]+i*4);
  for(unsigned a:dirtied)fabric.WriteLocal(worker,a,memory.at(a));
  fabric.SetControls(worker,command,counts);return pc;
 }
};
unsigned ExecuteGdnInputNativeBytes(const char* image,const PioneerQ38CanonicalModelPlan& p,unsigned worker,unsigned bank,
 ProjectionExecutionFabric& fabric){
 NativeGdnInputMachine machine(image,p,worker,bank,fabric);return machine.Execute();
}
