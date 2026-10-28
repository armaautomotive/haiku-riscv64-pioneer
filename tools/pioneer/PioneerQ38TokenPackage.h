/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TOKEN_PACKAGE_H
#define PIONEER_Q38_TOKEN_PACKAGE_H
#include "PioneerQ38TokenStatePlan.h"
// Original immutable pos0 package plus canonical state-copy spans in its
// reserved slack. CPU serialization ONLY, not a completed producer/launch.
class PioneerQ38TokenPackage {
 PioneerQ38GlobalChainPackage numeric;PioneerQ38TokenStatePlan state;
 const PioneerQ38CanonicalModelPlan* canonical=nullptr;unsigned layer=64;
 bool attempted=false,ready=false;
public:
 bool Build(const PioneerQ38CanonicalModelPlan& c,unsigned selected,unsigned bank,const PioneerQ38GdnIdentity& id,
  const uint32_t* sources,unsigned initialBank,uint32_t initialAddress){
  if(attempted)return false;
  attempted=true;
  if(!numeric.Build(c,selected,bank,id,sources,initialBank,initialAddress)
   || (!PioneerQ38GlobalChainLayout::Dense(selected) && !state.Build(c,selected,bank)))return false;
  canonical=&c;layer=selected;ready=true;return true;
 }
 bool Matches(const PioneerQ38CanonicalModelPlan& c)const{
  return ready && canonical==&c && numeric.Matches(c)
   && (PioneerQ38GlobalChainLayout::Dense(layer) || state.Matches(c));
 }
 const uint32_t* Boot()const{return ready?numeric.Boot():nullptr;}
 const uint32_t* Outer()const{return ready?numeric.Outer():nullptr;}
 unsigned Bytes()const{return ready?unsigned(PioneerQ38GlobalChainLayout::Stride):0;}
 bool Read(unsigned offset,uint8_t* output,unsigned bytes)const{
  namespace S=PioneerQ38TokenStateLayout;
  uintptr_t o=reinterpret_cast<uintptr_t>(output),self=reinterpret_cast<uintptr_t>(this),c=reinterpret_cast<uintptr_t>(canonical);
  if(!canonical || !Matches(*canonical) || !output || !bytes || ((offset|bytes)&63) || bytes>16384
   || offset>Bytes() || bytes>Bytes()-offset || o>UINTPTR_MAX-bytes
   || (o<self+sizeof(*this) && self<o+bytes) || (o<c+sizeof(*canonical) && c<o+bytes))return false;
  // Read crossing a section boundary is serialized without partial refusal.
  unsigned at=0;
  if(offset<numeric.Bytes()){
   at=numeric.Bytes()-offset;if(at>bytes)at=bytes;
   if(!numeric.Read(offset,output,at))return false;
  }
  for(;at<bytes;at++){
   unsigned p=offset+at;
   output[at]=!PioneerQ38GlobalChainLayout::Dense(layer) && p>=S::Offset && p<S::Offset+S::Bytes?
    uint8_t(state.Data()[(p-S::Offset)/4]>>((p&3)*8)):0;
  }
  return true;
 }
};
#endif
