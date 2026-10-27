/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_MIXED_LIVE_VALIDATION_REFERENCE 1
#define PIONEER_Q38_CPU_VALIDATION 1
#include "PioneerQ38MixedLiveValidationReference.h"
#include "PioneerQ38MixedLiveOriginalSource.h"
#include "PioneerQwen38PackedPlan.h"
#include <cassert>
#include <cstdio>
#include <unistd.h>
namespace {
constexpr const char* FixedModel="/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf";
#ifndef __HAIKU__
const char* hostOriginalPath=nullptr;
FILE* MixedSourceFopen(const char* path,const char* mode){
 assert(hostOriginalPath && !std::strcmp(path,FixedModel) && !std::strcmp(mode,"rb"));return fopen(hostOriginalPath,mode);
}
int MixedSourceExecv(const char* path,char* const* argv){
 assert(!std::strcmp(path,"/bin/sha256sum") && !std::strcmp(argv[0],"sha256sum") && !argv[1]);
 char name[]="shasum",flag[]="-a",bits[]="256";char* args[]={name,flag,bits,nullptr};return execv("/usr/bin/shasum",args);
}
#define fopen MixedSourceFopen
#define execv MixedSourceExecv
#endif
} // helper names have internal linkage; original model/source types stay global
#include "PioneerQ38PublicModelSource.h"
#ifndef __HAIKU__
#undef fopen
#undef execv
#endif
namespace {
class MixedOriginalSource final:public PioneerQ38MixedLiveOriginalSource {
 PioneerQ38PublicModelSource original;PioneerQwen38PackedPlan plan;
 std::unique_ptr<PioneerQ38MixedLiveReferenceProducts> reference;
 uint64_t epoch=0,fingerprint=0;unsigned reads=0;bool attempted=false,prepared=false,sealed=false;
 bool Current()const{return original.Ready() && original.Model().Epoch()==epoch && plan.MatchesSource(original.Model());}
 struct ReferenceBytes {
  MixedOriginalSource& owner;
  bool Ready()const{return !owner.sealed && owner.Current();}
  const auto& Model()const{return owner.original.Model();}
  bool ReadReferencePacked(unsigned tensor,uint64_t offset,unsigned bytes,void* output,unsigned capacity){
   if(!Ready())return false;
   owner.reads++;return owner.original.ReadReferencePacked(tensor,offset,bytes,output,capacity) && Ready();
  }
 };
public:
 bool Open(){return original.Open() && original.Ready();}
 bool Prepare()override{
  if(attempted || prepared || sealed || !original.Ready())return false;
  attempted=true;epoch=original.Model().Epoch();
  if(!plan.Build(original.Model(),8192,true,true) || !Current())return false;
  auto stage=std::make_unique<PioneerQ38MixedLiveReferenceProducts>();ReferenceBytes bytes{*this};
  if(!PioneerQ38MixedLiveOriginalReference(bytes,0,stage.get()) || reads!=3717 || !Current())return false;
  uint64_t hash=14695981039346656037ULL;const auto* p=reinterpret_cast<const uint8_t*>(stage.get());
  for(unsigned i=0;i<sizeof(*stage);i++){hash^=p[i];hash*=1099511628211ULL;}
  if(!hash || !Current())return false;
  reference=std::move(stage);fingerprint=hash;prepared=true;return true;
 }
 bool SealBeforeNativeContext()override{
  if(!prepared || sealed || !reference || !fingerprint || !Current())return false;
  sealed=true;return Ready();
 }
 bool Ready()const override{return prepared && sealed && reference && fingerprint && Current();}
 const PioneerQwen38Gguf* SealedModel()const override{return Ready()?&original.Model():nullptr;}
 bool ReadWeightPage(unsigned part,unsigned page,uint8_t* output,unsigned capacity)override{
  return Ready() && original.ReadWeightPage(plan,part,page,output,capacity) && Ready();
 } // ORIGINAL tiled BYTES from the SAME private fixed blueprint, not CPU products
 bool CompareFinal(const uint32_t* values,unsigned words,PioneerQ38MixedLiveReferenceComparison* output)const override{
  return CompareStage(PioneerQ38MixedFinal,values,words,output);
 }
 bool CompareStage(PioneerQ38MixedLiveReferenceStage which,const uint32_t* values,unsigned words,PioneerQ38MixedLiveReferenceComparison* output)const override{
  if(!Ready() || !output)return false;
  PioneerQ38MixedLiveReferenceComparison stage;
  // Preserve the reference comparison's pointer/overlap refusals on the
  // actual caller output, before staging an atomic external publication.
  uintptr_t a=reinterpret_cast<uintptr_t>(values),o=reinterpret_cast<uintptr_t>(output),r=reinterpret_cast<uintptr_t>(reference.get());
  if(o%alignof(PioneerQ38MixedLiveReferenceComparison) || o>UINTPTR_MAX-sizeof(*output)
   || (o<r+sizeof(*reference) && r<o+sizeof(*output))
   || (values && (a>UINTPTR_MAX-uint64_t(words)*4 || (a<o+sizeof(*output) && o<a+uint64_t(words)*4))))return false;
  if(!PioneerQ38CompareMixedLiveStage(values,words,*reference,which,&stage) || !Ready())return false;
  *output=stage;return true;
 }
 uint64_t ReferenceReads()const override{return reads;}
 uint64_t ReferenceFingerprint()const override{return Ready()?fingerprint:0;}
};
}
std::unique_ptr<PioneerQ38MixedLiveOriginalSource> OpenPioneerQ38MixedLiveOriginalSource(const char* path){
#ifdef __HAIKU__
 if(path && std::strcmp(path,FixedModel))return {};
#else
 if(!path || !*path)return {};
 hostOriginalPath=path;
#endif
 auto source=std::make_unique<MixedOriginalSource>();if(!source->Open())return {};return source;
}
