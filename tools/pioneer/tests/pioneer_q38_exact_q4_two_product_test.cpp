/* SPDX-License-Identifier: MIT */
// CPU VALIDATION ONLY. Independent original-q FP32 reference;no device.
#include "pioneer_q38_exact_q4_affine_machine.h"
#include "../PioneerQ38ExactQ4TwoProduct.h"
#ifdef PIONEER_Q38_TWO_PRODUCT_REPLAY_TEST
#include "../PioneerQ38ExactQ4TwoProductReplay.h"
#endif
struct TwoProductMachine:ExactAffineMachine {
 bool omitRestore=false,omitCast=false;
 TwoProductMachine(){twoProducts=true;
#ifdef PIONEER_Q38_TWO_PRODUCT_REPLAY_TEST
  allowRegisterReplay=true;
#endif
 }
 void Op(unsigned op){
  if(omitCast && op==0x90000660)return;
  if(omitRestore && op==(0x850a0000|(6<<8)))return;
  ExactAffineMachine::Op(op);
 }
};
static std::array<uint32_t,16> ExtremeFixture(TwoProductMachine& m,unsigned width,int input,unsigned q,uint32_t scale){
 using P=PioneerQ38ExactQ4Affine;assert(m.Clean() && input>=-128 && input<=127 && q<=15);
 const int z=int(std::min(q,14U))-7;const unsigned nibble=z<0?unsigned(-z)|8:unsigned(z);
 const int sum=input*16,dot=int(q)*sum;float expected=0;
 for(unsigned p=0;p<width/16;p++){
  const unsigned at=P::Source+p*352;for(unsigned i=0;i<352;i++)m.l1[at+i]=0;
  for(unsigned i=0;i<128;i++)m.l1[at+i]=uint8_t(nibble|(nibble<<4));
  for(unsigned i=0;i<32;i++)m.l1[at+128+i]=q==15?255:0;
  const unsigned bits=m.Bits(float(input));assert(!(bits&65535));
  for(unsigned k=0;k<16;k++){m.l1[at+160+k*2]=uint8_t(bits>>16);m.l1[at+161+k*2]=uint8_t(bits>>24);}
  for(unsigned n=0;n<16;n++){m.Write(at+192+n*4,0x3fa00000);m.Write(at+256+n*4,0xbf000000);}
  m.Write(at+320,scale);m.Write(at+324,unsigned(sum));
  const float product=(1.25f*m.Float(scale))*float(dot),correction=(-0.5f*m.Float(scale))*float(sum);
  expected=expected+(product+correction);
 }
 m.ObserveFence();std::array<uint32_t,16> result;result.fill(m.Bits(expected));return result;
}
int main(){
#ifdef PIONEER_Q38_TWO_PRODUCT_REPLAY_TEST
 using P=PioneerQ38ExactQ4TwoProductReplay;
 puts("TWO_PRODUCT_REPLAY13_LOCAL_ARITHMETIC_VALIDATION_ONLY_NO_DEVICE");
#else
 using P=PioneerQ38ExactQ4TwoProductAffine;
#endif
 unsigned jobs=0,products=0,refusals=0,extremes=0;
 for(unsigned width:{5120U,6144U,17408U})for(unsigned order=0;order<2;order++){
  TwoProductMachine m;m.delay[0]=order?1:100003;m.delay[1]=order?100003:1;
  for(unsigned pass=0;pass<2;pass++){
#ifdef PIONEER_Q38_TWO_PRODUCT_REPLAY_TEST
   m.replay.Poison(); // emulate arbitrary replay contents from an earlier job
#endif
   const auto expected=ExactAffineFixture(m,width,37+order*3+pass*139);
   assert(P::Run(m,width) && m.Results()==expected && m.Clean());jobs++;products+=width*16;
  }
  assert(!m.sevenWrites && m.matrixOps==4*width/16 && m.unpacks[0]==4*width/16
   && m.stageCalls==2*width/16 && m.metadataLoads==2*width/16 && m.zeroLoads==2 && m.affineStores==4*width/16);
#ifdef PIONEER_Q38_TWO_PRODUCT_REPLAY_TEST
  assert(m.replay.records==2 && m.replay.replays==4*width/16-2 && m.replay.expanded==4*width/16*13);
#endif
 }
 for(unsigned fault=0;fault<4;fault++){
  TwoProductMachine m;const auto expected=ExactAffineFixture(m,5120,37);bool detected=false;
  m.omitRestore=fault==0;m.omitCast=fault==1;m.dropProducer=fault==2?1:fault==3?2:0;
  try{if(P::Run(m,5120))detected=m.Results()!=expected;}catch(uint32_t marker){detected=marker==m.Unknown;}
  assert(detected);refusals++;
 }
 for(unsigned width:{5120U,6144U,17408U})for(int input:{-128,0,127})for(unsigned q:{0U,7U,14U,15U})for(unsigned scale:{0x3f800000U,0xbf800000U,0x80000000U}){
  TwoProductMachine m;const auto expected=ExtremeFixture(m,width,input,q,scale);
  assert(P::Run(m,width) && m.Results()==expected && m.Clean() && !m.sevenWrites);extremes++;
 }
 struct Inert {void Config(unsigned,unsigned){assert(false);}void Op(unsigned){assert(false);}void Drain(){assert(false);}
  void Escape(unsigned,unsigned){assert(false);}unsigned Mask(unsigned){assert(false);return 0;}
  void SealData(){assert(false);}void ZeroData(){assert(false);}bool StagePanel(unsigned,unsigned&,int&){assert(false);return false;}};
 for(unsigned width:{0U,16U,256U,5119U,5121U,17409U}){Inert m;assert(!P::Run(m,width));refusals++;}
 for(unsigned value:{0x2f7fffffU,0x47800001U,0x7f800000U,0x7fc00000U}){
  TwoProductMachine m;(void)ExactAffineFixture(m,5120,37);m.Write(P::Source+320,value);m.ObserveFence();
  assert(!P::Run(m,5120) && m.Clean() && !m.matrixOps && !m.affineStores && m.stageCalls==1);refusals++;
 }
 for(int sum:{-2049,2049}){
  TwoProductMachine m;(void)ExactAffineFixture(m,5120,37);m.Write(P::Source+324,unsigned(sum));m.ObserveFence();
  assert(!P::Run(m,5120) && m.Clean() && !m.matrixOps && !m.affineStores && m.stageCalls==1);refusals++;
 }
 printf("PASS_OFFBOARD_TWO_PRODUCT_Q4 jobs%u originalWeightProducts%u extremeFullKJobs%u negativeControls%u exact_original_perK16_FP32 "
  "two_not_three_FPU_products no_constant7_unpack_or_write;NO_NATIVE_DEVICE_MODEL_OR_TPS_GRANT\n",jobs,products,extremes,refusals);
}
