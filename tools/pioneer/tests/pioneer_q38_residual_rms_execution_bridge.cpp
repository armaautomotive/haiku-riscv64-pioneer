/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_RESIDUAL_RMS_MACHINE_ONLY
#include "pioneer_q38_residual_rms_image_test.cpp"
#include "pioneer_q38_residual_rms_execution_bridge.h"
struct ResidualNativeBytes:ResidualRmsMachine {
 ProjectionExecutionFabric& fabric;bool badWeight;
 ResidualNativeBytes(const char* image,const PioneerQ38CanonicalModelPlan& canonical,ProjectionExecutionFabric& f,bool bad)
  :ResidualRmsMachine(image,canonical,f.ReadLocal(0,TT_Q38_FFN_RESIDENT_DESCRIPTOR+41*4),f.ReadLocal(0,TT_Q38_FFN_RESIDENT_DESCRIPTOR+3*4)),fabric(f),badWeight(bad){
  for(unsigned i=0;i<PioneerQ38ProjectionGraphSnapshotWords(0);i++){
   unsigned a=PioneerQ38ProjectionGraphSnapshotAddress(0,i),v=fabric.ReadLocal(0,a);memory[a]=v;
   if(CodeAddress(a))l1[Index(a)]=v;
  }
  for(unsigned i=0;i<64;i++)descriptor[i]=memory.at(TT_Q38_FFN_RESIDENT_DESCRIPTOR+i*4);
  for(unsigned i=0;i<16;i++)source[i]=fabric.NoC(0,SourceOffset(i));
  const unsigned counter[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};
  for(unsigned i=0;i<8;i++)before[i]=counts[i]=fabric.NoC(0,counter[i]);
  for(unsigned i=0;i<10;i++)command[i]=fabric.NoC(0,i*4);
  auto& data=fabric.Gddr();for(unsigned i=0;i<5120;i++){
   raw[i]=backup[i]=data.at(TT_Q38_RMS_BACKUP/4+i);projected[i]=data.at(TT_Q38_GDN_OUTPUT_RESULT/4+i);initial[i]=resident[i]=data.at(i);
  }
 }
 void CopyPacket(bool write,bool learned,bool rawRead,bool normalized,unsigned offset,unsigned local,unsigned bytes)override{
  if(learned){
   auto* provider=fabric.LearnedBytes();std::vector<uint8_t> data(bytes);
   assert(provider && provider->Ready() && provider->Read(descriptor[12],descriptor[11]+offset,data.data(),bytes) && provider->Ready());
   if(badWeight && !offset){uint32_t nan=0x7fc00000;memcpy(data.data(),&nan,4);}
   for(unsigned i=0;i<bytes/4;i++){
    const auto* p=data.data()+i*4;weight[offset/4+i]=memory[local+i*4]=uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);
   }
   return;
  }
  auto& data=fabric.Gddr();unsigned remote=write?(normalized?0U:unsigned(TT_Q38_RMS_BACKUP)):rawRead?unsigned(TT_Q38_RMS_BACKUP):unsigned(TT_Q38_GDN_OUTPUT_RESULT);
  for(unsigned i=0;i<bytes/4;i++){
   if(!write)memory[local+i*4]=data.at((remote+offset)/4+i);
   else data.at((remote+offset)/4+i)=(normalized?resident:backup)[offset/4+i]=memory.at(local+i*4);
  }
 }
};
unsigned ExecuteResidualRmsNativeBytes(const char* image,const PioneerQ38CanonicalModelPlan& canonical,ProjectionExecutionFabric& fabric,bool badWeight){
 (void)&ResidualRmsPlanTests;ResidualNativeBytes m(image,canonical,fabric,badWeight);
 while(m.pc!=0x64038 && m.pc!=0x64058)m.StepScalar();
 if(m.pc==0x64038 && m.memory.at(0x17ffc4)==5120)m.Verify(); // independent TEST observer ONLY
 for(unsigned i=0;i<PioneerQ38ProjectionGraphSnapshotWords(0);i++){
  unsigned a=PioneerQ38ProjectionGraphSnapshotAddress(0,i);fabric.WriteLocal(0,a,m.CodeAddress(a)?m.l1[m.Index(a)]:m.memory.at(a));
 }
 fabric.SetControls(0,m.command,m.counts);return m.pc;
}
