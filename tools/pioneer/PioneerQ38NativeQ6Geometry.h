/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_NATIVE_Q6_GEOMETRY_H
#define PIONEER_Q38_NATIVE_Q6_GEOMETRY_H
#include "PioneerQ38NativeQ6Tiles.h"
#include "PioneerQwen38PackedPlan.h"

// OFFBOARD geometry proposal. Keep per-bank shard order and assignment,
// replacing (not duplicating) each Q6 tile. All later regions in a bank
// shift by its accumulated padding+growth, so neither their old addresses
// nor this proposal can be passed to the current native owner.
namespace PioneerQ38NativeQ6Geometry {
using Part=PioneerQ38PackedPart;
static inline bool Build(const PioneerQwen38Gguf& source,const PioneerQwen38PackedPlan& old,std::vector<Part>& result){
 if(!old.MatchesSource(source) || !old.Tiled() || old.DeltaSlots()!=2 || old.Parts().size()!=4678)return false;
 const auto& tensors=source.Tensors();const auto& parts=old.Parts();
 if(tensors.size()!=851)return false;
 uint64_t cur[7]={},before[7]={};std::vector<Part> candidate;candidate.reserve(parts.size());
 unsigned types=0;
 for(const auto& p:parts){
  using P=PioneerQwen38PackedPlan;
  if(p.bank>=7 || p.proposedAddress!=P::Base+before[p.bank] || !p.bytes || p.bytes>p.allocated
   || (p.allocated&4095) || p.allocated-p.bytes>=4096 || p.allocated>P::Capacity-before[p.bank])return false;
  before[p.bank]+=p.allocated;
  Part n=p;
  if(p.kind==PIONEER_Q38_WEIGHT){
   if(p.tensor>=tensors.size())return false;
   const auto& t=tensors[p.tensor];
   const bool route=(p.layer==64 && p.role==1) || (p.layer<64 && p.layer%4==3 && p.role==10);
   if((t.type==14)!=route)return false;
   if(route){
    namespace T=PioneerQ38NativeQ6Tiles;
    if(t.rank!=2 || t.inputs!=(p.layer==64?5120U:6144U) || !p.rows || ((p.firstRow|p.rows)&15)
     || p.firstRow+p.rows>t.outputs || p.rowBytes!=t.inputs/T::BlockK*(T::RawBytes/T::Rows)
     || uint64_t(p.rows)*p.rowBytes!=p.bytes)return false;
    n.rowBytes=unsigned(t.inputs/T::BlockK)*(T::TileBytes/T::Rows);
    const uint64_t bytes=uint64_t(n.rows)*n.rowBytes,aligned=(bytes+4095)&~UINT64_C(4095);
    if(aligned>UINT32_MAX)return false;
    n.bytes=uint32_t(bytes);n.allocated=uint32_t(aligned);types++;
   }
  }
  if(cur[p.bank]>P::Capacity || n.allocated>P::Capacity-cur[p.bank])return false;
  n.proposedAddress=uint32_t(P::Base+cur[p.bank]);cur[p.bank]+=n.allocated;
  candidate.push_back(n);
 }
 if(types!=119 || !old.MatchesSource(source))return false;
 for(unsigned i=0;i<7;i++)if(old.Used(i)!=before[i])return false;
 result.swap(candidate);return true;
}
}
#endif
