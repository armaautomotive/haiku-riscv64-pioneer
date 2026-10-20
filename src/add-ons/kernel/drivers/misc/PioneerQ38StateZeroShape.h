/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_STATE_ZERO_SHAPE_H
#define PIONEER_Q38_STATE_ZERO_SHAPE_H
#include "PioneerQ38SpanShape.h"
// Private descriptor/observer ONLY; NOT an ioctl or route authority. The
// future native owner must prove its reserved extent, source seals, image,
// all initiators/PCI state, physical terminal, and counters independently.
// No host address or PCI bus-mastering is needed by this device-only image.
enum { TT_Q38_ZERO_MARKER=0x315a3851,TT_Q38_ZERO_FAULT=0x465a3851,
 TT_Q38_ZERO_BYTES=0x100000,TT_Q38_ZERO_WORDS=64,TT_Q38_ZERO_STATS=32 };
static inline bool PioneerQ38StateZeroShape(const uint32_t* d) {
 if (!d || d[0]!=TT_Q38_ZERO_MARKER || d[1]!=1 || d[2]!=1 || d[3]
  || !d[4] || d[4]>TT_Q38_ZERO_BYTES || (d[4]&4095) || (d[5]&4095)
  || d[5]<0x04000000 || d[5]>=0xc4000000 || d[4]>0xc4000000-d[5]
  || !d[6] || d[6]>=0x7fffffff || d[7]>=7 || d[8]!=PioneerQ38SpanRoute(d[7])) return false;
 for (unsigned i=9;i<16;i++) if (d[i]) return false;
 for (unsigned i=32;i<64;i++) if (d[i]) return false;
 return true;
}
static inline bool PioneerQ38StateZeroSummary(const uint32_t* s,const uint32_t* d,const uint32_t* before,const uint32_t* after) {
 if (!s || !before || !after || !PioneerQ38StateZeroShape(d)) return false;
 unsigned chunks=(d[4]+16383)/16384,flits=d[4]/64;
 if (s[0]!=chunks || s[1]!=flits || s[2]!=chunks || s[3]!=flits || s[4]!=d[4]/4096 || s[5]!=d[4]
  || s[6]!=d[6] || s[7]!=TT_Q38_ZERO_MARKER || s[24]!=d[7] || s[25]!=d[8] || s[26]!=d[5]
  || s[27]!=0x84000 || s[28]!=0x144000 || s[29]!=1 || s[30] || s[31]
  || before[2]!=before[6] || after[2]!=after[6]) return false;
 const unsigned expected[]={chunks,flits,chunks*2,chunks,chunks,flits,chunks*2,chunks};
 for (unsigned i=0;i<8;i++) if (s[8+i]!=before[i] || s[16+i]!=after[i] || uint32_t(after[i]-before[i])!=expected[i]) return false;
 return true;
}
#endif
