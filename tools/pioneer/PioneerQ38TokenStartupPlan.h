/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TOKEN_STARTUP_PLAN_H
#define PIONEER_Q38_TOKEN_STARTUP_PLAN_H
#include "PioneerQ38TokenPackage.h"
#include "PioneerQ38TokenLayout.h"
#include "PioneerQ38GlobalHeadPlan.h"
#include <new>
// Complete immutable startup DATA for the persistent parent. This is NOT
// the three-layer prefix, an initialized native owner, source authentication,
// a completed embedding/state, RUN/STOP or permission to release hardware.
// A future distinct native owner must retain this SAME heap object, canonical
// plan, source, clock, bank reservation, upload ledger and session lifetimes.
class PioneerQ38TokenStartupPlan {
 const PioneerQ38CanonicalModelPlan* canonical=nullptr;
 PioneerQ38TokenPackage* layers[64]={};
 PioneerQ38GlobalHeadPlan head;
 PioneerQ38GdnIdentity identity={};
 uint32_t sources[1920]={},boot[64]={},session[128]={},prompt[8192]={};
 unsigned bank=7,workspace=0;bool attempted=false,ready=false,published=false;
 bool Binding(const PioneerQ38CanonicalModelPlan& c)const{
  return ready && canonical==&c && c.Ready() && bank<7 && identity.Valid()
   && c.Workspace(bank) && workspace==c.Base+c.Workspace(bank)->offset
   && PioneerQ38TokenLayout::Session(session,boot,bank,workspace);
 }
 bool Output(const void* output,unsigned bytes)const{
  const uintptr_t o=reinterpret_cast<uintptr_t>(output),self=reinterpret_cast<uintptr_t>(this),c=reinterpret_cast<uintptr_t>(canonical);
  if(!output || !bytes || o>UINTPTR_MAX-bytes || (o<self+sizeof(*this) && self<o+bytes)
   || (canonical && o<c+sizeof(*canonical) && c<o+bytes))return false;
  // Reject cross-layer aliases too, not only the selected package's object.
  for(const auto* layer:layers)if(layer){const uintptr_t p=reinterpret_cast<uintptr_t>(layer);
   if(o<p+sizeof(*layer) && p<o+bytes)return false;}
  return true;
 }
 bool ReadWords(const uint32_t* data,unsigned length,unsigned offset,uint8_t* output,unsigned bytes)const{
  if(!canonical || !Binding(*canonical) || ((offset|bytes)&63) || !bytes || bytes>16384
   || offset>length || bytes>length-offset || !Output(output,bytes))return false;
  for(unsigned at=0;at<bytes;at++)output[at]=uint8_t(data[(offset+at)/4]>>(((offset+at)&3)*8));
  return true;
 }
public:
 PioneerQ38TokenStartupPlan()=default;
 PioneerQ38TokenStartupPlan(const PioneerQ38TokenStartupPlan&)=delete;
 PioneerQ38TokenStartupPlan& operator=(const PioneerQ38TokenStartupPlan&)=delete;
 ~PioneerQ38TokenStartupPlan(){for(auto* layer:layers)delete layer;}
 bool Build(const PioneerQ38CanonicalModelPlan& c,unsigned selected,const PioneerQ38GdnIdentity& id,const uint32_t* source,
  const uint32_t* input,unsigned prompts,unsigned limit,bool stopEos,unsigned eos0,unsigned eos1){
  namespace T=PioneerQ38TokenLayout;namespace C=T::C;
  if(attempted)return false;
  attempted=true;
  if(!c.Ready() || selected>=7 || !id.Valid() || !source || !input || !prompts || prompts>limit || limit>T::Context
   || !limit || eos0>=248320 || eos1>=248320)return false;
  for(unsigned worker=0;worker<120;worker++)if(!source[worker*16])return false;
  for(unsigned i=0;i<prompts;i++)if(input[i]>=248320)return false;
  const auto* region=c.Workspace(selected);
  if(!region || region->kind!=c.WorkspaceKind || region->bank!=selected || region->bytes!=C::WorkspaceBytes
   || region->allocated!=C::WorkspaceBytes)return false;
  canonical=&c;identity=id;bank=selected;workspace=c.Base+region->offset;
  for(unsigned i=0;i<1920;i++)sources[i]=source[i];
  for(unsigned i=0;i<prompts;i++)prompt[i]=input[i];
  // The position-zero numeric template keeps its established initial raw
  // address. The REAL parent bootstrap later produces its own embedding and
  // TokenMetadata::Outer redirects layer0 to the provisional feedback slot.
  for(unsigned layer=0;layer<64;layer++){
   layers[layer]=new(std::nothrow) PioneerQ38TokenPackage;
   if(!layers[layer] || !layers[layer]->Build(c,layer,bank,id,sources,bank,workspace+0x600000))return false;
  }
  for(unsigned i=0;i<64;i++)boot[i]=layers[0]->Boot()[i];
  if(!head.Build(c,bank,boot))return false;
  const unsigned initial[]={T::Magic,1,128,bank,workspace,limit,prompts,prompt[0]};
  for(unsigned i=0;i<8;i++){session[i]=initial[i];session[24+i]=boot[8+i];}
  for(unsigned i=0;i<16;i++)session[8+i]=sources[i];
  session[32]=unsigned(stopEos);session[33]=eos0;session[34]=eos1;session[35]=T::Context;
  ready=true;if(Matches(c)){published=true;return true;}
  ready=false;return false;
 }
 bool Matches(const PioneerQ38CanonicalModelPlan& c)const{
  namespace C=PioneerQ38GlobalChainLayout;
  if(!Binding(c) || !head.Matches(c))return false;
  const uint64_t ids[]={identity.session,identity.sequence,identity.modelEpoch,identity.driverGeneration};
  for(unsigned i=0;i<4;i++)if(boot[8+i*2]!=uint32_t(ids[i]) || boot[9+i*2]!=uint32_t(ids[i]>>32))return false;
  for(unsigned i=0;i<16;i++)if(boot[16+i]!=sources[i])return false;
  for(unsigned layer=0;layer<64;layer++)if(!layers[layer] || !layers[layer]->Matches(c)
   || !C::LayerFrame(layers[layer]->Outer(),layer,bank,workspace,boot))return false;
  for(unsigned i=0;i<8192;i++)if(i<session[6]?prompt[i]>=248320:prompt[i]!=0)return false;
  return prompt[0]==session[7];
 }
 bool MatchesBinding(const PioneerQ38CanonicalModelPlan& c,unsigned selected,const PioneerQ38GdnIdentity& id,const uint32_t* source)const{
  if(!Binding(c) || bank!=selected || !(identity==id) || !source)return false;
  for(unsigned i=0;i<1920;i++)if(sources[i]!=source[i])return false;
  return Matches(c); // immutable relation ONLY; no native lifetime grant
 }
 bool MatchesPublishedBinding(const PioneerQ38CanonicalModelPlan& c,unsigned selected,const PioneerQ38GdnIdentity& id,const uint32_t* source)const{
  // Build publishes ONLY after the complete strict Matches(c) succeeds.
  // This object is noncopyable/one-shot;all fields+packages are private,
  // read APIs are const and reject aliases into ANY owned package/model.
  // A native owner may borrow this immutable publication under its SAME
  // lock/lifetime. This is NOT random-memory-corruption revalidation or a
  // source/initialization/launch permission. Strict MatchesBinding remains.
  if(!published || !Binding(c) || bank!=selected || !(identity==id) || !source)return false;
  for(unsigned i=0;i<1920;i++)if(sources[i]!=source[i])return false;
  return true;
 }
 static bool CanonicalLearnedPagesPresent(const PioneerQ38CanonicalModelPlan& model,const uint8_t* ledger,unsigned count){
  if(!model.Ready() || !ledger || count!=PioneerResidentBanks::Pages)return false;
  const auto* canonical=&model;
  unsigned parts[65]={};
  for(unsigned i=0;i<canonical->Count();i++){
   const auto* p=canonical->Part(i);if(!p)return false;
   if(p->kind!=canonical->WeightKind)continue;
   if(p->layer>64 || p->bank>=7 || !p->allocated || ((p->offset|p->allocated)&4095) || !p->bytes
    || p->bytes>p->allocated || p->offset>canonical->Capacity || p->allocated>canonical->Capacity-p->offset)return false;
   unsigned first=p->bank*unsigned(canonical->Capacity/4096)+p->offset/4096,pages=p->allocated/4096;
   if(first>=count || pages>count-first)return false;
   for(unsigned page=0;page<pages;page++)if(ledger[first+page]!=1)return false;
   parts[p->layer]++;
  }
  for(unsigned layer=0;layer<64;layer++)if(parts[layer]!=(PioneerQ38GlobalChainLayout::Dense(layer)?53U:56U))return false;
  return parts[64]==15; // ALL3551 learned parts/851 tensors, including embedding/head/norm.
 } // page PRESENCE only,not authenticated uploads/current/history/KV init
 bool LearnedPagesPresent(const uint8_t* ledger,unsigned count)const{
  return canonical && Binding(*canonical) && CanonicalLearnedPagesPresent(*canonical,ledger,count);
 }
 bool PackageAddress(unsigned layer,uint32_t& output)const{
  uint32_t address=0;
  if(!canonical || !Binding(*canonical) || !Output(&output,4)
   || !PioneerQ38GlobalChainLayout::Address(workspace,layer,address))return false;
  output=address;return true;
 }
 bool ReadPackage(unsigned layer,unsigned offset,uint8_t* output,unsigned bytes)const{
  return canonical && Binding(*canonical) && layer<64 && layers[layer] && Output(output,bytes)
   && layers[layer]->Read(offset,output,bytes);
 }
 bool ReadSession(unsigned offset,uint8_t* output,unsigned bytes)const{return ReadWords(session,512,offset,output,bytes);}
 bool ReadPrompt(unsigned offset,uint8_t* output,unsigned bytes)const{return ReadWords(prompt,8192*4,offset,output,bytes);}
 bool ReadHead(unsigned offset,uint8_t* output,unsigned bytes)const{
  return canonical && head.Matches(*canonical) && ReadWords(head.Data(),512,offset,output,bytes);
 }
 bool ReadBoot(unsigned worker,uint32_t* output)const{
  namespace G=PioneerQ38GlobalWorkerLayout;
  if(!canonical || !Binding(*canonical) || worker>=120 || (reinterpret_cast<uintptr_t>(output)&3) || !Output(output,256))return false;
  uint32_t data[64]={};
  if(!worker)for(unsigned i=0;i<64;i++)data[i]=boot[i];
  else{
   const unsigned header[]={G::Old::Magic,1,64,worker,G::Peer(worker),sources[worker*16],8192,64};
   for(unsigned i=0;i<8;i++){data[i]=header[i];data[8+i]=boot[8+i];}
   for(unsigned i=0;i<16;i++)data[16+i]=sources[worker*16+i];
   // Exactly G::BootShape's underlying grammar BEFORE adding its fixed
   // magic/witnesses. Avoid a second 256B temporary on the kernel stack;
   // no caller data can supply those six witness words.
   if(!G::Old::BootShape(data))return false;
   data[0]=G::Magic;for(unsigned i=40;i<46;i++)data[i]=G::BootWord(i);
  }
  for(unsigned i=0;i<64;i++)output[i]=data[i];
  return true;
 } // immutable boot DATA only;no ROM,source or participant ownership grant
 const uint32_t* Boot()const{return ready?boot:nullptr;}
 unsigned Bank()const{return ready?bank:7;}
 uint32_t Workspace()const{return ready?workspace:0;}
};
static_assert(sizeof(PioneerQ38TokenStartupPlan)>32768 && sizeof(PioneerQ38TokenStartupPlan)<65536,
 "complete startup DATA and prompt are HEAP ONLY,never kernel-stack objects");
#endif
