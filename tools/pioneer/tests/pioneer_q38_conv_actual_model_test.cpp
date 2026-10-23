/* SPDX-License-Identifier: MIT */
// Separate DEBUG OFFLINE numerical oracle ONLY. Real original learned
// bytes and independent QKV reference feed the simulated GDDR endpoint;
// this is NOT a private native QPS1 producer/owner capability or Work path.
#define PIONEER_Q38_CONV_MACHINE_ONLY
#include "pioneer_q38_conv_image_test.cpp"
#define PIONEER_Q38_PROJECTION_VALIDATION_CLIENT 1
#define PIONEER_Q38_CPU_VALIDATION 1
#define PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE 1
#include <unistd.h>
static const char* actualModelPath=nullptr;
static FILE* ActualConvModelFopen(const char* path,const char* mode){
 assert(actualModelPath && !strcmp(path,"/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf") && !strcmp(mode,"rb"));
 return fopen(actualModelPath,mode);
}
static int ActualConvModelExecv(const char* path,char* const* argv){
 assert(!strcmp(path,"/bin/sha256sum") && !strcmp(argv[0],"sha256sum") && !argv[1]);
 char name[]="shasum",flag[]="-a",bits[]="256";char* args[]={name,flag,bits,nullptr};return execv("/usr/bin/shasum",args);
}
#define fopen ActualConvModelFopen
#define execv ActualConvModelExecv
#include "../PioneerQ38PublicModelSource.h"
#undef fopen
#undef execv
#include "../PioneerQ38EmbeddingValidationReference.h"
#include "../PioneerQ38LearnedRmsValidationReference.h"
#include "../PioneerQ38ProjectionValidationReference.h"
int main(int argc,char** argv){
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);actualModelPath=argv[2];(void)&Plans;(void)&Sequence;
 puts("DEBUG_OFFLINE_QCV1_FULL_SAME_PUBLIC_GGUF_SHA_BEFORE_SIMULATED_IMAGE;NO_DEVICE_OR_NATIVE_OWNER");
 PioneerQ38PublicModelSource original;assert(original.Open() && original.Ready());
 puts("FULL_SAME_FILE_c600de03_SHA_AND_ALL851_TENSORS_PASS");
 auto* e=original.Model().Find("token_embd.weight");auto* n=original.Model().Find("blk.0.attn_norm.weight");
 auto* w=original.Model().Find("blk.0.ssm_conv1d.weight");assert(e && n && w && !w->type && w->inputs==4 && w->outputs==10240);
 unsigned ei=unsigned(e-original.Model().Tensors().data()),ni=unsigned(n-original.Model().Tensors().data()),wi=unsigned(w-original.Model().Tensors().data());
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 std::array<uint32_t,5120> raw,normalized;std::vector<uint32_t> qkv(10240),current(30720,0),next(30720),history(30720,0);
 unsigned step=0,jobs=0;
 for(unsigned token:{0U,15U,248319U}){
  assert(PioneerQ38EmbeddingReference(original,ei,token,raw.data()) && PioneerQ38LearnedRmsReference(original,ni,raw.data(),normalized.data())
   && PioneerQ38ProjectionReference(original,6,normalized.data(),5120,qkv.data(),10240));
  for(unsigned first=0;first<10240;first+=16){
   std::array<uint8_t,256> learned;assert(original.ReadReferencePacked(wi,uint64_t(first)*16,256,learned.data(),256) && original.Ready());
   ConvMachine m(argv[1],*canonical,0,first,0,(first/16)%120,0,step);
   for(unsigned i=0;i<16;i++)m.input[i]=qkv[first+i];
   for(unsigned i=0;i<64;i++){const auto* p=learned.data()+i*4;m.weight[i]=uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);}
   for(unsigned i=0;i<48;i++)m.previous[i]=current[first*3+i];
   assert(m.Run());m.Verify();
   for(unsigned c=0;c<16;c++){
    unsigned channel=first+c;float sum=0;
    for(unsigned t=0;t<3;t++)sum+=PioneerAttentionFloat(history[channel*3+t])*PioneerAttentionFloat(m.weight[c*4+t]);
    sum+=PioneerAttentionFloat(qkv[channel])*PioneerAttentionFloat(m.weight[c*4+3]);
    double reference=double(sum)/(1+exp(-double(sum))),actual=PioneerAttentionFloat(m.output[c]);
    assert(fabs(actual-reference)<=3e-6*(1+fabs(reference)));
    for(unsigned t=0;t<3;t++)next[channel*3+t]=m.next[c*3+t];
    history[channel*3]=history[channel*3+1];history[channel*3+1]=history[channel*3+2];history[channel*3+2]=qkv[channel];
   }jobs++;
  }
  assert(next==history && original.Ready());current.swap(next);
  printf("PASS QCV1 ACTUAL original GGUF token%u/position%u/ALL10240Conv4+SiLU outputs/30720next-state words,original F32 weights+independent QKV reference,actual emitted RV32 math+5NoCpackets perjob\n",token,step++);
 }
 printf("PASS DEBUG original-model QCV1 instruction numerical regression3tokens/%ujobs/30720outputs/92160next-state words;SIMULATED_HARDWARE_NOT_NATIVE_QPS1_OWNER_INTEGRATION_OR_SPEED\n",jobs);
}
