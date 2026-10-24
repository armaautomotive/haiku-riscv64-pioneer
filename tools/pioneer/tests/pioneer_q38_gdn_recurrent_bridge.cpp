/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_RECURRENT_MACHINE_ONLY
#include "pioneer_q38_gdn_recurrent_image_test.cpp"
#include "pioneer_q38_gdn_recurrent_bridge.h"
#include <set>
struct NativeGdnRecurrentMachine:GdnRecurrentMachine {
 ProjectionExecutionFabric& fabric;DecoderHistoryBytes& state;unsigned worker;std::set<unsigned> dirtied;
 std::array<uint32_t,16> learnedBlock[2];bool learnedRead[2]={};
 NativeGdnRecurrentMachine(const char* image,const PioneerQ38CanonicalModelPlan& p,unsigned w,unsigned bank,
  ProjectionExecutionFabric& f,DecoderHistoryBytes& s):GdnRecurrentMachine(image,p,0,w-1,bank),fabric(f),state(s),worker(w){
  assert(w && w<=96 && fabric.LearnedBytes() && fabric.LearnedBytes()->Ready() && fabric.ReadLocal(w,0x6000)==0x0005e06f);
  for(unsigned i=0;i<64;i++){
   inputDescriptor[i]=fabric.ReadLocal(w,TT_Q38_GDN_INPUT_DESCRIPTOR+i*4);descriptor[i]=fabric.ReadLocal(w,TT_Q38_RGDN_DESCRIPTOR+i*4);
  }
  assert(PioneerQ38GdnRecurrentShape(inputDescriptor.data(),descriptor.data()) && descriptor[9]==w-1 && !descriptor[11]);SetDescriptors();
  for(unsigned i=0;i<code.size()/4;i++)l1[Index(0x64000+i*4)]=fabric.ReadLocal(w,0x64000+i*4);
  for(unsigned i=0;i<16;i++)seals[i]=fabric.NoC(w,SourceOffset(i));
  const unsigned offsets[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};
  for(unsigned i=0;i<8;i++)initialCounts[i]=counts[i]=fabric.NoC(w,offsets[i]);
  for(unsigned i=0;i<10;i++)command[i]=fabric.NoC(w,i*4);
  auto copy=[&](unsigned at,unsigned bytes){for(unsigned i=0;i<bytes/4;i++)memory[at+i*4]=fabric.ReadLocal(w,at+i*4);};
  copy(0x6e000,8192);copy(0x80000,1296);copy(0x80800,256);copy(0x8e000,1288);copy(0x8e800,256);
  copy(0x90000,32768);copy(0xa0000,32768);copy(0x74000,16);copy(0x74600,128);
  copy(TT_Q38_GDN_INPUT_SUMMARY,128);copy(TT_Q38_RGDN_SUMMARY,128);copy(0x17ffc0,12);
 }
 uint32_t RawPayload(unsigned p,unsigned i)override{
  assert(p<7);
  if(p<5){unsigned offset=inputDescriptor[p==0?42:p==1?43:p==2?44:p==3?45:46];return fabric.Gddr().at(offset/4+i);}
  auto* original=fabric.LearnedBytes();assert(original && original->Ready());
  if(!learnedRead[p-5]){uint8_t bytes[64];
   assert(!i && original->Read(inputDescriptor[p==5?13:41],inputDescriptor[p==5?12:15],bytes,64) && original->Ready());
   for(unsigned j=0;j<16;j++){const auto* b=bytes+j*4;learnedBlock[p-5][j]=uint32_t(b[0])|(uint32_t(b[1])<<8)|(uint32_t(b[2])<<16)|(uint32_t(b[3])<<24);}
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
   for(unsigned i=0;i<4096;i++)memory[local+i*4]=prior[piece*4096+i]=values[i];
  }
  return true; // actual initialized/current/private-next device BYTES,not host arithmetic
 }
 void Store(uint32_t at,uint32_t value)override{GdnRecurrentMachine::Store(at,value);if(at<0xffb20000)dirtied.insert(at);}
 unsigned Execute(){
  while(pc!=0x64038 && pc!=0x64058)StepCombined();
  for(unsigned i=0;i<32;i++)StepCombined();
  // SB table-copy instructions write the actual saved stack. Copy the
  // complete8KiB endpoint back too;never fabricate an original stack seal.
  for(unsigned i=0;i<2048;i++)dirtied.insert(0x6e000+i*4);
  const unsigned local[]={0x80000,0x80200,0x80400,0x80800,0x80840,0x80880,0x808c0},sizes[]={512,512,256,64,64,64,64};
  for(unsigned p=0;p<7;p++)for(unsigned i=0;i<sizes[p]/4;i++)dirtied.insert(local[p]+i*4);
  for(unsigned at:dirtied)fabric.WriteLocal(worker,at,memory.at(at));
  fabric.SetControls(worker,command,counts);return pc;
 }
};
unsigned ExecuteGdnRecurrentNativeBytes(const char* image,const PioneerQ38CanonicalModelPlan& p,unsigned worker,unsigned bank,
 ProjectionExecutionFabric& fabric,DecoderHistoryBytes& state){
 (void)&TestWorker;NativeGdnRecurrentMachine machine(image,p,worker,bank,fabric,state);return machine.Execute();
}
