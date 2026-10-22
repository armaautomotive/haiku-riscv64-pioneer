/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_FFN_GRAPH_SHAPE_H
#define PIONEER_Q38_FFN_GRAPH_SHAPE_H
#include "PioneerQ38FfnPersistentShape.h"
// NEW OFFLINE complete-FFN coordinator DATA,NOT a native/job/lifetime grant.
// Kernel must derive ALL learned/mutable extents,source seals,identity and
// sequence baselines from the SAME retained exclusive context. Zero-page
// initialization/fields/tags/copies here do not prove an input producer ran.
// Work ONLY. These buffers overlap the OLD CHECKED peer-proof array;the
// coordinator may NEVER execute that checked/clear path or use its owner.
enum {TT_Q38_FFN_GRAPH_MARKER=0x31474651,TT_Q38_FFN_GRAPH_FAULT=0x46474651,
 TT_Q38_FFN_GRAPH_MANIFEST=0x75800,TT_Q38_FFN_GRAPH_SHADOW=0x78e00,
 TT_Q38_FFN_GRAPH_WORDS=3456,TT_Q38_FFN_GRAPH_ROLE_WORDS=1088,
 TT_Q38_FFN_GRAPH_COUNTERS=0x7c400,TT_Q38_FFN_GRAPH_SEQUENCE=0x7d300,
 TT_Q38_FFN_GRAPH_STATS=0x7d500,TT_Q38_FFN_GRAPH_PHASES=0x7d600};
// 0x3600B manifest:frame64,baseline119+9zero words,three4352B role records.
// Each role:16word header,119x8 learned parts,119 first rows,one zero pad.
// Capture the COMPLETE manifest once,including ALL inactive zero fields.
// Shadow and119x8 physical-counter snapshots are disjoint from the source
// table7d800,the existing75000..75800 NoC transfer staging,and all outputs.
static_assert(TT_Q38_FFN_GRAPH_MANIFEST+TT_Q38_FFN_GRAPH_WORDS*4==TT_Q38_FFN_GRAPH_SHADOW,"complete original span");
static_assert(TT_Q38_FFN_GRAPH_SHADOW+TT_Q38_FFN_GRAPH_WORDS*4==TT_Q38_FFN_GRAPH_COUNTERS,"complete private shadow");
static_assert(TT_Q38_FFN_GRAPH_COUNTERS+119*32<=TT_Q38_FFN_GRAPH_SEQUENCE,"independent before counters");
static_assert(TT_Q38_FFN_GRAPH_SEQUENCE+119*4<=TT_Q38_FFN_GRAPH_STATS,"private evolving sequences");
static_assert(TT_Q38_FFN_GRAPH_PHASES+7*32<=0x7d800,"never overlap original peer sources");
static inline bool PioneerQ38FfnGraphFrame(const uint32_t* d)
{
 if(!d || d[0]!=TT_Q38_FFN_GRAPH_MARKER || d[1]!=1 || d[2]!=67108864 || d[3]>=7
  || (d[4]&4095) || d[4]<0x04000000 || d[4]>0xc0000000 || (d[5]!=1 && d[5]!=2)
  || d[6]>(d[5]==1?0x7ffffff8U:0x7ffffffdU) || d[7]!=3 || d[8]!=PioneerQ38SpanRoute(d[3]) || d[9]!=129
  || d[10]!=TT_Q38_FFN_GRAPH_MANIFEST+256 || d[11]!=TT_Q38_FFN_GRAPH_ROLE_WORDS*4
  || d[12]!=TT_Q38_FFN_GRAPH_MANIFEST+768 || d[13]!=d[6]+(d[5]==1?5U:1U)
  || d[14]!=(d[5]==1?1U:0U) || d[15] || !d[16] || d[42]>=64)return false;
 for(unsigned i=32;i<40;i+=2)if(!(d[i]|d[i+1]))return false;
 for(unsigned i=43;i<64;i++)if(d[i])return false;
 return true; // DATA ONLY,not initialized/source/current-state authority
}
static inline unsigned PioneerQ38FfnGraphRoleOffset(unsigned role){return role<3?192+role*1088:~0U;}
static inline unsigned PioneerQ38FfnGraphPartBytes(unsigned type,unsigned inputs,unsigned groups)
{return groups*(inputs/256)*(type==12?2304U:type==14?3392U:0U);}
static inline bool PioneerQ38FfnGraphRole(const uint32_t* m,const uint32_t* d,const uint32_t* sources,unsigned role)
{
 if(!m || !sources || !PioneerQ38FfnGraphFrame(d) || role>2 || m[0]!=role || (m[1]!=12 && m[1]!=14)
  || !m[2] || m[2]>119 || m[3]!=(role==2?17408U:5120U) || m[4]!=(role==2?5120U:17408U))return false;
 for(unsigned i=5;i<16;i++)if(m[i])return false;
 if(m[1087])return false;
 const uint32_t* rows=m+16+119*8;unsigned total=0;
 for(unsigned w=0;w<119;w++) {
  const uint32_t* p=m+16+w*8;
  if(w>=m[2]){for(unsigned i=0;i<8;i++)if(p[i])return false;if(rows[w])return false;continue;}
  unsigned bytes=PioneerQ38FfnGraphPartBytes(m[1],m[3],p[2]);
  if(!p[2] || p[2]>16 || p[0]<0x04000000 || p[0]>=0xc4000000 || (p[0]&63) || bytes>0xc4000000-p[0]
   || p[1]>=7 || p[3] || p[4]!=PioneerQ38SpanRoute(p[1]) || !p[5] || p[5]!=sources[w*16]
   || p[6] || p[7] || (rows[w]&15) || rows[w]>=m[4] || p[2]*16>m[4]-rows[w]
   || (p[1]==d[3] && p[0]<d[4]+67108864 && d[4]<p[0]+bytes))return false;
  for(unsigned other=0;other<w;other++) {
   const uint32_t* q=m+16+other*8;
   if((rows[w]<rows[other]+q[2]*16 && rows[other]<rows[w]+p[2]*16)
    || (p[1]==q[1] && p[0]<q[0]+PioneerQ38FfnGraphPartBytes(m[1],m[3],q[2]) && q[0]<p[0]+bytes))return false;
  }
  total+=p[2]*16;
 }
 return total==m[4];
}
static inline bool PioneerQ38FfnGraphManifest(const uint32_t* m,const uint32_t* sources)
{
 if(!PioneerQ38FfnGraphFrame(m) || !sources)return false;
 for(unsigned w=0;w<119;w++)if(m[64+w]>m[6] || !sources[w*16])return false;
 for(unsigned w=183;w<192;w++)if(m[w])return false;
 for(unsigned role=0;role<3;role++)if(!PioneerQ38FfnGraphRole(m+PioneerQ38FfnGraphRoleOffset(role),m,sources,role))return false;
 return true; // complete shape,NEVER canonical model/learned ownership
}
static inline unsigned PioneerQ38FfnGraphNextPeer(const uint32_t* m,unsigned w)
{
 if(!m || w>=119)return ~0U;
 if(m[5]==2)return m[64+w]+1;
 unsigned next=m[64+w]+unsigned(w<20)+unsigned(w<68);
 for(unsigned role=0;role<3;role++)next+=unsigned(w<m[PioneerQ38FfnGraphRoleOffset(role)+2]);
 return next;
}
#endif
