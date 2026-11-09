/* SPDX-License-Identifier: MIT */
// Offline arithmetic parity for the saved CPU attention input, no device.
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(__FAST_MATH__)
#error CPU replay must not enter native or Work roles
#endif
#define PIONEER_Q38_FULL_MODEL_CPU_REFERENCE 1
#define PIONEER_Q38_CPU_VALIDATION 1
#include "pioneer_q38_cpu_reference_math.h"
#include "../PioneerQ38ProjectionInput.h"
#include <cstdio>
int main(int argc,char** argv){
 if(argc!=2 || std::fegetround()!=FE_TONEAREST)return 2;
 FILE* f=fopen(argv[1],"rb");if(!f)return 2;
 std::vector<float> input(6144);
 bool okay=fread(input.data(),sizeof(float),input.size(),f)==input.size() && fgetc(f)==EOF && !ferror(f);
 if(fclose(f))okay=false;if(!okay)return 2;
 std::vector<int> q;std::vector<float> scales;
 if(!Q38CpuReferenceMath::Input(14,input,q,scales))return 2;
 unsigned laneDiff=0,scaleDiff=0;
 for(unsigned at=0;at<input.size();at+=256){
  PioneerQ38ProjectionInput produced;if(!PioneerQ38ProjectionQuantize(14,input.data()+at,256,&produced))return 2;
  for(unsigned i=0;i<256;i++){
   int value=q[at+i];unsigned expected=0x4000U | (value<0 ? 0x8000U|unsigned(-value) : unsigned(value));
   unsigned actual=produced.operand[i/2]>>((i%2)*16)&0xffffU;laneDiff+=actual!=expected;
  }
  for(unsigned i=0;i<16;i++)scaleDiff+=produced.scale[i]!=(i?0:Q38CpuReferenceMath::Bits(scales[at/256]));
 }
 printf("DENSE_INPUT_QUANTIZER_REPLAY chunks=24 lanes=6144 lane_differences=%u scale_differences=%u; CPU_ONLY NOT_DEVICE\n",laneDiff,scaleDiff);
 return laneDiff || scaleDiff ? 1 : 0;
}
