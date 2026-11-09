/* SPDX-License-Identifier: MIT */
// First-token hypothesis screen only: no device execution or causal proof.
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(__FAST_MATH__)
#error Offline reference screen only
#endif
#define PIONEER_Q38_FULL_MODEL_CPU_REFERENCE 1
#define PIONEER_Q38_CPU_VALIDATION 1
#include "pioneer_q38_cpu_reference_math.h"
#include "pioneer_q38_whole_original_bytes.h"
#include <cstdio>
#include <map>
using namespace Q38CpuReferenceMath;
#include "actual_cpu_replay_impl.inc"

static bool Read(const char* path, long offset, unsigned count, std::vector<float>& output){
 FILE* f=std::fopen(path,"rb");if(!f)return false;std::vector<float> values(count);
 bool ok=!std::fseek(f,offset,SEEK_SET) && std::fread(values.data(),4,count,f)==count;
 if(std::fclose(f))ok=false;
 for(float v:values)if(!std::isfinite(v))ok=false;
 if(ok)output=std::move(values);return ok;
}
struct Screen {unsigned eligible=0,opposite=0,exceeds=0;double dot=0,nv=0,no=0;};
static void Measure(Screen& s,float value,float observed){
 if(std::fabs(value)<0.001f)return;
 s.eligible++;s.opposite+=double(value)*observed<0;
 s.exceeds+=std::fabs(observed)>std::fabs(value)*1.01f+0.00001f;
 s.dot+=double(value)*observed;s.nv+=double(value)*value;s.no+=double(observed)*observed;
}
int main(int argc,char** argv){
 static_assert(sizeof(float)==4 && __BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__,"little-endian f32 required");
 if(argc!=5 || std::fegetround()!=FE_TONEAREST)return 2;
 Screen check;Measure(check,2.f,1.f);Measure(check,-2.f,1.f);Measure(check,2.f,3.f);Measure(check,0.0001f,1.f);
 if(check.eligible!=3 || check.opposite!=1 || check.exceeds!=1 || check.dot!=6. || check.nv!=12. || check.no!=11.)return 1;
 std::vector<float> hidden,observed,weights,normalized,value,key,qg,reference;
 if(!Read(argv[2],2*5120*4,5120,hidden) || !Read(argv[3],0,6144,observed)
  || !Read(argv[4],0,6144,reference))return 1;
 WholeModelOriginalBytes model;if(!model.Open(argv[1]) || !model.Ready())return 1;
 Q38LayerReplay replay;if(!replay.Bind(model) || !replay.Weight(3,0,weights)
  || !Rms(hidden,weights,normalized) || !replay.Project(3,5,normalized,qg)
  || !replay.Project(3,6,normalized,key) || !replay.Project(3,7,normalized,value)
  || value.size()!=1024 || key.size()!=1024 || qg.size()!=12288)return 1;
 for(unsigned h=0;h<24;h++)for(unsigned i=0;i<256;i++){
  const float expected=Flush(Half(HalfRne(value[(h/6)*256+i]))*Sigmoid(qg[h*512+256+i]));
  if(Bits(expected)!=Bits(reference[h*256+i]))return 1;
 }
 std::puts("CPU reference attention input reproduced bit-for-bit; no device assertion");
 // A fixed shift hypothesis motivated by the earlier zero-lane pattern.
 // Cross-run and CPU-gate assumptions still apply; do not infer hardware
 // cause merely because a candidate reproduces the captured vector.
 {
  unsigned changed=0,compared=0;double error=0,denominator=0;
  for(unsigned h=0;h<24;h++)for(unsigned i=0;i<256;i++){
   const float v=(i&15)<4?0.f:Half(HalfRne(value[(h/6)*256+i-4]));
   const float expected=Flush(v*Sigmoid(qg[h*512+256+i]));const float actual=observed[h*256+i];
   changed+=Bits(expected)!=Bits(actual);compared++;
   error+=double(expected-actual)*double(expected-actual);denominator+=double(actual)*actual;
  }
  if(!(denominator>0))return 1;
  std::printf("FIXED_SHIFT4_SCREEN lanes=%u changed_bits=%u rel_l2=%.12g; CPU_GATE_ASSUMED CROSS_RUN HYPOTHESIS_ONLY\n",
   compared,changed,std::sqrt(error/denominator));
 }
 for(unsigned group=0;group<4;group++){
  unsigned disagreements=0,zeroLanes=0;
  for(unsigned i=0;i<256;i++){
   bool positive=false,negative=false;
   for(unsigned h=0;h<6;h++){float v=observed[(group*6+h)*256+i];positive|=v>0;negative|=v<0;}
   disagreements+=positive && negative;zeroLanes+=!positive && !negative;
  }
  std::printf("CARD_GROUP_SIGN group=%u disagreeing_dimensions=%u all_six_zero=%u of 256\n",group,disagreements,zeroLanes);
 }
 // Hypothesis only: assume the CPU reference's six gate values are close
 // to the separate card run. Fit a single shared V per output dimension,
 // then see if it resembles ANY dimension of that group's captured value.
 // This can suggest a lane permutation; it cannot prove one or exclude a
 // bad gate. Selecting a best match across 256 candidates can overfit.
 for(unsigned group=0;group<4;group++){
  unsigned fitted=0,near=0,same=0,sameBlock=0;double error=0,energy=0;unsigned modulo[16]={};
  for(unsigned i=0;i<256;i++){
   double numerator=0,denominator=0;
   for(unsigned h=0;h<6;h++){
    const unsigned head=group*6+h;const double g=Sigmoid(qg[head*512+256+i]);
    numerator+=g*observed[head*256+i];denominator+=g*g;
   }
   if(!(denominator>0))return 1;const double inferred=numerator/denominator;
   for(unsigned h=0;h<6;h++){
    const unsigned head=group*6+h;const double actual=observed[head*256+i],predicted=inferred*Sigmoid(qg[head*512+256+i]);
    error+=(actual-predicted)*(actual-predicted);energy+=actual*actual;
   }
   if(inferred==0)continue;fitted++;
   unsigned best=0;double delta=1e100;
   for(unsigned candidate=0;candidate<256;candidate++){
    const double v=Half(HalfRne(value[group*256+candidate]));const double diff=std::fabs(inferred-v);
    if(diff<delta){delta=diff;best=candidate;}
   }
   if(delta<=0.0001+std::fabs(inferred)*0.01){near++;same+=best==i;sameBlock+=best/16==i/16;modulo[(best-i)&15]++;}
  }
  if(!fitted || !(energy>0))return 1;
  std::printf("GATE_FIT_SCREEN group=%u nonzero=%u fit_rel_l2=%.9g nearest_1pct=%u same_lane=%u same_16block=%u shifts=",
   group,fitted,std::sqrt(error/energy),near,same,sameBlock);
  for(unsigned i=0;i<16;i++)std::printf("%s%u",i?",":"",modulo[i]);
  std::puts("; CPU_GATE_ASSUMED CROSS_RUN HYPOTHESIS_ONLY");
 }
 // Cross-family sign screening uses the first head of each shared group.
 // Selecting the best of many candidates overfits and is not proof of routing.
 for(unsigned group=0;group<4;group++)for(unsigned family=0;family<4;family++){
  unsigned best=0,bestEligible=0,bestOpposite=0;double bestFraction=2.;
  const unsigned count=family<2?4:24;
  for(unsigned candidate=0;candidate<count;candidate++){
   unsigned eligible=0,opposite=0;
   for(unsigned i=0;i<256;i++){
    const float v=family==0?value[candidate*256+i]:family==1?key[candidate*256+i]:
     qg[candidate*512+(family==3?256:0)+i];
    const float o=observed[group*6*256+i];
    if(std::fabs(v)<0.001f || o==0)continue;
    eligible++;opposite+=double(v)*o<0;
   }
   if(eligible<128)return 1;
   const double fraction=double(opposite)/eligible;
   if(fraction<bestFraction){bestFraction=fraction;best=candidate;bestEligible=eligible;bestOpposite=opposite;}
  }
  const char* names[]={"value","key","raw_query","raw_gate"};
  std::printf("PROJECTION_SIGN_SCREEN group=%u family=%s best=%u opposite=%u eligible=%u fraction=%.9g; HYPOTHESIS_ONLY\n",
   group,names[family],best,bestOpposite,bestEligible,bestFraction);
 }
 for(unsigned head=0;head<24;head++){
  for(unsigned group=0;group<4;group++){
   Screen screen;
   for(unsigned i=0;i<256;i++){
    const float v=Half(HalfRne(value[group*256+i])),o=observed[head*256+i];
    Measure(screen,v,o);
   }
   if(!screen.eligible || !(screen.nv>0) || !(screen.no>0) || !std::isfinite(screen.dot))return 1;
   std::printf("VALUE_SCREEN head=%u group=%u eligible=%u opposite=%u exceeds=%u cosine=%.9g%s\n",
    head,group,screen.eligible,screen.opposite,screen.exceeds,screen.dot/std::sqrt(screen.nv*screen.no),group==head/6?" canonical":"");
  }
 }
 if(!model.Ready())return 1;
 std::puts("CPU_ONLY first-position value/sign hypothesis, NOT card value capture, cause proof or speed");
}
