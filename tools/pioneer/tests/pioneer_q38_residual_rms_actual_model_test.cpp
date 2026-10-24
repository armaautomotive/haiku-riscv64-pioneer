/* SPDX-License-Identifier: MIT */
// Actual QRR1 instructions with fully authenticated ORIGINAL role1 weights.
// Inputs/NoC/GDDR remain test fixtures,not a physical/full-model benchmark.
#define PIONEER_Q38_CPU_VALIDATION 1
#define PIONEER_Q38_GDN_MUTABLE_STATE
#include <cassert>
#include <cstdio>
#include <cstring>
#include <unistd.h>
static const char* actualModelPath=nullptr;
static FILE* ResidualModelFopen(const char* path,const char* mode){
 assert(actualModelPath && !strcmp(path,"/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf") && !strcmp(mode,"rb"));
 return fopen(actualModelPath,mode);
}
static int ResidualModelExecv(const char* path,char* const* argv){
 assert(!strcmp(path,"/bin/sha256sum") && !strcmp(argv[0],"sha256sum") && !argv[1]);
 char name[]="shasum",flag[]="-a",bits[]="256";char* args[]={name,flag,bits,nullptr};return execv("/usr/bin/shasum",args);
}
#define fopen ResidualModelFopen
#define execv ResidualModelExecv
#include "../PioneerQ38PublicModelSource.h"
#undef fopen
#undef execv
#define PIONEER_Q38_RESIDUAL_RMS_MACHINE_ONLY
#include "pioneer_q38_residual_rms_image_test.cpp"
int main(int argc,char** argv){
 assert(argc==3);actualModelPath=argv[2];setvbuf(stdout,nullptr,_IOLBF,0);(void)&ResidualRmsPlanTests;
 puts("QRR1_FULL_SAME_FILE_PUBLIC_SHA_REQUIRED_BEFORE_ANY_INSTRUCTION_EXECUTION;NO_DEVICE_ACCESS");
 PioneerQ38PublicModelSource source;assert(source.Open() && source.Ready());
 const auto* tensor=source.Model().Find("blk.0.post_attention_norm.weight");
 assert(tensor && !tensor->type && tensor->rank==1 && tensor->inputs==5120 && tensor->outputs==1 && tensor->bytes==20480);
 unsigned index=unsigned(tensor-source.Model().Tensors().data());std::array<uint8_t,20480> original;
 assert(source.ReadReferencePacked(index,0,unsigned(original.size()),original.data(),unsigned(original.size())) && source.Ready());
 puts("PASS full SAME original public c600de03 SHA/all851 tensors/ORIGINAL post_attention_norm5120 weights BEFORE execution");
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 unsigned maxStack=0;
 for(unsigned trial:{0U,2U,3U,5U,7U,8U,9U}){
  ResidualRmsMachine m(argv[1],*canonical,trial&1?248319:0,trial%7,trial);
  for(unsigned i=0;i<5120;i++){const auto* p=original.data()+i*4;m.weight[i]=uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);}
  assert(source.Ready() && m.Run() && source.Ready());m.Verify();maxStack=std::max(maxStack,0x70000-m.minStack);
  printf("PASS actual QRR1 ORIGINAL post_attention_norm trial%u/ALL5120 residual+normalized BIT-EXACT independent F32-add/double-reduction/sqrtf;NO CPU products supplied to device\n",trial);
 }
 printf("PASS QRR1 original learned5120 weights/7fixture input domains/35840residual+35840normalized words/maxStack%u;NOT actual public GDN chain/physical card/TPS\n",maxStack);
}
