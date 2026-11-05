/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_EXACT_Q4_AFFINE_IMAGE_ONLY
#include "pioneer_q38_exact_q4_affine_image_test.cpp"
// Independent fixed synthetic USER/test oracle. No producer DATA helper
// is used to construct the expected original integer/FP32 calculation.
static std::array<uint32_t,16> FixedAffineExpected(unsigned seed){
 std::array<float,16> accumulator{};
 for(unsigned p=0;p<17408/16;p++){
  int sum=0,dot[16]={};
  for(unsigned k=0;k<16;k++){
   const int x=int((seed*29+p*13+k*37)%256)-128;sum+=x;
   for(unsigned n=0;n<16;n++)dot[n]+=int((seed+p*11+k*17+n*3)%16)*x;
  }
  const unsigned scale=0x3b800000U|(((seed+p/16)*7919)&0x7fffff)|(((seed+p/16)&1)?0x80000000U:0);
  for(unsigned n=0;n<16;n++){
   float coefficient=std::ldexp(float((seed+(p/2)*3+n)%64),-10);if((seed+p/2+n)&1)coefficient=-coefficient;
   const float minimum=-std::ldexp(float((seed*7+p/2+n*3)%64),-12);
   const float product=(coefficient*Float(scale))*float(dot[n]);const float correction=(minimum*Float(scale))*float(sum);
   accumulator[n]=accumulator[n]+(product+correction);
  }
 }
 std::array<uint32_t,16> result;for(unsigned n=0;n<16;n++)result[n]=Bits(accumulator[n]);return result;
}
struct FixedAffineImage:ExactAffineImage{
 FixedAffineImage(const char* directory):ExactAffineImage(directory,17408){expected[0]=FixedAffineExpected(37);expected[1]=FixedAffineExpected(176);}
 void NextFixture()override{} // no host-inserted operands during execution
 unsigned ExpectedSourceReads()const override{return 0;}
};
#ifndef PIONEER_Q38_EXACT_Q4_AFFINE_FIXED_IMAGE_ONLY
int main(int argc,char** argv){
 assert(argc==2);FixedAffineImage image(argv[1]);image.native.delay[0]=100003;image.native.delay[1]=1;
 assert(image.Execute() && !image.sourceReads);
 printf("PASS_ACTUAL_RV32_FIXED_Q4_FULLK_AFFINE changedDualJobs_K17408 originalFP32Words32 padding32;"
  "allSyntheticDataGeneratedOnCard_noHostOperandInjection stack%u/8192;NO_NATIVE_MODEL_OR_TPS_CLAIM\n",0x70000-image.minSP);
}
#endif
