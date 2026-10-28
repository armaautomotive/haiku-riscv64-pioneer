/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_RECURRENT_PREFIX_FROZEN_PLAN
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38RecurrentPrefixFrozenPlanRole.h"
#endif
#ifndef PIONEER_Q38_RECURRENT_PREFIX_PLAN_H
#define PIONEER_Q38_RECURRENT_PREFIX_PLAN_H
#include "PioneerQ38MixedRecurrentBlockPlan.h"
#include "PioneerQ38RecurrentPrefixLayout.h"
// Complete immutable HEAP DATA for the autonomous token0/position0 prefix.
// Not source authentication, bank/state ownership, a completed QPE1 producer,
// a native role, a launch or permission to publish a whole-token state update.
class PioneerQ38RecurrentPrefixPlan {
 const PioneerQ38CanonicalModelPlan* canonical=nullptr;
 PioneerQ38MixedRecurrentBlockPlan layers[3];
 uint32_t boots[120][64]={};PioneerQ38GdnIdentity identity={};
 unsigned bank=7;uint32_t workspace=0;bool attempted=false,ready=false;
 bool Output(const void* value,unsigned bytes)const{
  const uintptr_t o=reinterpret_cast<uintptr_t>(value),p=reinterpret_cast<uintptr_t>(this),c=reinterpret_cast<uintptr_t>(canonical);
  return value && bytes && o<=UINTPTR_MAX-bytes && !(o<p+sizeof(*this) && p<o+bytes)
   && (!canonical || !(o<c+sizeof(*canonical) && c<o+bytes));
 }
 uint32_t Word(unsigned layer,unsigned ordinal)const{
  namespace P=PioneerQ38RecurrentPrefixLayout;
  if(ordinal<P::RecipesOffset/4)return layers[layer].Outer()[ordinal];
  if(ordinal<P::ConvOffset/4){unsigned at=ordinal-P::RecipesOffset/4;return layers[layer].Recipe(at/1280)[at%1280];}
  return ordinal<P::GdnOffset/4?layers[layer].Conv()[ordinal-P::ConvOffset/4]:layers[layer].Gdn()[ordinal-P::GdnOffset/4];
 }
public:
 PioneerQ38RecurrentPrefixPlan()=default;
 PioneerQ38RecurrentPrefixPlan(const PioneerQ38RecurrentPrefixPlan&)=delete;
 PioneerQ38RecurrentPrefixPlan& operator=(const PioneerQ38RecurrentPrefixPlan&)=delete;
 bool Build(const PioneerQ38CanonicalModelPlan& c,unsigned selected,const PioneerQ38GdnIdentity& id,const uint32_t* sources){
  namespace P=PioneerQ38RecurrentPrefixLayout;namespace R=PioneerQ38MixedRecurrentBlockRootLayout;
  if(attempted)return false;
  attempted=true;
  if(!c.Ready() || selected>=7 || !id.Valid() || !sources)return false;
  const auto* region=c.Workspace(selected);
  if(!region || region->kind!=c.WorkspaceKind || region->bank!=selected || region->bytes!=67108864 || region->allocated!=67108864)return false;
  const uint32_t base=c.Base+region->offset;
  for(unsigned layer=0;layer<P::Layers;layer++)if(!layers[layer].Build(c,layer,0,0,selected,0,1001+layer*16,layer+1,id,sources,
   selected,base+(layer?unsigned(R::FinalOffset):0U)))return false;
  canonical=&c;identity=id;bank=selected;workspace=base;
  for(unsigned w=0;w<120;w++)for(unsigned i=0;i<64;i++)boots[w][i]=!w && i>=40 && i<46?P::BootWord(i):layers[0].Boot(w)[i];
  ready=true;if(Matches(c))return true;
  ready=false;return false;
 }
 bool Matches(const PioneerQ38CanonicalModelPlan& c)const{
  namespace P=PioneerQ38RecurrentPrefixLayout;namespace R=PioneerQ38MixedRecurrentBlockRootLayout;
  if(!ready || canonical!=&c || !c.Ready() || bank>=7 || !identity.Valid() || !P::BootShape(boots[0]))return false;
  const uint64_t ids[]={identity.session,identity.sequence,identity.modelEpoch,identity.driverGeneration};
  for(unsigned i=0;i<4;i++)if(boots[0][8+i*2]!=uint32_t(ids[i]) || boots[0][9+i*2]!=uint32_t(ids[i]>>32))return false;
  const auto* region=c.Workspace(bank);
  if(!region || region->kind!=c.WorkspaceKind || region->bank!=bank || region->bytes!=67108864 || region->allocated!=67108864
   || workspace!=c.Base+region->offset)return false;
  for(unsigned layer=0;layer<P::Layers;layer++){
   const auto& p=layers[layer];if(!p.Matches(c) || !P::Frame(p.Outer(),layer,bank,workspace,boots[0]))return false;
   if(p.Outer()[68]!=bank || p.Outer()[69]!=workspace+(layer?unsigned(R::FinalOffset):0U))return false;
   for(unsigned w=0;w<120;w++)for(unsigned i=0;i<64;i++){
    const unsigned expected=!w && i>=40 && i<46?P::BootWord(i):p.Boot(w)[i];
    if(boots[w][i]!=expected)return false;
   }
  }
  return true; // SAME canonical pointer, source identities and all exact recipes
 }
 bool MatchesBinding(const PioneerQ38CanonicalModelPlan& c,unsigned selected,const PioneerQ38GdnIdentity& id,const uint32_t* sources)const{
  if(!ready || canonical!=&c || bank!=selected || !(identity==id) || !id.Valid() || !sources)return false;
  for(unsigned w=0;w<120;w++)for(unsigned i=0;i<16;i++)if(boots[w][16+i]!=sources[w*16+i])return false;
  return Matches(c); // immutable relation ONLY; never a native source owner
 }
#ifdef PIONEER_Q38_RECURRENT_PREFIX_FROZEN_PLAN
 bool MatchesFrozenBindingValues(const PioneerQ38CanonicalModelPlan& c,unsigned selected,const PioneerQ38GdnIdentity& id,const uint32_t* sources)const{
  if(!ready || canonical!=&c || bank!=selected || !(identity==id) || !id.Valid() || !sources)return false;
  for(unsigned w=0;w<120;w++)for(unsigned i=0;i<16;i++)if(boots[w][16+i]!=sources[w*16+i])return false;
  return true; // DATA ONLY;caller must ALSO own its exact private frozen comparison
 }
#endif
 bool LearnedPagesPresent(const uint8_t* ledger,unsigned count)const{
  if(!canonical || !Matches(*canonical))return false;
  for(const auto& layer:layers)if(!layer.LearnedPagesPresent(ledger,count))return false;
  return true; // not current/history/embedding/metadata initialization
 }
 bool PackageAddress(unsigned layer,uint32_t& output)const{
  uint32_t address=0;
  if(!canonical || !Matches(*canonical) || !Output(&output,4) || !PioneerQ38RecurrentPrefixLayout::Address(workspace,layer,address))return false;
  output=address;return true;
 }
 bool ReadPackage(unsigned layer,unsigned offset,uint8_t* output,unsigned bytes)const{
  namespace P=PioneerQ38RecurrentPrefixLayout;
  if(!canonical || layer>=P::Layers || (offset&63) || !bytes || (bytes&63) || bytes>16384 || offset>P::Bytes || bytes>P::Bytes-offset
   || !Output(output,bytes) || !Matches(*canonical))return false;
  // Pure immutable CPU byte copy, no callback/IO/allocation/owner changes.
  // Transport must separately own staging and recheck its genuine call scope.
  for(unsigned i=0;i<bytes;i+=4){const uint32_t word=Word(layer,(offset+i)/4);
   for(unsigned b=0;b<4;b++)output[i+b]=uint8_t(word>>(b*8));
  }
  return true;
 }
 const PioneerQ38MixedRecurrentBlockPlan* Layer(unsigned layer)const{return ready && layer<3?&layers[layer]:nullptr;}
 const uint32_t* Boot(unsigned worker)const{return ready && worker<120?boots[worker]:nullptr;}
 unsigned Bank()const{return ready?bank:7;}
 uint32_t Workspace()const{return ready?workspace:0;}
};
static_assert(sizeof(PioneerQ38RecurrentPrefixPlan)>3*1048576 && sizeof(PioneerQ38RecurrentPrefixPlan)<6*1048576,
 "all3 immutable layer packages and bound plans HEAP ONLY, never kernel stack");
#endif
