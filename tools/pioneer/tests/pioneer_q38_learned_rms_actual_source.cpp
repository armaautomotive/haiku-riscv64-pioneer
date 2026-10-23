/* SPDX-License-Identifier: MIT */
// OFFLINE host oracle TU ONLY. No native/controller/device headers or code.
#define PIONEER_Q38_CPU_VALIDATION 1
#define PIONEER_Q38_LEARNED_RMS_VALIDATION_CLIENT 1
#define PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE 1
#include "pioneer_q38_learned_rms_actual_source.h"
#include "../PioneerQwen38PackedPlan.h"
#include <cassert>
#include <unistd.h>
static const char* actualPath=nullptr;
static FILE* ActualFopen(const char* path,const char* mode){
 assert(actualPath && !strcmp(path,"/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf") && !strcmp(mode,"rb"));return fopen(actualPath,mode);
}
static int ActualExecv(const char* path,char* const* argv){
 assert(!strcmp(path,"/bin/sha256sum") && !strcmp(argv[0],"sha256sum") && !argv[1]);
 char name[]="shasum",flag[]="-a",bits[]="256";char* args[]={name,flag,bits,nullptr};return execv("/usr/bin/shasum",args);
}
#define fopen ActualFopen
#define execv ActualExecv
#include "../PioneerQ38PublicModelSource.h"
#undef fopen
#undef execv
#include "../PioneerQ38EmbeddingValidationReference.h"
#include "../PioneerQ38LearnedRmsValidationReference.h"
struct RmsActualSource::Original {
 PioneerQ38PublicModelSource source;PioneerQwen38PackedPlan plan;unsigned embedding=UINT32_MAX,norm=UINT32_MAX;
};
RmsActualSource::RmsActualSource():original(new Original){}
RmsActualSource::~RmsActualSource(){delete original;}
bool RmsActualSource::Ready()const{return original && original->source.Ready() && original->plan.MatchesSource(original->source.Model());}
bool RmsActualSource::Open(const char* path){
 if(actualPath || !path)return false;actualPath=path;auto& o=*original;
 if(!o.source.Open() || !o.source.Ready() || !o.plan.Build(o.source.Model(),8192,true,true))return false;
 for(unsigned i=0;i<o.source.Model().Tensors().size();i++){
  const auto& t=o.source.Model().Tensors()[i];if(t.name=="token_embd.weight")o.embedding=i;if(t.name=="blk.0.attn_norm.weight")o.norm=i;
 }
 return Ready() && o.embedding!=UINT32_MAX && o.norm!=UINT32_MAX;
}
bool RmsActualSource::Token(unsigned token){
 if(!Ready() || token>=248320)return false;auto& o=*original;unsigned found=0,norms=0;
 for(unsigned i=0;i<o.plan.Parts().size();i++){
  const auto& p=o.plan.Parts()[i];if(p.kind!=PIONEER_Q38_WEIGHT)continue;
  if(p.tensor==o.embedding && token>=p.firstRow && token-p.firstRow<p.rows){
   unsigned offset=(token-p.firstRow)/16*46080,first=offset/4096,within=offset%4096;
   std::vector<uint8_t> pages(((within+46080+4095)/4096)*4096);
   for(unsigned page=0;page<pages.size()/4096;page++)if(!o.source.ReadWeightPage(o.plan,i,first+page,pages.data()+page*4096,4096))return false;
   memcpy(embedding.data(),pages.data()+within,46080);found++;
  }
  if(p.tensor==o.norm){
   if(p.bytes!=20480 || p.allocated!=20480 || p.rows!=1 || p.firstRow)return false;
   std::array<uint8_t,20480> packed;
   for(unsigned page=0;page<5;page++)if(!o.source.ReadWeightPage(o.plan,i,page,packed.data()+page*4096,4096))return false;
   for(unsigned j=0;j<5120;j++){const auto* q=packed.data()+j*4;norm[j]=uint32_t(q[0])|(uint32_t(q[1])<<8)|(uint32_t(q[2])<<16)|(uint32_t(q[3])<<24);}norms++;
  }
 }
 return found==1 && norms==1 && PioneerQ38EmbeddingReference(o.source,o.embedding,token,rawReference.data())
  && PioneerQ38LearnedRmsReference(o.source,o.norm,rawReference.data(),normalizedReference.data()) && Ready();
}
