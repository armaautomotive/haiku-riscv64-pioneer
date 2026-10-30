/* SPDX-License-Identifier: MIT */
// Included inside Projection, ONLY in the separate QGW3 returning profile.
// Actual ROOT receipt consumer,not a HOST ready/nonce setter or native grant.
static_assert(unsigned(L::Proofs)==unsigned(M::Mailbox),"phase-exclusive existing ROOT proof slots");
static unsigned PushedDelta(unsigned i){return i==5?5U:i==2 || i==4 || i==6 || i==7?2U:0U;}
static unsigned SlaveCounter(unsigned i){return i==0?0x2f0:i==1?0x2e0:i==2?0x2e8:0x2c4;}
static unsigned PushedSlot(unsigned w){if(w>=119)scalar_fault_park();return M::Mailbox+w*256;}
static __attribute__((noinline)) void PushedBegin(const uint32_t* d,uint32_t* tracked,uint32_t* slave){
 if(!d[7] || d[7]>119)scalar_fault_park();
 Seal(d);for(unsigned w=0;w<119;w++)FrameSeal(w);
 // All119 genuine prior WAIT/counters have been admitted BEFORE this call;
 // no previous sender can still own these slots. Dense summaries are dead.
 for(unsigned i=0;i<4;i++)slave[i]=*Words(0xffb20000+SlaveCounter(i));
 for(unsigned w=0;w<d[7];w++)for(unsigned i=0;i<64;i++)Words(PushedSlot(w))[i]=0;
 Fence(); // drains local clear stores AND flushes non-coherent L0 data cache.
 for(unsigned w=0;w<d[7];w++)for(unsigned i=0;i<64;i++)if(Words(PushedSlot(w))[i])scalar_fault_park();
 for(unsigned i=0;i<4;i++)if(*Words(0xffb20000+SlaveCounter(i))!=slave[i])scalar_fault_park();
 for(unsigned i=0;i<8;i++)if(*Words(0xffb20000+Counter(i))!=tracked[i])scalar_fault_park();
 Seal(d);
}
static __attribute__((noinline)) void PushedReceipt(const uint32_t* d,unsigned w,const uint32_t* before){
 uint32_t receipt[64];Fence();
 for(unsigned i=0;i<64;i++)receipt[i]=Words(PushedSlot(w))[i];
 Fence();
 if(Words(PushedSlot(w))[0]!=M::ReceiptMagic
  || !M::Receipt(receipt,Boot(w),reinterpret_cast<const uint32_t*>(L::PeerJobs+w*256),before))scalar_fault_park();
 q38_mixed_root_packet_seal(d);
}
static void PushedWait(const uint32_t* d,unsigned w,const uint32_t* before){
 for(unsigned poll=0;poll<65536;poll++){
  // Volatile alone cannot see another NoC client's updates through L0.
  Fence();unsigned marker=*Words(PushedSlot(w));
  if(marker==M::ReceiptMagic){PushedReceipt(d,w,before);return;}
  if(marker)scalar_fault_park();
  q38_mixed_root_packet_seal(d);
 }
 scalar_fault_park(); // no synthetic ready, later packet,retry,reset or release.
}
static __attribute__((noinline)) void PushedEnd(const uint32_t* d,const uint32_t* slave){
 if(!d[7] || d[7]>119)scalar_fault_park();
 bool done=false;
 for(unsigned poll=0;poll<65536;poll++){
  Fence();done=true;
  for(unsigned i=0;i<4;i++){
   const unsigned expected=d[7]*(i==1?5U:2U);
   const uint32_t delta=*Words(0xffb20000+SlaveCounter(i))-slave[i];
   if(delta>expected)scalar_fault_park();
   done &= delta==expected;
  }
  if(done)break;
  q38_mixed_root_packet_seal(d);
 }
 if(!done)scalar_fault_park();
 for(unsigned w=0;w<d[7];w++){
  uint32_t before[8];for(unsigned i=0;i<8;i++)before[i]=Words(L::Counters)[w*8+i]-PushedDelta(i);
  PushedReceipt(d,w,before);
 }
 Seal(d);
}
