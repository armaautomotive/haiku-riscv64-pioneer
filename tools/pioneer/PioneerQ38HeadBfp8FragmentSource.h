/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_HEAD_BFP8_FRAGMENT_SOURCE_H
#define PIONEER_Q38_HEAD_BFP8_FRAGMENT_SOURCE_H
#include "PioneerQ38HeadBlockFloatDiagnostic.h"
#include "PioneerQwen38PackedPlan.h"
#include <algorithm>
#include <array>

// Offline-only, one original Q6 output-head stripe -> NEW BFP8 tile pages.
// Output has a DIFFERENT length/encoding from the old Q6 stripe. This is a
// data source, not a bank address, cache, upload, runtime or owner capability.
// The caller must authenticate the entire same-open source separately. One
// instance is serialized, one-shot, and poisons itself after a read failure.
template<class Source>class PioneerQ38HeadBfp8FragmentSource {
 using Codec=PioneerBlackholeBlockFloat;
 Source* source=nullptr;PioneerQ38Tensor tensor;PioneerQ38PackedPart part={};uint64_t epoch=0;
 uint64_t bytes=0,allocated=0;unsigned cached=UINT32_MAX;bool attempted=false,bound=false,failed=false;
 std::vector<uint8_t> band;
 bool Current()const{
  if(!bound || failed || !source || !source->Ready() || !source->Stable() || source->Epoch()!=epoch
   || std::fegetround()!=FE_TONEAREST || part.tensor>=source->Tensors().size())return false;
  const auto& t=source->Tensors()[part.tensor];
  return t.name==tensor.name && t.type==tensor.type && t.rank==tensor.rank && t.inputs==tensor.inputs
   && t.outputs==tensor.outputs && t.bytes==tensor.bytes && t.offset==tensor.offset;
 }
 bool Lose(){failed=true;cached=UINT32_MAX;band.clear();return false;}
 bool Load(unsigned selected){
  if(!Current())return Lose();if(selected==cached)return true;
  if(selected>=part.rows/32)return Lose();std::vector<uint8_t> staged;
  if(!PioneerQ38HeadBlockFloatDiagnostic::PackBand(*source,part.tensor,part.firstRow+selected*32,
       Codec::Format::Bfp8B,staged) || staged.size()!=160U*1088 || !Current())return Lose();
  band=std::move(staged);cached=selected;return true;
 }
public:
 bool Bind(Source& input,const PioneerQ38PackedPart& fragment){
  if(attempted)return false;attempted=true;
  if(!input.Ready() || !input.Stable() || !input.Epoch() || std::fegetround()!=FE_TONEAREST
   || fragment.kind!=PIONEER_Q38_WEIGHT || fragment.layer!=64 || fragment.role!=1 || fragment.bank>=7
   || fragment.tensor>=input.Tensors().size() || !fragment.rows || (fragment.firstRow&63) || (fragment.rows&63))return false;
  const auto& t=input.Tensors()[fragment.tensor];PioneerQ38Tensor expected;
  if(!PioneerQ38Expected(64,1,expected) || t.name!=expected.name || t.type!=14 || t.rank!=2
   || t.inputs!=5120 || t.outputs!=248320 || t.bytes!=expected.bytes
   || fragment.firstRow>t.outputs || fragment.rows>t.outputs-fragment.firstRow
   || fragment.rowBytes!=4240 || uint64_t(fragment.rows)*fragment.rowBytes!=fragment.bytes
   || uint64_t(fragment.bytes)+4095>UINT32_MAX || fragment.allocated!=((fragment.bytes+4095U)&~4095U)
   || fragment.proposedAddress<PioneerQwen38PackedPlan::Base || uint64_t(fragment.proposedAddress)+fragment.allocated>
    uint64_t(PioneerQwen38PackedPlan::Base)+PioneerQwen38PackedPlan::Capacity)return false;
  const uint64_t nextBytes=uint64_t(fragment.rows)*5440,nextAllocated=(nextBytes+4095)&~uint64_t(4095);
  if(!nextBytes || nextAllocated>UINT32_MAX)return false;
  source=&input;tensor=t;part=fragment;epoch=input.Epoch();bytes=nextBytes;allocated=nextAllocated;bound=true;
  return Current() || Lose();
 }
 bool Ready()const{return Current();}
 unsigned Pages()const{return Current()?unsigned(allocated/4096):0;}
 uint64_t Bytes()const{return Current()?bytes:0;}
 bool ReadPage(unsigned page,uint8_t* output,unsigned capacity){
  if(!output || capacity<4096 || !bound || failed || page>=allocated/4096)return false;
  if(!Current())return Lose();
  constexpr uint64_t bandBytes=160U*1088;uint64_t at=uint64_t(page)*4096;
  std::array<uint8_t,4096> staged={};unsigned filled=0;
  while(filled<4096 && at<bytes){
   const unsigned selected=unsigned(at/bandBytes),within=unsigned(at%bandBytes);
   if(!Load(selected))return false;
   const unsigned n=unsigned(std::min(uint64_t(4096-filled),std::min(bandBytes-within,bytes-at)));
   if(!n || band.size()!=bandBytes)return Lose();
   std::memcpy(staged.data()+filled,band.data()+within,n);filled+=n;at+=n;
  }
  if(!Current())return Lose();std::memcpy(output,staged.data(),staged.size());return true;
 }
};
#endif
