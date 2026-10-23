/* SPDX-License-Identifier: MIT */
// Actual independent DEBUG-client reference against ORIGINAL requested
// GGUF and the separately qualified row oracle. No GPU context/FD exists.
#define PIONEER_Q38_PROJECTION_VALIDATION_CLIENT 1
#define PIONEER_Q38_CPU_VALIDATION 1
#define PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE 1
#include <cassert>
#include <cstdio>
#include <cstring>
#include <unistd.h>
static const char* actualModelPath=nullptr;
static FILE* ActualProjectionReferenceFopen(const char* path,const char* mode){
 assert(actualModelPath && !strcmp(path,"/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf") && !strcmp(mode,"rb"));
 return fopen(actualModelPath,mode);
}
static int ActualProjectionReferenceExecv(const char* path,char* const* argv){
 assert(!strcmp(path,"/bin/sha256sum") && !strcmp(argv[0],"sha256sum") && !argv[1]);
 char name[]="shasum",flag[]="-a",bits[]="256";char* args[]={name,flag,bits,nullptr};return execv("/usr/bin/shasum",args);
}
#define fopen ActualProjectionReferenceFopen
#define execv ActualProjectionReferenceExecv
#include "../PioneerQ38PublicModelSource.h"
#undef fopen
#undef execv
#include "../PioneerQ38EmbeddingValidationReference.h"
#include "../PioneerQ38LearnedRmsValidationReference.h"
#include "../PioneerQ38ProjectionValidationReference.h"
#include "../PioneerQ38CpuValidation.h"
#include <array>
int main(int argc,char** argv){
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);actualModelPath=argv[1];
 puts("DEBUG_CLIENT_REFERENCE_ACTUAL_FULL_SAME_FILE_SHA_BEFORE_REFERENCES;NO_DEVICE_ACCESS_OR_INFERENCE_SPEED");
 PioneerQ38PublicModelSource original;assert(original.Open() && original.Ready());
 puts("FULL_SAME_FILE_c600de03_SHA_AND_ALL851_TENSORS_PASS");
 auto* embedding=original.Model().Find("token_embd.weight");auto* norm=original.Model().Find("blk.0.attn_norm.weight");assert(embedding && norm);
 unsigned embeddingIndex=unsigned(embedding-original.Model().Tensors().data()),normIndex=unsigned(norm-original.Model().Tensors().data());
 unsigned projections=0,words=0;
 for(unsigned token:{0U,15U,248319U}){
  std::array<uint32_t,5120> raw,normalized;std::array<float,5120> input;
  assert(PioneerQ38EmbeddingReference(original,embeddingIndex,token,raw.data())
   && PioneerQ38LearnedRmsReference(original,normIndex,raw.data(),normalized.data()));
  auto before=normalized;for(unsigned i=0;i<5120;i++)input[i]=PioneerAttentionFloat(normalized[i]);
  for(unsigned role:{10U,11U,5U,6U}){
   unsigned rows=PioneerQ38ProjectionResultRows(role);std::vector<uint32_t> bits(rows);std::vector<float> values(rows);
   assert(PioneerQ38ProjectionReference(original,role,normalized.data(),5120,bits.data(),rows) && normalized==before);
   for(unsigned i=0;i<rows;i++)values[i]=PioneerAttentionFloat(bits[i]);
   PioneerQ38Tensor expected;assert(PioneerQ38Expected(0,role,expected));auto* tensor=original.Model().Find(expected.name.c_str());assert(tensor);
   assert(PioneerQ38CpuValidate(original,unsigned(tensor-original.Model().Tensors().data()),input.data(),5120,values.data(),rows) && original.Ready());
   words+=rows;projections++;
   printf("PASS actual DEBUG-client original-source reference token%u/role%u/ALL%uoutputs BIT-EXACT to separate qualified row oracle\n",token,role,rows);
  }
 }
 printf("PASS actual SAME public GGUF DEBUG-client independent reference ALL%uprojections/%uwords;NO native/FD/device inputs-or-outputs/fallback/model-speed claim\n",projections,words);
}
