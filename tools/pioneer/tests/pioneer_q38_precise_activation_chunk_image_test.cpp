/* SPDX-License-Identifier: MIT */
#define PIONEER_GRAPH_MATH_MACHINE_ONLY
#include "pioneer_graph_math_image_test.cpp"
#define PIONEER_Q38_PRECISE_ACTIVATION_CHUNK_IMAGE 1
#define PIONEER_Q38_SIGMOID_PRECISION_IMAGE 1
#include "pioneer_q38_precise_activation_reference.h"
struct PreciseChunkMachine:ScalarMachine {
 std::array<uint32_t,32> header={};std::array<float,256> gate={},up={};
 unsigned activatedWrites=0,packedWrites=0;
 PreciseChunkMachine(const char* image,unsigned type,unsigned trial):ScalarMachine(image,256,0){
  memory.clear();header[0]=0x31434151;header[1]=256;header[2]=1;header[3]=type;header[4]=576;SetHeader();
  PreciseChunkInputs(trial,gate.data(),up.data());SetInputs();
  for(unsigned i=0;i<256;i++)memory[0x81000+i*4]=0xdeadbeef;
  for(unsigned i=0;i<144;i++)memory[0x81800+i*4]=0xdeadbeef;
  for(unsigned a:{0x745fcU,0x74680U,0x7fffcU,0x80800U,0x80ffcU,0x81400U,0x817fcU,0x81a40U,0x17ffbcU,0x17ffccU})memory[a]=0xdecafbad;
  memory[0x17ffc0]=memory[0x17ffc4]=memory[0x17ffc8]=0;
 }
 void SetHeader(){for(unsigned i=0;i<32;i++)memory[0x74600+i*4]=header[i];}
 void SetInputs(){for(unsigned i=0;i<256;i++){memory[0x80000+i*4]=PioneerAttentionBits(gate[i]);memory[0x80400+i*4]=PioneerAttentionBits(up[i]);}}
 uint32_t Load(uint32_t a)override{
  assert(!(a&3));if(Range(a,0x64000,unsigned(code.size())))return l1[Index(a)];
  assert(Range(a,0x6e000,8192) || Range(a,0x74600,128) || Range(a,0x80000,2048)
   || Range(a,0x81000,1024) || Range(a,0x81800,576) || Range(a,0x17ffc0,12));
  return memory.at(a);
 }
 void Store(uint32_t a,uint32_t value)override{
  assert(!(a&3) && (Range(a,0x6e000,8192) || Range(a,0x81000,1024) || Range(a,0x81800,576) || Range(a,0x17ffc0,12)));
  if(Range(a,0x6e000,8192))minStack=std::min(minStack,unsigned(a));
  if(Range(a,0x81000,1024))activatedWrites++;if(Range(a,0x81800,576))packedWrites++;memory[a]=value;
 }
 bool Run(){
  while(pc!=0x64038 && pc!=0x64058){StepScalar();if(regs[2]){assert(regs[2]>=0x6e000 && regs[2]<=0x70000);minStack=std::min(minStack,unsigned(regs[2]));}}
  for(unsigned a:{0x745fcU,0x74680U,0x7fffcU,0x80800U,0x80ffcU,0x81400U,0x817fcU,0x81a40U,0x17ffbcU,0x17ffccU})assert(memory.at(a)==0xdecafbad);
  for(unsigned i=0;i<32;i++)assert(memory.at(0x74600+i*4)==header[i]);
  for(unsigned i=0;i<256;i++)assert(memory.at(0x80000+i*4)==PioneerAttentionBits(gate[i]) && memory.at(0x80400+i*4)==PioneerAttentionBits(up[i]));
  unsigned writes=activatedWrites+packedWrites;for(unsigned i=0;i<32;i++)StepScalar();assert(activatedWrites+packedWrites==writes);
  if(pc==0x64058){assert(memory.at(0x17ffc8)==0x46434151 && !memory.at(0x17ffc4));return false;}
  assert(memory.at(0x17ffc8)==0x31434151 && memory.at(0x17ffc0)==0x6400c);return memory.at(0x17ffc4)==256;
 }
 void NoSink()const{
  assert(!activatedWrites && !packedWrites);
  for(unsigned i=0;i<256;i++)assert(memory.at(0x81000+i*4)==0xdeadbeef);
  for(unsigned i=0;i<144;i++)assert(memory.at(0x81800+i*4)==0xdeadbeef);
 }
};
int main(int argc,char** argv){
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);unsigned accepted=0,refused=0,maxStack=0,maxSteps=0;
 for(unsigned type:{8U,12U,14U})for(unsigned trial=0;trial<12;trial++){
  PreciseChunkMachine machine(argv[1],type,trial);float expected[256];PioneerQ38ProjectionInput packed,independent;
  bool okay=CurrentRootActivation(machine.gate.data(),machine.up.data(),expected) && PioneerQ38ProjectionQuantize(type,expected,256,&packed);
  assert(machine.Run()==okay);
  if(okay){
   ProjectionInputReference(type,expected,independent);assert(!memcmp(&packed,&independent,sizeof(packed)));
   assert(machine.activatedWrites==256 && machine.packedWrites==144);
   for(unsigned i=0;i<256;i++)assert(machine.memory.at(0x81000+i*4)==PioneerAttentionBits(expected[i]));
   for(unsigned i=0;i<128;i++)assert(machine.memory.at(0x81800+i*4)==packed.operand[i]);
   for(unsigned i=0;i<16;i++)assert(machine.memory.at(0x81a00+i*4)==packed.scale[i]);accepted++;
  }else{machine.NoSink();refused++;}
  maxStack=std::max(maxStack,0x70000-machine.minStack);maxSteps=std::max(maxSteps,machine.steps);
 }
 for(unsigned word=0;word<32;word++)for(unsigned bit=0;bit<32;bit++){
  PreciseChunkMachine machine(argv[1],12,0);machine.header[word]^=1U<<bit;
  if(word==3 && (machine.header[word]==8 || machine.header[word]==12 || machine.header[word]==14))continue;
  machine.SetHeader();assert(!machine.Run());machine.NoSink();refused++;
 }
 for(unsigned which:{0U,1U})for(unsigned at:{0U,255U})for(unsigned bad:{1U,0x80000001U,0x007fffffU,
  0x7f800000U,0xff800000U,0x7fc00001U,0x267fffffU,0x47800000U}){
  PreciseChunkMachine machine(argv[1],12,0);(which?machine.up:machine.gate)[at]=PioneerAttentionFloat(bad);
  machine.SetInputs();assert(!machine.Run());machine.NoSink();refused++;
 }
 assert(maxStack<=4096);
 printf("PASS ACTUAL isolated QAC1 RV32IMF/accepted%u/refused%u/maxCombinedStack%u/maxSteps%u/current ROOT-order activation bits+independent packing/immutable input+header+guards/atomic outputs+inert terminal;NO MMIO,distributed worker,native,Work,physical timing,model or TPS grant\n",accepted,refused,maxStack,maxSteps);
}
