/* SPDX-License-Identifier: MIT */
#define B_DEVICE_OP_CODES_END 1000
#define PIONEER_GRAPH_MATH_MACHINE_ONLY
#include "pioneer_graph_math_image_test.cpp"
#include "pioneer_qwen38_activation_reference.h"
#include "../PioneerQ38ProjectionInput.h"
#include "../PioneerQwen38CohortHost.h"
#include "pioneer_q38_projection_input_reference.h"
using ProjectionInput=PioneerQ38ProjectionInput;
struct ProjectionInputMachine:ScalarMachine {
 unsigned writes=0;
 ProjectionInputMachine(const char* image,unsigned type,const float* values):ScalarMachine(image,256,0){
  memory.clear();memory[0x74000]=0x31495051;memory[0x74004]=256;memory[0x74008]=type;memory[0x7400c]=576;
  for(unsigned i=0;i<256;i++)memory[0x90000+i*4]=PioneerAttentionBits(values[i]);
  for(unsigned i=0;i<144;i++)memory[0x90c00+i*4]=0xdeadbeef;
  for(unsigned a:{0x73ffcU,0x74010U,0x8fffcU,0x90400U,0x90bfcU,0x90e40U})memory[a]=0xdecafbad;
  memory[0x17ffc0]=memory[0x17ffc4]=memory[0x17ffc8]=0;
 }
 uint32_t Load(uint32_t a)override{
  assert(!(a&3));if(Range(a,0x64000,unsigned(code.size())))return l1[Index(a)];
  assert(Range(a,0x74000,16) || Range(a,0x6e000,8192) || Range(a,0x90000,1024)
   || Range(a,0x90c00,576) || Range(a,0x17ffc0,12));
  assert(memory.count(a));return memory.at(a);
 }
 void Store(uint32_t a,uint32_t v)override{
  assert(!(a&3));assert(Range(a,0x6e000,8192) || Range(a,0x90c00,576) || Range(a,0x17ffc0,12));
  if(Range(a,0x6e000,8192))minStack=std::min(minStack,unsigned(a));
  if(Range(a,0x90c00,576))writes++;
  memory[a]=v;
 }
 bool Run(){
  while(pc!=0x64038 && pc!=0x64058)StepScalar();
  assert(l1[Index(pc)]==0x0000006f && minStack>=0x6e000);
  for(unsigned a:{0x73ffcU,0x74010U,0x8fffcU,0x90400U,0x90bfcU,0x90e40U})assert(memory.at(a)==0xdecafbad);
  if(pc==0x64058){assert(memory.at(0x17ffc8)==0x46495051 && !memory.at(0x17ffc4));return false;}
  assert(memory.at(0x17ffc0)==0x6400c && memory.at(0x17ffc8)==0x31495051);return memory.at(0x17ffc4)==1;
 }
 void Collect(ProjectionInput& out){
  for(unsigned i=0;i<128;i++)out.operand[i]=memory.at(0x90c00+i*4);
  for(unsigned i=0;i<16;i++)out.scale[i]=memory.at(0x90e00+i*4);
 }
 void Refuse(){assert(!Run() && !writes);for(unsigned i=0;i<144;i++)assert(memory.at(0x90c00+i*4)==0xdeadbeef);}
};
static float Value(unsigned trial,unsigned lane){
 if(trial==0)return PioneerAttentionFloat((lane&1)<<31);
 if(trial==1)return float(int((lane*37+19)%255)-127)*.25f;
 if(trial==2)return float(int(lane%31)-15)+.5f;
 if(trial==3)return float(int(lane%255)-127)*0x1p-25f;
 if(trial==4)return (lane&1?-127.0f:127.0f);
 uint32_t h=(lane+1)*2654435761U;return PioneerAttentionFloat((h&0x807fffffU)|((112+(h>>24)%24)<<23));
}
int main(int argc,char** argv){
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);unsigned blocks=0,vectors=0,faults=0,ranges=0,maxSteps=0,minStack=0x70000;
 for(unsigned type:{8U,12U,14U})for(unsigned inputs:{5120U,6144U,17408U})for(unsigned trial=0;trial<6;trial++){
  std::vector<float> full(inputs);for(unsigned i=0;i<inputs;i++)full[i]=Value(trial,i);
  PioneerQ38CohortInput assembled;assembled.type=type;assembled.inputs=inputs;
  for(unsigned at=0;at<inputs;at+=256){
   ProjectionInputMachine m(argv[1],type,full.data()+at);auto before=m.memory;assert(m.Run() && m.writes==144);
   ProjectionInput actual,wanted,host;m.Collect(actual);ProjectionInputReference(type,full.data()+at,wanted);
   assert(PioneerQ38ProjectionQuantize(type,full.data()+at,256,&host)
    && !memcmp(&actual,&wanted,sizeof(actual)) && !memcmp(&actual,&host,sizeof(actual)));
   for(const auto& p:before)if(!ScalarMachine::Range(p.first,0x90c00,576) && !ScalarMachine::Range(p.first,0x17ffc0,12))assert(m.memory.at(p.first)==p.second);
   memcpy(assembled.activation+at/2,actual.operand,512);
   for(unsigned b=0;b<(type==8?8U:1U);b++)assembled.scales[at/(type==8?32:256)+b]=actual.scale[b];
   maxSteps=std::max(maxSteps,m.steps);minStack=std::min(minStack,m.minStack);blocks++;
  }
  PioneerQ38CohortInput expected;assert(PioneerQ38CohortQuantize(type,inputs,full.data(),expected)
   && !memcmp(&assembled,&expected,sizeof(assembled)));vectors++;
  printf("PASS projection input type%u full%u trial%u ACTUAL RV32 ALL native operands/scales/padding;NOT_NATIVE_TIME\n",type,inputs,trial);
 }
 float valid[256];for(unsigned i=0;i<256;i++)valid[i]=Value(1,i);
 for(unsigned type:{8U,12U,14U}){
  for(unsigned field=0;field<4;field++)for(unsigned bit=0;bit<32;bit++){
   ProjectionInputMachine m(argv[1],type,valid);m.memory[0x74000+field*4]^=1U<<bit;
   unsigned changed=m.memory[0x74008];if(field==2 && (changed==8 || changed==12 || changed==14))continue;
   m.Refuse();faults++;
  }
  for(unsigned lane:{0U,31U,32U,127U,224U,255U})for(uint32_t bad:{1U,0x80000001U,0x7f800000U,0xff800000U,0x7fc00001U}){
   ProjectionInputMachine m(argv[1],type,valid);m.memory[0x90000+lane*4]=bad;m.Refuse();faults++;
  }
  ProjectionInput out;memset(&out,0xa5,sizeof(out));const auto before=out;
  for(unsigned lane=0;lane<256;lane++)for(uint32_t bad:{1U,0x80000001U,0x7f800000U,0xff800000U,0x7fc00001U}){
   float saved=valid[lane];valid[lane]=PioneerAttentionFloat(bad);
   assert(!PioneerQ38ProjectionQuantize(type,valid,256,&out) && !memcmp(&out,&before,sizeof(out)));valid[lane]=saved;faults++;
  }
  for(unsigned count:{0U,1U,255U,257U,17408U,UINT32_MAX}){
   assert(!PioneerQ38ProjectionQuantize(type,valid,count,&out) && !memcmp(&out,&before,sizeof(out)));ranges++;
  }
  for(unsigned target=0;target<2;target++)for(uintptr_t bad:{uintptr_t(0),uintptr_t(valid)+1,~uintptr_t(3)}){
   assert(!PioneerQ38ProjectionQuantize(type,target==0?reinterpret_cast<float*>(bad):valid,256,
    target==1?reinterpret_cast<ProjectionInput*>(bad):&out) && !memcmp(&out,&before,sizeof(out)));ranges++;
  }
  for(unsigned offset:{0U,4U,1020U}){
   const auto snapshot=std::vector<float>(valid,valid+256);
   assert(!PioneerQ38ProjectionQuantize(type,valid,256,reinterpret_cast<ProjectionInput*>(reinterpret_cast<uintptr_t>(valid)+offset))
    && !memcmp(valid,snapshot.data(),sizeof(valid)));ranges++;
  }
 }
 for(unsigned type:{0U,1U,9U,UINT32_MAX}){ProjectionInputMachine m(argv[1],8,valid);m.memory[0x74008]=type;m.Refuse();faults++;}
 printf("PASS ACTUAL projection-input RV32/FPU:blocks%u/full-wire-vectors%u/faults%u/host-ranges%u/maxsteps%u/minstack%08x;Q8_0 FP16+ties-away vs K signedF32+ties-even,late-block atomic refusal,strict local memory/NO NoC or driver;NO_NATIVE_SOURCE_OR_SPEED_AUTHORITY\n",blocks,vectors,faults,ranges,maxSteps,minStack);
}
