/* SPDX-License-Identifier: MIT */
// CPU-only offline replay of data captured by a completed card diagnostic.
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(__FAST_MATH__)
#error Offline projection replay must not enter native or Work roles
#endif
#define PIONEER_Q38_FULL_MODEL_CPU_REFERENCE 1
#define PIONEER_Q38_CPU_VALIDATION 1
#include "pioneer_q38_cpu_reference_math.h"
#include "pioneer_q38_whole_original_bytes.h"
#include <cstdio>
#include <map>
using namespace Q38CpuReferenceMath;
#include "actual_cpu_replay_impl.inc"

static bool ReadPrefix(const char* path,unsigned n,std::vector<float>& result){
 FILE* f=fopen(path,"rb");if(!f)return false;std::vector<float> values(n);
 bool okay=fread(values.data(),sizeof(float),n,f)==n;if(fclose(f))okay=false;
 for(float v:values)if(!std::isfinite(v))okay=false;
 if(okay)result=std::move(values);
 return okay;
}
static bool Compare(const char* label,const std::vector<float>& reference,const std::vector<float>& actual){
 if(reference.size()!=actual.size())return false;
 double err=0,norm=0,dot=0,observed=0;unsigned changed=0;
 for(unsigned i=0;i<reference.size();i++){
  double r=reference[i],a=actual[i],delta=a-r;err+=delta*delta;norm+=r*r;observed+=a*a;dot+=r*a;
  changed+=Bits(reference[i])!=Bits(actual[i]);
 }
 if(!(norm>0) || !(observed>0) || !std::isfinite(err) || !std::isfinite(dot))return false;
 printf("OBSERVED_INPUT_REPLAY %s changed=%u rel_l2=%.12g cosine=%.12g; CPU_ONLY\n",
  label,changed,std::sqrt(err/norm),dot/std::sqrt(norm*observed));return true;
}
int main(int argc,char** argv){
 static_assert(sizeof(float)==4 && __BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__,"little-endian f32 required");
 if(argc!=4 || std::fegetround()!=FE_TONEAREST)return 2;
 std::vector<float> input,prior,projection;
 if(!ReadPrefix(argv[2],6144,input) || !ReadPrefix(argv[3],5120,prior))return 1;
 WholeModelOriginalBytes model;if(!model.Open(argv[1]) || !model.Ready())return 1;
 Q38LayerReplay replay;if(!replay.Bind(model) || !replay.Project(3,10,input,projection)
  || projection.size()!=5120 || !model.Ready() || !Compare("reference_projection_vs_prior_card_projection",prior,projection))return 1;
 puts("CPU_ONLY on card-captured data; NOT same-run projection, direct device execution, cause proof or speed");
 return 0;
}
