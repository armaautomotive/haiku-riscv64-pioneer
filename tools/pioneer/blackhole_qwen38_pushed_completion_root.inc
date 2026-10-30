/* SPDX-License-Identifier: MIT */
// Included inside Projection, ONLY in the separate QGW3 returning profile.
// Shared code has fixed DATA geometry and private code-generated callbacks;
// no public callback/address selector or native/Work grant is added.
namespace PushedL=PioneerQ38MixedDenseRootLayout;
static_assert(unsigned(L::Proofs)==unsigned(M::Mailbox)
 && unsigned(L::PeerBoots)==unsigned(PushedL::PeerBoots)
 && unsigned(L::PeerJobs)==unsigned(PushedL::PeerJobs)
 && unsigned(L::Counters)==unsigned(PushedL::Counters)
 && unsigned(L::Nonces)==unsigned(PushedL::Nonces),"identical phase-exclusive shared receipt DATA fields");
using PushedSeal=void(*)(const uint32_t*);
using PushedFrameSeal=void(*)(unsigned);
// One ODR identity across components;shared bodies never refer to TU-local
// implementations. Normal function-specific COMDAT sections,not one shared
// explicit weak section,retain every actual function.
inline volatile uint32_t* PushedWords(unsigned a){return reinterpret_cast<volatile uint32_t*>(a);}
inline void PushedFence(){asm volatile("fence" ::: "memory");}
inline unsigned PushedDelta(unsigned i){
 // Same complete mapping,including zero outside the eight counter views.
 return i==5?5U:i<8?2U*((0xd4U>>i)&1U):0U;
}
inline unsigned PushedSlaveCounter(unsigned i){return i==0?0x2f0:i==1?0x2e0:i==2?0x2e8:0x2c4;}
inline unsigned PushedCounter(unsigned i){return i==0?0x208:i==1?0x20c:(i==2 || i==6)?0x210:i==3?0x214:i==4?0x204:i==5?0x220:0x228;}
inline unsigned PushedSlot(unsigned w){if(w>=119)scalar_fault_park();return PioneerQ38PushedWorkerLayout::Mailbox+w*256;}
extern "C" inline __attribute__((noinline,noclone)) void q38_pushed_begin_shared(
 const uint32_t* d,uint32_t* tracked,uint32_t* slave,PushedSeal seal,PushedFrameSeal frame){
 if(!d[7] || d[7]>119)scalar_fault_park();
 seal(d);for(unsigned w=0;w<119;w++)frame(w);
 // All119 prior closures have been admitted. A normal matrix may use its
 // retained receipt+expected frontier instead of a new remote WAIT audit.
 // Its commit was already received/ACK-sent;no later payload write exists.
 // The worker may still finish its final local seals before taking a queued
 // descriptor. Dense summaries are dead before these slots are cleared.
 for(unsigned i=0;i<4;i++)slave[i]=*PushedWords(0xffb20000+PushedSlaveCounter(i));
 for(unsigned w=0;w<d[7];w++)for(unsigned i=0;i<64;i++)PushedWords(PushedSlot(w))[i]=0;
 PushedFence(); // drains clear stores AND flushes non-coherent L0.
 for(unsigned w=0;w<d[7];w++)for(unsigned i=0;i<64;i++)if(PushedWords(PushedSlot(w))[i])scalar_fault_park();
 for(unsigned i=0;i<4;i++)if(*PushedWords(0xffb20000+PushedSlaveCounter(i))!=slave[i])scalar_fault_park();
 for(unsigned i=0;i<8;i++)if(*PushedWords(0xffb20000+PushedCounter(i))!=tracked[i])scalar_fault_park();
 seal(d);
}
extern "C" inline __attribute__((noinline,noclone)) void q38_pushed_receipt_shared(
 const uint32_t* d,unsigned w,const uint32_t* expectedAfter,PushedSeal packetSeal){
 uint32_t receipt[64];PushedFence();
 for(unsigned i=0;i<64;i++)receipt[i]=PushedWords(PushedSlot(w))[i];
 PushedFence();
 const auto* job=reinterpret_cast<const uint32_t*>(PushedL::PeerJobs+w*256);
 if(job[1]!=PushedWords(PushedL::Nonces)[w]
  || PushedWords(PushedSlot(w))[0]!=PioneerQ38PushedWorkerLayout::ReceiptMagic)scalar_fault_park();
 for(unsigned i=0;i<8;i++)if(receipt[56+i]+PushedDelta(i)!=expectedAfter[i])scalar_fault_park();
 // The receipt's counter words are DATA,not self-authenticating authority:
 // all eight were just bound to the independently retained expected frontier.
 if(!PioneerQ38PushedWorkerLayout::Receipt(receipt,
  reinterpret_cast<const uint32_t*>(PushedL::PeerBoots+w*256),job,receipt+56))scalar_fault_park();
 packetSeal(d);
}
static void PushedReceipt(const uint32_t* d,unsigned w,const uint32_t* expectedAfter){
 q38_pushed_receipt_shared(d,w,expectedAfter,q38_mixed_root_packet_seal);
}
static void PushedBegin(const uint32_t* d,uint32_t* tracked,uint32_t* slave){
 q38_pushed_begin_shared(d,tracked,slave,Seal,FrameSeal);
}
static void PushedWait(const uint32_t* d,unsigned w,const uint32_t* expectedAfter,unsigned groups,unsigned inputs){
 const unsigned limit=M::ReceiptPollBudget(groups,inputs);
 if(!limit)scalar_fault_park();
 for(unsigned poll=0;poll<limit;poll++){
  // Volatile alone cannot see another NoC client's updates through L0.
  Fence();unsigned marker=*Words(PushedSlot(w));
  if(marker==M::ReceiptMagic){PushedReceipt(d,w,expectedAfter);return;}
  if(marker)scalar_fault_park();
  q38_mixed_root_packet_seal(d);
 }
 scalar_fault_park(); // no synthetic ready, later packet,retry,reset or release.
}
extern "C" inline __attribute__((noinline,noclone)) void q38_pushed_end_shared(
 const uint32_t* d,const uint32_t* slave,PushedSeal seal,PushedSeal packetSeal){
 if(!d[7] || d[7]>119)scalar_fault_park();
 bool done=false;
 for(unsigned poll=0;poll<65536;poll++){
  PushedFence();done=true;
  for(unsigned i=0;i<4;i++){
   const unsigned expected=d[7]*(i==1?5U:2U);
   const uint32_t delta=*PushedWords(0xffb20000+PushedSlaveCounter(i))-slave[i];
   if(delta>expected)scalar_fault_park();
   done &= delta==expected;
  }
  if(done)break;
  packetSeal(d);
 }
 if(!done)scalar_fault_park();
 for(unsigned w=0;w<d[7];w++)q38_pushed_receipt_shared(d,w,
  reinterpret_cast<const uint32_t*>(PushedL::Counters+w*32),packetSeal);
 seal(d);
}
static void PushedEnd(const uint32_t* d,const uint32_t* slave){
 q38_pushed_end_shared(d,slave,Seal,q38_mixed_root_packet_seal);
}
