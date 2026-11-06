/* SPDX-License-Identifier: MIT */
#define PIONEER_GRAPH_MATH_MACHINE_ONLY
#include "pioneer_graph_math_image_test.cpp"
#define PIONEER_Q38_BF16_ACTIVATION_IMAGE 1
#define PIONEER_Q38_SIGMOID_PRECISION_IMAGE 1
#include "../PioneerQ38Bf16ActivationChunk.h"

struct Bf16ActivationMachine:ScalarMachine {
 std::array<uint32_t,32> frame={};std::array<float,256> gate={},up={};
 unsigned activatedWrites=0,packedWrites=0;
 Bf16ActivationMachine(const char* image,unsigned op,unsigned trial):ScalarMachine(image,256,0){
  memory.clear();regs[27]=0x13579bdf;
  frame[0]=0x31414251;frame[1]=1;frame[2]=256;frame[3]=op;frame[4]=512;frame[5]=17;SetFrame();
  unsigned random=0x51424131+trial;
  for(unsigned i=0;i<256;i++){
   if(!trial){gate[i]=PioneerAttentionFloat((i&1)<<31);up[i]=i&2?-1.f:1.f;}
   else if(trial==1){gate[i]=float(int(i)-128)*.125f;up[i]=float(int(i%17)-8)*.25f;}
   else if(trial==2){gate[i]=-110.f;up[i]=65504.f;}
   else if(trial==3){gate[i]=20.f;up[i]=(i&1?-1.f:1.f)*float(i%128)*.05f;}
   else if(trial==4){gate[i]=PioneerAttentionFloat(0x26800000);up[i]=2.f;}
   else if(trial==5){gate[i]=i==255?65504.f:0;up[i]=gate[i];}
   else if(trial==6){gate[i]=PioneerAttentionFloat(0x26800000);up[i]=gate[i];}
   else if(trial==7){gate[i]=PioneerAttentionFloat(0xc2d00000);up[i]=1.f;}
   else{
    random=random*1664525U+1013904223U;gate[i]=float(int(random%4097)-2048)*.00390625f;
    random=random*1664525U+1013904223U;up[i]=float(int(random%2049)-1024)*.00390625f;
   }
   memory[0x90800+i*4]=0xdeadbeef;if(i<128)memory[0x90c00+i*4]=0xdeadbeef;
  }
  SetInputs();for(unsigned a:{0x745fcU,0x74680U,0x8fffcU,0x90e00U,0x17ffbcU,0x17ffccU})memory[a]=0xdecafbad;
  memory[0x17ffc0]=memory[0x17ffc4]=memory[0x17ffc8]=0;
 }
 void SetFrame(){for(unsigned i=0;i<32;i++)memory[0x74600+i*4]=frame[i];}
 void SetInputs(){for(unsigned i=0;i<256;i++){memory[0x90000+i*4]=PioneerAttentionBits(gate[i]);memory[0x90400+i*4]=PioneerAttentionBits(up[i]);}}
 uint32_t Load(uint32_t a)override{
  assert(!(a&3));if(Range(a,0x64000,unsigned(code.size())))return l1[Index(a)];
  assert(Range(a,0x6e000,8192) || Range(a,0x74600,128) || Range(a,0x90000,2048) || Range(a,0x17ffc0,12));
  return memory.at(a); // No input payload override or NoC/device access.
 }
 void Store(uint32_t a,uint32_t value)override{
  assert(!(a&3) && (Range(a,0x6e000,8192) || Range(a,0x90800,1024) || Range(a,0x90c00,512) || Range(a,0x17ffc0,12)));
  if(Range(a,0x6e000,8192))minStack=std::min(minStack,unsigned(a));
  if(Range(a,0x90800,1024))activatedWrites++;if(Range(a,0x90c00,512))packedWrites++;memory[a]=value;
 }
 bool Run(){
  while(pc!=0x64038 && pc!=0x64058){
   StepScalar();assert(regs[27]==0x13579bdf);
   if(regs[2]){assert(regs[2]>=0x6e000 && regs[2]<=0x70000 && !(regs[2]&15));minStack=std::min(minStack,unsigned(regs[2]));}
  }
  for(unsigned a:{0x745fcU,0x74680U,0x8fffcU,0x90e00U,0x17ffbcU,0x17ffccU})assert(memory.at(a)==0xdecafbad);
  for(unsigned i=0;i<32;i++)assert(memory.at(0x74600+i*4)==frame[i]);
  for(unsigned i=0;i<256;i++)assert(memory.at(0x90000+i*4)==PioneerAttentionBits(gate[i]) && memory.at(0x90400+i*4)==PioneerAttentionBits(up[i]));
  const unsigned writes=activatedWrites+packedWrites;for(unsigned i=0;i<32;i++)StepScalar();
  assert(activatedWrites+packedWrites==writes && regs[27]==0x13579bdf);
  if(pc==0x64058){assert(memory.at(0x17ffc8)==0x46414251 && !memory.at(0x17ffc4));return false;}
  assert(memory.at(0x17ffc8)==0x31414251 && memory.at(0x17ffc0)==0x6400c);return memory.at(0x17ffc4)==256;
 }
 void NoSink()const{
  assert(!activatedWrites && !packedWrites);
  for(unsigned i=0;i<256;i++)assert(memory.at(0x90800+i*4)==0xdeadbeef);
  for(unsigned i=0;i<128;i++)assert(memory.at(0x90c00+i*4)==0xdeadbeef);
 }
};
#ifndef PIONEER_BF16_ACTIVATION_MACHINE_ONLY
int main(int argc,char** argv){
 assert(argc==2);unsigned accepted=0,refused=0,maxStack=0,outputs=0;
 for(unsigned op:{1U,2U})for(unsigned trial=0;trial<12;trial++){
  Bf16ActivationMachine m(argv[1],op,trial);std::array<uint32_t,256> bits={};std::array<uint32_t,128> packed={};bool okay=true;
  for(unsigned i=0;i<256;i++){
   float value=m.gate[i];
   if(op==2){float sigmoid;
    if(!PioneerQ38RmsDomain(value) || !PioneerQ38RmsDomain(m.up[i]) || !PioneerQ38SigmoidPrecision(value,sigmoid)){okay=false;break;}
    value=(value*sigmoid)*m.up[i];if(!PioneerQ38RmsDomain(value)){okay=false;break;}
    const double reference=(double(m.gate[i])/(1.+std::exp(-double(m.gate[i]))))*double(m.up[i]);
    assert(std::abs(double(value)-reference)<=3e-6*(1.+std::abs(reference)));
   }
   bits[i]=PioneerAttentionBits(value);
  }
  okay=okay && PioneerQ38Bf16Input::Pack(bits.data(),256,packed.data(),128);
  assert(m.Run()==okay);maxStack=std::max(maxStack,0x70000-m.minStack);
  if(!okay){m.NoSink();refused++;continue;}
  assert(m.packedWrites==128 && m.activatedWrites==(op==2?256U:0U));
  for(unsigned i=0;i<256;i++){
   const auto actual=m.memory.at(0x90c00+(i/2)*4);assert(uint16_t(actual>>((i&1)*16))==uint16_t(packed[i/2]>>((i&1)*16)));
   assert(m.memory.at(0x90800+i*4)==(op==2?bits[i]:0xdeadbeef));
   if(op==2 && trial==1 && i<128)printf("ACTUAL_BF16_PRODUCER bits%08x value%04x\n",bits[i],uint16_t(actual>>((i&1)*16)));
   outputs++;
  }
  accepted++;
 }
 for(unsigned field=0;field<32;field++){
  Bf16ActivationMachine m(argv[1],2,1);m.frame[field]^=0x80000000;m.SetFrame();assert(!m.Run());m.NoSink();refused++;
 }
 for(unsigned op:{1U,2U})for(unsigned which:{0U,1U})for(unsigned lane:{0U,255U})
  for(unsigned bad:{1U,0x80000001U,0x7f800000U,0xff800000U,0x7fc00001U}){
   if(op==1 && which)continue;
   Bf16ActivationMachine m(argv[1],op,1);(which?m.up:m.gate)[lane]=PioneerAttentionFloat(bad);m.SetInputs();
   assert(!m.Run());m.NoSink();refused++;
  }
 assert(accepted==22 && refused==64 && outputs==5632 && maxStack<=4096);
 printf("PASS ACTUAL BF16 activation RV32IMF accepted%u/refused%u/values%u/maxCombinedStack%u:projection and precise gate-sigmoid-up directly to BF16 at worker90000/90400/90800/90c00;atomic outputs,NO Q8 scale/quantizer/NoC/host inference;NOT full worker/ROOT/native/model-quality/hardware/TPS proof\n",accepted,refused,outputs,maxStack);
}
#endif
