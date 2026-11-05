/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_EXACT_Q4_MACHINE_ONLY
#include "pioneer_q38_exact_q4_primary_test.cpp"
#define PIONEER_NATIVE_BFP_IMAGE_ONLY
#include "pioneer_q38_native_bfp_image_test.cpp"
int main(int argc,char** argv) {
 assert(argc==2);using Image=NativeBfpRV32Image<ExactPrimaryMachine,16>;unsigned cases=0,maxStack=0;
 for(unsigned seed=0;seed<256;seed++) {
  Image image(argv[1]);std::array<uint8_t,128> primary{};std::array<int,16> input{};std::array<float,16> expected{};
  for(unsigned k=0;k<16;k++) {
   input[k]=int((seed*7+k*13)&255)-128;
   for(unsigned n=0;n<16;n++) {
    const int z=int((seed+k*17+n*3)%15)-7;const unsigned sm=z<0?unsigned(-z)|8:unsigned(z),i=k*16+n;
    primary[i/2]|=uint8_t(sm<<((i&1)*4));expected[n]+=float(z*input[k]);
   }
  }
  image.native.StagePrimary(primary.data(),input);
  image.native.delay[0]=seed&1?100003:1;image.native.delay[1]=seed&1?1:100003;
  assert(image.Execute());image.Exact(expected);
  assert(image.native.matrixOps==2 && image.native.visits[0]==2 && !image.native.visits[1]
   && !image.native.visits[2] && !image.native.visits[3]);
  maxStack=std::max(maxStack,0x70000-image.minSP);cases++;
 }
 for(unsigned fault=0;fault<2;fault++) {
  Image image(argv[1]);if(!fault)image.dropConfig=50;else image.dropAck=true;
  assert(!image.Execute() && !image.native.matrixOps && !image.native.outputReads && !image.resultStores);cases++;
 }
 for(unsigned missing:{1U,2U}) {
  Image image(argv[1]);std::array<uint8_t,128> primary{};std::array<int,16> input{};
  image.native.StagePrimary(primary.data(),input);image.native.dropProducer=missing;bool refused=false;
  try {image.Execute();}catch(uint32_t marker){refused=marker==image.native.Unknown;}
  assert(refused && !image.native.matrixOps && !image.native.outputReads && !image.resultStores);cases++;
 }
 // Setup has4 real completions;first Run publishes its two sources at5
 // and waits for matrix completion at6. Losing THAT completion is inert.
 {
  Image image(argv[1]);std::array<uint8_t,128> primary{};std::array<int,16> input{};
  image.native.StagePrimary(primary.data(),input);image.dropAckAt=6;
  assert(!image.Execute() && image.native.matrixOps==1 && image.native.mathPending
   && !image.native.outputReads && !image.resultStores);cases++;
 }
 printf("PASS_ACTUAL_RV32_EXACT_Q4_PRIMARY_IMAGE cases%u twoJobsPerFixture stack%u/8192;"
  "UNCHANGED_RV_DECODER_FORCED_EXPONENT_AND_BANK_FIDELITY_OBSERVER;NOT_CORRECTION_FULL_MODEL_NATIVE_OWNERSHIP_OR_HARDWARE_SPEED\n",cases,maxStack);
}
