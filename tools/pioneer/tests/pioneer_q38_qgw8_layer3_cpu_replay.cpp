/* SPDX-License-Identifier: MIT */
// CPU-only localization from a saved layer boundary; never linked into Work.
#define PIONEER_Q38_FULL_MODEL_CPU_REFERENCE 1
#define PIONEER_Q38_CPU_VALIDATION 1
#include "pioneer_q38_cpu_reference_math.h"
#include "pioneer_q38_whole_original_bytes.h"
#include "pioneer_q38_bfp4_full_cpu_backend.h"
#include <cassert>
#include <cstdio>
#include <map>
#include <memory>
using namespace Q38CpuReferenceMath;
#include "actual_cpu_replay_impl.inc"

static bool ReadLayer(const char* path, unsigned layer, std::vector<float>& out){
 FILE* f=fopen(path,"rb");if(!f)return false;
 const bool ok=fseek(f,long(layer)*5120*4,SEEK_SET)==0;
 std::vector<float> values(5120);
 const bool read=ok && fread(values.data(),sizeof(float),values.size(),f)==values.size();
 const bool closed=fclose(f)==0;
 if(!read || !closed)return false;
 for(float x:values)if(!std::isfinite(x))return false;
 out=std::move(values);return true;
}
static void Compare(const char* label,const std::vector<float>& expected,const std::vector<float>& actual){
 assert(expected.size()==actual.size());double sum=0,den=0;unsigned different=0;float maximum=0;
 for(size_t i=0;i<expected.size();i++){
  const double d=double(actual[i])-expected[i];sum+=d*d;den+=double(expected[i])*expected[i];
  maximum=std::max(maximum,float(std::abs(d)));different+=Bits(expected[i])!=Bits(actual[i]);
 }
 printf("LAYER3_CPU_REPLAY %s differences=%u max_abs=%.9g relative_l2=%.12g; CPU_ONLY\n",
  label,different,maximum,std::sqrt(sum/den));
}
int main(int argc,char** argv){
 if(argc!=4 || std::fegetround()!=FE_TONEAREST)return 2;
 setvbuf(stdout,nullptr,_IOLBF,0);
 std::vector<float> input,expected,observed;
 if(!ReadLayer(argv[2],2,input) || !ReadLayer(argv[2],3,expected) || !ReadLayer(argv[3],3,observed))return 1;
 WholeModelOriginalBytes original;if(!original.Open(argv[1]) || !original.Ready())return 1;
 Q38LayerReplay replay;if(!replay.Bind(original))return 1;
 std::vector<float> norm,post,normalized,projection,residual(5120),postNormalized,gate,up,activation(17408),down,final(5120);
 if(!replay.Weight(3,0,norm) || !replay.Weight(3,1,post) || !Rms(input,norm,normalized)
  || !replay.Dense(3,normalized,projection) || projection.size()!=5120)return 1;
 const float scales[]={1.f,0.f,.25f,.5f,.75f};
 for(float scale:scales){
  std::unique_ptr<void,decltype(&Q38Bfp4CpuClose)> backend(Q38Bfp4CpuOpen(argv[1],uint64_t(1)<<3),Q38Bfp4CpuClose);
  if(!backend)return 1;
  for(unsigned i=0;i<5120;i++)residual[i]=scale==0.f?input[i]:input[i]+scale*projection[i];
  if(!Rms(residual,post,postNormalized) || !Q38Bfp4CpuProject(backend.get(),3,2,postNormalized,gate)
   || !Q38Bfp4CpuProject(backend.get(),3,3,postNormalized,up) || gate.size()!=17408 || up.size()!=17408)return 1;
  for(unsigned i=0;i<17408;i++)activation[i]=(gate[i]*Sigmoid(gate[i]))*up[i];
  if(!Q38Bfp4CpuProject(backend.get(),3,4,activation,down) || down.size()!=5120
   || !Q38Bfp4CpuComplete(backend.get()) || !original.Ready())return 1;
  for(unsigned i=0;i<5120;i++)final[i]=residual[i]+down[i];
  if(scale==1.f){
   Compare("replay_vs_converted_reference",expected,final);
   Compare("card_vs_replay",final,observed);
   if(memcmp(expected.data(),final.data(),5120*4)){
    fputs("LAYER3_REPLAY_REFUSED converted CPU boundary did not reproduce exactly\n",stderr);return 1;
   }
  }else{
   char label[96];snprintf(label,sizeof(label),"card_vs_cpu_attention_scale_%.2f_hypothesis",double(scale));
   Compare(label,final,observed);
  }
 }
 puts("PASS_LAYER3_CONVERTED_CPU_REPLAY; NOT DEVICE EXECUTION OR CAUSE PROOF");return 0;
}
