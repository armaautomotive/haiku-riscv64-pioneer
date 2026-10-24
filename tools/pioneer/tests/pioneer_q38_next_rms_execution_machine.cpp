/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_RMS_MACHINE_ONLY
#include "pioneer_q38_learned_rms_image_test.cpp"
#include "pioneer_q38_next_rms_execution_machine.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionGraphFootprint.h"
struct NextRmsExecutionMachine::Impl:LearnedRmsMachine {
 ProjectionExecutionFabric& fabric;bool badWeight,weightFaulted=false;
 Impl(const char* image,const PioneerQ38CanonicalModelPlan& plan,ProjectionExecutionFabric& f,bool bad)
  :LearnedRmsMachine(image,plan,1,0,f.ReadLocal(0,TT_Q38_FFN_RESIDENT_DESCRIPTOR+3*4)),fabric(f),badWeight(bad){
  memory.clear();
  for(unsigned i=0;i<PioneerQ38ProjectionGraphSnapshotWords(0);i++){
   unsigned a=PioneerQ38ProjectionGraphSnapshotAddress(0,i),v=f.ReadLocal(0,a);memory[a]=v;if(CodeAddress(a))l1[Index(a)]=v;
  }
  for(unsigned i=0;i<64;i++)descriptor[i]=memory.at(TT_Q38_FFN_RESIDENT_DESCRIPTOR+i*4);
  assert(descriptor[5]==1 && !descriptor[7] && PioneerQ38LearnedRmsShape(descriptor.data()));
  for(unsigned i=0;i<16;i++)source[i]=f.NoC(0,SourceOffset(i));
  const unsigned counter[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};
  for(unsigned i=0;i<8;i++)before[i]=counts[i]=f.NoC(0,counter[i]);
  for(unsigned i=0;i<10;i++)command[i]=f.NoC(0,i*4);
  for(unsigned i=0;i<5120;i++){
   input[i]=resident[i]=f.Gddr().at(i);backup[i]=f.Gddr().at(TT_Q38_RMS_BACKUP/4+i);weight[i]=0x7fc00001;
  }
 }
 uint32_t Load(uint32_t a)override{
  for(unsigned i=0;i<16;i++)if(a==0xffb20000+SourceOffset(i))return fabric.NoC(0,SourceOffset(i));
  return LearnedRmsMachine::Load(a);
 }
 void CopyPacket(bool write,bool learned,bool normalized,unsigned offset,unsigned local,unsigned bytes)override{
  if(learned){
   auto* source=fabric.LearnedBytes();std::vector<uint8_t> data(bytes);
   assert(!write && source && source->Ready() && source->Read(descriptor[12],descriptor[11]+offset,data.data(),bytes) && source->Ready());
   if(badWeight && !offset){data[0]=data[1]=0;data[2]=0xc0;data[3]=0x7f;weightFaulted=true;}
   for(unsigned i=0;i<bytes/4;i++){const auto* p=data.data()+i*4;
    weight[offset/4+i]=memory[local+i*4]=uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);
   }
   return;
  }
  auto& data=fabric.Gddr();unsigned remote=write && !normalized?unsigned(TT_Q38_RMS_BACKUP):0;
  for(unsigned i=0;i<bytes/4;i++){
   if(write)data.at((remote+offset)/4+i)=(normalized?resident:backup)[offset/4+i]=memory.at(local+i*4);
   else memory[local+i*4]=data.at((remote+offset)/4+i);
  }
 }
 void Advance(){
  if(pc==0x64038 || pc==0x64058)return;
  for(unsigned i=0;i<8192 && pc!=0x64038 && pc!=0x64058;i++)StepScalar();
  fabric.SetControls(0,command,counts);
  if(pc==0x64038 || pc==0x64058)for(unsigned i=0;i<PioneerQ38ProjectionGraphSnapshotWords(0);i++){
   unsigned a=PioneerQ38ProjectionGraphSnapshotAddress(0,i);
   if(!PioneerQ38ProjectionGraphProgramImmutable(0,a))fabric.WriteLocal(0,a,memory.at(a));
  }
 }
};
NextRmsExecutionMachine::NextRmsExecutionMachine(const char* image,const PioneerQ38CanonicalModelPlan& plan,ProjectionExecutionFabric& f,bool bad){
 (void)&RmsPlanTests;implementation=new Impl(image,plan,f,bad);
}
NextRmsExecutionMachine::~NextRmsExecutionMachine(){delete implementation;}
void NextRmsExecutionMachine::Advance(){implementation->Advance();}
unsigned NextRmsExecutionMachine::Pc()const{return implementation->pc;}
unsigned NextRmsExecutionMachine::Packets()const{return implementation->packets;}
bool NextRmsExecutionMachine::FaultInjected()const{return implementation->weightFaulted;}
void NextRmsExecutionMachine::Verify(){assert(implementation->pc==0x64038);implementation->LearnedRmsMachine::Verify();}
