/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GDN_OUTPUT_GATHER_SHAPE_H
#define PIONEER_Q38_GDN_OUTPUT_GATHER_SHAPE_H
#include "PioneerQ38GdnOutputInputShape.h"
#include "PioneerQ38ProjectionGatherShape.h"
// Distinct fixed QOG1 ROOT DATA only. Exact native producer/ALL24 actual
// receipts/current physical sources/owners are still required BEFORE entry.
// Never assume peer NIU source0 equals ROOT source0. Original per-peer
// sources and their private immutable shadow live inside the existing
// complete ROOT runtime snapshot;neither address is caller-selectable.
enum {TT_Q38_GDN_OUTPUT_GATHER_MARKER=0x31474f51,TT_Q38_GDN_OUTPUT_GATHER_FAULT=0x46474f51,
 TT_Q38_GDN_OUTPUT_GATHER_SOURCES=0x7d800,TT_Q38_GDN_OUTPUT_GATHER_SHADOW=0x7e800};
static inline bool PioneerQ38GdnOutputGatherShape(const uint32_t* d){
 if(!d || d[0]!=TT_Q38_GDN_OUTPUT_GATHER_MARKER || d[1]!=1 || d[2]!=67108864 || d[3]>=7
  || (d[4]&4095) || d[4]<0x04000000 || d[4]>0xc0000000 || d[5]!=24 || !d[6] || d[6]>=0x7fffffff
  || d[7] || d[8]!=PioneerQ38ProjectionRoute(d[3]) || d[9]!=129
  || d[10]!=TT_Q38_PROJECTION_NATIVE_OFFSET || d[11]!=TT_Q38_PROJECTION_LINES_OFFSET
  || d[12]!=TT_Q38_PROJECTION_COMPACT_OFFSET || d[13]!=12288 || d[14]!=768 || d[15]!=8 || !d[16]
  || d[40] || !d[41] || d[41]>=0x7ffffffd || d[6]!=d[41]+1 || d[42] || d[43]!=13
  || d[44]!=TT_Q38_GDN_RENDEZVOUS_MARKER || d[45]>=248320
  || d[46]!=TT_Q38_GDN_OUTPUT_GATHER_SOURCES || d[47]!=24)return false;
 for(unsigned i=32;i<40;i+=2)if(!(d[i]|d[i+1]))return false;
 for(unsigned i=48;i<64;i++)if(d[i])return false;
 return true;
}
static inline bool PioneerQ38GdnOutputGatherLine(const uint32_t* line,const uint32_t* d,unsigned chunk,const uint32_t* peerSource){
 if(!peerSource || !peerSource[0] || !PioneerQ38GdnOutputGatherShape(d) || chunk>=24)return false;
 uint32_t p[64];for(unsigned i=0;i<64;i++)p[i]=d[i];
 p[0]=TT_Q38_GDN_OUTPUT_INPUT_MARKER;p[5]=chunk;p[7]=chunk+1;p[9]=PioneerQ38ProjectionPeer(chunk);
 p[10]=TT_Q38_GDN_GATED_OUTPUT+chunk*1024;p[11]=6144;p[12]=TT_Q38_PROJECTION_NATIVE_OFFSET+chunk*512;
 p[13]=TT_Q38_PROJECTION_LINES_OFFSET+chunk*128;p[14]=8;p[15]=24;p[46]=p[47]=0;
 for(unsigned i=0;i<16;i++)p[16+i]=peerSource[i];
 return PioneerQ38GdnOutputInputLine(line,p);
}
static inline bool PioneerQ38GdnOutputGatherSummary(const uint32_t* s,const uint32_t* d,const uint32_t* before,const uint32_t* after){
 if(!s || !before || !after || !PioneerQ38GdnOutputGatherShape(d) || before[2]!=before[6] || after[2]!=after[6])return false;
 const unsigned fields[]={25,240,1,12,24,12288,d[6],TT_Q38_GDN_OUTPUT_GATHER_MARKER};
 const unsigned delta[]={25,240,26,25,1,12,26,1};
 for(unsigned i=0;i<8;i++)if(s[i]!=fields[i] || s[8+i]!=before[i] || s[16+i]!=after[i] || uint32_t(after[i]-before[i])!=delta[i])return false;
 const unsigned tail[]={d[3],d[4],d[45],d[41],8,768,6144,129};
 for(unsigned i=0;i<8;i++)if(s[24+i]!=tail[i])return false;
 return true; // DATA only,not a native publication/cleanup or read capability
}
static_assert(TT_Q38_GDN_OUTPUT_GATHER_SOURCES>=0x74000
 && TT_Q38_GDN_OUTPUT_GATHER_SOURCES+24*16*4<=TT_Q38_GDN_OUTPUT_GATHER_SHADOW
 && TT_Q38_GDN_OUTPUT_GATHER_SHADOW+24*16*4<=0xac000,"disjoint tables INSIDE existing full ROOT runtime snapshot");
#endif
