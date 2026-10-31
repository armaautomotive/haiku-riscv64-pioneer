/* SPDX-License-Identifier: MIT */
// Posthoc HOST arithmetic localization only. Never device observations,
// accelerator inputs, a native controller, or an inference fallback.
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_ATTENTION_FREESTANDING) || defined(__FAST_MATH__)
#error CPU-input quantization replay must remain separate from device and Work code
#endif
#define PIONEER_Q38_FULL_MODEL_CPU_REFERENCE 1
#define PIONEER_Q38_CPU_VALIDATION 1
#include "pioneer_q38_cpu_reference_math.h"
#include "pioneer_q38_whole_original_bytes.h"
#include "../PioneerQ38F32HiddenRms.h"
#include "../PioneerQ38ProjectionInput.h"
#include <cstdio>
#include <sys/stat.h>
#include <unistd.h>

static bool ReadInputs(const char* path,std::vector<uint32_t>& words){
 static_assert(__BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__,"fixed CPU-only snapshot byte order");
 FILE* file=fopen(path,"rb");if(!file)return false;
 struct stat before={},after={};constexpr size_t Count=16+3*65*5120;
 bool okay=!fstat(fileno(file),&before) && S_ISREG(before.st_mode) && before.st_size==off_t(Count*4);
 if(okay){words.resize(Count);okay=fread(words.data(),4,Count,file)==Count && fgetc(file)==EOF && !ferror(file);}
 okay=okay && !fstat(fileno(file),&after) && before.st_dev==after.st_dev && before.st_ino==after.st_ino
  && before.st_size==after.st_size && before.st_mtime==after.st_mtime && before.st_ctime==after.st_ctime;
#ifdef __APPLE__
 okay=okay && before.st_mtimespec.tv_nsec==after.st_mtimespec.tv_nsec && before.st_ctimespec.tv_nsec==after.st_ctimespec.tv_nsec;
#endif
 if(fclose(file))okay=false;
 const uint32_t header[]={0x31524d51,1,3,65,5120,0,0,0,0,0,0,0,0,0,0,0};
 return okay && !memcmp(words.data(),header,sizeof(header));
}

int main(int argc,char** argv){
 (void)&PioneerQ38LearnedRms;(void)&PioneerQ38F32HiddenResidualRms;
 if(argc!=3 || std::fegetround()!=FE_TONEAREST)return 2;
 setvbuf(stdout,nullptr,_IOLBF,0);
 std::vector<uint32_t> inputs;if(!ReadInputs(argv[2],inputs))return 2;
 WholeModelOriginalBytes original;if(!original.Open(argv[1]) || !original.Ready())return 2;
 std::array<unsigned,65> tensor={};std::array<bool,65> found={};
 for(unsigned i=0;i<851;i++){
  Q38OriginalTensor t;if(!original.Describe(i,t))return 2;
  for(unsigned layer=0;layer<65;layer++){
   const std::string name=layer==64?"output_norm.weight":"blk."+std::to_string(layer)+".attn_norm.weight";
   if(t.name!=name)continue;
   if(found[layer] || t.type || t.rank!=1 || t.inputs!=5120 || t.outputs!=1 || t.bytes!=20480)return 2;
   found[layer]=true;tensor[layer]=i;
  }
 }
 for(bool seen:found)if(!seen)return 2;
 unsigned rows=0,chunks=0,laneDifferences=0,scaleDifferences=0,normalizationDifferences=0;
 unsigned firstReports=0;
 std::vector<float> hidden(5120),weight(5120),normalized,deviceNormalized(5120);
 for(unsigned position=0;position<3;position++)for(unsigned layer=0;layer<65;layer++){
  memcpy(hidden.data(),inputs.data()+16+(position*65+layer)*5120,20480);
  if(!original.ReadOriginal(tensor[layer],0,20480,weight.data(),20480) || !original.Ready()
   || !Q38CpuReferenceMath::Rms(hidden,weight,normalized)
   || !PioneerQ38F32HiddenRms(hidden.data(),weight.data(),deviceNormalized.data()))return 2;
  for(unsigned i=0;i<5120;i++)normalizationDifferences+=
   Q38CpuReferenceMath::Bits(normalized[i])!=PioneerAttentionBits(deviceNormalized[i]);
  // BOTH quantizers consume the SAME independent normalized CPU row. A
  // preceding normalization difference is counted,not mixed into this test.
  for(unsigned type:{8U,12U,14U}){
   std::vector<int> expected;std::vector<float> scales;
   if(!Q38CpuReferenceMath::Input(type,normalized,expected,scales))return 2;
   for(unsigned at=0;at<5120;at+=256){
    PioneerQ38ProjectionInput actual;
    if(!PioneerQ38ProjectionQuantize(type,normalized.data()+at,256,&actual))return 2;
    for(unsigned lane=0;lane<256;lane++){
     const int q=expected[at+lane];
     // Independent definition of native sign-magnitude16. Do not ask the
     // packing helper under test to construct its own expected operands.
     const unsigned wanted=0x4000U|(q<0?0x8000U|unsigned(-q):unsigned(q));
     const unsigned got=(actual.operand[lane/2]>>((lane%2)*16))&65535;
     if(got==wanted)continue;
     laneDifferences++;
     if(firstReports++<16)printf("CPU_TRACE_QUANTIZATION_LANE position%u boundary%u type%u lane%u expected%04x actual%04x\n",
      position,layer,type,at+lane,wanted,got);
    }
    for(unsigned i=0;i<16;i++){
     const unsigned valid=type==8?8:1,width=type==8?32:256;
     const uint32_t wanted=i<valid?Q38CpuReferenceMath::Bits(scales[at/width+i]):0;
     if(actual.scale[i]==wanted)continue;
     scaleDifferences++;
     if(firstReports++<16)printf("CPU_TRACE_QUANTIZATION_SCALE position%u boundary%u type%u chunk%u word%u expected%08x actual%08x\n",
      position,layer,type,at/256,i,wanted,actual.scale[i]);
    }
    chunks++;
   }
   rows++;
  }
 }
 if(!original.Ready())return 2;
 const bool different=laneDifferences || scaleDifferences || normalizationDifferences;
 printf("%s_CPU_TRACE_QUANTIZATION_REPLAY rows%u chunks%u lanes%u scale_words%u lane_differences%u scale_differences%u normalization_differences%u;HOST_ARITHMETIC_ON_PINNED_CPU_INPUTS_ONLY_NOT_DEVICE_INPUTS_FULL_MODEL_NATIVE_WORK_OR_TPS\n",
  different?"FAIL":"PASS",rows,chunks,chunks*256,chunks*16,laneDifferences,scaleDifferences,normalizationDifferences);
 return different?1:0;
}
