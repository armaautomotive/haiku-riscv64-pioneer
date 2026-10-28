/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GLOBAL_CHAIN_PACKAGE_H
#define PIONEER_Q38_GLOBAL_CHAIN_PACKAGE_H
#include "PioneerQ38LayerChainPackage.h"
#include "PioneerQ38GlobalChainLayout.h"
// Serialized numeric outer/recipe DATA is intentionally identical. The
// proposed ROOT boot is DISTINCT QGC1;this creates no old native ownership.
class PioneerQ38GlobalChainPackage {
 PioneerQ38LayerChainPackage numeric;uint32_t boot[64]={};
 const PioneerQ38CanonicalModelPlan* canonical=nullptr;unsigned layer=64,bank=7;
 bool attempted=false,ready=false;
public:
 bool Build(const PioneerQ38CanonicalModelPlan& c,unsigned selectedLayer,unsigned selectedBank,const PioneerQ38GdnIdentity& id,
  const uint32_t* sources,unsigned initialBank,uint32_t initialAddress){
  namespace C=PioneerQ38GlobalChainLayout;
  if(attempted)return false;
  attempted=true;
  if(!numeric.Build(c,selectedLayer,selectedBank,id,sources,initialBank,initialAddress))return false;
  for(unsigned i=0;i<64;i++)boot[i]=numeric.Boot()[i];
  boot[0]=C::Magic;for(unsigned i=40;i<46;i++)boot[i]=C::BootWord(i);
  canonical=&c;layer=selectedLayer;bank=selectedBank;
  ready=C::LayerFrame(numeric.Outer(),layer,bank,c.Base+c.Workspace(bank)->offset,boot);return ready;
 }
 bool Matches(const PioneerQ38CanonicalModelPlan& c)const{
  return ready && canonical==&c && numeric.Matches(c) && PioneerQ38GlobalChainLayout::LayerFrame(numeric.Outer(),layer,bank,c.Base+c.Workspace(bank)->offset,boot);
 }
 const uint32_t* Boot()const{return ready?boot:nullptr;}
 const uint32_t* Outer()const{return ready?numeric.Outer():nullptr;}
 unsigned Bytes()const{return ready?numeric.Bytes():0;}
 bool Read(unsigned offset,uint8_t* output,unsigned bytes)const{
  uintptr_t o=reinterpret_cast<uintptr_t>(output),self=reinterpret_cast<uintptr_t>(this);
  return canonical && Matches(*canonical) && output && o<=UINTPTR_MAX-bytes && !(o<self+sizeof(*this) && self<o+bytes)
   && numeric.Read(offset,output,bytes);
 }
};
#endif
