/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RECURRENT_PREFIX_LAYOUT_H
#define PIONEER_Q38_RECURRENT_PREFIX_LAYOUT_H
#include "PioneerQ38MixedRecurrentBlockRootLayout.h"
#include "PioneerQ38RecurrentHistoryLayout.h"
// Immutable three-layer DATA geometry. No model authentication, bank lease,
// initialized/current-state producer, native entry or whole-token authority.
namespace PioneerQ38RecurrentPrefixLayout {
namespace R=PioneerQ38MixedRecurrentBlockRootLayout;
namespace C=PioneerQ38MixedRecurrentCohortLayout;
enum {Marker=0x31585051,Layers=3,Offset=0x1000000,Stride=0x80000,
 OuterOffset=0,RecipesOffset=512,RecipeBytes=8*R::RecipeStride,
 ConvOffset=RecipesOffset+RecipeBytes,ConvBytes=C::ConvWords*4,
 GdnOffset=ConvOffset+ConvBytes,GdnBytes=C::GdnWords*4,Bytes=GdnOffset+GdnBytes};
static inline bool Address(uint32_t workspace,unsigned layer,uint32_t& output){
 if((workspace&4095) || workspace<0x04000000 || workspace>0xc0000000 || layer>=Layers)return false;
 output=workspace+Offset+layer*Stride;return true;
}
static inline unsigned BootWord(unsigned i){
 return i==40?unsigned(Marker):i==41?unsigned(Layers):i==42?unsigned(Offset):i==43?unsigned(Stride):i==44?unsigned(Bytes):i==45?1U:0U;
}
static inline bool BootShape(const uint32_t* b){
 if(!b)return false;
 uint32_t old[64];
 for(unsigned i=0;i<64;i++){
  if(i>=40 && i<46 && b[i]!=BootWord(i))return false;
  old[i]=i>=40 && i<46?0U:b[i];
 }
 return R::BootShape(old); // old zero-tail boot is explicitly NOT accepted
}
static inline bool Frame(const uint32_t* d,unsigned layer,unsigned bank,uint32_t workspace,const uint32_t* boot){
 if(!BootShape(boot) || !R::OuterShape(d) || layer>=Layers || bank>=7
  || d[3]!=layer || d[4] || d[6]!=1001+layer*16 || d[7]!=bank || d[9]!=workspace
  || d[12] || d[13] || d[48]!=layer+1)return false;
 uint32_t package=0;if(!Address(workspace,layer,package))return false;
 if(layer && (d[68]!=bank || d[69]!=workspace+R::FinalOffset))return false;
 for(unsigned i=0;i<16;i++)if(d[16+i]!=boot[16+i])return false;
 for(unsigned i=0;i<8;i++)if(d[32+i]!=boot[8+i])return false;
 return true; // the future native owner must independently prove all producers
}
static_assert(Bytes<=Stride && !(Bytes&63) && !(ConvOffset&63) && !(GdnOffset&63),"bounded aligned immutable layer packages");
static_assert(PioneerQ38RecurrentHistoryLayout::Offset+PioneerQ38RecurrentHistoryLayout::Layers*PioneerQ38RecurrentHistoryLayout::Stride<=Offset
 && Offset+Layers*Stride<=67108864,"prefix metadata cannot alias provisional histories or leave the reserved workspace");
}
#endif
