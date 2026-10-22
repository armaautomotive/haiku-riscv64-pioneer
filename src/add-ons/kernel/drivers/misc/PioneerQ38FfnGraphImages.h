/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_FFN_GRAPH_IMAGES_H
#define PIONEER_Q38_FFN_GRAPH_IMAGES_H
#include "PioneerQ38FfnGraphRootMain.h"
#include "PioneerQ38FfnGraphRootPrefix.h"
#include "PioneerQ38FfnGraphRootGraph.h"
#include "PioneerQ38FfnGraphPeerMain.h"
#include "PioneerQ38FfnGraphPeerAffine.h"
#include "PioneerQ38FfnGraphPeerHelper.h"
// PRIVATE frozen code DATA ONLY. Not a selector or a mapping/write/launch
// grant. No legacy DispatchRoleWord/registry includes or accepts this ABI.
// A new exclusive owner must save/prove the ENTIRE role-specific prefix,
// existing panel and boot before its first write and retain them on UNKNOWN.
enum {TT_Q38_FFN_GRAPH_BOOT=0x0006c06f,TT_Q38_FFN_GRAPH_PREFIX_BASE=0x64000,
 TT_Q38_FFN_GRAPH_ROOT_PREFIX_BYTES=40960,TT_Q38_FFN_GRAPH_PEER_PREFIX_BYTES=12288};
static inline unsigned PioneerQ38FfnGraphPrefixBytes(unsigned worker)
{return worker>=120?0:worker?TT_Q38_FFN_GRAPH_PEER_PREFIX_BYTES:TT_Q38_FFN_GRAPH_ROOT_PREFIX_BYTES;}
static inline bool PioneerQ38FfnGraphProgramAddress(unsigned worker,uint32_t a)
{
 if(worker>=120 || (a&3))return false;
 return a==0x6000 || (a>=0x64000 && a-0x64000<PioneerQ38FfnGraphPrefixBytes(worker)) || (a>=0x70000 && a<0x74000);
}
static inline uint32_t PioneerQ38FfnGraphProgramWord(unsigned worker,uint32_t a)
{
 if(!PioneerQ38FfnGraphProgramAddress(worker,a))return UINT32_MAX;
 if(a==0x6000)return TT_Q38_FFN_GRAPH_BOOT;
 if(!worker){
  if(a>=0x64000 && a-0x64000<sizeof(PioneerQ38FfnGraphRootPrefix))return PioneerQ38FfnGraphRootPrefix[(a-0x64000)/4];
  if(a>=0x68000 && a-0x68000<sizeof(PioneerQ38FfnGraphRootGraph))return PioneerQ38FfnGraphRootGraph[(a-0x68000)/4];
  if(a>=0x72000 && a-0x72000<sizeof(PioneerQ38FfnGraphRootMain))return PioneerQ38FfnGraphRootMain[(a-0x72000)/4];
 }else{
  if(a>=0x64000 && a-0x64000<sizeof(PioneerQ38FfnGraphPeerHelper))return PioneerQ38FfnGraphPeerHelper[(a-0x64000)/4];
  if(a>=0x70b00 && a-0x70b00<sizeof(PioneerQ38FfnGraphPeerAffine))return PioneerQ38FfnGraphPeerAffine[(a-0x70b00)/4];
  if(a>=0x72000 && a-0x72000<sizeof(PioneerQ38FfnGraphPeerMain))return PioneerQ38FfnGraphPeerMain[(a-0x72000)/4];
 }
 return 0; // zero padding INSIDE a new owner's complete saved footprint ONLY
}
static inline bool PioneerQ38FfnGraphProgramImmutable(unsigned worker,uint32_t a)
{
 // The old peer's entire70000..70b00 scratch is non-code. Do not mistake
 // freshly initialized scratch for immutable code during running audits.
 return PioneerQ38FfnGraphProgramAddress(worker,a) && (!worker || a<0x70000 || a>=0x70b00);
}
static inline bool PioneerQ38FfnGraphExecutableAddress(unsigned worker,uint32_t a)
{
 if(worker>=120 || (a&3))return false;
 if(a==0x6000)return true;
 if(!worker)return (a>=0x64000 && a-0x64000<sizeof(PioneerQ38FfnGraphRootPrefix))
  || (a>=0x68000 && a-0x68000<sizeof(PioneerQ38FfnGraphRootGraph))
  || (a>=0x72000 && a-0x72000<sizeof(PioneerQ38FfnGraphRootMain));
 return (a>=0x64000 && a-0x64000<sizeof(PioneerQ38FfnGraphPeerHelper))
  || (a>=0x70b00 && a-0x70b00<sizeof(PioneerQ38FfnGraphPeerAffine))
  || (a>=0x72000 && a-0x72000<sizeof(PioneerQ38FfnGraphPeerMain));
}
static_assert(0x64000+TT_Q38_FFN_GRAPH_ROOT_PREFIX_BYTES==0x6e000,"never overlap the existing8KiB stack");
static_assert(sizeof(PioneerQ38FfnGraphRootPrefix)<=16384 && sizeof(PioneerQ38FfnGraphRootGraph)<=24576,"complete root prefix lease");
static_assert(sizeof(PioneerQ38FfnGraphPeerHelper)<=12288 && 0x70b00+sizeof(PioneerQ38FfnGraphPeerAffine)<=0x72000,"complete peer helper lease");
static_assert(sizeof(PioneerQ38FfnGraphRootMain)<=8192 && sizeof(PioneerQ38FfnGraphPeerMain)<=8192,"existing main windows unchanged");
#endif
