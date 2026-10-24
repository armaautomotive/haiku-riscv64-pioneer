/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_FFN_RESIDUAL_GRAPH_IMAGES_H
#define PIONEER_Q38_FFN_RESIDUAL_GRAPH_IMAGES_H
#include "PioneerQ38FfnGraphImages.h"
#include "PioneerQ38FfnResidualRootMain.h"
#include "PioneerQ38FfnResidualRootPrefix.h"
#include "PioneerQ38FfnResidualRootGraph.h"
// Frozen OFFLINE DATA ONLY,not a new native image selector. The future
// SAME decoder owner must claim actual completed post-attention normalized
// AND raw residual lineages,retain original snapshots/host/clock/sources,
// prove STOP+holds before replacement,and quarantine on first UNKNOWN.
// Reuse unchanged QFP2 peer math,NOT the old QFG1/QFG2 ownership systems.
static inline uint32_t PioneerQ38FfnResidualProgramWord(unsigned w,uint32_t a){
 if(!PioneerQ38FfnGraphProgramAddress(w,a))return UINT32_MAX;
 if(w)return PioneerQ38FfnGraphProgramWord(w,a);
 if(a==0x6000)return TT_Q38_FFN_GRAPH_BOOT;
 if(a>=0x64000 && a-0x64000<sizeof(PioneerQ38FfnResidualRootPrefix))return PioneerQ38FfnResidualRootPrefix[(a-0x64000)/4];
 if(a>=0x68000 && a-0x68000<sizeof(PioneerQ38FfnResidualRootGraph))return PioneerQ38FfnResidualRootGraph[(a-0x68000)/4];
 if(a>=0x72000 && a-0x72000<sizeof(PioneerQ38FfnResidualRootMain))return PioneerQ38FfnResidualRootMain[(a-0x72000)/4];
 return 0;
}
static inline bool PioneerQ38FfnResidualExecutableAddress(unsigned w,uint32_t a){
 if(w>=120 || (a&3))return false;
 if(w)return PioneerQ38FfnGraphExecutableAddress(w,a);
 return a==0x6000 || (a>=0x64000 && a-0x64000<sizeof(PioneerQ38FfnResidualRootPrefix))
  || (a>=0x68000 && a-0x68000<sizeof(PioneerQ38FfnResidualRootGraph))
  || (a>=0x72000 && a-0x72000<sizeof(PioneerQ38FfnResidualRootMain));
}
static_assert(sizeof(PioneerQ38FfnResidualRootPrefix)<=16384 && sizeof(PioneerQ38FfnResidualRootGraph)<=24576,
 "no code/stack lease expansion for residual FFN");
static_assert(sizeof(PioneerQ38FfnResidualRootMain)<=8192,"existing main window unchanged");
#endif
