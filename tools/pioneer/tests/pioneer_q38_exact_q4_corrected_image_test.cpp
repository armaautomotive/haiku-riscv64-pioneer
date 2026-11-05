/* SPDX-License-Identifier: MIT */
#include "pioneer_q38_exact_q4_corrected_machine.h"
#define PIONEER_NATIVE_BFP_IMAGE_ONLY
#include "pioneer_q38_native_bfp_image_test.cpp"
struct CorrectedImage:NativeBfpRV32Image<ExactCorrectedMachine,16,2> {
 using Base=NativeBfpRV32Image<ExactCorrectedMachine,16,2>;
 unsigned observedMaskLoads=0,scratchStores=0;bool change=false;unsigned nextSeed=0;
 CorrectedImage(const char* path):Base(path){}
 void BeforeStep()override {
  assert(CodeAddress(pc));const unsigned op=l1[Index(pc)];
  if((op&127)==15)native.ObserveFence(); // same executed RV fence,NOT an ACK
 }
 void Store(unsigned a,unsigned v)override {
  if(Range(a,0x90100,128)){native.Seven((a-0x90100)/4,v);return;}
  if(Range(a,0x90200,64)){native.Escape((a-0x90200)/4,v);scratchStores++;return;}
  Base::Store(a,v);
  if(change && resultStores==16 && a==0x9203c){assert(native.Clean());Stage(nextSeed);change=false;}
 }
 unsigned Load(unsigned a)override {
  if(Range(a,0x90300,32)){assert(!(a&3));observedMaskLoads++;return native.Mask((a-0x90300)/4);}
  return Base::Load(a);
 }
 static std::array<float,16> Expected(unsigned seed) {
  std::array<float,16> expected{};
  for(unsigned k=0;k<16;k++)for(unsigned n=0;n<16;n++)expected[n]+=float(int((seed+k*17+n*3)&15)*(int((seed*7+k*13)&255)-128));
  return expected;
 }
 void Stage(unsigned seed) {
  std::array<uint8_t,128> primary{};std::array<uint8_t,32> mask{};std::array<int,16> input{};
  for(unsigned k=0;k<16;k++) {
   input[k]=int((seed*7+k*13)&255)-128;
   for(unsigned n=0;n<16;n++) {
    const unsigned q=(seed+k*17+n*3)&15,i=k*16+n;const int z=int(q==15?14:q)-7;
    primary[i/2]|=uint8_t((z<0?unsigned(-z)|8:unsigned(z))<<((i&1)*4));
    mask[i/8]|=uint8_t(unsigned(q==15)<<(i&7));
   }
  }
  native.StageCorrected(primary.data(),mask.data(),input);
 }
 void Check(unsigned first,unsigned second) {
  for(unsigned pass=0;pass<2;pass++) {const auto expected=Expected(pass?second:first);
   for(unsigned n=0;n<16;n++)assert(l1[Index(0x92000+pass*64+n*4)]==Bits(expected[n]));}
 }
};
#ifndef PIONEER_Q38_EXACT_Q4_CORRECTED_IMAGE_ONLY
int main(int argc,char** argv) {
 assert(argc==2);unsigned cases=0,maxStack=0,maxSteps=0;
 for(unsigned seed=0;seed<256;seed++) {
  CorrectedImage image(argv[1]);image.Stage(seed);image.change=true;image.nextSeed=seed^0xad;
  image.native.delay[0]=seed&1?100003:1;image.native.delay[1]=seed&1?1:100003;
  assert(image.Execute());image.Check(seed,seed^0xad);
  assert(image.native.matrixOps==6 && image.native.visits[0]==6 && !image.native.visits[1] && !image.native.visits[2] && !image.native.visits[3]
   && image.observedMaskLoads==16 && image.scratchStores==32 && image.native.sevenWrites==32);
  maxStack=std::max(maxStack,0x70000-image.minSP);maxSteps=std::max(maxSteps,image.steps);cases++;
 }
 for(unsigned fault=0;fault<3;fault++) {
  CorrectedImage image(argv[1]);image.Stage(15);
  if(fault==0)image.dropConfig=50;
  if(fault==1)image.dropAck=true;
  if(fault==2)image.dropAckAt=9; //5 setup +3 unpack completions +FINAL matrix completion
  assert(!image.Execute() && !image.native.outputReads && !image.resultStores);
  if(fault==2)assert(image.native.matrixOps==3 && image.native.mathPending);
  else assert(!image.native.matrixOps);cases++;
 }
 for(unsigned missing:{1U,2U}) {
  CorrectedImage image(argv[1]);image.Stage(7);image.native.dropProducer=missing;bool refused=false;
  try{image.Execute();}catch(uint32_t marker){refused=marker==image.native.Unknown;}
  assert(refused && !image.native.matrixOps && !image.resultStores && !image.native.outputReads);cases++;
 }
 printf("PASS_ACTUAL_RV32_CORRECTED_Q4 cases%u changedSecondInputAndMaskJobs256 fullOriginalIntegerDot;"
  "scalarReadsONLYMask32B_noPrimaryOrActivationReads;stack%u/8192 maxRVSteps%u;NOT_FULLK_AFFINE_NATIVE_HARDWARE_OR_TPS\n",cases,maxStack,maxSteps);
}
#endif
