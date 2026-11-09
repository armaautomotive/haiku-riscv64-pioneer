/* SPDX-License-Identifier: MIT */
// CPU-only replay of dense layer-3 normalization and input quantization.
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(__FAST_MATH__)
#error This replay must not enter a native or Work role
#endif
#define PIONEER_Q38_FULL_MODEL_CPU_REFERENCE 1
#define PIONEER_Q38_CPU_VALIDATION 1
#include "pioneer_q38_cpu_reference_math.h"
#include "pioneer_q38_whole_original_bytes.h"
#include "../PioneerQ38F32HiddenRms.h"
#include "../PioneerQ38ProjectionInput.h"
#include <cstdio>
#include <map>
using namespace Q38CpuReferenceMath;
#include "actual_cpu_replay_impl.inc"
int main(int argc,char** argv){
 (void)&PioneerQ38LearnedRms;(void)&PioneerQ38F32HiddenResidualRms;
 if(argc!=3 || std::fegetround()!=FE_TONEAREST)return 2;
 FILE* f=fopen(argv[2],"rb");if(!f)return 2;std::vector<float> hidden(5120);
 bool okay=fseek(f,2*5120*4,SEEK_SET)==0 && fread(hidden.data(),sizeof(float),hidden.size(),f)==hidden.size();
 if(fclose(f))okay=false;if(!okay)return 2;
 WholeModelOriginalBytes model;if(!model.Open(argv[1]) || !model.Ready())return 2;
 Q38LayerReplay replay;std::vector<float> weight,expected,actual(5120);
 if(!replay.Bind(model) || !replay.Weight(3,0,weight) || !Rms(hidden,weight,expected)
  || !PioneerQ38F32HiddenRms(hidden.data(),weight.data(),actual.data()))return 2;
 unsigned normDiff=0,laneDiff=0,scaleDiff=0;double normErr=0,normDen=0;
 for(unsigned i=0;i<5120;i++){
  double d=double(actual[i])-expected[i];normErr+=d*d;normDen+=double(expected[i])*expected[i];
  normDiff+=Bits(actual[i])!=Bits(expected[i]);
 }
 std::vector<int> q;std::vector<float> scales;if(!Input(8,expected,q,scales))return 2;
 for(unsigned at=0;at<5120;at+=256){
  PioneerQ38ProjectionInput output;if(!PioneerQ38ProjectionQuantize(8,actual.data()+at,256,&output))return 2;
  for(unsigned i=0;i<256;i++){
   int v=q[at+i];unsigned want=0x4000U|(v<0?0x8000U|unsigned(-v):unsigned(v));
   laneDiff+=(output.operand[i/2]>>((i%2)*16)&65535U)!=want;
  }
  for(unsigned i=0;i<16;i++)scaleDiff+=output.scale[i]!=(i<8?Bits(scales[at/32+i]):0);
 }
 if(!(normDen>0) || !model.Ready())return 2;
 printf("DENSE_QKV_INPUT_REPLAY norm_bit_differences=%u norm_rel_l2=%.12g quant_lane_differences=%u scale_differences=%u; CPU_ONLY NOT_DEVICE\n",
  normDiff,std::sqrt(normErr/normDen),laneDiff,scaleDiff);
 return normDiff || laneDiff || scaleDiff ? 1 : 0;
}
