/* SPDX-License-Identifier: MIT */
// Actual fixed ORIGINAL GGUF/full SHA only. NO accelerator FD, native
// controller, tiled products, hardware readback, model run or Work mode.
#define PIONEER_Q38_MIXED_LIVE_VALIDATION_REFERENCE 1
#define PIONEER_Q38_CPU_VALIDATION 1
#include "../PioneerQ38MixedLiveValidationReference.h"
#include "../PioneerQwen38PackedPlan.h"
#include <cassert>
#include <cstdio>
#include <unistd.h>
static const char* originalPath=nullptr;
#ifndef __HAIKU__
static FILE* MixedOriginalFopen(const char* path,const char* mode){
 assert(originalPath && !std::strcmp(path,"/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf") && !std::strcmp(mode,"rb"));return fopen(originalPath,mode);
}
static int MixedOriginalExecv(const char* path,char* const* argv){
 assert(!std::strcmp(path,"/bin/sha256sum") && !std::strcmp(argv[0],"sha256sum") && !argv[1]);
 char name[]="shasum",flag[]="-a",bits[]="256";char* args[]={name,flag,bits,nullptr};return execv("/usr/bin/shasum",args);
}
#define fopen MixedOriginalFopen
#define execv MixedOriginalExecv
#endif
#include "../PioneerQ38PublicModelSource.h"
#ifndef __HAIKU__
#undef fopen
#undef execv
#endif
struct CountedOriginal {
 PioneerQ38PublicModelSource& source;unsigned reads=0;
 bool Ready()const{return source.Ready();}const auto& Model()const{return source.Model();}
 bool ReadReferencePacked(unsigned tensor,uint64_t offset,unsigned bytes,void* output,unsigned capacity){
  reads++;return source.ReadReferencePacked(tensor,offset,bytes,output,capacity);
 }
};
int main(int argc,char** argv){
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);originalPath=argv[1];
 PioneerQ38PublicModelSource original;assert(original.Open() && original.Ready());
 puts("PASS ORIGINAL fixed c600de03 FULL SHA/open file identity/all851 tensor schema;NO accelerator context");
 CountedOriginal source{original};auto output=std::make_unique<PioneerQ38MixedLiveReferenceProducts>();
 assert(PioneerQ38MixedLiveOriginalReference(source,0,output.get()) && original.Ready() && source.reads==3717);
 uint64_t hash=14695981039346656037ULL;const auto* p=reinterpret_cast<const uint8_t*>(output.get());
 for(unsigned i=0;i<sizeof(*output);i++){hash^=p[i];hash*=1099511628211ULL;}
 using namespace PioneerQ38MixedLiveReferenceDetail;float embeddingMax=0,finalMax=0;
 for(unsigned i=0;i<5120;i++){embeddingMax=std::fmax(embeddingMax,std::fabs(Float(output->embedding[i])));finalMax=std::fmax(finalMax,std::fabs(Float(output->finalHidden[i])));}
 printf("PASS ORIGINAL dense-block ideal CPU reference/3717reads/12sealed tensors/actual Q8_0 QG+K+V/Q6_K O/Q4_K FFN/embedding+RMS+position0attention+originalresidual+FFN+final fingerprint%016llx embeddingMax%.9g finalMax%.9g;STANDALONE layer3fromembedding,NOT GDN0..2/full64/token/cardcomparison/Work/TPS\n",(unsigned long long)hash,embeddingMax,finalMax);
}
