/* SPDX-License-Identifier: MIT */
// Execute the linked probe's actual RV32 instructions under the alternative
// SrcA model. No driver, exclusive ownership or silicon claim.
#define PIONEER_Q38_HYBRID_MATRIX_TRANSITION_MACHINE_ONLY
#include "pioneer_q38_hybrid_matrix_attention_transition_test.cpp"

struct ProbeImage:TransitionImage {
 ProbeImage(const char* worker,const char* leaf,const char* probe):TransitionImage(worker,leaf){
  rom.clear();
  rom.push_back({0x6000,Read((std::string(probe)+"/boot.bin").c_str())});
  rom.push_back({0x70000,Read((std::string(probe)+"/text.bin").c_str())});
  pc=0x6000;regs[27]=0;denseMode=true;
  native.expectedWeight=0xa5000;native.alternateWeight=0xa6000;
  native.expectedInput=0xa7000;native.alternateInput=0xa7400;
  for(auto& operand:native.source)for(auto& bank:operand)for(auto& row:bank.values)for(auto& v:row)v=0;
 }
 uint32_t Load(uint32_t a)override{
  unsigned base=0;if(auto* part=Rom(a,base))return Word(*part,a-base);
  if(a<0x180000)return WordRead(a);
  return TransitionImage::Load(a);
 }
 void Store(uint32_t a,uint32_t value)override{
  assert(!CodeAddress(a));
  if(a<0x180000){WordWrite(a,value);return;}
  TransitionImage::Store(a,value);
 }
};
int main(int argc,char** argv){
 assert(argc==4);ProbeImage image(argv[1],argv[2],argv[3]);
 for(unsigned i=0;i<256;i+=2){
  const unsigned lo=PioneerAttentionToHalf(float(i+1)/256),hi=PioneerAttentionToHalf(float(i+2)/256);
  image.WordWrite(0xa0000+i*2,lo|(hi<<16));
 }
 image.WordWrite(0x82000,PioneerAttentionBits(1.f));
 while(image.WordRead(0x17ffc8)!=0x31504353){
  assert(image.WordRead(0x17ffc8)!=0x46504353);
  image.StepCombined();
 }
 assert(image.WordRead(0x17ffc4)==2);
 for(unsigned pass=0;pass<2;pass++)for(unsigned i=0;i<32;i++){
  const float expected=pass?float(i+1)/256:((i&15)<4?0.f:float(i-3)/256);
  const float actual=PioneerAttentionFloat(image.WordRead(0x92000+(pass*32+i)*4));
  if(actual!=expected)fprintf(stderr,"pass=%u lane=%u actual=%g expected=%g\n",pass,i,actual,expected);
  assert(actual==expected);
 }
 assert(image.native.Clean());
 puts("PASS linked RV32 two-base probe, alternative SrcA model; NOT SILICON");
}
