/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_EXACT_Q4_AFFINE_IMAGE_ONLY
#include "pioneer_q38_exact_q4_affine_image_test.cpp"
// CPU VALIDATION ONLY: unchanged RV32 instruction decoder consumes the
// actual compiled candidate;ISA observers consume its actual staged bytes.
int main(int argc,char** argv){
 assert(argc==4);unsigned cases=0,maxStack=0,maxSteps=0;
 for(unsigned i=0;i<3;i++)for(unsigned schedule=0;schedule<2;schedule++){
  const unsigned width=i==0?5120:i==1?6144:17408;ExactAffineImage image(argv[i+1],width,true);
  image.native.delay[0]=schedule?1:100003;image.native.delay[1]=schedule?100003:1;
  assert(image.Execute());maxStack=std::max(maxStack,0x70000-image.minSP);maxSteps=std::max(maxSteps,image.steps);cases++;
 }
 for(unsigned fault=0;fault<3;fault++){
  ExactAffineImage image(argv[1],5120,true);if(fault==0)image.dropConfig=50;if(fault==1)image.dropAck=true;if(fault==2)image.dropMatrixDrain=true;
  assert(!image.Execute() && !image.native.outputReads && !image.results);
  if(fault==2)assert(image.native.matrixOps==2 && image.native.mathPending && !image.native.affineStores);
  else assert(!image.native.matrixOps);cases++;
 }
 for(unsigned producer:{1U,2U}){
  ExactAffineImage image(argv[1],5120,true);image.native.dropProducer=producer;bool refused=false;
  try{image.Execute();}catch(uint32_t marker){refused=marker==image.native.Unknown;}
  assert(refused && !image.native.outputReads && !image.results);cases++;
 }
 {
  ExactAffineImage image(argv[1],5120,true);image.native.Write(PioneerQ38ExactQ4Affine::Source+3*352+320,0x7f800000);image.native.ObserveFence();
  assert(!image.Execute() && image.native.matrixOps==6 && image.native.affineStores==6 && !image.results);cases++;
 }
 printf("PASS_ACTUAL_RV32_TWO_PRODUCT_Q4_AFFINE cases%u widths5120_6144_17408 dualJobs_changedSource;"
  "exactOriginalFP32Outputs untouchedPadding no_constant7_write_or_unpack two_matrix_products;"
  "stack%u/8192 maxRVSteps%u;NOT_NATIVE_HARDWARE_MODEL_OR_TPS\n",cases,maxStack,maxSteps);
}
