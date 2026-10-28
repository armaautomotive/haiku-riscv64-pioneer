/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_LAYER_CHAIN_PACKAGE_H
#define PIONEER_Q38_LAYER_CHAIN_PACKAGE_H
#include "PioneerQ38LayerChainLayout.h"
#include "PioneerQ38MixedRecurrentBlockPlan.h"
#include "PioneerQ38MixedLiveBlockPlan.h"
// Canonical per-layer serialized DATA, heap only. No file authentication,
// completed embedding/state, native initialization/ownership or token grant.
class PioneerQ38LayerChainPackage {
 const PioneerQ38CanonicalModelPlan* canonical=nullptr;
 PioneerQ38MixedRecurrentBlockPlan recurrent;PioneerQ38MixedLiveBlockPlan dense;
 uint32_t boot[64]={};unsigned layer=64,bank=7;bool attempted=false,ready=false;
 const uint32_t* OuterData()const{return PioneerQ38LayerChainLayout::Dense(layer)?dense.Outer():recurrent.Outer();}
 uint32_t Word(unsigned word)const{
  namespace L=PioneerQ38LayerChainLayout;
  if(word<128)return OuterData()[word];
  if(L::Dense(layer)){
   unsigned at=word-L::DenseRecipes/4;
   return word<L::DenseFrames/4?dense.Recipe(at/1280)[at%1280]:dense.Dense((word-L::DenseFrames/4)/64)[(word-L::DenseFrames/4)%64];
  }
  if(word<L::P::ConvOffset/4){unsigned at=word-L::P::RecipesOffset/4;return recurrent.Recipe(at/1280)[at%1280];}
  return word<L::P::GdnOffset/4?recurrent.Conv()[word-L::P::ConvOffset/4]:recurrent.Gdn()[word-L::P::GdnOffset/4];
 }
 bool Output(const void* output,unsigned bytes)const{
  const uintptr_t p=reinterpret_cast<uintptr_t>(this),c=reinterpret_cast<uintptr_t>(canonical),o=reinterpret_cast<uintptr_t>(output);
  return output && bytes && o<=UINTPTR_MAX-bytes && !(o<p+sizeof(*this) && p<o+bytes)
   && (!canonical || !(o<c+sizeof(*canonical) && c<o+bytes));
 }
public:
 PioneerQ38LayerChainPackage()=default;
 PioneerQ38LayerChainPackage(const PioneerQ38LayerChainPackage&)=delete;
 PioneerQ38LayerChainPackage& operator=(const PioneerQ38LayerChainPackage&)=delete;
 bool Build(const PioneerQ38CanonicalModelPlan& c,unsigned selectedLayer,unsigned selectedBank,const PioneerQ38GdnIdentity& id,
  const uint32_t* sources,unsigned initialBank,uint32_t initialAddress){
  namespace L=PioneerQ38LayerChainLayout;namespace A=L::A;
  if(attempted)return false;
  attempted=true;
  if(!c.Ready() || selectedLayer>=64 || selectedBank>=7 || !id.Valid() || !sources || initialBank>=7)return false;
  const auto* workspace=c.Workspace(selectedBank);if(!workspace || workspace->bytes!=L::WorkspaceBytes || workspace->allocated!=L::WorkspaceBytes)return false;
  const uint32_t base=c.Base+workspace->offset;
  const unsigned inputBank=selectedLayer?selectedBank:initialBank;
  const uint32_t input=selectedLayer?base+L::R::FinalOffset:initialAddress;
  const uint32_t* oldBoot;
  if(L::Dense(selectedLayer)){
   if(!dense.Build(c,selectedLayer,0,0,selectedBank,1001+selectedLayer*16,selectedLayer+1,id,sources,inputBank,input))return false;
   oldBoot=dense.Boot(0);
  }else{
   if(!recurrent.Build(c,selectedLayer,0,0,selectedBank,0,1001+selectedLayer*16,selectedLayer+1,id,sources,inputBank,input))return false;
   oldBoot=recurrent.Boot(0);
  }
  canonical=&c;layer=selectedLayer;bank=selectedBank;
  for(unsigned i=0;i<64;i++)boot[i]=oldBoot[i];
  boot[0]=A::Magic;
  for(unsigned i=40;i<46;i++)boot[i]=A::BootWord(i);
  ready=L::Frame(OuterData(),layer,bank,base,boot);return ready;
 }
 bool Matches(const PioneerQ38CanonicalModelPlan& c)const{
  namespace L=PioneerQ38LayerChainLayout;
  if(!ready || canonical!=&c || !c.Ready() || layer>=64 || bank>=7)return false;
  return (L::Dense(layer)?dense.Matches(c):recurrent.Matches(c))
   && L::Frame(OuterData(),layer,bank,c.Base+c.Workspace(bank)->offset,boot);
 }
 unsigned Bytes()const{return ready?PioneerQ38LayerChainLayout::Bytes(layer):0;}
 const uint32_t* Outer()const{return ready?OuterData():nullptr;}
 const uint32_t* Boot()const{return ready?boot:nullptr;}
 bool Read(unsigned offset,uint8_t* output,unsigned bytes)const{
  if(!canonical || !Matches(*canonical) || (offset&63) || !bytes || (bytes&63) || bytes>16384
   || offset>Bytes() || bytes>Bytes()-offset || !Output(output,bytes))return false;
  for(unsigned at=0;at<bytes;at+=4){const uint32_t value=Word((offset+at)/4);for(unsigned b=0;b<4;b++)output[at+b]=uint8_t(value>>(b*8));}
  return true; // immutable CPU serialization only,never device IO or readiness
 }
};
static_assert(sizeof(PioneerQ38LayerChainPackage)>1048576 && sizeof(PioneerQ38LayerChainPackage)<3*1048576,"per-layer canonical PACKAGE heap only");
#endif
