/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_LAYOUT_H
#define PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_LAYOUT_H
#include "PioneerQ38MixedRecurrentProjectionRootLayout.h"
#include "PioneerQ38MixedRecurrentCohortLayout.h"
namespace PioneerQ38MixedRecurrentBlockProjectionLayout {
using namespace PioneerQ38MixedRecurrentProjectionRootLayout;
// First two small matrices touch only three workers. Other virgin workers
// remain admissible through Z in this FIRST root request, never afterward.
static inline bool Virgin(const uint32_t* d){return d && d[48]==1 && (d[43]==10 || d[43]==11 || d[43]==5);}
static inline bool FirstMatrix(const uint32_t* d){return d && d[43]==10;}
static inline unsigned RunDelta(unsigned w){return (w<3?10:w<96?8:7)
#if defined(PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION) || defined(PIONEER_Q38_TOKEN_PRECISE_PROGRAMS)
 +unsigned(w<68)
#endif
 ;}
}
namespace PioneerQ38MixedRecurrentBlockRootLayout {
using namespace PioneerQ38MixedRecurrentProjectionRootLayout;
enum {Magic=0x31425251,Fault=0x46425251,StopMagic=0x31535251,Stop=2,StopSummary=0x75600,
 Code=0x40000,Outer=0x75000,OuterWords=128,JointSummary=0x75500,
 Recipes=0xb0000,RecipeCopies=0xb6000,RecipeStride=5120,RecipeBytes=4636,
 Original=0xbc000,OriginalCopy=0xc2000,Weight=0xdb000,
 PostResidual=0x156000,PostNormalized=0x15b000,Final=0x160000,
 MetaOffset=0x440000,ConvMetaOffset=0x501000,GdnMetaOffset=0x52e000,FinalOffset=0x427000};
struct PrivateSeal {uint32_t boot[64],outer[OuterWords],frame[64],phase;};
static inline unsigned Role(unsigned slot){return slot==0?10:slot==1?11:slot==2?5:slot==3?6:slot==4?13:slot==5?2:slot==6?3:slot==7?4:~0U;}
static inline unsigned PhaseDelta(unsigned slot){return slot<4?slot:slot+2;}
static inline bool BootShape(const uint32_t* b){
 if(!b || b[0]!=Magic)return false;
 uint32_t old[64];for(unsigned i=0;i<64;i++)old[i]=b[i];old[0]=PioneerQ38MixedRecurrentProjectionRootLayout::Magic;
 return PioneerQ38MixedRecurrentProjectionRootLayout::BootShape(old);
}
static inline bool OuterShape(const uint32_t* d){
 if(!d || d[0]!=Magic || d[1]!=1 || d[2]!=128 || d[3]>=64 || d[3]%4==3 || d[4]>=8192 || d[5]!=129
  || !d[6] || d[6]>=0x7ffffff0 || d[7]>=7 || d[8]!=PioneerQ38ProjectionRoute(d[7]) || (d[9]&4095)
  || d[9]<0x04000000 || d[9]>0xc0000000 || d[10]!=5120 || d[11]!=5120 || d[12]>1 || d[13]>=248320
  || !d[16] || !d[48] || d[48]>=0x7ffffffe || d[49]!=119 || d[50]!=Recipes || d[51]!=RecipeCopies
  || d[52]!=RecipeStride || d[53]!=Original || d[54]!=OriginalCopy || d[55]!=Final || d[56]!=FinalOffset
  || d[57]!=d[9]+MetaOffset || d[58]!=d[9]+ConvMetaOffset || d[59]!=d[9]+GdnMetaOffset
  || d[60]!=PioneerQ38MixedRecurrentCohortLayout::ConvWords || d[61]!=PioneerQ38MixedRecurrentCohortLayout::GdnWords
  || d[62]!=8 || d[63]!=1 || d[68]>=7 || d[69]<0x04000000 || d[69]>0xc3ffb000 || (d[69]&63) || d[70]!=5120 || d[71]!=1)return false;
 for(unsigned i=14;i<16;i++)if(d[i])return false;
 for(unsigned i=32;i<40;i+=2)if(!(d[i]|d[i+1]))return false;
 for(unsigned i=40;i<48;i++)if(d[i])return false;
 for(unsigned i=72;i<128;i++)if(d[i])return false;
 for(unsigned i=0;i<2;i++){
  unsigned bank=d[64+i*2],address=d[65+i*2];
  if(bank>=7 || address<0x04000000 || address>0xc3ffb000 || (address&63)
   || (bank==d[7] && address<d[9]+67108864 && d[9]<address+20480))return false;
 }
 if(d[64]==d[66] && d[65]<d[67]+20480 && d[67]<d[65]+20480)return false;
 return true; // canonical source/state/input producer and lifetime remain separate
}
static inline bool StopShape(const uint32_t* d){
 if(!d || d[0]!=Magic || d[1]!=1 || d[2]!=128 || d[5]!=129 || d[48]<=1 || d[48]>=0x7fffffff
  || d[49]!=119 || d[72]!=Stop || !d[16])return false;
 for(unsigned i=0;i<128;i++)if(i!=0 && i!=1 && i!=2 && i!=5 && !(i>=16 && i<40)
  && i!=48 && i!=49 && i!=72 && d[i])return false;
 for(unsigned i=32;i<40;i+=2)if(!(d[i]|d[i+1]))return false;
 return true; // ONLY after an actual completed layer;NOT native disposal authority
}
static_assert(Recipes+4*RecipeStride<=RecipeCopies && RecipeCopies+4*RecipeStride<=Original
 && Original+20480<=OriginalCopy && OriginalCopy+20480<=PioneerQ38MixedRecurrentCohortLayout::Conv,
 "Original hidden survives the complete Conv/GDN scratch phase");
static_assert(Outer+OuterWords*4<=Boot && sizeof(PrivateSeal)==1028,"distinct complete recurrent private frame");
static_assert(JointSummary+256<=StopSummary && StopSummary+256<=Parts,"separate recurrent STOP receipt");
static_assert(MetaOffset+8*RecipeStride<=ConvMetaOffset
 && ConvMetaOffset+PioneerQ38MixedRecurrentCohortLayout::ConvWords*4==GdnMetaOffset
 && GdnMetaOffset+PioneerQ38MixedRecurrentCohortLayout::GdnWords*4<=67108864,"new phase metadata is disjoint GDDR DATA");
}
#endif
