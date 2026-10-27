/* SPDX-License-Identifier: MIT */
// TEST ONLY authenticated raw layer0 bytes. No reference products, native
// owner, source initialization or permission to submit a device operation.
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_ROOT_CAPSULE_IMAGE)
#error Original test-byte cache is not a kernel, Work or image capability
#endif
#ifndef PIONEER_Q38_MIXED_RECURRENT_BLOCK_ORIGINAL_BYTES_TEST_H
#define PIONEER_Q38_MIXED_RECURRENT_BLOCK_ORIGINAL_BYTES_TEST_H
#include "../PioneerQ38OriginalWeightBytes.h"
#include "../PioneerQ38PublicModelSource.h"
#include "../PioneerQ38MixedRecurrentBlockRootLayout.h"
#include <array>
#include <map>
class RecurrentBlockOriginalBytes final:public ProjectionExecutionLearnedBytes {
 struct Span {unsigned route;uint32_t address;std::vector<uint8_t> bytes;};
 struct Descriptor {unsigned index;PioneerQ38Tensor tensor;};
 PioneerQ38PublicModelSource source;PioneerQwen38PackedPlan plan;
 std::map<uint64_t,Span> spans;std::vector<Descriptor> descriptors;
 uint64_t epoch=0,total=0;unsigned pages=0;bool attempted=false,prepared=false,leased=false;
 static uint64_t Key(unsigned route,unsigned address){return uint64_t(route)<<32|address;}
 const Span* Find(unsigned route,uint32_t address,unsigned bytes)const{
  if(!bytes || uint64_t(address)+bytes>UINT64_C(0x100000000))return nullptr;
  auto found=spans.upper_bound(Key(route,address));if(found==spans.begin())return nullptr;--found;const auto& s=found->second;
  return s.route==route && address>=s.address && uint64_t(address)-s.address<=s.bytes.size()
   && bytes<=s.bytes.size()-(address-s.address)?&s:nullptr;
 }
 bool Current()const{
  if(!source.Ready() || source.Model().Epoch()!=epoch || source.Model().Tensors().size()!=851 || !plan.MatchesSource(source.Model()))return false;
  for(const auto& saved:descriptors){
   const auto& a=source.Model().Tensors()[saved.index];const auto& b=saved.tensor;
   if(a.name!=b.name || a.inputs!=b.inputs || a.outputs!=b.outputs || a.offset!=b.offset || a.bytes!=b.bytes || a.type!=b.type || a.rank!=b.rank)return false;
  }
  return true;
 }
public:
 bool Prepare(){
  if(attempted || prepared)return false;
  attempted=true;if(!source.Open() || !source.Ready() || !plan.Build(source.Model(),8192,true,true))return false;
  epoch=source.Model().Epoch();
  for(const auto& p:plan.Parts())if(p.kind==PIONEER_Q38_WEIGHT && !p.layer){
   bool found=false;for(const auto& d:descriptors)found |= d.index==p.tensor;
   if(!found)descriptors.push_back({p.tensor,source.Model().Tensors()[p.tensor]});
  }
  if(descriptors.size()!=14 || !Current())return false;
  std::map<uint64_t,Span> stage;uint64_t bytes=0;unsigned count=0;
  for(unsigned i=0;i<plan.Parts().size();i++){
   const auto& p=plan.Parts()[i];if(p.kind!=PIONEER_Q38_WEIGHT || p.layer)continue;
   Span span{PioneerQ38ProjectionRoute(p.bank),p.proposedAddress,std::vector<uint8_t>(p.bytes)};
   std::array<uint8_t,4096> page;
   for(unsigned at=0;at<p.bytes;at+=4096){
    if(!Current() || !source.ReadWeightPage(plan,i,at/4096,page.data(),4096) || !Current())return false;
    std::memcpy(span.bytes.data()+at,page.data(),std::min(4096U,p.bytes-at));count++;
   }
   bytes+=p.bytes;if(!stage.emplace(Key(span.route,span.address),std::move(span)).second)return false;
  }
  if(stage.size()!=56 || bytes!=280925056 || !Current())return false;
  spans=std::move(stage);total=bytes;pages=count;prepared=true;return Ready();
 }
 bool Ready()const override{return prepared && Current();}
 bool Read(unsigned route,uint32_t address,uint8_t* output,unsigned bytes)override{
  uintptr_t o=reinterpret_cast<uintptr_t>(output);
  if(!Ready() || !output || !bytes || bytes>16384 || o>UINTPTR_MAX-bytes)return false;
  const auto* s=Find(route,address,bytes);if(!s)return false;
  uintptr_t base=reinterpret_cast<uintptr_t>(s->bytes.data());if(o<base+s->bytes.size() && base<o+bytes)return false;
  // Publication is atomic because the underlying source is only observed
  // here; no reference, upload, input or device callback is performed.
  std::memcpy(output,s->bytes.data()+address-s->address,bytes);return true;
 }
 bool Contains(unsigned route,uint32_t address,unsigned bytes)const{return prepared && Find(route,address,bytes);}
 bool BeginRawPacket(){if(leased || !Ready())return false;leased=true;return true;}
 bool EndRawPacket(){if(!leased)return false;bool okay=Ready();leased=false;return okay;}
 bool Word(unsigned route,uint32_t address,uint32_t& output)const{
  if(!prepared || (address&3) || (!leased && !Ready()))return false;
  const auto* s=Find(route,address,4);if(!s)return false;const auto* p=s->bytes.data()+address-s->address;
  uint32_t value=uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);
  if(!leased && !Ready())return false;output=value;return true;
 }
 uint64_t Bytes()const{return Ready()?total:0;}
 unsigned Pages()const{return Ready()?pages:0;}
 unsigned Spans()const{return Ready()?unsigned(spans.size()):0;}
};
#endif
