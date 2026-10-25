/* SPDX-License-Identifier: MIT */
// Actual emitted QSP1,not a call to the host candidate arithmetic.
#define PIONEER_GRAPH_MATH_MACHINE_ONLY
#include "pioneer_graph_math_image_test.cpp"
#include <array>
static uint32_t SiluPrecisionReference(float value){
 double x=value,e=exp(-fabs(x));uint32_t result=PioneerAttentionBits(float(x*(x>=0?1/(1+e):e/(1+e))));
 return (result&0x7fffffff)<0x00800000?result&0x80000000:result;
}
struct SiluPrecisionMachine:ScalarMachine {
 std::array<uint32_t,32> header={};std::array<uint32_t,64> input={};unsigned outputWrites=0;
 SiluPrecisionMachine(const char* image,unsigned trial):ScalarMachine(image,64,0){
  memory.clear();header[0]=0x31505351;header[1]=64;header[2]=1;SetHeader();
  uint32_t random=0x6a09e667+trial;
  for(unsigned i=0;i<64;i++){
   float x=-128.f+256.f*float(trial*64+i)/8191.f;
   if(trial>=128){random^=random<<13;random^=random>>17;random^=random<<5;
    uint32_t magnitude=0x00800000+random%(0x7f7fffff-0x00800000+1);x=PioneerAttentionFloat(magnitude|(random&0x80000000));}
   if(trial>=192){
    uint32_t lo=0x42b40000,hi=0x42bc0000;
    while(lo<hi){uint32_t mid=lo+(hi-lo)/2;if(SiluPrecisionReference(-PioneerAttentionFloat(mid))&0x7fffffff)lo=mid+1;else hi=mid;}
    x=-PioneerAttentionFloat(lo-512+(trial-192)*64+i); // ALL1024adjacent values around exact FTZ boundary
   }
   if(trial==0 && i<32){
    const uint32_t edge[]={0,0x00800000,0x00fffffd,0x00fffffe,0x00ffffff,0x01000000,0x01000001,0x32800000,
     0x419fffff,0x41a00000,0x41a00001,0x42b7ffff,0x42b80000,0x42d00000,0x477fe000,0x7f7fffff};
    x=PioneerAttentionFloat(edge[i/2]|((i&1)?0x80000000:0));
   }
   input[i]=PioneerAttentionBits(x);memory[0x80000+i*4]=input[i];memory[0x81000+i*4]=0xdeadbeef;
  }
  for(unsigned a:{0x745fcU,0x74680U,0x7fffcU,0x80100U,0x80ffcU,0x81100U,0x17ffbcU,0x17ffccU})memory[a]=0xdecafbad;
  memory[0x17ffc0]=memory[0x17ffc4]=memory[0x17ffc8]=0;
 }
 void SetHeader(){for(unsigned i=0;i<32;i++)memory[0x74600+i*4]=header[i];}
 uint32_t Load(uint32_t a)override{
  assert(!(a&3));if(Range(a,0x64000,unsigned(code.size())))return l1[Index(a)];
  assert(Range(a,0x6e000,8192) || Range(a,0x74600,128) || Range(a,0x80000,256) || Range(a,0x81000,256) || Range(a,0x17ffc0,12));
  assert(memory.count(a));return memory.at(a);
 }
 void Store(uint32_t a,uint32_t v)override{
  assert(!(a&3));assert(Range(a,0x6e000,8192) || Range(a,0x81000,256) || Range(a,0x17ffc0,12));
  if(Range(a,0x6e000,8192))minStack=std::min(minStack,unsigned(a));if(Range(a,0x81000,256))outputWrites++;
  memory[a]=v; // no code/input/header/NoC/CSR/PCI/reset/resident-GDDR writes
 }
 bool Run(){
  while(pc!=0x64038 && pc!=0x64058){StepScalar();if(regs[2]){assert(regs[2]>=0x6e000 && regs[2]<=0x70000);minStack=std::min(minStack,unsigned(regs[2]));}}
  for(unsigned a:{0x745fcU,0x74680U,0x7fffcU,0x80100U,0x80ffcU,0x81100U,0x17ffbcU,0x17ffccU})assert(memory.at(a)==0xdecafbad);
  for(unsigned i=0;i<64;i++)assert(memory.at(0x80000+i*4)==input[i]);for(unsigned i=0;i<32;i++)assert(memory.at(0x74600+i*4)==header[i]);
  unsigned writes=outputWrites;for(unsigned i=0;i<32;i++)StepScalar();assert(outputWrites==writes);
  if(pc==0x64058){assert(memory.at(0x17ffc8)==0x46505351 && !memory.at(0x17ffc4));return false;}
  assert(memory.at(0x17ffc8)==0x31505351 && memory.at(0x17ffc0)==0x6400c);return memory.at(0x17ffc4)==64;
 }
 unsigned Verify(){
  assert(outputWrites==64);unsigned maximum=0;
  for(unsigned i=0;i<64;i++){
   uint32_t expected=SiluPrecisionReference(PioneerAttentionFloat(input[i]));
   uint32_t actual=memory.at(0x81000+i*4);assert((actual>>31)==(expected>>31));
   unsigned distance=actual>expected?actual-expected:expected-actual;
   if(distance>1)fprintf(stderr,"ACTUAL_QSP1_FAIL input%08x expected%08x actual%08x ULP%u\n",input[i],expected,actual,distance);
   assert(distance<=1);maximum=std::max(maximum,distance);
  }return maximum;
 }
 void NoSink()const{assert(!outputWrites);for(unsigned i=0;i<64;i++)assert(memory.at(0x81000+i*4)==0xdeadbeef);}
};
#ifndef PIONEER_Q38_SILU_PRECISION_MACHINE_ONLY
int main(int argc,char** argv){
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);unsigned positives=0,refused=0,maxUlp=0,maxStack=0,maxSteps=0;
 for(unsigned trial=0;trial<208;trial++){
  SiluPrecisionMachine machine(argv[1],trial);assert(machine.Run());maxUlp=std::max(maxUlp,machine.Verify());positives++;
  maxStack=std::max(maxStack,0x70000-machine.minStack);maxSteps=std::max(maxSteps,machine.steps);
 }
 for(unsigned word=0;word<32;word++)for(unsigned bit=0;bit<32;bit++){
  SiluPrecisionMachine machine(argv[1],0);machine.header[word]^=1U<<bit;machine.SetHeader();assert(!machine.Run());machine.NoSink();refused++;
 }
 for(unsigned at:{0U,63U})for(uint32_t bad:{1U,0x007fffffU,0x7f800000U,0x7fc00000U})for(uint32_t sign:{0U,0x80000000U}){
  SiluPrecisionMachine machine(argv[1],0);machine.input[at]=bad|sign;machine.memory[0x80000+at*4]=bad|sign;
  assert(!machine.Run());machine.NoSink();refused++;
 }
 printf("PASS ACTUAL QSP1 RV32IMF positives%u/values%u/refusals%u/maxULP%u/maxStack%u/maxSteps%u/input-header immutable/ALL arithmetic before output/terminal inert/no MMIO;NO native or physical fix or TPS claim\n",positives,positives*64,refused,maxUlp,maxStack,maxSteps);
}
#endif
