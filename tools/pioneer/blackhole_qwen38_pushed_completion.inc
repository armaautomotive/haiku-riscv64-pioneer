/* SPDX-License-Identifier: MIT */
// Included INSIDE the distinct QGW3 worker namespace after local MMIO helpers.
// Two non-inline,non-posted writes. Payload ACK precedes commit issuance.
// Counter semantics: tenstorrent/tt-isa-documentation,BlackholeA0/NoC/Counters.md.
// Existing ROOT proof consumers are NOT compatible with these extra writes.
static void PushSourceSeal(const uint32_t* boot){
 for(unsigned i=0;i<64;i++)if(Words(M::Boot)[i]!=boot[i])block_fault_park();
 for(unsigned i=0;i<16;i++)if(*Words(0xffb20000+Source(i))!=boot[16+i])block_fault_park();
 if(!Quiet())block_fault_park();
} // deliberately NO job/doorbell reads AFTER the final commit is visible.
static void PushWrite(const uint32_t* boot,uint32_t* counters,unsigned destination,unsigned bytes){
 if((bytes!=256 && bytes!=4) || destination!=M::Slot(boot[3]))block_fault_park();
 q38_mixed_seal();PushSourceSeal(boot);
 uint32_t expected[8];
 for(unsigned i=0;i<8;i++){
  if(*Words(0xffb20000+Counter(i))!=counters[i])block_fault_park();
  const unsigned delta=i==5?(bytes+63)/64:i==2 || i==4 || i==6 || i==7?1U:0U;
  expected[i]=counters[i]+delta;
 }
 const uint32_t command[]={M::PushBuffer,0,boot[4],destination,0,M::Root,0,0x212,bytes,0};
 for(unsigned i=0;i<10;i++)Words(0xffb20000)[i]=command[i];
 for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])block_fault_park();
 Fence();*Words(0xffb20040)=1;(void)*Words(0xffb20040);Fence(); // readback BEFORE counters.
 for(unsigned poll=0;poll<65536;poll++){
  bool done=!*Words(0xffb20040);
  for(unsigned i=0;i<8;i++)done &= *Words(0xffb20000+Counter(i))==expected[i];
  if(done){
   PushSourceSeal(boot);
   for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])block_fault_park();
   for(unsigned i=0;i<8;i++)counters[i]=expected[i];
   return;
  }
 }
 block_fault_park(); // unknown;NO clear,rebase,retry,reset or cleanup.
}
static void PushCompletion(const uint32_t* boot,const uint32_t* job,uint32_t* counters){
 if(!job[1] || job[2]!=1 || !M::JobShape(job,boot,job[1]-1,0))block_fault_park();
 auto* r=Words(M::PushBuffer);for(unsigned i=0;i<64;i++)r[i]=0;
 const uint32_t header[]={0,M::ReceiptWords,boot[3],job[1],job[5],job[6],boot[4],M::Root};
 for(unsigned i=0;i<8;i++){r[i]=header[i];r[56+i]=counters[i];}
 for(unsigned i=0;i<32;i++)r[8+i]=job[i];
 for(unsigned i=0;i<16;i++)r[40+i]=boot[16+i];
 PushWrite(boot,counters,M::Slot(boot[3]),256);
 r[0]=M::ReceiptMagic;Fence();PushWrite(boot,counters,M::Slot(boot[3]),4);
} // last packet permits a queued next job;only boot/source/counter checks follow.
