/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_REFERENCE 1
#define PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_REFERENCE 1
#define PIONEER_Q38_CPU_VALIDATION 1
#include "PioneerQ38RecurrentPrefixValidationReference.h"
#include "PioneerQ38RecurrentPrefixOriginalSource.h"
#include <cassert>
#include <cstdio>
#include <iterator>
#include <map>
#include <unistd.h>
namespace {
#ifndef __HAIKU__
thread_local const char* prefixOriginalPath=nullptr;
#endif
FILE* PrefixReferenceFopen(const char* path,const char* mode){
#ifdef __HAIKU__
 return fopen(path,mode);
#else
 assert(prefixOriginalPath && !std::strcmp(path,"/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf") && !std::strcmp(mode,"rb"));
 return fopen(prefixOriginalPath,mode);
#endif
}
int PrefixReferenceExecv(const char* path,char* const* argv){
#ifdef __HAIKU__
 return execv(path,argv);
#else
 assert(!std::strcmp(path,"/bin/sha256sum") && !std::strcmp(argv[0],"sha256sum") && !argv[1]);
 char name[]="shasum",flag[]="-a",bits[]="256";char* args[]={name,flag,bits,nullptr};return execv("/usr/bin/shasum",args);
#endif
}
}
#define fopen PrefixReferenceFopen
#define execv PrefixReferenceExecv
#include "PioneerQ38PublicModelSource.h"
#undef fopen
#undef execv
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionInputShape.h"
class PioneerRecurrentPrefixOriginalSource final:public PioneerQ38RecurrentPrefixOriginalSource {
 struct Span {unsigned route;uint32_t address;std::vector<uint8_t> bytes;};
 struct Descriptor {unsigned index;PioneerQ38Tensor tensor;};
 PioneerQ38PublicModelSource source;PioneerQwen38PackedPlan plan;std::string path;
 std::map<uint64_t,Span> spans;std::vector<Descriptor> descriptors;
 std::map<uintptr_t,uintptr_t> rawOutputRanges;uintptr_t rawFirst=0,rawEnd=0;
 std::unique_ptr<PioneerRecurrentPrefixReferenceProducts> reference;
 uint64_t epoch=0,total=0,reads=0;std::array<uint64_t,3> fingerprint={};
 unsigned pages=0;bool attempted=false,prepared=false,sealed=false,packet=false;
 static uint64_t Key(unsigned route,uint32_t address){return uint64_t(route)<<32|address;}
 bool Current()const{
  if(!source.Ready() || source.Model().Epoch()!=epoch || source.Model().Tensors().size()!=851 || !plan.MatchesSource(source.Model()))return false;
  for(const auto& saved:descriptors){const auto& a=source.Model().Tensors()[saved.index];const auto& b=saved.tensor;
   if(a.name!=b.name || a.inputs!=b.inputs || a.outputs!=b.outputs || a.offset!=b.offset || a.bytes!=b.bytes || a.type!=b.type || a.rank!=b.rank)return false;
  }
  return true;
 }
 const Span* Find(unsigned route,uint32_t address,unsigned bytes)const{
  if(!bytes || uint64_t(address)+bytes>UINT64_C(0x100000000))return nullptr;
  auto found=spans.upper_bound(Key(route,address));if(found==spans.begin())return nullptr;--found;const auto& s=found->second;
  return s.route==route && address>=s.address && uint64_t(address)-s.address<=s.bytes.size()
   && bytes<=s.bytes.size()-(address-s.address)?&s:nullptr;
 }
 bool Output(const void* output,uint64_t bytes)const{
  const uintptr_t o=reinterpret_cast<uintptr_t>(output),self=reinterpret_cast<uintptr_t>(this),r=reinterpret_cast<uintptr_t>(reference.get());
  if(!output || !bytes || bytes>UINTPTR_MAX || o>UINTPTR_MAX-bytes || (o<self+sizeof(*this) && self<o+bytes)
   || (reference && o<r+sizeof(*reference) && r<o+bytes))return false;
  if(!rawOutputRanges.empty() && o<rawEnd && rawFirst<o+bytes){
   auto next=rawOutputRanges.upper_bound(o);
   if(next!=rawOutputRanges.end() && next->first<o+bytes)return false;
   if(next!=rawOutputRanges.begin() && o<std::prev(next)->second)return false;
  } // exact same all-span overlap refusal, without a linear scan per raw word
  return true;
 }
 struct ReferenceBytes {
  PioneerRecurrentPrefixOriginalSource& owner;
  bool Ready()const{return !owner.sealed && owner.Current();}
  const auto& Model()const{return owner.source.Model();}
  bool ReadReferencePacked(unsigned tensor,uint64_t at,unsigned bytes,void* output,unsigned capacity){
   if(!Ready())return false;
   owner.reads++;return owner.source.ReadReferencePacked(tensor,at,bytes,output,capacity) && Ready();
  }
 };
public:
 explicit PioneerRecurrentPrefixOriginalSource(const char* selected):path(selected){}
 bool Prepare()override{
  if(attempted || prepared || sealed)return false;
  attempted=true;bool opened=false;
#ifdef __HAIKU__
  opened=source.Open();
#else
  if(prefixOriginalPath)return false;
  {struct Scope {~Scope(){prefixOriginalPath=nullptr;}} scope;prefixOriginalPath=path.c_str();opened=source.Open();}
#endif
  if(!opened || !source.Ready() || !plan.Build(source.Model(),8192,true,true))return false;
  epoch=source.Model().Epoch();
  auto selected=[](const auto& p){return p.kind==PIONEER_Q38_WEIGHT && (p.layer<3 || (p.layer==64 && !p.role && !p.firstRow));};
  for(const auto& p:plan.Parts())if(selected(p)){
   bool found=false;for(const auto& d:descriptors)found|=d.index==p.tensor;
   if(!found)descriptors.push_back({p.tensor,source.Model().Tensors()[p.tensor]});
  }
  if(descriptors.size()!=43 || !Current())return false;
  std::map<uint64_t,Span> stage;uint64_t bytes=0;unsigned count=0;
  for(unsigned i=0;i<plan.Parts().size();i++){
   const auto& p=plan.Parts()[i];if(!selected(p))continue;
   unsigned length=p.layer==64?46080:p.bytes;if(length>p.bytes || p.bank>=7)return false;
   Span span{PioneerQ38ProjectionRoute(p.bank),p.proposedAddress,std::vector<uint8_t>(length)};std::array<uint8_t,4096> page;
   for(unsigned at=0;at<length;at+=4096){
    if(!Current() || !source.ReadWeightPage(plan,i,at/4096,page.data(),4096) || !Current())return false;
    std::memcpy(span.bytes.data()+at,page.data(),std::min(4096U,length-at));count++;
   }
   const uint64_t key=Key(span.route,span.address);auto next=stage.lower_bound(key);
   if(next!=stage.end() && next->second.route==span.route && uint64_t(span.address)+length>next->second.address)return false;
   if(next!=stage.begin()){const auto& prior=std::prev(next)->second;if(prior.route==span.route && uint64_t(prior.address)+prior.bytes.size()>span.address)return false;}
   bytes+=length;if(!stage.emplace(key,std::move(span)).second)return false;
  }
  if(stage.size()!=169 || bytes!=842821248 || count!=205785 || !Current())return false;
  auto expected=std::make_unique<PioneerRecurrentPrefixReferenceProducts>();ReferenceBytes original{*this};
  if(!PioneerRecurrentPrefixOriginalReference(original,expected.get()) || reads!=11557 || !Current())return false;
  std::array<uint64_t,3> hashes;
  for(unsigned layer=0;layer<3;layer++){
   uint64_t hash=14695981039346656037ULL;const auto* data=reinterpret_cast<const uint8_t*>(&expected->layer[layer]);
   for(unsigned i=0;i<sizeof(expected->layer[layer]);i++){hash^=data[i];hash*=1099511628211ULL;}
   if(!hash)return false;
   hashes[layer]=hash;
  }
  if(hashes[0]!=UINT64_C(0xbb911e57059b179f) || !Current())return false; // frozen qualified layer0 unchanged
  std::map<uintptr_t,uintptr_t> ranges;
  for(const auto& entry:stage){const uintptr_t begin=reinterpret_cast<uintptr_t>(entry.second.bytes.data());
   if(!begin || begin>UINTPTR_MAX-entry.second.bytes.size() || !ranges.emplace(begin,begin+entry.second.bytes.size()).second)return false;
  }
  if(ranges.size()!=stage.size())return false;
  for(auto i=std::next(ranges.begin());i!=ranges.end();++i)if(std::prev(i)->second>i->first)return false;
  rawFirst=ranges.begin()->first;rawEnd=ranges.rbegin()->second;rawOutputRanges=std::move(ranges);
  reference=std::move(expected);fingerprint=hashes;spans=std::move(stage);total=bytes;pages=count;prepared=true;return true;
 }
 bool SealBeforeNativeContext()override{
  if(!prepared || sealed || packet || !reference || !Current() || std::fegetround()!=FE_TONEAREST)return false;
  sealed=true;return Ready();
 }
 bool Ready()const override{return prepared && sealed && reference && fingerprint[0] && fingerprint[1] && fingerprint[2] && Current() && std::fegetround()==FE_TONEAREST;}
 bool Contains(unsigned route,uint32_t address,unsigned bytes)const override{return prepared && Find(route,address,bytes);}
 bool Read(unsigned route,uint32_t address,uint8_t* output,unsigned bytes)override{
  if(!Ready() || !bytes || bytes>16384 || !Output(output,bytes))return false;
  const auto* span=Find(route,address,bytes);if(!span)return false;
  std::memcpy(output,span->bytes.data()+address-span->address,bytes);return true;
 }
 bool BeginRawPacket()override{if(packet || !Ready())return false;packet=true;return true;}
 bool EndRawPacket()override{if(!packet)return false;bool okay=Ready();packet=false;return okay;}
 bool Word(unsigned route,uint32_t address,uint32_t& output)const override{
  if(!prepared || (address&3) || (!packet && !Ready()) || !Output(&output,4))return false;
  const auto* span=Find(route,address,4);if(!span)return false;const auto* p=span->bytes.data()+address-span->address;
  uint32_t value=uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);
  if(!packet && !Ready())return false;
  output=value;return true;
 }
 bool CompareStage(unsigned layer,PioneerRecurrentOriginalStage which,const uint32_t* values,unsigned words,PioneerRecurrentOriginalComparison* output)const override{
  if(layer>=3 || !Ready() || packet || !Output(output,sizeof(*output)))return false;
  PioneerRecurrentOriginalComparison result;
  const uintptr_t a=reinterpret_cast<uintptr_t>(values),o=reinterpret_cast<uintptr_t>(output);
  if(o%alignof(PioneerRecurrentOriginalComparison) || (values && (a>UINTPTR_MAX-uint64_t(words)*4 || (a<o+sizeof(*output) && o<a+uint64_t(words)*4))))return false;
  if(!PioneerRecurrentCompareStage(which,values,words,reference->layer[layer],&result) || !Ready())return false;
  *output=result;return true;
 }
 uint64_t ReferenceReads()const override{return reads;}
 uint64_t ReferenceFingerprint(unsigned layer)const override{return layer<3 && Ready()?fingerprint[layer]:0;}
 uint64_t Bytes()const override{return Ready()?total:0;}
 unsigned Pages()const override{return Ready()?pages:0;}
 unsigned Spans()const override{return Ready()?unsigned(spans.size()):0;}
 bool Extent(unsigned index,PioneerRecurrentOriginalExtent& output)const override{
  if(!Ready() || packet || index>=spans.size() || !Output(&output,sizeof(output)))return false;
  auto found=spans.begin();std::advance(found,index);const auto& span=found->second;
  PioneerRecurrentOriginalExtent value;value.route=span.route;value.address=span.address;value.bytes=unsigned(span.bytes.size());value.pages=(value.bytes+4095)/4096;
  for(unsigned bank=0;bank<7;bank++)if(PioneerQ38ProjectionRoute(bank)==span.route)value.bank=bank;
  if(value.bank>=7 || !value.bytes || !Contains(value.route,value.address,value.bytes) || !Ready())return false;
  output=value;return true;
 }
};
std::unique_ptr<PioneerQ38RecurrentPrefixOriginalSource> OpenPioneerQ38RecurrentPrefixOriginalSource(const char* path){
#ifdef __HAIKU__
 if(path && std::strcmp(path,"/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf"))return {};
 return std::make_unique<PioneerRecurrentPrefixOriginalSource>("/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf");
#else
 if(!path || !*path || path[0]!='/')return {};
 return std::make_unique<PioneerRecurrentPrefixOriginalSource>(path);
#endif
}
