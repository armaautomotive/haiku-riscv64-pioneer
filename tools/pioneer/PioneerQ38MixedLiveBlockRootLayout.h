/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_LAYOUT_H
#define PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_LAYOUT_H
#include "PioneerQ38MixedBlockRootLayout.h"
namespace PioneerQ38MixedLiveBlockRootLayout {
using namespace PioneerQ38MixedBlockRootLayout;
enum {Magic=0x314c4251,Fault=0x464c4251,StopMagic=0x31534c51,StopSummary=0x75600,Run=1,Stop=2};
static_assert(StopSummary+256<=Parts,"separate stop receipt,never a block-result alias");
static inline bool BootShape(const uint32_t* b){
 if(!b || b[0]!=Magic)return false;
 uint32_t old[64];for(unsigned i=0;i<64;i++)old[i]=b[i];old[0]=PioneerQ38MixedBlockRootLayout::Magic;
 return PioneerQ38MixedBlockRootLayout::BootShape(old);
}
static inline bool RunShape(const uint32_t* d){
 if(!d || d[0]!=Magic || d[72]!=Run || d[73]>=7 || d[74]<0x04000000 || d[74]>0xc3ffb000 || (d[74]&63) || d[75]!=5120)return false;
 for(unsigned i=76;i<128;i++)if(d[i])return false;
 uint32_t old[128];for(unsigned i=0;i<128;i++)old[i]=i<72?d[i]:0;old[0]=PioneerQ38MixedBlockRootLayout::Magic;
 return PioneerQ38MixedBlockRootLayout::OuterShape(old);
} // RAW GDDR geometry only;NOT an original-input producer/source/native grant
static inline bool StopShape(const uint32_t* d){
 if(!d || d[0]!=Magic || d[1]!=1 || d[2]!=128 || d[5]!=129 || d[48]<=1 || d[48]>=0x7fffffff || d[49]!=119 || d[72]!=Stop || !d[16])return false;
 for(unsigned i=0;i<128;i++)if(i!=0 && i!=1 && i!=2 && i!=5 && !(i>=16 && i<40) && i!=48 && i!=49 && i!=72 && d[i])return false;
 for(unsigned i=32;i<40;i+=2)if(!(d[i]|d[i+1]))return false;
 return true; // can ONLY be used after a genuinely completed block
}
static inline bool FirstMatrix(const uint32_t* d){
 return d && d[0]==PioneerQ38MixedProjectionRootLayout::Magic && d[43]==5;
}
static inline bool Virgin(const uint32_t* d){return FirstMatrix(d) && d[48]==1;}
static inline unsigned RunDelta(unsigned w){return (w<64?10:w<96?8:5)
#ifdef PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION
 +unsigned(w<68)
#endif
 ;}
}
#endif
