/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_BFP4_FRAGMENT_SOURCE_H
#define PIONEER_Q38_BFP4_FRAGMENT_SOURCE_H
#if !defined(PIONEER_Q38_BFP4_PREPARATION_DIAGNOSTIC) || PIONEER_Q38_BFP4_PREPARATION_DIAGNOSTIC != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__)
#error BFP4 fragment conversion is separate offline preparation, not driver, Work or CPU reference
#endif
#include "PioneerQwen38PackedPlan.h"
#include "PioneerBlackholeBlockFloat.h"
#include <array>
#include <cfenv>

// Converts ONE exact canonical FFN fragment into a new, incompatible BFP4_B
// tile-major byte representation, preserving its original byte length. This
// is DATA ONLY: no filesystem, runtime mode, device address/ownership, upload,
// CPU model arithmetic or permission to replace original Q4 weights. Caller
// MUST authenticate the complete original model independently. A later cache
// must bind the original SHA + exact conversion recipe and must never serve
// these bytes to an old Q4 consumer. Serialized; not safe for shared threads.
template<class Source> class PioneerQ38Bfp4FragmentSource {
 Source* source=nullptr;PioneerQ38Tensor tensor;PioneerQ38PackedPart part={};uint64_t epoch=0;
 bool attempted=false,bound=false,failed=false;unsigned cachedBand=UINT32_MAX;
 std::vector<uint8_t> band;
 bool Current()const {
  if(!bound || failed || !source || !source->Ready() || !source->Stable() || source->Epoch()!=epoch || std::fegetround()!=FE_TONEAREST
   || part.tensor>=source->Tensors().size())return false;
  const auto& t=source->Tensors()[part.tensor];
  return t.name==tensor.name && t.inputs==tensor.inputs && t.outputs==tensor.outputs && t.type==tensor.type
   && t.rank==tensor.rank && t.bytes==tensor.bytes && t.offset==tensor.offset;
 }
 bool Lose(){failed=true;cachedBand=UINT32_MAX;band.clear();return false;}
 bool Load(unsigned selected){
  if(!Current())return Lose();
  if(cachedBand==selected)return true;
  const unsigned width=unsigned(tensor.inputs),bands=part.rows/32;
  if(selected>=bands)return false;
  std::vector<float> rows(size_t(32)*width);std::array<float,1024> tile={};
  std::vector<uint8_t> staged(size_t(width/32)*576);
  for(unsigned row=0;row<32;row++)if(!source->ReadRow(part.tensor,part.firstRow+selected*32+row,rows.data()+size_t(row)*width,width) || !Current())return Lose();
  for(unsigned col=0;col<width/32;col++){
   for(unsigned row=0;row<32;row++)for(unsigned k=0;k<32;k++)tile[row*32+k]=rows[size_t(row)*width+col*32+k];
   if(!PioneerBlackholeBlockFloat::PackTile(PioneerBlackholeBlockFloat::Format::Bfp4B,tile.data(),1024,staged.data()+size_t(col)*576,576))return Lose();
  }
  if(!Current())return Lose();
  band=std::move(staged);cachedBand=selected;return true;
 }
public:
 bool Bind(Source& input,const PioneerQ38PackedPart& fragment){
  if(attempted)return false;
  attempted=true;
  if(!input.Ready() || !input.Stable() || !input.Epoch() || std::fegetround()!=FE_TONEAREST || fragment.kind!=PIONEER_Q38_WEIGHT
   || fragment.layer>=64 || fragment.role<2 || fragment.role>4 || fragment.tensor>=input.Tensors().size() || fragment.bank>=7
   || !fragment.rows || (fragment.firstRow%64) || (fragment.rows%64))return false;
  const auto& t=input.Tensors()[fragment.tensor];PioneerQ38Tensor expected;
  if(!PioneerQ38Expected(fragment.layer,fragment.role,expected) || t.name!=expected.name || t.type!=12 || t.rank!=2
   || t.inputs!=expected.inputs || t.outputs!=expected.outputs || t.bytes!=expected.bytes
   || fragment.firstRow>t.outputs || fragment.rows>t.outputs-fragment.firstRow
   || fragment.rowBytes!=t.inputs/256*144 || uint64_t(fragment.rows)*fragment.rowBytes!=fragment.bytes
   || uint64_t(fragment.bytes)+4095>UINT32_MAX || fragment.allocated!=((fragment.bytes+4095U)&~4095U)
   || fragment.proposedAddress<PioneerQwen38PackedPlan::Base || uint64_t(fragment.proposedAddress)+fragment.allocated>
    uint64_t(PioneerQwen38PackedPlan::Base)+PioneerQwen38PackedPlan::Capacity)return false;
  source=&input;tensor=t;part=fragment;epoch=input.Epoch();bound=true;
  if(!Current())return Lose();
  return true;
 }
 bool Ready()const{return Current();}
 unsigned Pages()const{return Current()?part.allocated/4096:0;}
 bool ReadPage(unsigned page,uint8_t* output,unsigned capacity){
  if(!output || capacity<4096 || !bound || failed || page>=part.allocated/4096)return false;
  if(!Current())return Lose();
  const uint64_t bandBytes=tensor.inputs/32*576;uint64_t at=uint64_t(page)*4096;
  std::array<uint8_t,4096> staged={};unsigned filled=0;
  while(filled<4096 && at<part.bytes){
   const unsigned selected=unsigned(at/bandBytes),within=unsigned(at%bandBytes);
   if(!Load(selected))return false;
   const unsigned count=unsigned(std::min(uint64_t(4096-filled),std::min(bandBytes-within,uint64_t(part.bytes)-at)));
   if(!count || band.size()!=bandBytes)return Lose();
   std::memcpy(staged.data()+filled,band.data()+within,count);filled+=count;at+=count;
  }
  if(!Current())return Lose();
  std::memcpy(output,staged.data(),staged.size());return true;
 }
};
#endif
