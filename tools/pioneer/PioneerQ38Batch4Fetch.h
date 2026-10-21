/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_BATCH4_FETCH_H
#define PIONEER_Q38_BATCH4_FETCH_H
#ifndef PIONEER_ATTENTION_FREESTANDING
#include <stdint.h>
#else
typedef __UINT32_TYPE__ uint32_t;
#endif
// OFFLINE metadata ONLY. NO source/initialized-page/owner/NoC authority.
// UP TO four original-layout tiles fit scratch98000..99a80,inside
// the ORIGINAL saved6e000..b0000 peer window,disjoint from90000 raw tile,
// 91000 metadata,94000 scales,95000 sums,97000 affine and a0000 inputs.
struct PioneerQ38Batch4FetchPlan {
 uint32_t source=0,destination=0,bytes=0,copySource=0,copyWords=0;
 bool issue=false;
};
// Private local-input selector ONLY,not source/NoC/completion authority.
// The checked path retains90000;Work may select only one WHOLE original
// tile inside its known-complete bounded batch. No arbitrary L1 address.
static inline bool PioneerQ38Batch4RawInput(unsigned type,uint32_t base) {
 unsigned tile=type==8?576:type==12?2304:type==14?3392:0;
 if(!tile)return false;
 return base==0x90000 || base==0x98000 || base==0x98000+tile
  || (type==8 && (base==0x98000+2*tile || base==0x98000+3*tile));
}
static inline bool PioneerQ38Batch4Fetch(unsigned type,unsigned inputs,unsigned groups,unsigned group,unsigned block,
 uint32_t first,PioneerQ38Batch4FetchPlan& output) {
 unsigned tile=type==8?576:type==12?2304:type==14?3392:0;
 if(!tile || (inputs!=5120 && inputs!=6144 && inputs!=17408) || !groups || groups>16 || group>=groups
  || first<0x04000000 || first>=0xc4000000 || (first&31))return false;
 unsigned blocks=inputs>>(type==8?5:8);
 unsigned batch=type==8?4:2,mask=batch-1;
 // Validated shape bounds imply groups*blocks*tile<=5,013,504;32-bit
 // arithmetic is exact even on freestanding RV32 (no runtime helpers).
 if((blocks&mask) || block>=blocks || groups*blocks*tile>0xc4000000-first)return false;
 PioneerQ38Batch4FetchPlan p;
 p.source=first+(group*blocks+(block&~mask))*tile;
 p.destination=0x98000;p.bytes=tile*batch;p.copySource=p.destination+(block&mask)*tile;p.copyWords=tile/4;p.issue=!(block&mask);
 // Precaution,not a universal hardware fix:vendor report52270 observed
 // size/address-dependent hangs below nominal16KiB. Keep NEW DRAM reads
 // <=8KiB;Q8 four tiles2304B,Q4 two4608B,Q6 two6784B.
 // https://github.com/tenstorrent/tt-metal/issues/52270
 if(p.bytes>8192 || p.destination+p.bytes>0x99a80 || p.copySource+tile>p.destination+p.bytes)return false;
 output=p;return true;
}
#endif
