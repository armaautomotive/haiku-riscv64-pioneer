/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GDN_LAYER_PROGRESS_H
#define PIONEER_Q38_GDN_LAYER_PROGRESS_H
#include "PioneerQ38GdnIdentity.h"
// CPU metadata ONLY,not a producer,initialized-state/read/compute grant,
// token commit,or evidence that intervening full-attention layers ran.
// The future SAME bank/decoder owner must keep this object PRIVATE and
// call it only after its independent original-source/producer/receipt/
// ALL-core-hold proofs. In particular BindAfterFirstLayer does NOT create
// the existing native first-layer seal or replace OwnsFirstLayer.
// One token-position0 private-NEXT traversal;NO reset/rebind/current flip.
class PioneerQ38GdnLayerProgress {
 const void* owner=nullptr;
 PioneerQ38GdnIdentity identity={};
 uint64_t published=0;
 unsigned phases[48]={},lastPhase=0,nextLayer=1,pendingPhase=0;
 bool bound=false,pending=false,unsafe=false;
 static unsigned Ordinal(unsigned layer){return layer<64 && layer%4!=3?layer-layer/4:48;}
 bool Matches(const void* who,const PioneerQ38GdnIdentity& id)const{
  return bound && !unsafe && who && who==owner && id==identity;
 }
public:
 PioneerQ38GdnLayerProgress()=default;
 PioneerQ38GdnLayerProgress(const PioneerQ38GdnLayerProgress&)=delete;
 PioneerQ38GdnLayerProgress& operator=(const PioneerQ38GdnLayerProgress&)=delete;
 bool BindAfterFirstLayer(const void* who,const PioneerQ38GdnIdentity& id,unsigned phase){
  if(bound || unsafe || !who || !id.Valid() || !phase || phase>=0x7fffffff)return false;
  owner=who;identity=id;published=1;phases[0]=lastPhase=phase;bound=true;return true;
 }
 bool ClaimNext(const void* who,const PioneerQ38GdnIdentity& id,unsigned layer,unsigned phase){
  if(!Matches(who,id) || pending || layer!=nextLayer || Ordinal(layer)>=48
   || phase<=lastPhase || phase>=0x7fffffff)return false;
  pending=true;pendingPhase=phase;return true; // metadata only,NO IO/producer authority
 }
 bool OwnsPending(const void* who,const PioneerQ38GdnIdentity& id,unsigned layer,unsigned phase)const{
  return Matches(who,id) && pending && layer==nextLayer && Ordinal(layer)<48 && phase==pendingPhase;
 }
 bool PublishNext(const void* who,const PioneerQ38GdnIdentity& id,unsigned layer,unsigned phase){
  if(!OwnsPending(who,id,layer,phase))return false;
  unsigned ordinal=Ordinal(layer);uint64_t bit=UINT64_C(1)<<ordinal;
  if(published&bit)return false;
  published|=bit;phases[ordinal]=lastPhase=phase;pending=false;pendingPhase=0;
  nextLayer=layer+1;if(nextLayer%4==3)nextLayer++;
  return true; // private per-layer NEXT metadata,NEVER whole-token/current commit
 }
 bool Published(const void* who,const PioneerQ38GdnIdentity& id,unsigned layer,unsigned phase)const{
  unsigned ordinal=Ordinal(layer);
  return Matches(who,id) && ordinal<48 && (published&(UINT64_C(1)<<ordinal)) && phases[ordinal]==phase;
 }
 bool Snapshot(const void* who,const PioneerQ38GdnIdentity& id,uint64_t& mask,unsigned& next,bool& active)const{
  if(!Matches(who,id))return false;
  mask=published;next=nextLayer;active=pending;return true; // counts are NOT a native readiness seal
 }
 void Fault(){unsafe=true;} // sticky;NO automatic reset/rebind/release
};
static_assert(sizeof(PioneerQ38GdnLayerProgress)<=320,"small CPU metadata,not per-layer state arrays or numeric products");
#endif
