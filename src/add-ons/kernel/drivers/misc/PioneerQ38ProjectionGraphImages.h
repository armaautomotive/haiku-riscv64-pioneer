/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_PROJECTION_GRAPH_IMAGES_H
#define PIONEER_Q38_PROJECTION_GRAPH_IMAGES_H
#ifdef PIONEER_Q38_PROJECTION_RESULT_STASH
#if PIONEER_Q38_PROJECTION_RESULT_STASH != 1
#error Exact separate fixed projection result image required
#endif
#include "PioneerQ38ProjectionGraphRootStashMain.h"
#include "PioneerQ38ProjectionGraphRootStashPrefix.h"
#include "PioneerQ38ProjectionGraphRootStashGraph.h"
#define PIONEER_Q38_ACTIVE_ROOT_MAIN PioneerQ38ProjectionGraphRootStashMain
#define PIONEER_Q38_ACTIVE_ROOT_PREFIX PioneerQ38ProjectionGraphRootStashPrefix
#define PIONEER_Q38_ACTIVE_ROOT_GRAPH PioneerQ38ProjectionGraphRootStashGraph
#else
#include "PioneerQ38ProjectionGraphRootMain.h"
#include "PioneerQ38ProjectionGraphRootPrefix.h"
#include "PioneerQ38ProjectionGraphRootGraph.h"
#define PIONEER_Q38_ACTIVE_ROOT_MAIN PioneerQ38ProjectionGraphRootMain
#define PIONEER_Q38_ACTIVE_ROOT_PREFIX PioneerQ38ProjectionGraphRootPrefix
#define PIONEER_Q38_ACTIVE_ROOT_GRAPH PioneerQ38ProjectionGraphRootGraph
#endif
#include "PioneerQ38ProjectionGraphPeerMain.h"
#include "PioneerQ38ProjectionGraphPeerAffine.h"
#include "PioneerQ38ProjectionGraphPeerHelper.h"
// PRIVATE complete code DATA ONLY. No old dispatch registry/owner includes
// this catalog. The exact shared four-byte boot is NOT image authority.
// A new actual native owner MUST save/prove ALL footprints before ANY
// write,retain real sources/initialized data/producer/cookie/clock/host
// and ALL120 roles,and keep EVERY snapshot/mapping on UNKNOWN.
enum {TT_Q38_PROJECTION_GRAPH_BOOT=0x0006c06f,
 TT_Q38_PROJECTION_GRAPH_ROOT_PREFIX_BYTES=40960,TT_Q38_PROJECTION_GRAPH_PEER_PREFIX_BYTES=16384};
static inline unsigned PioneerQ38ProjectionGraphPrefixBytes(unsigned w){
 return w>=120?0:w?TT_Q38_PROJECTION_GRAPH_PEER_PREFIX_BYTES:TT_Q38_PROJECTION_GRAPH_ROOT_PREFIX_BYTES;
}
static inline bool PioneerQ38ProjectionGraphProgramAddress(unsigned w,uint32_t a){
 return w<120 && !(a&3) && (a==0x6000 || (a>=0x64000 && a-0x64000<PioneerQ38ProjectionGraphPrefixBytes(w))
  || (a>=0x70000 && a<0x74000));
}
static inline uint32_t PioneerQ38ProjectionGraphProgramWord(unsigned w,uint32_t a){
 if(!PioneerQ38ProjectionGraphProgramAddress(w,a))return UINT32_MAX;
 if(a==0x6000)return TT_Q38_PROJECTION_GRAPH_BOOT;
 if(!w){
  if(a>=0x64000 && a-0x64000<sizeof(PIONEER_Q38_ACTIVE_ROOT_PREFIX))return PIONEER_Q38_ACTIVE_ROOT_PREFIX[(a-0x64000)/4];
  if(a>=0x68000 && a-0x68000<sizeof(PIONEER_Q38_ACTIVE_ROOT_GRAPH))return PIONEER_Q38_ACTIVE_ROOT_GRAPH[(a-0x68000)/4];
  if(a>=0x72000 && a-0x72000<sizeof(PIONEER_Q38_ACTIVE_ROOT_MAIN))return PIONEER_Q38_ACTIVE_ROOT_MAIN[(a-0x72000)/4];
 }else{
  if(a>=0x64000 && a-0x64000<sizeof(PioneerQ38ProjectionGraphPeerHelper))return PioneerQ38ProjectionGraphPeerHelper[(a-0x64000)/4];
  if(a>=0x70b00 && a-0x70b00<sizeof(PioneerQ38ProjectionGraphPeerAffine))return PioneerQ38ProjectionGraphPeerAffine[(a-0x70b00)/4];
  if(a>=0x72000 && a-0x72000<sizeof(PioneerQ38ProjectionGraphPeerMain))return PioneerQ38ProjectionGraphPeerMain[(a-0x72000)/4];
 }
 return 0; // padding ONLY inside a separately saved complete code footprint
}
static inline bool PioneerQ38ProjectionGraphProgramImmutable(unsigned w,uint32_t a){
 return PioneerQ38ProjectionGraphProgramAddress(w,a) && (!w || a<0x70000 || a>=0x70b00);
}
static inline bool PioneerQ38ProjectionGraphExecutableAddress(unsigned w,uint32_t a){
 if(w>=120 || (a&3))return false;
 if(a==0x6000)return true;
 if(!w)return (a>=0x64000 && a-0x64000<sizeof(PIONEER_Q38_ACTIVE_ROOT_PREFIX))
  || (a>=0x68000 && a-0x68000<sizeof(PIONEER_Q38_ACTIVE_ROOT_GRAPH))
  || (a>=0x72000 && a-0x72000<sizeof(PIONEER_Q38_ACTIVE_ROOT_MAIN));
 return (a>=0x64000 && a-0x64000<sizeof(PioneerQ38ProjectionGraphPeerHelper))
  || (a>=0x70b00 && a-0x70b00<sizeof(PioneerQ38ProjectionGraphPeerAffine))
  || (a>=0x72000 && a-0x72000<sizeof(PioneerQ38ProjectionGraphPeerMain));
}
static inline unsigned PioneerQ38ProjectionGraphCodeWords(unsigned w){
 return w<120?1+PioneerQ38ProjectionGraphPrefixBytes(w)/4+4096:0;
}
static inline unsigned PioneerQ38ProjectionGraphCodeBase(unsigned w){
 return w<120?(w?14337+(w-1)*8193:0):UINT32_MAX;
}
static inline uint32_t PioneerQ38ProjectionGraphCodeAddress(unsigned w,unsigned i){
 if(w>=120 || i>=PioneerQ38ProjectionGraphCodeWords(w))return UINT32_MAX;
 unsigned prefix=PioneerQ38ProjectionGraphPrefixBytes(w)/4;
 return !i?0x6000:i<=prefix?0x64000+(i-1)*4:0x70000+(i-prefix-1)*4;
}
static_assert(14337+119*8193==989304,"complete ALL120 code snapshot words,NOT old867448");
static_assert(sizeof(PIONEER_Q38_ACTIVE_ROOT_PREFIX)<=16384 && sizeof(PIONEER_Q38_ACTIVE_ROOT_GRAPH)<=24576
 && 0x64000+TT_Q38_PROJECTION_GRAPH_ROOT_PREFIX_BYTES==0x6e000,"ROOT excludes existing8KiB stack");
static_assert(sizeof(PioneerQ38ProjectionGraphPeerHelper)>12288 && sizeof(PioneerQ38ProjectionGraphPeerHelper)<=16384
 && 0x64000+TT_Q38_PROJECTION_GRAPH_PEER_PREFIX_BYTES==0x68000,"ENTIRE new16KiB peer,NOT old12KiB lease");
static_assert(sizeof(PioneerQ38ProjectionGraphPeerAffine)<=0x1500 && sizeof(PIONEER_Q38_ACTIVE_ROOT_MAIN)<=8192
 && sizeof(PioneerQ38ProjectionGraphPeerMain)<=8192,"fixed existing main/affine windows");
#undef PIONEER_Q38_ACTIVE_ROOT_MAIN
#undef PIONEER_Q38_ACTIVE_ROOT_PREFIX
#undef PIONEER_Q38_ACTIVE_ROOT_GRAPH
#endif
