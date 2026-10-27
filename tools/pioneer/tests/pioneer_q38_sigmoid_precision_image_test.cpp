/* SPDX-License-Identifier: MIT */
// ACTUAL QSG1 bytes, not a call to the host candidate implementation.
#define PIONEER_GRAPH_MATH_MACHINE_ONLY
#include "pioneer_graph_math_image_test.cpp"
#include <array>
static uint32_t SigmoidReference(float value){
 double e=exp(-fabs(double(value)));unsigned result=PioneerAttentionBits(float(value>=0?1/(1+e):e/(1+e)));
 return result&0x7f800000?result:0;
}
struct SigmoidPrecisionMachine:ScalarMachine {
 std::array<uint32_t,32> header={};std::array<uint32_t,64> input={};unsigned outputWrites=0;
 SigmoidPrecisionMachine(const char* image,unsigned trial):ScalarMachine(image,64,0){
  memory.clear();header[0]=0x31475351;header[1]=64;header[2]=1;SetHeader();uint32_t random=0x51534731+trial;
  for(unsigned i=0;i<64;i++){
   float value=-104.f+124.f*float(trial*64+i)/4095.f;
   if(trial>=64){random=random*1664525+1013904223;value=PioneerAttentionFloat((0x00800000+random%(0x7f800000-0x00800000))|(random&0x80000000));}
   if(!trial && i<32){
    const uint32_t edges[]={0,0x00800000,0x32800000,0x32800001,0x41851592,0x419fffff,0x41a00000,0x41a00001,
     0x42a00000,0x42aeac4f,0x42af0000,0x42b00000,0x42d00000,0x477fe000,0x7f000000,0x7f7fffff};
    value=PioneerAttentionFloat(edges[i/2]|(i%2?0x80000000:0));
   }
   input[i]=PioneerAttentionBits(value);memory[0x80000+i*4]=input[i];memory[0x81000+i*4]=0xdeadbeef;
  }
  for(unsigned a:{0x745fcU,0x74680U,0x7fffcU,0x80100U,0x80ffcU,0x81100U,0x17ffbcU,0x17ffccU})memory[a]=0xdecafbad;
  memory[0x17ffc0]=memory[0x17ffc4]=memory[0x17ffc8]=0;
 }
 void SetHeader(){for(unsigned i=0;i<32;i++)memory[0x74600+i*4]=header[i];}
 uint32_t Load(uint32_t a)override{
  assert(!(a&3));if(Range(a,0x64000,unsigned(code.size())))return l1[Index(a)];
  assert(Range(a,0x6e000,8192) || Range(a,0x74600,128) || Range(a,0x80000,256) || Range(a,0x81000,256) || Range(a,0x17ffc0,12));
  return memory.at(a);
 }
 void Store(uint32_t a,uint32_t v)override{
  assert(!(a&3) && (Range(a,0x6e000,8192) || Range(a,0x81000,256) || Range(a,0x17ffc0,12)));
  if(Range(a,0x6e000,8192))minStack=std::min(minStack,unsigned(a));if(Range(a,0x81000,256))outputWrites++;
  memory[a]=v;
 }
 bool Run(){
  while(pc!=0x64038 && pc!=0x64058){StepScalar();if(regs[2]){assert(regs[2]>=0x6e000 && regs[2]<=0x70000);minStack=std::min(minStack,unsigned(regs[2]));}}
  for(unsigned a:{0x745fcU,0x74680U,0x7fffcU,0x80100U,0x80ffcU,0x81100U,0x17ffbcU,0x17ffccU})assert(memory.at(a)==0xdecafbad);
  for(unsigned i=0;i<64;i++)assert(memory.at(0x80000+i*4)==input[i]);for(unsigned i=0;i<32;i++)assert(memory.at(0x74600+i*4)==header[i]);
  unsigned writes=outputWrites;for(unsigned i=0;i<32;i++)StepScalar();assert(outputWrites==writes);
  if(pc==0x64058){assert(memory.at(0x17ffc8)==0x46475351 && !memory.at(0x17ffc4));return false;}
  assert(memory.at(0x17ffc8)==0x31475351 && memory.at(0x17ffc0)==0x6400c);return memory.at(0x17ffc4)==64;
 }
 void NoSink()const{assert(!outputWrites);for(unsigned i=0;i<64;i++)assert(memory.at(0x81000+i*4)==0xdeadbeef);}
};
#ifndef PIONEER_Q38_SIGMOID_PRECISION_MACHINE_ONLY
int main(int argc,char** argv){
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);unsigned maxUlp=0,maxStack=0,maxSteps=0,refused=0;
 for(unsigned trial=0;trial<96;trial++){
  SigmoidPrecisionMachine machine(argv[1],trial);assert(machine.Run() && machine.outputWrites==64);
  for(unsigned i=0;i<64;i++){
   unsigned actual=machine.memory.at(0x81000+i*4),expected=SigmoidReference(PioneerAttentionFloat(machine.input[i]));
   unsigned distance=actual>expected?actual-expected:expected-actual;
   if(distance>1)fprintf(stderr,"QSG1_FAIL input%08x actual%08x expected%08x ulp%u\n",machine.input[i],actual,expected,distance);
   assert(distance<=1);maxUlp=std::max(maxUlp,distance);
  }
  maxStack=std::max(maxStack,0x70000-machine.minStack);maxSteps=std::max(maxSteps,machine.steps);
 }
 for(unsigned word=0;word<32;word++)for(unsigned bit=0;bit<32;bit++){
  SigmoidPrecisionMachine machine(argv[1],0);machine.header[word]^=1U<<bit;machine.SetHeader();assert(!machine.Run());machine.NoSink();refused++;
 }
 for(unsigned at:{0U,63U})for(unsigned bad:{1U,0x007fffffU,0x7f800000U,0x7fc00000U})for(unsigned sign:{0U,0x80000000U}){
  SigmoidPrecisionMachine machine(argv[1],0);machine.input[at]=bad|sign;machine.memory[0x80000+at*4]=bad|sign;
  assert(!machine.Run());machine.NoSink();refused++;
 }
 printf("PASS ACTUAL isolated QSG1 RV32IMF batches96/values6144/refusals%u/maxUlp%u/maxStack%u/maxSteps%u/input+header+guards immutable/atomic output+inert terminal/NO MMIO,native,Work,physical timing or model grant\n",refused,maxUlp,maxStack,maxSteps);
}
#endif
