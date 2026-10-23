/* SPDX-License-Identifier: MIT */
// OFFLINE LOCAL arithmetic qualifier,not a native/Work selector or launch.
// Fixed raw q/k/v/gates -> existing recurrent-worker LOCAL input layout.
// Future integration must read actual resident convolution/projections and
// original A/dtBias on CARD and combine prep+recurrence in one worker image.
// This standalone diagnostic MUST NOT become a host stop/reload Work path.
#define PIONEER_ATTENTION_FREESTANDING
#define PIONEER_Q38_GDN_PREPARE_IMAGE 1
#define PIONEER_Q38_DELTA_VECTOR_PROBE_IMAGE
#include "PioneerQ38GdnPrepare.h"
#include "PioneerQ38DeltaVectorLayout.h"
namespace L=PioneerQ38DeltaVectorLayout;
static volatile uint32_t* Words(unsigned at){return reinterpret_cast<volatile uint32_t*>(at);}
extern "C" void scalar_fault_park() __attribute__((noreturn));
extern "C" unsigned scalar_main(){
 // Fixed32word input descriptor,DATA ONLY;half0..95 is provenance data,
 // NOT a granted worker identity,source capability or initialized-state seal.
 // Caller MUST require fresh QGP1/count64. A malformed header returns0
 // without changing older local data;the descriptor stamp alone is no proof.
 if(Words(0x74600)[0]!=0x31504751 || Words(0x74600)[1]!=128
  || Words(0x74600)[2]!=64 || Words(0x74600)[3]>=96)return 0;
 for(unsigned i=4;i<32;i++)if(Words(0x74600)[i])return 0;
 Words(L::Descriptor)[0]=0; // invalidate stale local prep before arithmetic
 for(unsigned i=0;i<64;i++)if(!PioneerQ38PrepareNormal(PioneerAttentionFloat(Words(0x80400)[i])))scalar_fault_park();
 float gates[2];
 if(!PioneerQ38PrepareHead(reinterpret_cast<const float*>(0x80000),reinterpret_cast<const float*>(0x80200),
  PioneerAttentionFloat(Words(0x80500)[0]),PioneerAttentionFloat(Words(0x80500)[1]),
  PioneerAttentionFloat(Words(0x80500)[2]),PioneerAttentionFloat(Words(0x80500)[3]),
  reinterpret_cast<float*>(L::Query),reinterpret_cast<float*>(L::Key),gates))scalar_fault_park();
 for(unsigned i=0;i<64;i++)Words(L::Value)[i]=Words(0x80400)[i]; // BIT EXACT,including -0
 *Words(L::Decay)=PioneerAttentionBits(gates[0]);*Words(L::Beta)=PioneerAttentionBits(gates[1]);
 Words(L::Descriptor)[1]=128;Words(L::Descriptor)[2]=64;Words(L::Descriptor)[3]=0;
 asm volatile("fence" ::: "memory");Words(L::Descriptor)[0]=L::Magic;
 asm volatile("fence" ::: "memory");return 64; // LOCAL data only;NO NoC/GDDR/CSR/PCI/reset
}
