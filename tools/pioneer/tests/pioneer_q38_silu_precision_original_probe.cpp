/* SPDX-License-Identifier: MIT */
// ORIGINAL source + independent CPU references;LOCAL emitted diagnostics
// live in separate TUs. NO native owner/device FD/Work/fallback.
#define PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_CLIENT 1
#define PIONEER_Q38_CPU_VALIDATION 1
#define PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE 1
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <unistd.h>
static const char* originalPath=nullptr;
static FILE* PrecisionFopen(const char* path,const char* mode){
 assert(originalPath && !strcmp(path,"/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf") && !strcmp(mode,"rb"));return fopen(originalPath,mode);
}
static int PrecisionExecv(const char* path,char* const* argv){
 assert(!strcmp(path,"/bin/sha256sum") && !strcmp(argv[0],"sha256sum") && !argv[1]);
 char name[]="shasum",flag[]="-a",bits[]="256";char* args[]={name,flag,bits,nullptr};return execv("/usr/bin/shasum",args);
}
#define fopen PrecisionFopen
#define execv PrecisionExecv
#include "../PioneerQ38PublicModelSource.h"
#undef fopen
#undef execv
#include "../PioneerQ38EmbeddingValidationReference.h"
#include "../PioneerQ38LearnedRmsValidationReference.h"
#include "../PioneerQ38GdnRendezvousValidationReference.h"
#include "pioneer_q38_silu_precision_probe_bridge.h"
#include "pioneer_q38_gdn_gated_norm_bridge.h"
#ifdef PIONEER_Q38_CONV_PRECISION_PROBE_ENABLED
#include "pioneer_q38_conv_batch_precision_probe_bridge.h"
#endif
using Prepared=std::array<std::array<uint32_t,322>,2>;
static std::array<uint32_t,128> Reconstruct(const Prepared& prepared){
 std::array<uint32_t,128> output={};
 for(unsigned lane=0;lane<128;lane++){
  const auto& p=prepared[lane/64];float delta=PioneerAttentionFloat(p[256+lane%64])*PioneerAttentionFloat(p[321]),sum=0;
  for(unsigned key=0;key<128;key++){float state=PioneerAttentionFloat(p[128+key])*delta;sum+=state*PioneerAttentionFloat(p[key]);}
  output[lane]=PioneerAttentionBits(sum*.0883883461356163f);
 }return output;
}
int main(int argc,char** argv){
 assert(argc==7 || argc==8);setvbuf(stdout,nullptr,_IOLBF,0);originalPath=argv[1];
 PioneerQ38PublicModelSource source;assert(source.Open() && source.Ready());
 puts("SILU_PROBE_FULL_ORIGINAL_C600_SOURCE_AUTH_BEFORE_REFERENCES;NO_DEVICE_OR_NATIVE_CONTEXT");
 auto index=[&](const char* name){const auto* t=source.Model().Find(name);assert(t);return unsigned(t-source.Model().Tensors().data());};
 std::vector<uint32_t> embedding(5120),normalized(5120),qkv(10240),conv(40960),alpha(48),beta(48),z(6144),reference(829632);
 assert(PioneerQ38EmbeddingReference(source,index("token_embd.weight"),0,embedding.data())
  && PioneerQ38LearnedRmsReference(source,index("blk.0.attn_norm.weight"),embedding.data(),normalized.data())
  && PioneerQ38ProjectionReference(source,6,normalized.data(),5120,qkv.data(),10240)
  && PioneerQ38ResidentDecoderReference(source,qkv.data(),10240,conv.data(),40960)
  && PioneerQ38ProjectionReference(source,10,normalized.data(),5120,alpha.data(),48)
  && PioneerQ38ProjectionReference(source,11,normalized.data(),5120,beta.data(),48)
  && PioneerQ38ProjectionReference(source,5,normalized.data(),5120,z.data(),6144)
  && PioneerQ38GdnRendezvousReference(source,conv.data(),40960,alpha.data(),beta.data(),48,z.data(),6144,reference.data(),829632));
 assert(reference[820975]==0x3a86371e);
 std::array<uint32_t,1936> captured;FILE* file=fopen(argv[6],"rb");assert(file);
 assert(fread(captured.data(),1,sizeof(captured),file)==sizeof(captured) && fgetc(file)==EOF && !fclose(file));
 assert(captured[0]==0x31505343 && captured[1]==1 && captured[2]==47 && captured[3]==49 && captured[4]==820975
  && captured[5]==reference[820975] && !captured[7]);
 for(unsigned side=0;side<2;side++)for(unsigned i=0;i<322;i++)assert(captured[8+side*322+i]==reference[(94+side)*8642+i]);
 for(unsigned i=0;i<128;i++)assert(captured[1296+i]==reference[(94+i/64)*8642+8514+i%64] && captured[1552+i]==z[47*128+i]);
 for(unsigned key=0;key<128;key++)assert(captured[1680+key]==reference[94*8642+322+key*64+49]);
 std::vector<uint8_t> weights(163840);assert(source.ReadReferencePacked(index("blk.0.ssm_conv1d.weight"),0,unsigned(weights.size()),weights.data(),unsigned(weights.size())));
 std::vector<uint32_t> sums(10240),legacy(10240),precise(10240),originalWeights(40960);
 for(unsigned c=0;c<10240;c++){
  float sum=0;for(unsigned tap=0;tap<4;tap++){const auto* b=weights.data()+(c*4+tap)*4;
   uint32_t w=uint32_t(b[0])|(uint32_t(b[1])<<8)|(uint32_t(b[2])<<16)|(uint32_t(b[3])<<24);
   originalWeights[c*4+tap]=w;
   sum+=(tap==3?PioneerAttentionFloat(qkv[c]):0.f)*PioneerAttentionFloat(w);
  }sums[c]=PioneerAttentionBits(sum);
 }
 unsigned legacyConvDifferent=0,preciseConvDifferent=0,preciseMaxUlp=0;
 for(unsigned first=0;first<10240;first+=64){
  ExecuteSiluPrecisionProbe(argv[2],sums.data()+first,legacy.data()+first);
  ExecuteSiluPrecisionProbe(argv[3],sums.data()+first,precise.data()+first);
  for(unsigned i=first;i<first+64;i++){
   legacyConvDifferent+=legacy[i]!=conv[i];preciseConvDifferent+=precise[i]!=conv[i];
   assert((precise[i]>>31)==(conv[i]>>31));unsigned distance=precise[i]>conv[i]?precise[i]-conv[i]:conv[i]-precise[i];
   preciseMaxUlp=std::max(preciseMaxUlp,distance);assert(distance<=1);
  }
 }
#ifdef PIONEER_Q38_CONV_PRECISION_PROBE_ENABLED
 if(argc==8){
  std::vector<uint32_t> emitted(10240),next(30720);
  auto stats=ExecuteConvBatchPrecisionProbe(argv[7],qkv.data(),originalWeights.data(),emitted.data(),next.data());
  for(unsigned c=0;c<10240;c++)assert(emitted[c]==precise[c] && emitted[c]==conv[c]
   && !next[c*3] && !next[c*3+1] && next[c*3+2]==qkv[c]);
  precise.swap(emitted); // subsequent probe uses ACTUAL QCB1 output,not QSP1 math
  printf("SILU_ACTUAL_QCB1_ORIGINAL workers%u/groups%u/conv10240BIT_EXACT/next30720BIT_EXACT/maxStack%u/maxSteps%u;NO_NATIVE_COORDINATOR_OR_PHYSICAL_TIMING\n",
   stats.workers,stats.groups,stats.maxStack,stats.maxSteps);
 }
#else
 assert(argc==7);
#endif
 uint32_t negativeA[48],dt[48],norm[128];
 assert(source.ReadReferencePacked(index("blk.0.ssm_a"),0,192,negativeA,192)
  && source.ReadReferencePacked(index("blk.0.ssm_dt.bias"),0,192,dt,192)
  && source.ReadReferencePacked(index("blk.0.ssm_norm.weight"),0,512,norm,512));
 auto prepare=[&](const std::vector<uint32_t>& activated,unsigned head,Prepared& output){
  for(unsigned side=0;side<2;side++){
   std::array<uint32_t,324> input;
   for(unsigned i=0;i<128;i++){input[i]=activated[(head%16)*128+i];input[128+i]=activated[2048+(head%16)*128+i];}
   for(unsigned i=0;i<64;i++)input[256+i]=activated[4096+head*128+side*64+i];
   input[320]=alpha[head];input[321]=beta[head];input[322]=negativeA[head];input[323]=dt[head];
   ExecuteUnscaledPrepareProbe(argv[4],head*2+side,input.data(),output[side].data());
  }
 };
 Prepared legacyPrepared,precisePrepared;prepare(legacy,47,legacyPrepared);prepare(precise,47,precisePrepared);
 unsigned legacyPreparedDifferent=0,precisePreparedDifferent=0;
 for(unsigned side=0;side<2;side++)for(unsigned i=0;i<322;i++){
  legacyPreparedDifferent+=legacyPrepared[side][i]!=captured[652+side*322+i];
  precisePreparedDifferent+=precisePrepared[side][i]!=captured[8+side*322+i];
 }
 auto legacyRaw=Reconstruct(legacyPrepared),preciseRaw=Reconstruct(precisePrepared);unsigned legacyRawDifferent=0,preciseRawDifferent=0,legacyStateDifferent=0;
 for(unsigned i=0;i<128;i++){legacyRawDifferent+=legacyRaw[i]!=captured[1424+i];preciseRawDifferent+=preciseRaw[i]!=captured[1296+i];
  float delta=PioneerAttentionFloat(legacyPrepared[0][305])*PioneerAttentionFloat(legacyPrepared[0][321]);
  legacyStateDifferent+=PioneerAttentionBits(PioneerAttentionFloat(legacyPrepared[0][128+i])*delta)!=captured[1808+i];
 }
 std::array<uint32_t,128> legacyGated,preciseGated;
 for(unsigned side=0;side<2;side++){
  ExecuteGdnGatedNormLocal(argv[5],94+side,legacyRaw.data(),norm,z.data()+47*128,legacyGated.data()+side*64);
  ExecuteGdnGatedNormLocal(argv[5],94+side,preciseRaw.data(),norm,z.data()+47*128,preciseGated.data()+side*64);
 }
 double maximum=0;unsigned worst=0;
 for(unsigned i=0;i<128;i++){
  double expected=PioneerAttentionFloat(reference[(94+i/64)*8642+8578+i%64]),actual=PioneerAttentionFloat(preciseGated[i]);
  double error=fabs(expected)>=PioneerAttentionFloat(0x00800000)?fabs(actual-expected)/fabs(expected):actual==0?0:INFINITY;
  if(error>maximum){maximum=error;worst=i;}
 }
 printf("SILU_ORIGINAL10240 legacy_different%u precise_different%u precise_maxULP%u\n",legacyConvDifferent,preciseConvDifferent,preciseMaxUlp);
 printf("SILU_HEAD47 legacy_vs_physical_prepared_different%u raw_different%u selected_NEXT_row_different%u;precise_vs_original_prepared_different%u raw_different%u\n",
  legacyPreparedDifferent,legacyRawDifferent,legacyStateDifferent,precisePreparedDifferent,preciseRawDifferent);
 printf("SILU_HEAD47_LANE49 legacy_raw%08x physical_raw%08x precise_raw%08x original_raw%08x legacy_gated%08x physical_gated%08x precise_gated%08x original_gated%08x max_precise_relative%.17g worst%u;LOCAL_COUNTERFACTUAL_NOT_PHYSICAL_FIX_OR_NATIVE_QUALIFICATION\n",
  legacyRaw[49],captured[1424+49],preciseRaw[49],captured[1296+49],legacyGated[49],captured[6],preciseGated[49],captured[5],maximum,worst);
 assert(!legacyPreparedDifferent && !legacyRawDifferent && !legacyStateDifferent && legacyGated[49]==captured[6]);
 unsigned allPreparedDifferent=0,allRawDifferent=0,allStateDifferent=0,gatedWords=0,worstHead=0,worstLane=0;
 double allMaximum=0;
 for(unsigned head=0;head<48;head++){
  Prepared prepared;prepare(precise,head,prepared);auto raw=Reconstruct(prepared);
  std::array<uint32_t,128> gated;
  for(unsigned side=0;side<2;side++){
   unsigned base=(head*2+side)*8642;
   for(unsigned i=0;i<322;i++)allPreparedDifferent+=prepared[side][i]!=reference[base+i];
   // Independent CPU counterfactual observes the zero-state recurrence;
   // this is NOT an execution claim for the recurrent device image.
   for(unsigned lane=0;lane<64;lane++){
    float delta=PioneerAttentionFloat(prepared[side][256+lane])*PioneerAttentionFloat(prepared[side][321]);
    for(unsigned key=0;key<128;key++)allStateDifferent+=PioneerAttentionBits(PioneerAttentionFloat(prepared[side][128+key])*delta)!=reference[base+322+key*64+lane];
   }
   ExecuteGdnGatedNormLocal(argv[5],head*2+side,raw.data(),norm,z.data()+head*128,gated.data()+side*64);
   for(unsigned lane=0;lane<64;lane++){
    unsigned at=side*64+lane;allRawDifferent+=raw[at]!=reference[base+8514+lane];
    double expected=PioneerAttentionFloat(reference[base+8578+lane]),actual=PioneerAttentionFloat(gated[at]);
    double error=fabs(expected)>=PioneerAttentionFloat(0x00800000)?fabs(actual-expected)/fabs(expected):actual==0?0:INFINITY;
    if(error>allMaximum){allMaximum=error;worstHead=head;worstLane=at;}gatedWords++;
   }
  }
 }
 printf("SILU_ALL48_HEADS precise_prepared_different%u zero_state_counterfactual_different%u raw_different%u emitted_QGN_words%u max_relative%.17g worst_head%u/lane%u;NOT_EMITTED_RECURRENT_OR_PHYSICAL_PROOF\n",
  allPreparedDifferent,allStateDifferent,allRawDifferent,gatedWords,allMaximum,worstHead,worstLane);
 assert(gatedWords==6144 && allMaximum<=3e-6);
 assert(maximum<=3e-6 && source.Ready());
 puts("PASS ORIGINAL_SOURCE_LOCAL_LEGACY_REPRODUCER_AND_PRECISE_COUNTERFACTUAL;NO_DEVICE_FD_NATIVE_PRODUCER_WORK_OR_SPEED_GRANT;UNOBSERVED_PHYSICAL_INPUTS_REMAIN_UNPROVEN");
}
