/* SPDX-License-Identifier: MIT */
// CPU-only localization: replay arithmetic on authenticated CPU trace inputs.
// These outputs are NEVER accelerator observations or inference inputs.
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(__FAST_MATH__)
#error CPU trace replay is not kernel, native, fast-math or Work code
#endif
#include "pioneer_q38_whole_original_bytes.h"
#include "../PioneerQ38F32HiddenRms.h"
#include <array>
#include <cfenv>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

static bool ReadInputs(const char* path,std::vector<uint32_t>& words){
 static_assert(__BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__,"fixed replay byte order");
 FILE* file=fopen(path,"rb");if(!file)return false;
 struct stat before={},after={};
 constexpr size_t Count=16+3*65*5120;
 bool okay=!fstat(fileno(file),&before) && S_ISREG(before.st_mode) && before.st_size==off_t(Count*4);
 if(okay){words.resize(Count);okay=fread(words.data(),4,Count,file)==Count && fgetc(file)==EOF && !ferror(file);}
 okay=okay && !fstat(fileno(file),&after) && before.st_dev==after.st_dev && before.st_ino==after.st_ino
  && before.st_size==after.st_size && before.st_mtime==after.st_mtime && before.st_ctime==after.st_ctime;
#ifdef __APPLE__
 okay=okay && before.st_mtimespec.tv_nsec==after.st_mtimespec.tv_nsec && before.st_ctimespec.tv_nsec==after.st_ctimespec.tv_nsec;
#endif
 if(fclose(file))okay=false;
 if(!okay)return false;
 const uint32_t header[]={0x31524d51,1,3,65,5120,0,0,0,0,0,0,0,0,0,0,0};
 return !memcmp(words.data(),header,sizeof(header));
}
int main(int argc,char** argv){
 (void)&PioneerQ38LearnedRms;(void)&PioneerQ38F32HiddenResidualRms;
 if(argc!=3 || std::fegetround()!=FE_TONEAREST)return 2;
 setvbuf(stdout,nullptr,_IOLBF,0);
 std::vector<uint32_t> inputs;if(!ReadInputs(argv[2],inputs))return 2;
 WholeModelOriginalBytes original;
 if(!original.Open(argv[1]) || !original.Ready())return 2;
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
 unsigned rows=0,differentRows=0,differentWords=0,meanDifferences=0;
 std::vector<float> hidden(5120),weight(5120),actual(5120);
 for(unsigned position=0;position<3;position++)for(unsigned layer=0;layer<65;layer++){
  memcpy(hidden.data(),inputs.data()+16+(position*65+layer)*5120,20480);
  if(!original.ReadOriginal(tensor[layer],0,20480,weight.data(),20480) || !original.Ready())return 2;
  double sum=0;PioneerQ38RmsSum deviceSum;
  for(unsigned i=0;i<5120;i++){
   if(!PioneerQ38F32HiddenDomain(hidden[i]) || !PioneerQ38RmsDomain(weight[i]))return 2;
   const float square=hidden[i]*hidden[i];sum+=double(square);if(!deviceSum.Add(square))return 2;
  }
  const float mean=float(sum/5120.0),scale=1.f/std::sqrt(mean+1e-6f);float deviceMean=0;
  if(!deviceSum.Mean5120(deviceMean) || !PioneerQ38F32HiddenRms(hidden.data(),weight.data(),actual.data()))return 2;
  const bool meanDifferent=PioneerAttentionBits(mean)!=PioneerAttentionBits(deviceMean);
  unsigned differences=0,first=5120,wantedFirst=0,actualFirst=0;
  for(unsigned i=0;i<5120;i++){
   const float expected=(hidden[i]*scale)*weight[i];
   if(PioneerAttentionBits(expected)==PioneerAttentionBits(actual[i]))continue;
   if(first==5120){first=i;wantedFirst=PioneerAttentionBits(expected);actualFirst=PioneerAttentionBits(actual[i]);}
   differences++;
  }
  if(differences || meanDifferent)printf("CPU_TRACE_RMS_DIFFERENCE position%u input_boundary%u words%u mean%08x/%08x first%u expected%08x actual%08x\n",
   position,layer,differences,PioneerAttentionBits(mean),PioneerAttentionBits(deviceMean),first,wantedFirst,actualFirst);
  rows++;differentRows+=differences!=0;differentWords+=differences;meanDifferences+=meanDifferent;
 }
 if(!original.Ready())return 2;
 printf("%s_CPU_TRACE_RMS_REPLAY rows%u words%u different_rows%u different_words%u mean_differences%u;HOST_ARITHMETIC_ON_PINNED_CPU_INPUTS_ONLY_NOT_DEVICE_HIDDEN_OR_POST_ATTENTION_RMS_NATIVE_WORK_OR_TPS\n",
  differentRows || meanDifferences?"FAIL":"PASS",rows,rows*5120,differentRows,differentWords,meanDifferences);
 return differentRows || meanDifferences?1:0;
}
