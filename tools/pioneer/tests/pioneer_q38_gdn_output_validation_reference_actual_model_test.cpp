/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_OUTPUT_VALIDATION_CLIENT 1
#define PIONEER_Q38_CPU_VALIDATION 1
#include <cassert>
#include <cstdio>
#include <cstring>
#include <unistd.h>
static const char* actualModelPath=nullptr;
static FILE* OutputReferenceFopen(const char* path,const char* mode){
 assert(actualModelPath && !strcmp(path,"/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf") && !strcmp(mode,"rb"));
 return fopen(actualModelPath,mode);
}
static int OutputReferenceExecv(const char* path,char* const* argv){
 assert(!strcmp(path,"/bin/sha256sum") && !strcmp(argv[0],"sha256sum") && !argv[1]);
 char name[]="shasum",flag[]="-a",bits[]="256";char* args[]={name,flag,bits,nullptr};return execv("/usr/bin/shasum",args);
}
#define fopen OutputReferenceFopen
#define execv OutputReferenceExecv
#include "../PioneerQ38PublicModelSource.h"
#undef fopen
#undef execv
#include "../PioneerQ38GdnOutputValidationReference.h"
#include "../PioneerQ38CpuValidation.h"
#include <array>
int main(int argc,char** argv){
 assert(argc==2);actualModelPath=argv[1];setvbuf(stdout,nullptr,_IOLBF,0);
 puts("OUTPUT_DEBUG_REFERENCE_FULL_SAME_FILE_SHA_BEFORE_REFERENCES;NO_DEVICE_ACCESS_OR_GDN_OR_INFERENCE_SPEED");
 PioneerQ38PublicModelSource source;assert(source.Open() && source.Ready());
 puts("FULL_SAME_FILE_c600de03_SHA_AND_ALL851_TENSORS_PASS");
 const auto* tensor=source.Model().Find("blk.0.ssm_out.weight");assert(tensor);unsigned index=unsigned(tensor-source.Model().Tensors().data());
 for(unsigned trial=0;trial<4;trial++){
  std::array<uint32_t,6144> gated;std::array<float,6144> input;std::array<float,5120> actual;
  for(unsigned i=0;i<6144;i++){
   input[i]=trial==0?0.f:trial==1?float(int((i*37+13)%255)-127)*.03125f:
    trial==2?float(int((i*37+13)%255)-127)*0x1p-29f:(i%32==31?127.f:(i&1?-.5f:.5f));
   gated[i]=PioneerAttentionBits(input[i]);
  }
  auto before=gated;PioneerQ38GdnOutputReferenceResult result;
  assert(PioneerQ38GdnOutputReference(source,gated.data(),6144,&result) && gated==before && source.Ready());
  PioneerQ38CohortInput quantized;assert(PioneerQ38CohortQuantize(8,6144,input.data(),quantized));
  assert(!memcmp(result.operand,quantized.activation,sizeof(result.operand)) && !memcmp(result.scale,quantized.scales,sizeof(result.scale)));
  for(unsigned row=0;row<5120;row++)actual[row]=PioneerAttentionFloat(result.values[row]);
  assert(PioneerQ38CpuValidate(source,index,input.data(),6144,actual.data(),5120) && source.Ready());
  printf("PASS original public ssm_out 6144->5120 trial%u/ALL5120 rows BIT-EXACT second oracle/ALL3072 native operand words+192scales;CPU_ONLY\n",trial);
 }
 puts("PASS actual original public ssm_out weights/4 input domains/20480 rows;NOT actual GDN chain/physical card/model throughput");
}
