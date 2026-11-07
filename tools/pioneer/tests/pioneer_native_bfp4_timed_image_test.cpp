/* SPDX-License-Identifier: MIT */
#define PIONEER_NATIVE_BFP4_BAND_IMAGE_ONLY
#include "pioneer_q38_native_bfp4_band_image_test.cpp"
#include "../PioneerNativeBfp4BandTiming.h"

// Simulates only read-only mcycle CSR sampling in the actual linked RV32
// image. Test cycles are synthetic and are NEVER physical timing evidence.
struct TimedImage:Band4RV32 {
 bool unstable=false;unsigned reads=0;
 TimedImage(const char* folder,unsigned width,bool direct):Band4RV32(folder,width,direct){expectedMarker=0x5442464e;}
 uint32_t ReadCycleCsr(unsigned csr)override{
  assert(csr==0xb00 || csr==0xb80);reads++;
  if(unstable && csr==0xb80)return reads;
  uint64_t clock=UINT64_C(0xffff0000)+uint64_t(steps)*37;
  return csr==0xb00?uint32_t(clock):uint32_t(clock>>32);
 }
};
int main(int argc,char** argv){
 assert(argc==4);unsigned width=unsigned(std::stoul(argv[2]));
 assert(!strcmp(argv[3],"mmio") || !strcmp(argv[3],"direct"));bool direct=!strcmp(argv[3],"direct");
 for(unsigned failure=0;failure<2;failure++){
  Fixture f(width);TimedImage image(argv[1],width,direct);f.Stage(image.native);image.unstable=failure;
  assert(image.Execute() && image.Matches(f.expected) && image.minSP>=0x6e040);
  assert(image.reads==(failure?48U:12U));
  unsigned r[16];for(unsigned i=0;i<16;i++)r[i]=image.l1[image.Index(0x6e000+i*4)];
  for(unsigned pass=0;pass<2;pass++)assert(PioneerNativeBfp4BandTiming::State(r+pass*8,width,pass)==(failure?2U:1U));
  if(!failure){
   assert(PioneerNativeBfp4BandTiming::Later(r+8+4,r+6));
   if(width==5120)assert(r[5]<r[7]); // first measured interval crosses low-word wrap
   for(unsigned pass=0;pass<2;pass++){
    uint64_t start=uint64_t(r[pass*8+4])|(uint64_t(r[pass*8+5])<<32),end=uint64_t(r[pass*8+6])|(uint64_t(r[pass*8+7])<<32);
    assert(end>start && end-start>width/32);
   }
   for(unsigned i=0;i<16;i++){unsigned copy[16];memcpy(copy,r,sizeof(copy));copy[i]^=0x40000000;
    assert(PioneerNativeBfp4BandTiming::State(copy+(i/8)*8,width,i/8)!=1 || (i%8>=4 && i%8<=7));}
  }
 }
 printf("PASS actual timed BFP4 RV32 K%u %s: two numerical jobs, bounded CSR reads, valid and unavailable clocks, isolated stack; SYNTHETIC cycles, NO hardware or speed proof\n",width,argv[3]);
}
