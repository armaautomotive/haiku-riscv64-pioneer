/* SPDX-License-Identifier: MIT */
// CPU-only hypothesis screen. This is not a device implementation or test.
#define PIONEER_Q38_FULL_MODEL_CPU_REFERENCE 1
#define PIONEER_Q38_CPU_VALIDATION 1
#include "pioneer_q38_cpu_reference_math.h"
#include "pioneer_q38_whole_original_bytes.h"
#include <array>
#include <cstdio>
#include <map>
using namespace Q38CpuReferenceMath;
#include "actual_cpu_replay_impl.inc"

static bool ReadSlot(const char* path,unsigned slot,std::vector<float>& data){
 FILE* f=fopen(path,"rb");if(!f)return false;
 std::vector<float> tmp(5120);
 bool okay=fseek(f,long(slot)*5120*4,SEEK_SET)==0 && fread(tmp.data(),4,tmp.size(),f)==tmp.size();
 if(fclose(f))okay=false;
 for(float x:tmp)if(!std::isfinite(x))okay=false;
 if(okay)data=std::move(tmp);
 return okay;
}
static bool Compare(const char* name,const std::vector<float>& reference,const std::vector<float>& actual){
 double error=0,den=0,dot=0,norm=0;
 for(unsigned i=0;i<5120;i++){
  double a=actual[i],r=reference[i],d=a-r;error+=d*d;den+=r*r;dot+=a*r;norm+=a*a;
 }
 if(!(den>0) || !(norm>0) || !std::isfinite(error) || !std::isfinite(dot))return false;
 printf("GROUP_HYPOTHESIS %s rel_l2_to_card=%.9g cosine=%.9g; CPU_ONLY\n",name,std::sqrt(error/den),dot/std::sqrt(den*norm));
 return true;
}
int main(int argc,char** argv){
 if(argc!=5 || std::fegetround()!=FE_TONEAREST)return 2;
 std::vector<float> input,card,cpu,norm,normalized,qg,value;
 if(!ReadSlot(argv[2],2,input) || !ReadSlot(argv[3],0,card) || !ReadSlot(argv[4],0,cpu))return 1;
 WholeModelOriginalBytes model;if(!model.Open(argv[1]) || !model.Ready())return 1;
 Q38LayerReplay replay;if(!replay.Bind(model) || !replay.Weight(3,0,norm) || !Rms(input,norm,normalized)
  || !replay.Project(3,5,normalized,qg) || !replay.Project(3,7,normalized,value)
  || qg.size()!=12288 || value.size()!=1024)return 1;
 for(unsigned mode=0;mode<4;mode++){
  std::vector<float> attention(6144),projection;
  for(unsigned h=0;h<24;h++)for(unsigned i=0;i<256;i++){
   unsigned group=mode==0?h/6:mode==1?h%4:mode==2?3-h/6:0;
   float v=Half(HalfRne(value[group*256+i]));
   attention[h*256+i]=Flush(v*Sigmoid(qg[h*512+256+i]));
  }
  if(!replay.Project(3,10,attention,projection) || projection.size()!=5120)return 1;
  if(mode==0 && memcmp(projection.data(),cpu.data(),5120*sizeof(float))){
   fputs("GROUP_HYPOTHESIS_REFUSED normal CPU projection did not reproduce pinned reference\n",stderr);return 1;
  }
  const char* names[]={"normal","interleaved","reverse","all_group_zero"};
  if(!Compare(names[mode],card,projection))return 1;
 }
 if(!model.Ready())return 1;
 puts("CPU_ONLY_HYPOTHESES; not device provenance, proof of cause, or performance");
}
