/* SPDX-License-Identifier: MIT */
// Host TEST oracle/byte TU ONLY. No native or device execution code.
#define PIONEER_Q38_CPU_VALIDATION 1
#define PIONEER_Q38_BENCHMARK_HOST 1
#include "pioneer_q38_global_output_original_source.h"
#include "../PioneerQwen38PackedPlan.h"
#include <cassert>
#include <unistd.h>
static const char* globalOriginalPath=nullptr;
static FILE* GlobalOriginalFopen(const char* path,const char* mode){
 assert(globalOriginalPath && !strcmp(path,"/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf") && !strcmp(mode,"rb"));return fopen(globalOriginalPath,mode);
}
static int GlobalOriginalExecv(const char* path,char* const* argv){
 assert(!strcmp(path,"/bin/sha256sum") && !strcmp(argv[0],"sha256sum") && !argv[1]);
 char name[]="shasum",flag[]="-a",bits[]="256";char* args[]={name,flag,bits,nullptr};return execv("/usr/bin/shasum",args);
}
#define fopen GlobalOriginalFopen
#define execv GlobalOriginalExecv
#include "../PioneerQ38PublicModelSource.h"
#undef fopen
#undef execv
struct GlobalOutputOriginalSource::Original {PioneerQ38PublicModelSource source;unsigned tensor=UINT32_MAX;};
GlobalOutputOriginalSource::GlobalOutputOriginalSource():original(new Original){}
GlobalOutputOriginalSource::~GlobalOutputOriginalSource(){delete original;}
bool GlobalOutputOriginalSource::Ready()const{return original && original->tensor!=UINT32_MAX && original->source.Ready();}
bool GlobalOutputOriginalSource::Open(const char* path){
 if(!path || globalOriginalPath)return false;globalOriginalPath=path;
 if(!original->source.Open())return false;
 const auto& model=original->source.Model();const auto* t=model.Find("output.weight");
 if(!t || t->rank!=2 || t->type!=14 || t->inputs!=5120 || t->outputs!=248320 || t->bytes!=UINT64_C(248320)*20*210)return false;
 original->tensor=unsigned(t-model.Tensors().data());return Ready();
}
bool GlobalOutputOriginalSource::PrepareFork(){return Ready() && original->source.PrepareBenchmarkFork();}
bool GlobalOutputOriginalSource::Tile(unsigned firstRow,unsigned block,uint8_t* output,unsigned bytes){
 if(!Ready() || !output || bytes!=3392 || firstRow>248320-16 || (firstRow&15) || block>=20)return false;
 uint8_t stage[3392]={};
 for(unsigned row=0;row<16;row++){
  uint8_t raw[210];uint64_t offset=(uint64_t(firstRow+row)*20+block)*210;
  if(!original->source.ReadReferencePacked(original->tensor,offset,210,raw,sizeof(raw)))return false;
  for(unsigned i=0;i<210;i++)stage[i*16+row]=raw[i];
 }
 if(!Ready())return false;memcpy(output,stage,sizeof(stage));return true;
}
