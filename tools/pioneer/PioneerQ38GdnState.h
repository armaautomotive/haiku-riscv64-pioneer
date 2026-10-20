/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GDN_STATE_H
#define PIONEER_Q38_GDN_STATE_H
#include <stdint.h>
// CPU METADATA policy for ONE model/sequence/session. No allocation,address,
// NoC/DMA,hardware proof,initialization,reset or model capability lives here.
// A native coordinator must independently own/initialize BOTH state buffers
// and call CompletedHalf ONLY after its full hardware completion proof.
// Neither a ticket nor a slot reference is a device-access capability.
struct PioneerQ38GdnIdentity {
 uint64_t session,sequence,modelEpoch,driverGeneration;
 bool operator==(const PioneerQ38GdnIdentity& b) const {
  return session==b.session && sequence==b.sequence && modelEpoch==b.modelEpoch && driverGeneration==b.driverGeneration;
 }
};
struct PioneerQ38GdnTicket {
 PioneerQ38GdnIdentity owner;
 uint64_t position,nonce;
 unsigned layer,readSlot,writeSlot;
};
struct PioneerQ38GdnStateSlice {
 PioneerQ38GdnIdentity owner;
 uint64_t position,nonce;
 unsigned layer,slot,head,half,keyHead,firstWord,words;
};
class PioneerQ38GdnState {
 enum Phase { Empty,Ready,Token,Layer,Poisoned,Closed } phase=Empty;
 PioneerQ38GdnIdentity identity={};
 uint64_t position=0,nonce=0;
 unsigned slot=0,completedLayers=0;
 uint64_t completed[2]={};
 static unsigned Ordinal(unsigned layer) { return layer<64 && layer%4!=3?layer-layer/4:~0U; }
 bool Header(const PioneerQ38GdnTicket& t) const {
  return t.owner==identity && t.position==position && t.nonce==nonce && t.readSlot==slot && t.writeSlot==(slot^1)
   && Ordinal(t.layer)<48;
 }
 bool Matches(const PioneerQ38GdnTicket& t) const {
  return Header(t) && Ordinal(t.layer)==completedLayers;
 }
 void Describe(const PioneerQ38GdnTicket& t,unsigned worker,bool next,PioneerQ38GdnStateSlice& output) const {
  unsigned head=worker/2;
  output={identity,position,nonce,t.layer,next?t.writeSlot:t.readSlot,head,worker%2,head%16,worker*8192,8192};
 }
public:
 PioneerQ38GdnState()=default;
 PioneerQ38GdnState(const PioneerQ38GdnState&)=delete;
 PioneerQ38GdnState& operator=(const PioneerQ38GdnState&)=delete;
 // Call ONLY after native zero-initialization/import has independently
 // completed. Binding metadata does NOT initialize any device memory.
 bool Bind(const PioneerQ38GdnIdentity& id) {
  if (phase!=Empty || !id.session || !id.sequence || !id.modelEpoch || !id.driverGeneration) return false;
  identity=id; phase=Ready; return true;
 }
 bool BeginToken(const PioneerQ38GdnIdentity& id,uint64_t at) {
  if (phase!=Ready || !(id==identity) || at!=position || position==UINT64_MAX || nonce==UINT64_MAX) return false;
  nonce++; completedLayers=0; phase=Token; return true;
 }
 bool BeginLayer(unsigned layer,PioneerQ38GdnTicket& output) {
  if (phase!=Token || Ordinal(layer)!=completedLayers || completedLayers>=48) return false;
  completed[0]=completed[1]=0; phase=Layer;
  output={identity,position,nonce,layer,slot,slot^1}; return true;
 }
 bool Slice(const PioneerQ38GdnTicket& t,unsigned worker,bool next,PioneerQ38GdnStateSlice& output) const {
  if (phase!=Layer || !Matches(t) || worker>=96) return false;
  Describe(t,worker,next,output);
  return true;
 }
 bool CompletedStateView(const PioneerQ38GdnTicket& t,unsigned worker,PioneerQ38GdnStateSlice& output) const {
  if ((phase!=Token && phase!=Layer) || !Header(t) || Ordinal(t.layer)>=completedLayers || worker>=96) return false;
  Describe(t,worker,true,output); return true;
 }
 bool CompletedHalf(const PioneerQ38GdnTicket& t,unsigned worker) {
  if (phase!=Layer || !Matches(t) || worker>=96) return false;
  uint64_t mask=UINT64_C(1)<<(worker%64);
  if (completed[worker/64]&mask) return false; // no duplicate/stale proof acceptance
  completed[worker/64]|=mask; return true;
 }
 bool CompleteLayer(const PioneerQ38GdnTicket& t) {
  if (phase!=Layer || !Matches(t) || completed[0]!=UINT64_MAX || completed[1]!=UINT64_C(0xffffffff)) return false;
  completedLayers++; phase=Token; return true;
 }
 // ALL48 recurrent layers complete BEFORE the current slot flips ONCE.
 // Conv/KV/output publication still needs the outer whole-model owner;
 // this object alone does NOT commit an entire model token.
 bool CommitToken(const PioneerQ38GdnIdentity& id,uint64_t at) {
  if (phase!=Token || !(id==identity) || at!=position || completedLayers!=48) return false;
  slot^=1; position++; phase=Ready; return true;
 }
 bool Snapshot(PioneerQ38GdnIdentity& id,uint64_t& at,unsigned& currentSlot) const {
  if (phase!=Ready) return false;
  id=identity; at=position; currentSlot=slot; return true;
 }
 void Fault() { if (phase!=Closed) phase=Poisoned; } // sticky;NO rollback/reset/rebind
 bool Unsafe() const { return phase==Poisoned || phase==Token || phase==Layer; }
 bool Close() { if (phase!=Ready) return false; phase=Closed; return true; }
};
#endif
