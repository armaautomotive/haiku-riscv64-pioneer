/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_MIXED_BLOCK_ROOT_LAYOUT_H
#define PIONEER_Q38_MIXED_BLOCK_ROOT_LAYOUT_H
#include "PioneerQ38MixedJoinedRootLayout.h"
namespace PioneerQ38MixedBlockRootLayout {
using namespace PioneerQ38MixedJoinedRootLayout;
enum {Magic=0x31524251,Fault=0x46524251,Code=0x40000,OuterWords=128,
 Original=0xc8000,OriginalCopy=0xc2000,Raw=0xd0000,RawCopy=0xd6000,Weight=0xdb000,
 InputShadow=0xe0000,NativeShadow=0xf2000,ScaleShadow=0xfab00,
 FfnRecipes=0x110000,
#if defined(PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION) || defined(PIONEER_Q38_TOKEN_PUSHED_PROGRAMS)
 // G3 retains ordinary-matrix receipts at Proofs through the NEXT admission.
 // Only the attention frame SHADOW is dead after attention has returned;
 // Frames/DenseCopies still belong to Joined::MetadataSeal throughout FFN.
 FfnCopies=Shadow,
#else
 FfnCopies=0x118000,
#endif
 PostResidual=0x156000,PostNormalized=0x15b000,Final=0x160000,
 GateOffset=0x400000,UpOffset=0x411000,DownOffset=0x422000,FinalOffset=0x427000,MetaOffset=0x440000,
 MatrixMagic=0x314d4251};
struct PrivateSeal {uint32_t boot[64],outer[OuterWords],frame[64],phase;};
static_assert(OriginalCopy+20480<=Original && Original+20480<=Raw && Raw+20480<=RawCopy && RawCopy+20480<=Weight,"original and normalized residual inputs stay disjoint");
static_assert(Weight+20480<=InputShadow && InputShadow+17408*4<=NativeShadow && NativeShadow+17408*2<=ScaleShadow
 && ScaleShadow+640<=0x100000,"maximum Q8 scales need640 bytes;Q4 down input17408 stays separate");
#if defined(PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION) || defined(PIONEER_Q38_TOKEN_PUSHED_PROGRAMS)
static_assert(unsigned(FfnCopies)==unsigned(Shadow) && Frames+96*256<=FfnCopies
 && FfnCopies+3*RecipeStride<=FfnRecipes && FfnRecipes+3*RecipeStride<=Proofs,
 "G3 FFN copies reuse dead attention shadow,NEVER live frames or retained pushed receipts");
#else
static_assert(FfnRecipes+3*RecipeStride<=FfnCopies && FfnCopies+3*RecipeStride<=0x120000,"FFN metadata reuses only closed attention receipts");
#endif
static_assert(Input+17408*4<=PostResidual && PostResidual+20480<=PostNormalized && PostNormalized+20480<=Final
 && Final+20480<=PeerBoots,"complete block hidden and FFN buffers stay disjoint");
static_assert(Projected+17408*4<=0x92000 && 0x92800<=Native && Native+17408*2<=Scales && Scales+640<=Recipes,"projection/native/activation/source metadata disjoint");
static_assert(Outer+OuterWords*4<=Boot && sizeof(PrivateSeal)==1028,"NEW complete-block outer and private ABI");
static inline unsigned Tile(unsigned type){return type==8?576:type==12?2304:type==14?3392:0;}
static inline unsigned Offset(unsigned role){return role==2?unsigned(GateOffset):role==3?unsigned(UpOffset):role==4?unsigned(DownOffset):~0U;}
static inline bool BootShape(const uint32_t* b){
 if(!b || b[0]!=Magic)return false;
 uint32_t old[64];for(unsigned i=0;i<64;i++)old[i]=b[i];old[0]=PioneerQ38MixedJoinedRootLayout::Magic;
 return PioneerQ38MixedJoinedRootLayout::BootShape(old);
}
static inline bool OuterShape(const uint32_t* d){
 if(!d || d[0]!=Magic || d[2]!=OuterWords || !d[6] || d[6]>=0x7ffffff7 || d[47]!=RawCopy || d[51]!=Raw
  || d[55]!=Original || d[56]!=OriginalCopy || d[57]!=FfnRecipes || d[58]!=FfnCopies || d[59]!=Final
  || d[64]>=7 || d[66]>=7 || d[65]<0x04000000 || d[67]<0x04000000 || d[65]>0xc3ffb000 || d[67]>0xc3ffb000
  || ((d[65]|d[67])&63) || d[68]!=d[9]+MetaOffset || d[69]!=d[8] || d[70]!=3 || d[71]!=RecipeStride)return false;
 for(unsigned i=60;i<64;i++)if(d[i])return false;
 for(unsigned i=72;i<OuterWords;i++)if(d[i])return false;
 uint32_t old[64];for(unsigned i=0;i<64;i++)old[i]=i>=55?0:d[i];old[0]=PioneerQ38MixedJoinedRootLayout::Magic;old[2]=64;
 old[47]=PioneerQ38MixedJoinedRootLayout::RawCopy;old[51]=PioneerQ38MixedJoinedRootLayout::Raw;
 if(!PioneerQ38MixedJoinedRootLayout::OuterShape(old))return false;
 for(unsigned i=0;i<2;i++){
  unsigned bank=d[64+i*2],address=d[65+i*2];
  if(bank==d[7] && address<d[9]+67108864 && d[9]<address+20480)return false;
 }
 if(d[64]==d[66] && d[65]<d[67]+20480 && d[67]<d[65]+20480)return false;
 return true; // geometry only, not original source/native/commit authority
}
}
#endif
