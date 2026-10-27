/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38MixedLiveOriginalSource.h" // repeat role interlocks even on late inclusion
#ifndef PIONEER_Q38_MIXED_LIVE_ORIGINAL_BYTES_TEST_H
#define PIONEER_Q38_MIXED_LIVE_ORIGINAL_BYTES_TEST_H
// TEST coordinator only. The instruction TU sees ONLY the neutral byte
// interface, never this source, its private CPU reference, or its plan.
#include "../PioneerQ38OriginalWeightBytes.h"
#include "../PioneerQwen38PackedPlan.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionInputShape.h"
#include <array>
#include <cstring>
class MixedLiveOriginalBytes final:public ProjectionExecutionLearnedBytes {
 struct Span {unsigned route;uint32_t address;std::vector<uint8_t> bytes;};
 struct Descriptor {unsigned index;PioneerQ38Tensor tensor;};
 PioneerQ38MixedLiveOriginalSource& source;
 const PioneerQwen38Gguf* model=nullptr;
 const PioneerQ38Tensor* descriptors=nullptr;
 uint64_t epoch=0,total=0;unsigned pageReads=0;bool attempted=false,prepared=false;
 std::vector<Span> spans;std::vector<Descriptor> selected;
 bool Current()const{
  if(!model || !source.Ready() || source.SealedModel()!=model || model->Epoch()!=epoch
   || model->Tensors().size()!=851 || model->Tensors().data()!=descriptors)return false;
  for(const auto& d:selected){
   const auto& a=model->Tensors()[d.index];const auto& b=d.tensor;
   if(a.name!=b.name || a.inputs!=b.inputs || a.outputs!=b.outputs || a.type!=b.type
    || a.rank!=b.rank || a.offset!=b.offset || a.bytes!=b.bytes)return false;
  }
  return true;
 }
public:
 explicit MixedLiveOriginalBytes(PioneerQ38MixedLiveOriginalSource& s):source(s){}
 bool Prepare(){
  if(attempted || prepared || !source.Ready() || !source.SealedModel())return false;
  attempted=true;model=source.SealedModel();epoch=model->Epoch();descriptors=model->Tensors().data();
  PioneerQwen38PackedPlan plan;
  if(!plan.Build(*model,8192,true,true) || !Current())return false;
  // Capture ALL selected descriptors BEFORE the first raw page read.
  for(const auto& p:plan.Parts())if(p.kind==PIONEER_Q38_WEIGHT && (p.layer==3 || (p.layer==64 && !p.role && !p.firstRow))){
   bool found=false;for(const auto& d:selected)found|=d.index==p.tensor;
   if(!found)selected.push_back({p.tensor,model->Tensors()[p.tensor]});
  }
  if(selected.size()!=12 || !Current())return false;
  std::vector<Span> stage;uint64_t stagedBytes=0;unsigned stagedPages=0;
  for(unsigned i=0;i<plan.Parts().size();i++){
   const auto& p=plan.Parts()[i];
   if(p.kind!=PIONEER_Q38_WEIGHT || (p.layer!=3 && !(p.layer==64 && !p.role && !p.firstRow)))continue;
   // Only token0's complete original sixteen-row embedding tile. Dense
   // layer3 includes every learned stripe, but NO state/workspace bytes.
   unsigned bytes=p.layer==64?46080:p.bytes;
   if(bytes>p.bytes || p.bank>=7)return false;
   Span span{PioneerQ38ProjectionRoute(p.bank),p.proposedAddress,std::vector<uint8_t>(bytes)};
   std::array<uint8_t,4096> page;
   for(unsigned at=0;at<bytes;at+=4096){
    if(!Current() || !source.ReadWeightPage(i,at/4096,page.data(),4096) || !Current())return false;
    unsigned count=std::min(4096U,bytes-at);std::memcpy(span.bytes.data()+at,page.data(),count);stagedPages++;
   }
   stagedBytes+=bytes;stage.push_back(std::move(span));
  }
  if(stage.size()!=54 || !Current())return false;
  spans=std::move(stage);total=stagedBytes;pageReads=stagedPages;prepared=true;return Ready();
 }
 bool Ready()const override{return prepared && Current();}
 bool Read(unsigned route,uint32_t address,uint8_t* output,unsigned bytes)override{
  uintptr_t o=reinterpret_cast<uintptr_t>(output);
  if(!Ready() || !output || !bytes || bytes>46080 || o>UINTPTR_MAX-bytes || uint64_t(address)+bytes>UINT64_C(0x100000000))return false;
  for(const auto& s:spans)if(s.route==route && address>=s.address && uint64_t(address)-s.address<=s.bytes.size()
   && bytes<=s.bytes.size()-(address-s.address)){
   uintptr_t a=reinterpret_cast<uintptr_t>(s.bytes.data());
   if(o<a+s.bytes.size() && a<o+bytes)return false;
   // Readiness brackets publication; no failed read exposes partial data.
   if(!Current())return false;
   std::memcpy(output,s.bytes.data()+address-s.address,bytes);return true;
  }
  return false;
 }
 uint64_t CachedBytes()const{return Ready()?total:0;}
 unsigned PageReads()const{return Ready()?pageReads:0;}
};
#endif
