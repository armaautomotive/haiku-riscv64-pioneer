/* SPDX-License-Identifier: MIT */
#define PIONEER_NATIVE_BFP4_BAND_IMAGE_ONLY
#include "pioneer_q38_native_bfp4_band_image_test.cpp"
#include "pioneer_blackhole_arithmetic_observer.h"
#include "../PioneerNativeBfp4BandGolden.h"

// Separate rounded arithmetic wrapper. The ordinary opcode/bank observer is
// unchanged and must accept every instruction before a value is published.
struct ArithmeticBand4Image:Band4RV32 {
 using Band4RV32::Band4RV32;
 void Instruction(uint32_t op)override{
  if((op>>24)==0x26 && ((native.fidelity+native.thread[11])&3)==native.dropPhase){
   const unsigned row=op&1023;const bool hadValue=native.valid[row];
   Band4RV32::Instruction(op);
   // The injected missing first term leaves an empty accumulator, not NaN.
   if(!hadValue)for(unsigned c=0;c<16;c++)native.destination[row][c]=0;
  }
  else if((op>>24)==0x26)PioneerBlackholeArithmeticObserver::Matrix(native,op,true,[&]{Band4RV32::Instruction(op);});
  else Band4RV32::Instruction(op);
 }
};
int main(int argc,char** argv){
 assert(argc==4);unsigned width=unsigned(std::stoul(argv[2]));
 assert(!strcmp(argv[3],"mmio") || !strcmp(argv[3],"direct"));bool direct=!strcmp(argv[3],"direct");
 const uint32_t* golden=PioneerNativeBfp4BandGolden(width);assert(golden && !PioneerNativeBfp4BandGolden(0));
 uint32_t expected[32];for(unsigned row=0;row<32;row++){assert(scanf("%x",&expected[row])==1);assert(expected[row]==golden[row]);}
 Fixture f(width);ArithmeticBand4Image image(argv[1],width,direct);f.Stage(image.native);
 image.native.delay[0]=7;image.native.delay[1]=100003;assert(image.Execute());unsigned idealDifferences=0;
 for(unsigned row=0;row<32;row++){
  for(unsigned pass=0;pass<2;pass++)assert(image.l1[image.Index(0x130000+pass*128+row*4)]==expected[row]);
  idealDifferences+=expected[row]!=NativeBfpMachine::Bits(f.expected[row]);
 }
 assert(idealDifferences);
 printf("PASS BFP4 rounded OFFBOARD RV32 K%u %s: two jobs,64 exact words vs independent rational predictions, %u ideal-different rows; NO physical/speed/model-quality claim\n",width,argv[3],idealDifferences);
 if(width==5120 && !direct){
  for(unsigned phase:{0U,2U}){
   ArithmeticBand4Image bad(argv[1],width,false);f.Stage(bad.native);bad.native.dropPhase=phase;assert(bad.Execute());
   bool differs=false;for(unsigned row=0;row<32;row++)differs|=bad.l1[bad.Index(0x130000+row*4)]!=expected[row];assert(differs);
  }
  puts("PASS rounded BFP4 actual-image omitted phase controls; OFFBOARD only");
 }
}
