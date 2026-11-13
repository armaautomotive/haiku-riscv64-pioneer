/* SPDX-License-Identifier: MIT */
// Compare observer against the independently exported reference for the
// original exporter's BF16 input. Host-only; no device access.
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
extern "C" bool Q38ModelRecurrentExpected(const char*,const float*,double*);
int main(int argc,char** argv){
 assert(argc==3);float input[5120];double expected[64];
 for(unsigned i=0;i<5120;i++){
  uint32_t bits=uint32_t(0x3f00+(i*13)%256+((i&3)==2?0x8000:0))<<16;
  std::memcpy(&input[i],&bits,4);
 }
 for(unsigned role=1;role<3;role++){
  assert(Q38ModelRecurrentExpected(argv[role],input,expected));FILE* f=std::fopen(argv[role],"rb");assert(f);
  for(unsigned band=0;band<2;band++){
   assert(!std::fseek(f,32+band*(160*1088+128)+160*1088,SEEK_SET));
   uint32_t bits[32];assert(std::fread(bits,4,32,f)==32);
   for(unsigned row=0;row<32;row++){
    float actual;std::memcpy(&actual,&bits[row],4);
    assert(std::isfinite(actual) && std::fabs(double(actual)-expected[band*32+row])<=0.001+std::fabs(expected[band*32+row])*0.0001);
   }
  }
  assert(!std::fclose(f));
 }
 puts("PASS model-derived alpha/beta observer against separately exported reference; NO DEVICE OR SPEED");
}
