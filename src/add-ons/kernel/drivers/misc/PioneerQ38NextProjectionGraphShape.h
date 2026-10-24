/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_NEXT_PROJECTION_GRAPH_SHAPE_H
#define PIONEER_Q38_NEXT_PROJECTION_GRAPH_SHAPE_H
#include "PioneerQ38NextProjectionMatrixShape.h"
// NEW complete-projection parent DATA. Requires independent canonical
// learned/source/initialized/input-producer/code/sequence/owner authority.
// Resident F32 input is ALREADY at workspace0 before entry. This marker,
// input epoch or shape does NOT make a caller a valid GDDR producer.
enum {
 TT_Q38_NEXT_PROJECTION_GRAPH_MARKER=0x32535051,TT_Q38_NEXT_PROJECTION_GRAPH_FAULT=0x46535032, // QPS2:NOT QPS1/QPR1
 TT_Q38_NEXT_PROJECTION_GRAPH_VERSION=2,TT_Q38_NEXT_PROJECTION_GRAPH_MANIFEST=0xb0000,
 TT_Q38_NEXT_PROJECTION_GRAPH_SHADOW=0xc0000,TT_Q38_NEXT_PROJECTION_GRAPH_RECORD=192,
 TT_Q38_NEXT_PROJECTION_GRAPH_MAX_JOBS=1024,TT_Q38_NEXT_PROJECTION_GRAPH_WORDS=8384,
 TT_Q38_NEXT_PROJECTION_GRAPH_COUNTERS=0x7c400,TT_Q38_NEXT_PROJECTION_GRAPH_SEQUENCE=0x7d300,
 TT_Q38_NEXT_PROJECTION_GRAPH_STATS=0x7d500,TT_Q38_NEXT_PROJECTION_GRAPH_SOURCES=0x7d800};
static_assert(TT_Q38_NEXT_PROJECTION_GRAPH_RECORD+TT_Q38_NEXT_PROJECTION_GRAPH_MAX_JOBS*8==TT_Q38_NEXT_PROJECTION_GRAPH_WORDS,"complete manifest including inactive records");
static_assert(TT_Q38_NEXT_PROJECTION_GRAPH_MANIFEST+TT_Q38_NEXT_PROJECTION_GRAPH_WORDS*4<=TT_Q38_NEXT_PROJECTION_GRAPH_SHADOW,"disjoint fixed manifest/shadow views");
static_assert(TT_Q38_NEXT_PROJECTION_GRAPH_COUNTERS+119*32<=TT_Q38_NEXT_PROJECTION_GRAPH_SEQUENCE
 && TT_Q38_NEXT_PROJECTION_GRAPH_SEQUENCE+119*4<=TT_Q38_NEXT_PROJECTION_GRAPH_STATS
 && TT_Q38_NEXT_PROJECTION_GRAPH_STATS+256<=TT_Q38_NEXT_PROJECTION_GRAPH_SOURCES
 && TT_Q38_NEXT_PROJECTION_GRAPH_SOURCES+119*64<=0x7f800,"disjoint metadata/source/stats");
static inline unsigned PioneerQ38NextProjectionGraphWorkers(const uint32_t* m,unsigned wave){
 if(!m || wave>=m[7])return 0;
 unsigned left=m[12]-wave*119;return left<119?left:119;
}
static inline bool PioneerQ38NextProjectionGraphFrame(const uint32_t* d){
 if(!d)return false;
 if(!d[42] || d[42]>=64 || d[42]%4==3 || !PioneerQ38ProjectionResultRows(d[43]))return false;
 unsigned type=0,k=0,n=0;
 if(!PioneerQ38NextProjectionGeometry(d[42],d[43],type,k,n))return false;
 unsigned jobs=PioneerQ38NextProjectionJobCount(n),waves=jobs==973?9:1;
 if(d[0]!=TT_Q38_NEXT_PROJECTION_GRAPH_MARKER || d[1]!=TT_Q38_NEXT_PROJECTION_GRAPH_VERSION || d[2]!=67108864 || d[3]>=7
  || (d[4]&4095) || d[4]<0x04000000 || d[4]>0xc0000000 || (d[5]!=1 && d[5]!=2)
  || d[6]>0x7ffffffeU-(d[5]==1?waves+1:1) || d[7]!=waves || d[8]!=PioneerQ38ProjectionRoute(d[3]) || d[9]!=129
  || d[10]!=TT_Q38_NEXT_PROJECTION_GRAPH_MANIFEST+TT_Q38_NEXT_PROJECTION_GRAPH_RECORD*4 || d[11]!=8 || d[12]!=jobs
  || d[13]!=d[6]+(d[5]==1?waves+1:1) || d[14]!=(d[5]==1?1U:0U) || d[15] || !d[16]
  || d[44]!=k || d[45]!=n || d[48]!=type || (d[5]==1?!(d[46]|d[47]):(d[46]|d[47])!=0))return false;
 for(unsigned i=32;i<40;i+=2)if(!(d[i]|d[i+1]))return false;
 for(unsigned i=49;i<64;i++)if(d[i])return false;
 return true;
}
// Exact per-stripe group geometry of the fixed public model. No variable
// integer division in the device parser. This grammar admits only the
// 47*4 nonzero linear-layer subset,checked against the canonical model.
static inline unsigned PioneerQ38NextProjectionStripeGroups(unsigned rows,unsigned s){
 if(s>=7)return 0;
 if(rows==48)return s?0:3;
 if(rows==1024)return s<2?12:8;
 if(rows==5120)return s<3?48:44;
 if(rows==6144)return s<5?56:52;
 if(rows==10240)return s<6?92:88;
 if(rows==12288)return s<3?112:108;
 if(rows==17408)return s<6?156:152;
 if(rows==248320)return s<2?2220:2216;
 return 0;
}
static inline bool PioneerQ38NextProjectionGraphManifest(const uint32_t* m,const uint32_t* sources){
 if(!sources || !PioneerQ38NextProjectionGraphFrame(m))return false;
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
  first[s]=row;groups[s]=PioneerQ38NextProjectionStripeGroups(n,s);row+=groups[s]*16;
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
static inline unsigned PioneerQ38NextProjectionGraphNextPeer(const uint32_t* m,unsigned w){
 if(!m || w>=119)return ~0U;
 if(m[5]==2)return m[64+w]+1;
 unsigned next=m[64+w]+unsigned(w<m[44]/256);
 for(unsigned wave=0;wave<m[7];wave++)next+=unsigned(w<PioneerQ38NextProjectionGraphWorkers(m,wave));
 return next;
}
#endif
