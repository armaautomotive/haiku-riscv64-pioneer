/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_MIXED_RECURRENT_PROJECTION_ROOT_LAYOUT_H
#define PIONEER_Q38_MIXED_RECURRENT_PROJECTION_ROOT_LAYOUT_H
#include "PioneerQ38MixedProjectionRootLayout.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38MixedRecurrentRecipe.h"
namespace PioneerQ38MixedRecurrentProjectionRootLayout {
// Reuse only DATA and returning arithmetic, not an older image/boot ABI.
using namespace PioneerQ38MixedProjectionRootLayout;
enum {Magic=0x31505251,Fault=0x46505251,InputShadow=0xe0000,NativeShadow=0xf2000,ScaleShadow=0xfab00};
static inline unsigned Tile(unsigned type){return type==8?576:type==12?2304:0;}
static inline bool Shape(const uint32_t* d){
 unsigned type=0,k=0,n=0;
 if(!d || d[0]!=Magic || !PioneerQ38MixedRecurrentGeometry(d[42],d[43],type,k,n)
  || !d[48] || d[48]>=0x7fffffff || d[49]!=119 || d[50]!=Parts || d[51]!=MatrixHeader || d[52]!=Banks
  || d[13]!=Rows || d[53]!=(d[43]==13?Output:Input) || d[54]!=Projected || d[55]!=PeerJobs
  || d[56]!=Nonces || d[57]!=PeerBoots)return false;
 for(unsigned i=58;i<64;i++)if(d[i])return false;
 return true; // complete recipe is checked before any peer IO
}
static inline bool BootShape(const uint32_t* b){
 if(!b || b[0]!=Magic)return false;
 uint32_t old[64];for(unsigned i=0;i<64;i++)old[i]=b[i];old[0]=PioneerQ38MixedDenseRootLayout::Magic;
 return PioneerQ38MixedDenseRootLayout::BootShape(old);
}
static_assert(InputShadow+17408*4<=NativeShadow && NativeShadow+17408*2<=ScaleShadow && ScaleShadow+768<=0x100000,
 "recurrent/FFN quantization original, native and scale shadows do not overlap");
static_assert(Projected+17408*4<=Native && Native+17408*2<=Scales && Scales+768<=0xe0000,
 "largest FFN gather and native inputs remain disjoint");
static_assert(Input+17408*4<=0x156000 && Output+6144*4<=Input,"maximum down and recurrent output projection inputs");
}
#endif
