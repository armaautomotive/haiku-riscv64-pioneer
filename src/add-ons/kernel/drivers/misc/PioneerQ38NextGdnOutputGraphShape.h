/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_NEXT_GDN_OUTPUT_GRAPH_SHAPE_H
#define PIONEER_Q38_NEXT_GDN_OUTPUT_GRAPH_SHAPE_H
#include "PioneerQ38ProjectionGraphShape.h"
#include "PioneerQ38NextGdnOutputGatherShape.h"
// Distinct fixed-geometry nonzero recurrent-layer ssm_out DATA. No native route, producer capability,
// input epoch, host activation, or cleanup authority follows from shape.
enum {TT_Q38_NEXT_GDN_OUTPUT_GRAPH_MARKER=0x32504f51,TT_Q38_NEXT_GDN_OUTPUT_GRAPH_FAULT=0x46504f52,
 TT_Q38_NEXT_GDN_OUTPUT_MATRIX_MARKER=0x324d4f51,
 TT_Q38_NEXT_GDN_OUTPUT_GRAPH_SOURCES=0x78000,TT_Q38_NEXT_GDN_OUTPUT_GRAPH_SOURCE_SHADOW=0x7a000,
 TT_Q38_NEXT_GDN_OUTPUT_RESULT=TT_Q38_GDN_GATED_OUTPUT+6144*4};
static_assert(TT_Q38_NEXT_GDN_OUTPUT_GRAPH_SOURCES+119*64<=TT_Q38_NEXT_GDN_OUTPUT_GRAPH_SOURCE_SHADOW
 && TT_Q38_NEXT_GDN_OUTPUT_GRAPH_SOURCE_SHADOW+119*64<=TT_Q38_PROJECTION_GRAPH_COUNTERS,
 "ALL119 source caches disjoint from counters and QOG2 source/shadow");
static_assert(TT_Q38_NEXT_GDN_OUTPUT_RESULT+5120*4<=2097152,"disjoint complete output inside initialized prefix");
static inline uint32_t PioneerQ38NextGdnOutputResultOffset(unsigned layer,unsigned role){
 return PioneerQ38NextGdnOutputLayer(layer) && role==13?uint32_t(TT_Q38_NEXT_GDN_OUTPUT_RESULT):~uint32_t(0);
}
static inline bool PioneerQ38NextGdnOutputGraphFrame(const uint32_t* d){
 if(!d || d[0]!=TT_Q38_NEXT_GDN_OUTPUT_GRAPH_MARKER || d[1]!=1 || d[2]!=67108864 || d[3]>=7
  || (d[4]&4095) || d[4]<0x04000000 || d[4]>0xc0000000 || (d[5]!=1 && d[5]!=2)
  || !d[6] || d[6]>=0x7ffffffd || d[7]!=1 || d[8]!=PioneerQ38ProjectionRoute(d[3]) || d[9]!=129
  || d[10]!=TT_Q38_PROJECTION_GRAPH_MANIFEST+192*4 || d[11]!=8 || d[12]!=119
  || d[13]!=d[6]+(d[5]==1?2U:1U) || d[14]!=(d[5]==1?1U:0U) || d[15] || !d[16] || d[40]>=8192 || d[41]!=d[6]
  || !PioneerQ38NextGdnOutputLayer(d[42]) || d[43]!=13 || d[44]!=6144 || d[45]!=5120 || d[48]!=8
  || (d[5]==1?(d[46]!=TT_Q38_GDN_RENDEZVOUS_MARKER || d[47]>=248320):(d[46]|d[47])!=0))return false;
 for(unsigned i=32;i<40;i+=2)if(!(d[i]|d[i+1]))return false;
 for(unsigned i=49;i<64;i++)if(d[i])return false;
 return true; // independent of old RMS/result-stash grammar
}
static inline bool PioneerQ38NextGdnOutputGraphManifest(const uint32_t* m,const uint32_t* sources){
 if(!sources || !PioneerQ38NextGdnOutputGraphFrame(m))return false;
 for(unsigned w=0;w<119;w++)if(m[64+w]>m[6] || !sources[w*16])return false;
 for(unsigned i=183;i<192;i++)if(m[i])return false;
 unsigned n=m[45],stripes=n==48?1:7,groups[7],quota[7],done[7],emitted[7],bank[7],base[7],first[7],minimum=0,row=0;
 // Word-only volatile initialization:the freestanding device image must
 // not acquire a compiler-generated libc memset/byte-store dependency.
 for(unsigned i=0;i<7;i++){
  reinterpret_cast<volatile unsigned*>(groups)[i]=0;reinterpret_cast<volatile unsigned*>(quota)[i]=0;
  reinterpret_cast<volatile unsigned*>(done)[i]=0;reinterpret_cast<volatile unsigned*>(emitted)[i]=0;
  reinterpret_cast<volatile unsigned*>(base)[i]=0;reinterpret_cast<volatile unsigned*>(first)[i]=0;bank[i]=7;
 }
 for(unsigned s=0;s<stripes;s++){
  first[s]=row;groups[s]=PioneerQ38ProjectionStripeGroups(n,s);row+=groups[s]*16;
  quota[s]=(groups[s]+15)/16;minimum+=quota[s];
 }
 if(row!=n)return false;
 for(unsigned extra=minimum;extra<m[12];extra++){
  unsigned best=stripes;
  for(unsigned s=0;s<stripes;s++)if(quota[s]<groups[s]
   && (best==stripes || groups[s]*quota[best]>groups[best]*quota[s]))best=s;
  if(best==stripes)return false;
  quota[best]++;
 }
 unsigned blocks=m[44]>>(m[48]==8?5:8),tile=m[48]==8?576:m[48]==12?2304:3392,groupBytes=blocks*tile;
 for(unsigned ordinal=0;ordinal<1024;ordinal++){
  const uint32_t* p=m+192+ordinal*8;
  if(ordinal>=m[12]){for(unsigned i=0;i<8;i++)if(p[i])return false;continue;}
  if(p[0]<0x04000000 || p[0]>=0xc4000000 || (p[0]&63) || p[1]>=7 || !p[2] || p[2]>16
   || p[4]!=PioneerQ38ProjectionRoute(p[1]) || p[5] || p[6] || p[7] || p[2]*groupBytes>0xc4000000-p[0]
   || (p[3]&15) || p[3]>=n || p[2]*16>n-p[3]
   || (p[1]==m[3] && p[0]<m[4]+67108864 && m[4]<p[0]+p[2]*groupBytes))return false;
  unsigned s=0;while(s<stripes && !(p[3]>=first[s] && p[3]<first[s]+groups[s]*16))s++;
  if(s==stripes || emitted[s]>=quota[s])return false;
  unsigned quotient=0,remainder=groups[s];while(remainder>=quota[s]){remainder-=quota[s];quotient++;}
  if(p[2]!=quotient+unsigned(emitted[s]<remainder) || p[3]!=first[s]+done[s]*16 || p[2]>groups[s]-done[s])return false;
  if(bank[s]==7){
   bank[s]=p[1];base[s]=p[0];for(unsigned other=0;other<stripes;other++)if(other!=s && bank[other]==bank[s])return false;
  }
  if(p[1]!=bank[s] || p[0]!=base[s]+done[s]*groupBytes)return false;
  done[s]+=p[2];emitted[s]++;
 }
 for(unsigned s=0;s<stripes;s++)if(done[s]!=groups[s] || emitted[s]!=quota[s])return false;
 // Exact same bank-sorted round-robin job order,not merely disjoint rows.
 for(unsigned s=0;s<stripes;s++)emitted[s]=0;
 unsigned ordinal=0;
 while(ordinal<m[12])for(unsigned b=0;b<7;b++)for(unsigned s=0;s<stripes;s++)if(bank[s]==b && emitted[s]<quota[s]){
  if(m[192+ordinal*8+1]!=b)return false;
  emitted[s]++;ordinal++;
 }
 return true; // complete DATA shape,not canonical bank/model/input authority
}
#ifdef PIONEER_Q38_NEXT_GDN_OUTPUT_GRAPH_ROOT
#define Q38_NEXT_GDN_OUTPUT_MATRIX_FN __attribute__((noinline,section(".q38_projection_graph")))
#else
#define Q38_NEXT_GDN_OUTPUT_MATRIX_FN inline
#endif
static Q38_NEXT_GDN_OUTPUT_MATRIX_FN bool PioneerQ38NextGdnOutputMatrixShape(const uint32_t* d,const uint32_t* h,const uint32_t* banks,
 const uint32_t parts[119][8],const uint32_t* first){
 if(!d || !h || !banks || !parts || !first || d[0]!=TT_Q38_NEXT_GDN_OUTPUT_MATRIX_MARKER
  || d[1]!=1 || d[2]!=67108864 || d[3]>=7 || (d[4]&4095) || d[4]<0x04000000 || d[4]>0xc0000000
  || d[5] || d[7]!=119 || d[8]!=PioneerQ38ProjectionRoute(d[3]) || d[9]!=129
  || d[11]!=5120 || d[12]!=20480 || d[13]!=TT_Q38_PROJECTION_MATRIX_ROWS || !d[16] || d[44]!=119 || d[45]!=1
  || h[0]!=TT_Q38_NEXT_GDN_OUTPUT_MATRIX_MARKER || h[1]!=1 || h[2]!=1 || h[3]!=8 || h[4]!=6144
  || h[5]!=119 || h[6]!=d[6] || h[7]!=TT_Q38_PROJECTION_MATRIX_INPUT || h[8]!=1
  || d[40]>=8192 || !d[41] || d[41]>=0x7ffffffd || d[6]!=d[41]+2 || !PioneerQ38NextGdnOutputLayer(d[42]) || d[43]!=13
  || d[14]!=6144 || d[15]!=8 || d[10]!=TT_Q38_NEXT_GDN_OUTPUT_RESULT
  || d[46]!=TT_Q38_GDN_RENDEZVOUS_MARKER || d[47]>=248320)return false;
 for(unsigned i=9;i<16;i++)if(h[i])return false;
 for(unsigned i=32;i<40;i+=2)if(!(d[i]|d[i+1]))return false;
 for(unsigned i=48;i<64;i++)if(d[i])return false;
 for(unsigned b=0;b<7;b++)if(banks[b]!=PioneerQ38ProjectionRoute(b))return false;
 if(banks[7])return false;
 unsigned rows=0;
 for(unsigned w=0;w<119;w++){
  const auto* p=parts[w];unsigned bytes=p[2]*192*576;
  if(!p[2] || p[2]>16 || !p[3] || p[3]>d[6] || !p[5] || p[6] || p[7] || p[1]>=7
   || p[4]!=banks[p[1]] || p[0]<0x04000000 || p[0]>=0xc4000000 || (p[0]&63)
   || bytes>0xc4000000-p[0] || (first[w]&15) || first[w]>=5120 || p[2]*16>5120-first[w])return false;
  for(unsigned previous=0;previous<w;previous++){
   if(first[w]<first[previous]+parts[previous][2]*16 && first[previous]<first[w]+p[2]*16)return false;
   if(p[1]==parts[previous][1] && p[0]<parts[previous][0]+parts[previous][2]*192*576
    && parts[previous][0]<p[0]+bytes)return false;
  }
  rows+=p[2]*16;
 }
 return rows==5120; // complete fixed result,not a partial generic wave
}
#undef Q38_NEXT_GDN_OUTPUT_MATRIX_FN
#endif
