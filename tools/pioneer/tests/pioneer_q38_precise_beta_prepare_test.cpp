/* SPDX-License-Identifier: MIT */
// Execute the actual candidate RV32/F32 bytes. Host libm only observes.
#define PIONEER_Q38_GDN_PREPARE_MACHINE_ONLY
#include "pioneer_q38_gdn_prepare_image_test.cpp"
static uint32_t ExpectedBeta(uint32_t bits){
 double logit=PioneerAttentionFloat(bits),e=exp(-fabs(logit));
 uint32_t out=PioneerAttentionBits(float(logit>=0?1/(1+e):e/(1+e)));
 return out&0x7f800000?out:0;
}
static void VerifyCombined(PrepareMachine& machine,bool exactBeta){
 assert(machine.localWrites==322 && machine.descriptorWrites==5 && machine.memory.at(GL::Descriptor)==GL::Magic
  && machine.memory.at(GL::Descriptor+4)==128 && machine.memory.at(GL::Descriptor+8)==64 && !machine.memory.at(GL::Descriptor+12));
 float qs=0,ks=0;for(unsigned i=0;i<128;i++){
  float q=PioneerAttentionFloat(machine.input[i]),k=PioneerAttentionFloat(machine.input[128+i]);qs+=q*q;ks+=k*k;
 }
 const float qi=1.f/sqrtf(qs+1e-6f),ki=1.f/sqrtf(ks+1e-6f);
 for(unsigned i=0;i<128;i++)for(unsigned which=0;which<2;which++){
  const float expected=PioneerAttentionFloat(machine.input[which*128+i])*(which?ki:qi);
  assert(machine.memory.at((which?GL::Key:GL::Query)+i*4)==PioneerAttentionBits(expected));
 }
 for(unsigned i=0;i<64;i++)assert(machine.memory.at(GL::Value+i*4)==machine.input[256+i]);
 const float shifted=PioneerAttentionFloat(machine.input[320])+PioneerAttentionFloat(machine.input[323]);
 const float sp=float(SoftplusReference(shifted)),product=PioneerAttentionFloat(machine.input[322])*sp;
 const double decay=exp(double(product)),actual=PioneerAttentionFloat(machine.memory.at(GL::Decay));
 if(decay>=double(PioneerAttentionFloat(0x00800000)))assert(actual>0 && fabs(actual-decay)<=3e-6*decay);else assert(actual==0);
 unsigned wanted=ExpectedBeta(machine.input[321]),got=machine.memory.at(GL::Beta),ulp=got>wanted?got-wanted:wanted-got;
 assert(ulp<=(exactBeta?0U:1U));
}
int main(int argc,char** argv){
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);unsigned positives=0,critical=0,refused=0,stack=0;
 for(unsigned half=0;half<96;half++)for(unsigned trial=0;trial<5;trial++){
  PrepareMachine m(argv[1],half,trial);assert(m.Run());VerifyCombined(m,false);
  stack=std::max(stack,0x70000-m.minStack);positives++;
 }
 // Actual original layer29 beta logits that exposed the precision issue.
 // Their adjacent representable values also exercise rounding boundaries.
 const uint32_t logits[]={0x3f81fec2,0x3fc12b4a,0xc08cfd77,0xbf8e5cc0,0xbfdc3033,
  0xc063c49f,0xc09dca42,0xc09dde93,0x3f13ece0,0x404a01a7,0,0x80000000,0x477fe000,0xc77fe000};
 for(unsigned bits:logits)for(unsigned delta=0;delta<3;delta++){
  uint32_t input=bits;if((bits&0x7fffffff) && (bits&0x7fffffff)<0x477fe000)input+=int(delta)-1;
  PrepareMachine m(argv[1],critical%96);m.input[321]=input;m.memory[0x80504]=input;
  assert(m.Run());VerifyCombined(m,true);critical++;stack=std::max(stack,0x70000-m.minStack);
 }
 for(unsigned word=0;word<32;word++)for(unsigned bit=0;bit<32;bit++){
  if(word==3 && (1U<<bit)<96)continue; // valid half provenance, already covered by all96 positive heads
  PrepareMachine m(argv[1],0);m.header[word]^=1U<<bit;m.SetHeader();
  assert(!m.Run() && !m.localWrites && !m.descriptorWrites);refused++;
 }
 for(unsigned at:{0U,127U,128U,255U,256U,319U,320U,321U,322U,323U})
  for(unsigned bad:{1U,0x007fffffU,0x7f800000U,0x7fc00000U})for(unsigned sign:{0U,0x80000000U}){
   PrepareMachine m(argv[1],0);m.RefuseValue(at,bad|sign);refused++;
  }
 printf("PASS_ACTUAL_PRECISE_BETA_PREPARATION fullHeads%u criticalBetaCases%u bitExactQK1 criticalBetaBitExact1 refusals%u maxCombinedStack%u/8192;ACTUAL_RV32_FTZ_F32_LOCAL_INPUTS_ONLY_NO_NOC_NATIVE_PHYSICAL_WORK_OR_TPS\n",
  positives,critical,refused,stack);
}
