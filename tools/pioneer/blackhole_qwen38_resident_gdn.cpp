/* SPDX-License-Identifier: MIT */
// OFFLINE one-shot resident half-head prerequisite,NO native selection.
// Two reserved GDDR->L1 reads,EXACT post-dot FP32 vector arithmetic,two
// L1->DIFFERENT reserved GDDR writes. NO host/PCI/DMA/CPU-reference work.
#define PIONEER_Q38_DELTA_VECTOR_PROBE_IMAGE
#define PIONEER_Q38_DELTA_VECTOR_POST_SCALE
#define scalar_main q38_local_recurrent_step
#include "blackhole_qwen38_delta_vector.cpp"
#undef scalar_main
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentGdnShape.h"
namespace RGdn {
static unsigned CounterOffset(unsigned i) {
 const unsigned offset[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228}; return offset[i];
}
static unsigned SourceOffset(unsigned i) { return !i?0x100:i<=6?0x118+(i-1)*4:i<=12?0x130+(i-7)*4:i==13?0x150:i==14?0x154:0x16c; }
static bool Idle() {
 bool idle=true;
 for (unsigned i=0;i<4;i++) idle &= !*Words(0xffb20040+i*0x800);
 for (unsigned i=0;i<32;i++) idle &= !(*Words(0xffb20240+i*4)&255);
 return idle;
}
static bool Sealed(const uint32_t* d) {
 // command word5 is the LAST PACKET destination coordinate,NOT a tile
 // identity register. Physical worker identity belongs to the independent
 // coordinator/source proof;do not mistake a writable command for it.
 bool same=true;
 for (unsigned i=0;i<16;i++) same &= *Words(0xffb20000+SourceOffset(i))==d[16+i];
 for (unsigned i=0;i<64;i++) same &= Words(TT_Q38_RGDN_DESCRIPTOR)[i]==d[i];
 return same;
}
#ifdef PIONEER_Q38_RGDN_ROOT_QUALIFICATION
static void Packet(unsigned operation,unsigned at,const uint32_t* d) {
 const bool write=operation==1;
 if (operation>2) scalar_fault_park();
#else
static void Packet(bool write,unsigned at,const uint32_t* d) {
#endif
 if ((at!=0 && at!=16384) || !Sealed(d) || !Idle()) scalar_fault_park();
#ifdef PIONEER_Q38_RGDN_ROOT_QUALIFICATION
 unsigned remote=d[operation?7:5]+at,local=(write?0xa0000:operation==2?0x98000:0x90000)+at,route=d[operation?13:12];
#else
 unsigned remote=d[write?7:5]+at,local=(write?0xa0000:0x90000)+at,route=d[write?13:12];
#endif
 const unsigned delta[]={write?0U:1U,write?0U:256U,1,write?0U:1U,write?1U:0U,write?256U:0U,1,write?1U:0U};
 uint32_t expected[8]; for (unsigned i=0;i<8;i++) expected[i]=*Words(0xffb20000+CounterOffset(i))+delta[i];
 const uint32_t command[]={write?local:remote,0,write?d[15]:route,write?remote:local,0,write?route:d[15],0,write?0x212U:0,16384,0};
 for (unsigned i=0;i<10;i++) Words(0xffb20000)[i]=command[i];
 for (unsigned i=0;i<10;i++) if (Words(0xffb20000)[i]!=command[i]) scalar_fault_park();
 Fence(); *Words(0xffb20040)=1; (void)*Words(0xffb20040); Fence();
 for (unsigned attempt=0;attempt<65536;attempt++) {
  bool complete=!*Words(0xffb20040);
  for (unsigned i=0;i<8;i++) complete &= *Words(0xffb20000+CounterOffset(i))==expected[i];
  if (complete) {
   if (!Idle() || !Sealed(d)) scalar_fault_park();
   Words(TT_Q38_RGDN_SUMMARY)[write?2:0]++; Words(TT_Q38_RGDN_SUMMARY)[write?3:1]+=256; Fence(); return;
  }
 }
 scalar_fault_park(); // UNKNOWN:inert fault terminal,NO later packet/retry
}
}
#ifdef PIONEER_Q38_RGDN_COMBINED_STAGE
#if PIONEER_Q38_RGDN_COMBINED_STAGE != 1 || defined(PIONEER_Q38_RGDN_ROOT_QUALIFICATION)
#error Exact offline combined-state stage role,not root readback validation
#endif
extern "C" bool q38_resident_gdn_state_shape(const uint32_t*);
extern "C" unsigned q38_resident_gdn_state_stage() {
#else
extern "C" unsigned scalar_main() {
#endif
 uint32_t d[64]; for (unsigned i=0;i<64;i++) d[i]=Words(TT_Q38_RGDN_DESCRIPTOR)[i];
#ifdef PIONEER_Q38_RGDN_COMBINED_STAGE
 if (!q38_resident_gdn_state_shape(d)) return 0;
#else
 if (!PioneerQ38ResidentGdnShape(d)) return 0;
#endif
 // malformed shape:NO MMIO/packets/SFPU
 if (Words(L::Descriptor)[0]!=L::Magic || Words(L::Descriptor)[1]!=128 || Words(L::Descriptor)[2]!=64 || Words(L::Descriptor)[3]) return 0;
 if (!PioneerQ38DeltaVectorGate(*Words(L::Decay)) || !PioneerQ38DeltaVectorGate(*Words(L::Beta))) return 0;
 for (unsigned i=0;i<320;i++) if (!InputNormalOrZero(Words(L::Query)[i])) return 0;
 if (!RGdn::Sealed(d) || !RGdn::Idle()) scalar_fault_park();
 volatile uint32_t* s=Words(TT_Q38_RGDN_SUMMARY); for (unsigned i=0;i<32;i++) s[i]=0;
 s[5]=32768; s[6]=d[8]; s[7]=TT_Q38_RGDN_MARKER; s[24]=d[9]; s[25]=d[10]; s[26]=d[11];
 s[27]=d[11]^1; s[28]=d[15]; s[29]=
#ifdef PIONEER_Q38_RGDN_ROOT_QUALIFICATION
 2;
#else
 1;
#endif
 s[30]=d[5]; s[31]=d[7];
 for (unsigned i=0;i<8;i++) s[8+i]=*Words(0xffb20000+RGdn::CounterOffset(i));
 RGdn::Packet(0,0,d); RGdn::Packet(0,16384,d);
 if (q38_local_recurrent_step()!=64 || !RGdn::Sealed(d) || !RGdn::Idle()) scalar_fault_park();
 // Only the private next slot is written,never the input/current state.
 // Completion is DATA ONLY;the coordinator must independently prove ALL
 // participants and its token/slot epoch BEFORE publishing a slot flip.
 RGdn::Packet(1,0,d); RGdn::Packet(1,16384,d);
#ifdef PIONEER_Q38_RGDN_ROOT_QUALIFICATION
 // Separate diagnostic ONLY. Independent GDDR readback into a DISJOINT
 // local buffer;compare ALL next-state words BEFORE completion. This is
 // transport validation,NOT CPU numerical fallback or the Work image.
 for (unsigned i=0;i<8192;i++) Words(0x98000)[i]=0x7fc00000;
 Fence(); for (unsigned i=0;i<8192;i++) if (Words(0x98000)[i]!=0x7fc00000) scalar_fault_park();
 RGdn::Packet(2,0,d); RGdn::Packet(2,16384,d);
 for (unsigned i=0;i<8192;i++) if (Words(0x98000)[i]!=Words(L::Next)[i]) scalar_fault_park();
#endif
 if (!RGdn::Sealed(d) || !RGdn::Idle()) scalar_fault_park();
 for (unsigned i=0;i<8;i++) s[16+i]=*Words(0xffb20000+RGdn::CounterOffset(i));
 s[4]=64; Fence(); return 64;
}
