/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_FFN_INPUT_GRAPH_IMAGES_H
#define PIONEER_Q38_FFN_INPUT_GRAPH_IMAGES_H
#include "PioneerQ38FfnGraphImages.h"
#include "PioneerQ38FfnInputGraphRootMain.h"
#include "PioneerQ38FfnInputGraphRootPrefix.h"
#include "PioneerQ38FfnInputGraphRootGraph.h"
// Separate QFG2 CODE DATA ONLY. The unchanged QFP2 companions share the
// exact old pinned bytes. NO registry/native selector accepts this header.
// Same complete40KiB root/12KiB peer prefix and boot/panel footprints.
// In particular this catalog grants NO access to new staged-input80000..
// 85000/readback88000..8c000 or mutable metadata. A REAL retained native
// owner must separately own/prove those ranges,model/source/clock/ALL
// host/participants and current terminals BEFORE any future execution.
static inline uint32_t PioneerQ38FfnInputGraphProgramWord(unsigned w,uint32_t a) {
 if(!PioneerQ38FfnGraphProgramAddress(w,a))return UINT32_MAX;
 if(w || a==0x6000)return PioneerQ38FfnGraphProgramWord(w,a);
 if(a>=0x64000 && a-0x64000<sizeof(PioneerQ38FfnInputGraphRootPrefix))return PioneerQ38FfnInputGraphRootPrefix[(a-0x64000)/4];
 if(a>=0x68000 && a-0x68000<sizeof(PioneerQ38FfnInputGraphRootGraph))return PioneerQ38FfnInputGraphRootGraph[(a-0x68000)/4];
 if(a>=0x72000 && a-0x72000<sizeof(PioneerQ38FfnInputGraphRootMain))return PioneerQ38FfnInputGraphRootMain[(a-0x72000)/4];
 return 0; // ONLY padding INSIDE the complete privately saved code footprint
}
static inline bool PioneerQ38FfnInputGraphExecutableAddress(unsigned w,uint32_t a) {
 if(w)return PioneerQ38FfnGraphExecutableAddress(w,a);
 if(a&3)return false;
 return a==0x6000 || (a>=0x64000 && a-0x64000<sizeof(PioneerQ38FfnInputGraphRootPrefix))
  || (a>=0x68000 && a-0x68000<sizeof(PioneerQ38FfnInputGraphRootGraph))
  || (a>=0x72000 && a-0x72000<sizeof(PioneerQ38FfnInputGraphRootMain));
}
static_assert(sizeof(PioneerQ38FfnInputGraphRootPrefix)<=16384 && sizeof(PioneerQ38FfnInputGraphRootGraph)<=24576
 && sizeof(PioneerQ38FfnInputGraphRootMain)<=8192,"QFG2 does not widen any code or stack window");
#endif
