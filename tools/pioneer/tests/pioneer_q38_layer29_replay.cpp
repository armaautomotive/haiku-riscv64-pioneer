/* SPDX-License-Identifier: MIT */
// CPU-only differential localization. NEVER compiled into native or Work.
#define PIONEER_Q38_FULL_MODEL_CPU_REFERENCE 1
#define PIONEER_Q38_CPU_VALIDATION 1
#include "pioneer_q38_cpu_reference_math.h"
#include "pioneer_q38_whole_original_bytes.h"
#include "pioneer_q38_layer29_math_bridge.h"
#include "pioneer_q38_sigmoid_precision_host_bridge.h"
#ifdef PIONEER_Q38_LAYER29_EMITTED_DELTA_DIAGNOSTIC
#include "pioneer_q38_layer29_emitted_delta_bridge.h"
#endif
#include <cassert>
#include <cstdio>
#include <map>
#include <sys/stat.h>
using namespace Q38CpuReferenceMath;
// Runner losslessly extracts the ACTUAL complete CPU Impl, changing ONLY
// its type name. This exposes no new public CPU/native/Work API or callback.
#include "actual_cpu_replay_impl.inc"
static unsigned Different(const std::vector<float>& a,const std::vector<float>& b){
 assert(a.size()==b.size());unsigned n=0;for(unsigned i=0;i<a.size();i++)n+=Bits(a[i])!=Bits(b[i]);return n;
}
static void CheckActualCardQuantizer(const std::vector<float>& input,unsigned type,unsigned mode,const char* stage){
 std::vector<int> expected;std::vector<float> scales;assert(Input(type,input,expected,scales));
 unsigned laneDiff=0,scaleDiff=0;
 for(unsigned at=0;at<input.size();at+=256){
  int lanes[256];float actual[8];assert(Q38Layer29CardQuantize(type,input.data()+at,lanes,actual));
  for(unsigned i=0;i<256;i++)laneDiff+=lanes[i]!=expected[at+i];
  const unsigned width=type==8?32:256;
  for(unsigned i=0;i<(type==8?8U:1U);i++)scaleDiff+=Bits(actual[i])!=Bits(scales[at/width+i]);
 }
 printf("LAYER29_ACTUAL_CARD_QUANTIZER mode%u stage%s type%u values%zu laneDifferences%u scaleDifferences%u;HOST component NOT emitted/physical/Work/TPS\n",
  mode,stage,type,input.size(),laneDiff,scaleDiff);
 // Exact component comparison,not permission to replace a model's inputs.
 assert(!laneDiff && !scaleDiff);
}
// CPU-LOCAL localization of the existing SFPU primitive's F32 reduction
// order. This is NOT an instruction-engine execution or native product.
static bool ScalarOrderGdn(const float* q,const float* k,const float* value,float decay,float beta,float* state,float* out){
 for(unsigned v=0;v<128;v++){
  float prediction=0;
  for(unsigned key=0;key<128;key++)prediction=Flush(prediction+Flush(Flush(state[key*128+v]*decay)*k[key]));
  float delta=Flush(Flush(value[v]-prediction)*beta),sum=0;
  for(unsigned key=0;key<128;key++){
   const unsigned at=key*128+v;
   state[at]=Flush(Flush(state[at]*decay)+Flush(k[key]*delta));
   sum=Flush(sum+Flush(state[at]*q[key]));
  }
  out[v]=Flush(sum*0.0883883461356163f);if(!Normal(out[v]))return false;
 }
 return true;
}
int main(int argc,char** argv){
 assert(argc==
#ifdef PIONEER_Q38_LAYER29_EMITTED_DELTA_DIAGNOSTIC
 4
#else
 3
#endif
 && std::fegetround()==FE_TONEAREST);setvbuf(stdout,nullptr,_IOLBF,0);
 FILE* file=fopen(argv[2],"rb");assert(file);struct stat st={};constexpr unsigned Count=16+3*5120;
 std::vector<uint32_t> words(Count);assert(!fstat(fileno(file),&st) && S_ISREG(st.st_mode) && st.st_size==Count*4
  && fread(words.data(),4,Count,file)==Count && fgetc(file)==EOF && !ferror(file) && !fclose(file));
 const uint32_t header[]={0x31524c51,1,29,5120,3,0,0,0,0,0,0,0,0,0,0,0};assert(!memcmp(words.data(),header,sizeof(header)));
 std::vector<float> hidden(5120),expected(5120),native(5120);memcpy(hidden.data(),words.data()+16,20480);
 memcpy(expected.data(),words.data()+16+5120,20480);memcpy(native.data(),words.data()+16+10240,20480);
 WholeModelOriginalBytes original;assert(original.Open(argv[1]) && original.Ready());Q38LayerReplay replay;assert(replay.Bind(original));
 const unsigned layer=29;std::vector<float> norm,post,normalized,z,qkv,alpha,beta,convWeight,a,dt,gatedNorm;
 assert(replay.Weight(layer,0,norm) && replay.Weight(layer,1,post) && Rms(hidden,norm,normalized));
 assert(replay.Project(layer,5,normalized,z) && replay.Project(layer,6,normalized,qkv)
  && replay.Project(layer,10,normalized,alpha) && replay.Project(layer,11,normalized,beta)
  && replay.Weight(layer,12,convWeight) && replay.Weight(layer,7,a) && replay.Weight(layer,8,dt) && replay.Weight(layer,9,gatedNorm));
 std::vector<float> baselineGated,baselineResidual,baselineActivation;unsigned nativeMatches=0;
 for(unsigned mode:{0U,1U,2U,4U,7U,8U,16U,34U,39U,64U,71U,128U,135U,192U,199U
#ifdef PIONEER_Q38_LAYER29_EMITTED_DELTA_DIAGNOSTIC
 ,256U,320U,327U
#endif
 }){
  std::vector<float> conv(10240),raw(6144),gated(6144),state(786432,0),projected;
#ifdef PIONEER_Q38_LAYER29_EMITTED_DELTA_DIAGNOSTIC
  uint64_t emittedSteps=0,emittedOps=0;
#endif
  if(mode&1){for(unsigned c=0;c<10240;c+=16)assert(Q38Layer29CardConv(qkv.data()+c,convWeight.data()+c*4,conv.data()+c));}
  else for(unsigned c=0;c<10240;c++){
   float sum=0;for(unsigned tap=0;tap<4;tap++)sum+=(tap==3?qkv[c]:0.f)*convWeight[c*4+tap];
   conv[c]=float(double(sum)*SigmoidDouble(sum));assert(Normal(conv[c]));
  }
  for(unsigned h=0;h<48;h++){
   float query[128],key[128],gates[2];const float* inQ=conv.data()+(h%16)*128;const float* inK=conv.data()+2048+(h%16)*128;
   {
    float qs=0,ks=0;for(unsigned i=0;i<128;i++){qs+=inQ[i]*inQ[i];ks+=inK[i]*inK[i];}
    float qi=1.f/std::sqrt(qs+1e-6f),ki=1.f/std::sqrt(ks+1e-6f);
    for(unsigned i=0;i<128;i++){query[i]=inQ[i]*qi;key[i]=inK[i]*ki;}
    double shifted=alpha[h]+dt[h];float softplus=float(shifted>0?shifted+std::log1p(std::exp(-shifted)):std::log1p(std::exp(shifted)));
    gates[0]=Flush(float(std::exp(double(a[h]*softplus))));gates[1]=Flush(Sigmoid(beta[h]));
   }
   if(mode&(2|8|16)){
    float cardQ[128],cardK[128],cardGates[2];assert(Q38Layer29CardPrepare(inQ,inK,alpha[h],beta[h],a[h],dt[h],cardQ,cardK,cardGates));
    if(mode==2 && (Bits(cardGates[0])!=Bits(gates[0]) || Bits(cardGates[1])!=Bits(gates[1])))
     printf("LAYER29_PREPARE_GATE head%u beta_logit%08x CPU_beta%08x card_beta%08x CPU_decay%08x card_decay%08x\n",
      h,Bits(beta[h]),Bits(gates[1]),Bits(cardGates[1]),Bits(gates[0]),Bits(cardGates[0]));
    if(mode&(2|8)){memcpy(query,cardQ,sizeof(query));memcpy(key,cardK,sizeof(key));}
    if(mode&(2|16))memcpy(gates,cardGates,sizeof(gates));
   }
   if(mode&32){uint32_t precise=0;assert(ProbeQ38SigmoidPrecision(Bits(beta[h]),&precise));gates[1]=Float(precise);}
   if(mode&64)assert(Q38Layer29CardPreparePrecise(inQ,inK,alpha[h],beta[h],a[h],dt[h],query,key,gates));
#ifdef PIONEER_Q38_LAYER29_EMITTED_DELTA_DIAGNOSTIC
   if(mode&256){
    std::vector<float> scalarState(state.begin()+h*16384,state.begin()+(h+1)*16384),scalarOutput(128);
    assert(ScalarOrderGdn(query,key,conv.data()+4096+h*128,gates[0],gates[1],scalarState.data(),scalarOutput.data()));
    uint64_t steps=0,ops=0;
    assert(Q38Layer29EmittedDelta(argv[3],query,key,conv.data()+4096+h*128,gates[0],gates[1],state.data()+h*16384,raw.data()+h*128,&steps,&ops));
    for(unsigned i=0;i<16384;i++)assert(Bits(state[h*16384+i])==Bits(scalarState[i]));
    for(unsigned i=0;i<128;i++)assert(Bits(raw[h*128+i])==Bits(scalarOutput[i]));
    emittedSteps+=steps;emittedOps+=ops;
    if((h+1)%8==0)printf("LAYER29_EMITTED_LOCAL_RECURRENCE mode%u heads%u/48 all_state_and_output_words_exact_to_F32_order;HOST instruction engine NOT physical timing\n",mode,h+1);
   }else
#endif
   {
    auto recurrence=(mode&128)?ScalarOrderGdn:Gdn;
    assert(recurrence(query,key,conv.data()+4096+h*128,gates[0],gates[1],state.data()+h*16384,raw.data()+h*128));
   }
   if(mode&4)assert(Q38Layer29CardGated(raw.data()+h*128,gatedNorm.data(),z.data()+h*128,gated.data()+h*128));
   else{
    float sum=0;for(unsigned i=0;i<128;i++){float x=raw[h*128+i];sum+=Flush(x*x);}
    float inverse=1.f/std::sqrt(Flush(sum*.0078125f)+1e-6f);
    for(unsigned i=0;i<128;i++){unsigned at=h*128+i;float weighted=Flush(Flush(raw[at]*inverse)*gatedNorm[i]);
     gated[at]=Flush(float(double(weighted)*double(z[at])*SigmoidDouble(z[at])));assert(Normal(gated[at]));}
   }
  }
  CheckActualCardQuantizer(gated,8,mode,"gated");
  assert(replay.Project(layer,13,gated,projected));std::vector<float> residual(5120),postNormalized,gate,up,activation(17408),down,result(5120);
  for(unsigned i=0;i<5120;i++)residual[i]=hidden[i]+projected[i];
  assert(Rms(residual,post,postNormalized) && replay.Project(layer,2,postNormalized,gate) && replay.Project(layer,3,postNormalized,up));
  for(unsigned i=0;i<17408;i++)activation[i]=(gate[i]*Sigmoid(gate[i]))*up[i];
  CheckActualCardQuantizer(activation,12,mode,"ffn");
  assert(replay.Project(layer,4,activation,down));for(unsigned i=0;i<5120;i++)result[i]=residual[i]+down[i];
  if(!mode){assert(!Different(result,expected));baselineGated=gated;baselineResidual=residual;baselineActivation=activation;}
  std::vector<int> q0,q1;std::vector<float> s0,s1;assert(Input(8,baselineGated,q0,s0) && Input(8,gated,q1,s1));
  unsigned lanes=0;for(unsigned i=0;i<q0.size();i++)lanes+=q0[i]!=q1[i];
  unsigned cpuDiff=Different(result,expected),nativeDiff=Different(result,native);nativeMatches+=nativeDiff==0;
  if((mode&64) && !(mode&128) && !(mode&256))assert(!cpuDiff); // Original CPU-order replay,not the actual SFPU reduction.
#ifdef PIONEER_Q38_LAYER29_EMITTED_DELTA_DIAGNOSTIC
  if(mode&256){
   assert(cpuDiff==5120 && !nativeDiff && lanes==0 && Different(s0,s1)==1);
   printf("LAYER29_EMITTED_LOCAL_REPLAY mode%u half_images96 state_words786432 output_words6144 RV32steps%llu SFPUops%llu known_CPU_difference_REMAINS_FAILURE native_DATA_exact1;NOT current linked peer/native history/physical/Work/TPS\n",
    mode,static_cast<unsigned long long>(emittedSteps),static_cast<unsigned long long>(emittedOps));
  }
#endif
  printf("LAYER29_CPU_LOCAL_REPLAY mode%u gated_words%u gated_q8_lanes%u gated_q8_scales%u residual_words%u activation_words%u CPU_hidden_differences%u native_partial_hidden_differences%u;NOT emitted/physical/Work/TPS\n",
   mode,Different(baselineGated,gated),lanes,Different(s0,s1),Different(baselineResidual,residual),Different(baselineActivation,activation),cpuDiff,nativeDiff);
 }
 assert(original.Ready() && replay.Current(true));
 printf("COMPLETED_CPU_LOCAL_LAYER29_LOCALIZATION exact_CPU_baseline1 native_matching_variants%u;NO native or physical completion grant\n",nativeMatches);
}
