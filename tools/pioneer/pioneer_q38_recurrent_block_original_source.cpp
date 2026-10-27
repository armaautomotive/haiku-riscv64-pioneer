/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_REFERENCE 1
#define PIONEER_Q38_CPU_VALIDATION 1
#include "PioneerQ38RecurrentBlockValidationReference.h"
#include <cassert>
#include <cstdio>
#include <iterator>
#include <unistd.h>
namespace {
#ifndef __HAIKU__
const char* hostOriginalPath=nullptr;
#endif
FILE* RecurrentReferenceFopen(const char* path,const char* mode){
#ifdef __HAIKU__
 return fopen(path,mode);
#else
 assert(hostOriginalPath && !std::strcmp(path,"/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf") && !std::strcmp(mode,"rb"));return fopen(hostOriginalPath,mode);
#endif
}
int RecurrentReferenceExecv(const char* path,char* const* argv){
#ifdef __HAIKU__
 return execv(path,argv);
#else
 assert(!std::strcmp(path,"/bin/sha256sum") && !std::strcmp(argv[0],"sha256sum") && !argv[1]);
 char name[]="shasum",flag[]="-a",bits[]="256";char* args[]={name,flag,bits,nullptr};return execv("/usr/bin/shasum",args);
#endif
}
}
#define fopen RecurrentReferenceFopen
#define execv RecurrentReferenceExecv
#include "tests/pioneer_q38_mixed_recurrent_block_original_bytes.h"
#undef fopen
#undef execv
// This is the sole friend of the raw TEST cache. The SAME authenticated
// already-open file supplies private row-major CPU reference and cached
// tiled bytes. No public Model/FILE/product accessor or pathname reopen.
class PioneerRecurrentOriginalSource final:public PioneerQ38RecurrentBlockOriginalSource {
 RecurrentBlockOriginalBytes raw;
 std::unique_ptr<PioneerRecurrentReferenceProducts> reference;
 uint64_t fingerprint=0,reads=0;bool attempted=false,prepared=false,sealed=false,packet=false;
 struct ReferenceBytes {
  PioneerRecurrentOriginalSource& owner;
  bool Ready()const{return !owner.sealed && owner.raw.Ready();}
  const auto& Model()const{return owner.raw.source.Model();}
  bool ReadReferencePacked(unsigned tensor,uint64_t at,unsigned bytes,void* output,unsigned capacity){
   if(!Ready())return false;
   owner.reads++;return owner.raw.source.ReadReferencePacked(tensor,at,bytes,output,capacity) && Ready();
  }
 };
public:
 bool Prepare()override{
  if(attempted || prepared || sealed)return false;
  attempted=true;
  if(!raw.Prepare(true) || !raw.Ready())return false;
  auto stage=std::make_unique<PioneerRecurrentReferenceProducts>();ReferenceBytes bytes{*this};
  if(!PioneerRecurrentOriginalReference(bytes,stage.get()) || !raw.Ready() || !reads)return false;
  uint64_t hash=14695981039346656037ULL;const auto* data=reinterpret_cast<const uint8_t*>(stage.get());
  for(unsigned i=0;i<sizeof(*stage);i++){hash^=data[i];hash*=1099511628211ULL;}
  if(!hash || !raw.Ready())return false;
  reference=std::move(stage);fingerprint=hash;prepared=true;return true;
 }
 bool SealBeforeNativeContext()override{
  if(!prepared || sealed || packet || !reference || !fingerprint || !raw.Ready())return false;
  sealed=true;return Ready();
 }
 bool Ready()const override{return prepared && sealed && reference && fingerprint && raw.Ready() && std::fegetround()==FE_TONEAREST;}
 bool Read(unsigned route,uint32_t address,uint8_t* out,unsigned bytes)override{return Ready() && raw.Read(route,address,out,bytes) && Ready();}
 bool Contains(unsigned route,uint32_t address,unsigned bytes)const override{return raw.Contains(route,address,bytes);}
 bool BeginRawPacket()override{if(packet || !Ready() || !raw.BeginRawPacket())return false;
 packet=true;return true;}
 bool EndRawPacket()override{if(!packet)return false;
 bool okay=raw.EndRawPacket() && Ready();packet=false;return okay;}
 bool Word(unsigned route,uint32_t address,uint32_t& output)const override{
  if(!packet && !Ready())return false;
  uint32_t value;if(!raw.Word(route,address,value) || (!packet && !Ready()))return false;
  output=value;return true;
 }
 bool CompareStage(PioneerRecurrentOriginalStage which,const uint32_t* values,unsigned words,PioneerRecurrentOriginalComparison* output)const override{
  if(!Ready() || packet || !output)return false;
  PioneerRecurrentOriginalComparison result;
  uintptr_t a=reinterpret_cast<uintptr_t>(values),o=reinterpret_cast<uintptr_t>(output),r=reinterpret_cast<uintptr_t>(reference.get());
  if(o%alignof(PioneerRecurrentOriginalComparison) || o>UINTPTR_MAX-sizeof(*output) || (o<r+sizeof(*reference) && r<o+sizeof(*output))
   || (values && (a>UINTPTR_MAX-uint64_t(words)*4 || (a<o+sizeof(*output) && o<a+uint64_t(words)*4))))return false;
  if(!PioneerRecurrentCompareStage(which,values,words,*reference,&result) || !Ready())return false;
  *output=result;return true;
 }
 uint64_t ReferenceReads()const override{return reads;}
 uint64_t ReferenceFingerprint()const override{return Ready()?fingerprint:0;}
 uint64_t Bytes()const override{return Ready()?raw.Bytes():0;}
 unsigned Pages()const override{return Ready()?raw.Pages():0;}
 unsigned Spans()const override{return Ready()?raw.Spans():0;}
 bool Extent(unsigned index,PioneerRecurrentOriginalExtent& output)const override{
  const uintptr_t out=reinterpret_cast<uintptr_t>(&output),owner=reinterpret_cast<uintptr_t>(this),expected=reinterpret_cast<uintptr_t>(reference.get());
  if(out>UINTPTR_MAX-sizeof(output) || (out<owner+sizeof(*this) && owner<out+sizeof(output))
   || (reference && out<expected+sizeof(*reference) && expected<out+sizeof(output)) || !Ready() || packet || index>=raw.spans.size())return false;
  auto found=raw.spans.begin();std::advance(found,index);const auto& span=found->second;
  PioneerRecurrentOriginalExtent value;value.route=span.route;value.address=span.address;value.bytes=unsigned(span.bytes.size());
  value.pages=(value.bytes+4095)/4096;
  for(unsigned bank=0;bank<7;bank++)if(PioneerQ38ProjectionRoute(bank)==span.route)value.bank=bank;
  if(value.bank>=7 || !value.bytes || !raw.Contains(value.route,value.address,value.bytes) || !Ready())return false;
  output=value;return true;
 } // sealed RAW extent only;no model/FILE/reference/native state accessor
};
std::unique_ptr<PioneerQ38RecurrentBlockOriginalSource> OpenPioneerQ38RecurrentBlockOriginalSource(const char* path){
#ifdef __HAIKU__
 if(path && std::strcmp(path,"/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf"))return {};
#else
 if(!path || !*path || path[0]!='/')return {};hostOriginalPath=path;
#endif
 return std::make_unique<PioneerRecurrentOriginalSource>();
}
