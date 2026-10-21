/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RESIDENT_GDN_SHAPE_H
#define PIONEER_Q38_RESIDENT_GDN_SHAPE_H
#include "PioneerQ38SpanShape.h"
// OFFLINE private descriptor only,NOT an ioctl,allocation,ownership or
// initialized/read/compute capability. A future coordinator MUST bind BOTH
// extents to its exact zero-initialized mutable owner and current token.
enum { TT_Q38_RGDN_MARKER=0x31523851,TT_Q38_RGDN_FAULT=0x46523851,
 TT_Q38_RGDN_STATE_BYTES=32768,TT_Q38_RGDN_DESCRIPTOR=0x74200,TT_Q38_RGDN_SUMMARY=0x74300 };
static inline uint32_t PioneerQ38ResidentGdnPeer(unsigned worker) {
 const unsigned x[]={1,2,3,4,5,6,7,10,11,12,13,14};
 // Logical half-head0..95 maps ONLY physical non-root worker1..96.
 if (worker>=96) return ~uint32_t(0);
 unsigned column=0,row=worker+1;
 while (row>=10) { row-=10; column++; } // same qualified bounded route arithmetic,no DIV/REM
 return x[column]|((row+2)<<6);
}
static inline bool PioneerQ38ResidentGdnAddress(uint32_t address) {
 return !(address&4095) && address>=0x04000000 && address<=0xc4000000-TT_Q38_RGDN_STATE_BYTES;
}
static inline bool PioneerQ38ResidentGdnShape(const uint32_t* d) {
 if (!d || d[0]!=TT_Q38_RGDN_MARKER || d[1]!=1 || d[2]!=128 || d[3]!=64
  || d[4]>=7 || !PioneerQ38ResidentGdnAddress(d[5]) || d[6]>=7 || !PioneerQ38ResidentGdnAddress(d[7])
  || !d[8] || d[8]>=0x7fffffff || d[9]>=96 || d[10]>=64 || d[10]%4==3 || d[11]>=2
  || d[12]!=PioneerQ38SpanRoute(d[4]) || d[13]!=PioneerQ38SpanRoute(d[6])
  || d[14]!=TT_Q38_RGDN_STATE_BYTES || d[15]!=PioneerQ38ResidentGdnPeer(d[9]) || !d[16]) return false;
 // A next slot may NEVER overlap this invocation's read slot,even partly.
 if (d[4]==d[6] && d[5]<d[7]+TT_Q38_RGDN_STATE_BYTES && d[7]<d[5]+TT_Q38_RGDN_STATE_BYTES) return false;
 for (unsigned i=32;i<40;i+=2) if (!(d[i]|d[i+1])) return false; // four nonzero64-bit owner epochs
 for (unsigned i=42;i<64;i++) if (d[i]) return false;
 return true;
}
static inline bool PioneerQ38ResidentGdnSummary(const uint32_t* s,const uint32_t* d,const uint32_t* before,const uint32_t* after) {
 if (!s || !before || !after || !PioneerQ38ResidentGdnShape(d)
  || s[0]!=2 || s[1]!=512 || s[2]!=2 || s[3]!=512 || s[4]!=64 || s[5]!=32768 || s[6]!=d[8] || s[7]!=TT_Q38_RGDN_MARKER
  || s[24]!=d[9] || s[25]!=d[10] || s[26]!=d[11] || s[27]!=(d[11]^1) || s[28]!=d[15] || s[29]!=1
  || s[30]!=d[5] || s[31]!=d[7] || before[2]!=before[6] || after[2]!=after[6]) return false;
 const unsigned delta[]={2,512,4,2,2,512,4,2};
 for (unsigned i=0;i<8;i++) if (s[8+i]!=before[i] || s[16+i]!=after[i] || uint32_t(after[i]-before[i])!=delta[i]) return false;
 return true;
}
#endif
