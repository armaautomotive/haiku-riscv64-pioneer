/* SPDX-License-Identifier: MIT */
// Actual fixed ORIGINAL GGUF/full SHA only. NO accelerator FD, native
// controller, tiled products, hardware readback, model run or Work mode.
#define PIONEER_Q38_MIXED_LIVE_VALIDATION_REFERENCE 1
#define PIONEER_Q38_CPU_VALIDATION 1
#include "../PioneerQ38MixedLiveValidationReference.h"
#include "../PioneerQwen38PackedPlan.h"
// TEST observer only, after the independent reference definition. The
// reference itself does not call this production arithmetic helper.
#include "../PioneerQ38ProjectionInput.h"
#include "pioneer_q38_sigmoid_precision_host_bridge.h"
#ifdef PIONEER_Q38_SIGMOID_IMAGE_PROBE
#include "pioneer_q38_sigmoid_precision_probe_bridge.h"
#endif
#include <cassert>
#include <cstdio>
#include <unistd.h>
static const char* originalPath=nullptr;
#ifndef __HAIKU__
static FILE* MixedOriginalFopen(const char* path,const char* mode){
 assert(originalPath && !std::strcmp(path,"/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf") && !std::strcmp(mode,"rb"));return fopen(originalPath,mode);
}
static int MixedOriginalExecv(const char* path,char* const* argv){
 assert(!std::strcmp(path,"/bin/sha256sum") && !std::strcmp(argv[0],"sha256sum") && !argv[1]);
 char name[]="shasum",flag[]="-a",bits[]="256";char* args[]={name,flag,bits,nullptr};return execv("/usr/bin/shasum",args);
}
#define fopen MixedOriginalFopen
#define execv MixedOriginalExecv
#endif
#include "../PioneerQ38PublicModelSource.h"
#ifndef __HAIKU__
#undef fopen
#undef execv
#endif
struct CountedOriginal {
 PioneerQ38PublicModelSource& source;unsigned reads=0;
 bool Ready()const{return source.Ready();}const auto& Model()const{return source.Model();}
 bool ReadReferencePacked(unsigned tensor,uint64_t offset,unsigned bytes,void* output,unsigned capacity){
  reads++;return source.ReadReferencePacked(tensor,offset,bytes,output,capacity);
 }
};
int main(int argc,char** argv){
#ifdef PIONEER_Q38_SIGMOID_IMAGE_PROBE
 assert(argc==3);
#else
 assert(argc==2);
#endif
 setvbuf(stdout,nullptr,_IOLBF,0);originalPath=argv[1];
 PioneerQ38PublicModelSource original;assert(original.Open() && original.Ready());
 puts("PASS ORIGINAL fixed c600de03 FULL SHA/open file identity/all851 tensor schema;NO accelerator context");
 CountedOriginal source{original};auto output=std::make_unique<PioneerQ38MixedLiveReferenceProducts>();
 assert(PioneerQ38MixedLiveOriginalReference(source,0,output.get()) && original.Ready() && source.reads==3717);
 uint64_t hash=14695981039346656037ULL;const auto* p=reinterpret_cast<const uint8_t*>(output.get());
 for(unsigned i=0;i<sizeof(*output);i++){hash^=p[i];hash*=1099511628211ULL;}
 using namespace PioneerQ38MixedLiveReferenceDetail;float embeddingMax=0,finalMax=0;
 for(unsigned i=0;i<5120;i++){embeddingMax=std::fmax(embeddingMax,std::fabs(Float(output->embedding[i])));finalMax=std::fmax(finalMax,std::fabs(Float(output->finalHidden[i])));}
 printf("PASS ORIGINAL dense-block ideal CPU reference/3717reads/12sealed tensors/actual Q8_0 QG+K+V/Q6_K O/Q4_K FFN/embedding+RMS+position0attention+originalresidual+FFN+final fingerprint%016llx embeddingMax%.9g finalMax%.9g;STANDALONE layer3fromembedding,NOT GDN0..2/full64/token/cardcomparison/Work/TPS\n",(unsigned long long)hash,embeddingMax,finalMax);
 struct Check {const char* name;unsigned type;const uint32_t* values;unsigned words;};
 const Check checks[]={{"QG/K/V",8,output->normalized,5120},{"O",14,output->attention,6144},
  {"FFNgate/up",12,output->postNormalized,5120},{"FFNdown",12,output->activated,17408}};
 for(const auto& check:checks){
  std::vector<int> q;std::vector<float> scale;assert(Input(check.type,check.values,check.words,q,scale));
  unsigned laneDifferences=0,scaleDifferences=0;
  for(unsigned at=0;at<check.words;at+=256){
   std::array<float,256> input;for(unsigned i=0;i<256;i++)input[i]=Float(check.values[at+i]);PioneerQ38ProjectionInput actual;
   assert(PioneerQ38ProjectionQuantize(check.type,input.data(),256,&actual));
   for(unsigned i=0;i<256;i++){
    unsigned raw=actual.operand[i/2]>>((i%2)*16)&65535;int value=int(raw&1023)*(raw&0x8000?-1:1);
    laneDifferences+=value!=q[at+i];
   }
   unsigned n=check.type==8?8:1,first=at/(check.type==8?32:256);
   for(unsigned i=0;i<16;i++)scaleDifferences+=actual.scale[i]!=(i<n?Bits(scale[first+i]):0);
  }
  printf("ORIGINAL_EXPECTED_INPUT_QUANTIZER %s/type%u/words%u/laneDifferences%u/scaleDifferences%u;CPU arithmetic parity only,NOT actual instructions\n",
   check.name,check.type,check.words,laneDifferences,scaleDifferences);
  assert(!laneDifferences && !scaleDifferences);
 }
 for(unsigned kind=0;kind<2;kind++){
  const auto* input=kind?output->ffnGate:output->gate;unsigned words=kind?17408:6144,differences=0,maxUlp=0;
  float minGate=0,maxGate=0;
  for(unsigned i=0;i<words;i++){
   float gate=Float(input[i]);minGate=std::fmin(minGate,gate);maxGate=std::fmax(maxGate,gate);
   uint32_t actual=0xdeadbeef,expected=Bits(Flush(SigmoidIdeal(gate)));assert(ProbeQ38SigmoidPrecision(input[i],&actual));
   differences+=actual!=expected;unsigned distance=actual>expected?actual-expected:expected-actual;maxUlp=std::max(maxUlp,distance);
  }
  printf("ORIGINAL_SIGMOID_PRECISION_HOST %s/words%u/differences%u/maxUlp%u/gateRange[%.9g,%.9g];HOST candidate,NOT emitted instructions or driver\n",
   kind?"FFN":"attention",words,differences,maxUlp,minGate,maxGate);
  assert(!differences);
#ifdef PIONEER_Q38_SIGMOID_IMAGE_PROBE
  unsigned emittedDifferences=0;
  for(unsigned at=0;at<words;at+=64){
   uint32_t actual[64];ExecuteSigmoidPrecisionProbe(argv[2],input+at,actual);
   for(unsigned i=0;i<64;i++)emittedDifferences+=actual[i]!=Bits(Flush(SigmoidIdeal(Float(input[at+i]))));
  }
  printf("ORIGINAL_SIGMOID_PRECISION_EMITTED %s/values%u/differences%u/ACTUAL QSG1 local RV32 instructions;CPU supplied primitive INPUTS only,NOT genuine producer/fullblock/native/card/Work/TPS\n",
   kind?"FFN":"attention",words,emittedDifferences);
  assert(!emittedDifferences);
#endif
 }
}
