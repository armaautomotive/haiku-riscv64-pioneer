/* SPDX-License-Identifier: MIT */
// OFFLINE device-only zero+readback candidate. NOT driver-selected/native
// qualified. Only a future private mutable-bank owner may seal descriptors.
#define PIONEER_ATTENTION_FREESTANDING
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38StateZeroShape.h"
extern "C" void block_fault_park() __attribute__((noreturn));
static volatile uint32_t* Words(uint32_t a) { return reinterpret_cast<volatile uint32_t*>(a); }
static void Fence() { asm volatile("fence" ::: "memory"); }
static unsigned Counter(unsigned i) {
 const unsigned offsets[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228}; return offsets[i];
}
static unsigned Source(unsigned i) { return !i?0x100:i<=6?0x118+(i-1)*4:i<=12?0x130+(i-7)*4:i==13?0x150:i==14?0x154:0x16c; }
static bool Sources(const uint32_t* d) {
 bool same=true; for (unsigned i=0;i<16;i++) same &= *Words(0xffb20000+Source(i))==d[16+i]; return same;
}
static bool Idle() {
 bool idle=true;
 for (unsigned i=0;i<4;i++) idle &= !*Words(0xffb20040+i*0x800);
 for (unsigned i=0;i<32;i++) idle &= !(*Words(0xffb20240+i*4)&255);
 return idle;
}
static void Packet(bool write,unsigned at,unsigned bytes,const uint32_t* d) {
 if (!bytes || bytes>16384 || (bytes&4095) || (at&4095) || at>=d[4] || bytes>d[4]-at || !Sources(d) || !Idle()) block_fault_park();
 unsigned remote=d[5]+at,local=write?0x84000:0x144000;
 const unsigned delta[]={write?0U:1U,write?0U:bytes/64,1,write?0U:1U,write?1U:0U,write?bytes/64:0U,1,write?1U:0U};
 uint32_t expected[8]; for (unsigned i=0;i<8;i++) expected[i]=*Words(0xffb20000+Counter(i))+delta[i];
 // Only reserved GDDR <-> two fixed local L1 buffers. NEVER host traffic.
 const uint32_t command[]={write?local:remote,0,write?129U:d[8],write?remote:local,0,write?d[8]:129U,0,write?0x212U:0,bytes,0};
 for (unsigned i=0;i<10;i++) Words(0xffb20000)[i]=command[i];
 for (unsigned i=0;i<10;i++) if (Words(0xffb20000)[i]!=command[i]) block_fault_park();
 Fence(); *Words(0xffb20040)=1; (void)*Words(0xffb20040); Fence();
 for (unsigned attempt=0;attempt<65536;attempt++) {
  bool complete=!*Words(0xffb20040);
  for (unsigned i=0;i<8;i++) complete &= *Words(0xffb20000+Counter(i))==expected[i];
  if (complete) {
   if (!Idle() || !Sources(d)) block_fault_park();
   Words(0x143000)[write?2:0]++; Words(0x143000)[write?3:1]+=bytes/64; Fence(); return;
  }
 }
 block_fault_park(); // UNKNOWN: inert park, no retry/next packet/reset
}
extern "C" unsigned block_main() {
 uint32_t d[64]; for (unsigned i=0;i<64;i++) d[i]=Words(0x140000)[i];
 if (!PioneerQ38StateZeroShape(d)) return 0; // malformed shape: zero MMIO
 if (!Sources(d) || !Idle()) block_fault_park();
 volatile uint32_t* s=Words(0x143000);
 for (unsigned i=0;i<32;i++) s[i]=0;
 s[5]=d[4]; s[6]=d[6]; s[7]=TT_Q38_ZERO_MARKER; s[24]=d[7]; s[25]=d[8]; s[26]=d[5];
 s[27]=0x84000; s[28]=0x144000; s[29]=1;
 for (unsigned i=0;i<8;i++) s[8+i]=*Words(0xffb20000+Counter(i));
 for (unsigned i=0;i<4096;i++) Words(0x84000)[i]=0;
 Fence(); for (unsigned i=0;i<4096;i++) if (Words(0x84000)[i]) block_fault_park();
 for (unsigned at=0;at<d[4];at+=16384) {
  unsigned bytes=d[4]-at; if (bytes>16384) bytes=16384;
  // Independently poison+prove readback before every transfer, including
  // repeated chunks whose previous result was zero. No stale-zero proof.
  for (unsigned i=0;i<bytes/4;i++) Words(0x144000)[i]=0xa55aa55a;
  Fence(); for (unsigned i=0;i<bytes/4;i++) if (Words(0x144000)[i]!=0xa55aa55a) block_fault_park();
  Packet(true,at,bytes,d); Packet(false,at,bytes,d);
  for (unsigned i=0;i<bytes/4;i++) if (Words(0x144000)[i]) block_fault_park();
 }
 if (!Sources(d) || !Idle()) block_fault_park();
 for (unsigned i=0;i<64;i++) if (Words(0x140000)[i]!=d[i]) block_fault_park();
 s[4]=d[4]/4096;
 for (unsigned i=0;i<8;i++) s[16+i]=*Words(0xffb20000+Counter(i));
 Fence(); return d[4]/4096;
}
