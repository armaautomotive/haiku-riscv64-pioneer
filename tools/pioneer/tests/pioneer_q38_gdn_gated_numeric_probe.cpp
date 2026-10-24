/* SPDX-License-Identifier: MIT */
// CPU-only diagnostic,not a native owner/Work/fallback or physical result.
#define PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_CLIENT 1
#define PIONEER_Q38_CPU_VALIDATION 1
#define PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE 1
#include <cassert>
#include <cstdio>
#include <cstring>
#include <unistd.h>
static const char* originalPath=nullptr;
static FILE* ProbeFopen(const char* path,const char* mode){
 assert(originalPath && !strcmp(path,"/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf") && !strcmp(mode,"rb"));
 return fopen(originalPath,mode);
}
static int ProbeExecv(const char* path,char* const* argv){
 assert(!strcmp(path,"/bin/sha256sum") && !strcmp(argv[0],"sha256sum") && !argv[1]);
 char name[]="shasum",flag[]="-a",bits[]="256";char* args[]={name,flag,bits,nullptr};return execv("/usr/bin/shasum",args);
}
#define fopen ProbeFopen
#define execv ProbeExecv
#include "../PioneerQ38PublicModelSource.h"
#undef fopen
#undef execv
#include "../PioneerQ38EmbeddingValidationReference.h"
#include "../PioneerQ38LearnedRmsValidationReference.h"
#include "../PioneerQ38GdnRendezvousValidationReference.h"
#include "pioneer_q38_gdn_gated_norm_bridge.h"
int main(int argc,char** argv){
 assert(argc==3 || argc==4);setvbuf(stdout,nullptr,_IOLBF,0);originalPath=argv[1];
 PioneerQ38PublicModelSource source;assert(source.Open() && source.Ready());
 puts("PROBE_FULL_ORIGINAL_GGUF_SHA_BEFORE_REFERENCES;NO_DEVICE_OR_NATIVE_CONTEXT");
 auto index=[&](const char* name){const auto* t=source.Model().Find(name);assert(t);return unsigned(t-source.Model().Tensors().data());};
 PioneerQwen38PackedPlan plan;assert(plan.Build(source.Model(),8192,true,true));
 std::vector<uint32_t> embedding(5120),normalized(5120),qkv(10240),conv(40960),alpha(48),beta(48),z(6144),reference(829632);
 assert(PioneerQ38EmbeddingReference(source,index("token_embd.weight"),0,embedding.data()));
 assert(PioneerQ38LearnedRmsReference(source,index("blk.0.attn_norm.weight"),embedding.data(),normalized.data()));
 assert(PioneerQ38ProjectionReference(source,6,normalized.data(),5120,qkv.data(),10240));
 assert(PioneerQ38ResidentDecoderReference(source,qkv.data(),10240,conv.data(),40960));
 assert(PioneerQ38ProjectionReference(source,10,normalized.data(),5120,alpha.data(),48));
 assert(PioneerQ38ProjectionReference(source,11,normalized.data(),5120,beta.data(),48));
 assert(PioneerQ38ProjectionReference(source,5,normalized.data(),5120,z.data(),6144));
 assert(PioneerQ38GdnRendezvousReference(source,conv.data(),40960,alpha.data(),beta.data(),48,z.data(),6144,reference.data(),829632));
 assert(reference[820975]==0x3a86371e); // authenticate the independently reproduced failing reference,not an execution result
 uint32_t norm[128],raw[128],output[64];
 assert(source.ReadReferencePacked(index("blk.0.ssm_norm.weight"),0,512,norm,512) && source.Ready());
 for(unsigned i=0;i<128;i++)raw[i]=reference[(94+i/64)*8642+8514+i%64];
 puts("PROBE_ORIGINAL_TOKEN0_HEAD47_PREFIX_PREPARED;QGN1_LOCAL_INSTRUCTIONS_ONLY_NEXT");
 double maximum=0;unsigned worst=0,failures=0;
 for(unsigned half=94;half<96;half++){
  ExecuteGdnGatedNormLocal(argv[2],half,raw,norm,z.data()+47*128,output);
  for(unsigned i=0;i<64;i++){
   unsigned at=(half-94)*64+i;uint32_t expected=reference[half*8642+8578+i];
   double e=PioneerAttentionFloat(expected),a=PioneerAttentionFloat(output[i]);assert(isfinite(a));
   double relative=fabs(e)>=double(PioneerAttentionFloat(0x00800000))?fabs(a-e)/fabs(e):a==0?0:INFINITY;
   if(relative>maximum){maximum=relative;worst=at;}if(relative>3e-6)failures++;
   if(at==49)printf("PROBE_SAME_ORIGINAL_INPUT lane49 raw%08x norm%08x z%08x expected%08x emitted%08x physical_prior3a86373a relative%.17g;NO_PHYSICAL_INPUTS_OBSERVED\n",
    raw[at],norm[at],z[47*128+at],expected,output[i],relative);
  }
 }
 assert(source.Ready());
 printf("PROBE_EXECUTION_COMPLETE head47_original_inputs128_emitted_outputs max_relative%.17g worst_lane%u failures_over_3ppm=%u;DIAGNOSTIC_ONLY_NOT_PHYSICAL_OR_MODEL_QUALIFICATION\n",maximum,worst,failures);
 if(argc==4){
  uint32_t context[392];FILE* captured=fopen(argv[3],"rb");assert(captured);
  assert(fread(context,1,sizeof(context),captured)==sizeof(context) && fgetc(captured)==EOF && !fclose(captured));
  assert(context[0]==0x31504351 && context[1]==1 && context[2]==47 && context[3]==49 && context[4]==820975 && !context[5]
   && context[6]==reference[820975]);
  for(unsigned i=0;i<128;i++){
   assert(context[8+i]==raw[i] && context[264+i]==z[47*128+i]); // exact original-file correlation,NOT trusted log identity
   for(unsigned at:{8U,136U,264U}){uint32_t magnitude=context[at+i]&0x7fffffff;
    assert(magnitude<=0x477fe000 && (!magnitude || magnitude>=0x00800000));
   }
  }
  uint32_t emitted[128];
  for(unsigned half=94;half<96;half++)ExecuteGdnGatedNormLocal(argv[2],half,context+136,norm,context+264,emitted+(half-94)*64);
  double a=PioneerAttentionFloat(emitted[49]),physical=PioneerAttentionFloat(context[7]);
  assert(isfinite(physical) && source.Ready());
  printf("PROBE_RETURNED_RAW_CONTEXT head47 lane49 original_raw%08x returned_raw%08x original_norm%08x original_z%08x original_gated%08x emitted_with_returned_raw%08x physical_returned_gated%08x bit_equal%u residual_relative%.17g;ASSUMES_UNOBSERVED_PHYSICAL_Z_NORM_UNCHANGED_LOCAL_NOT_FUSED_PATH_NO_NATIVE_VALIDATION_OR_WORK_GRANT\n",
   raw[49],context[136+49],norm[49],z[47*128+49],context[6],emitted[49],context[7],unsigned(emitted[49]==context[7]),physical?fabs(a-physical)/fabs(physical):a==0?0:INFINITY);
 }
}
