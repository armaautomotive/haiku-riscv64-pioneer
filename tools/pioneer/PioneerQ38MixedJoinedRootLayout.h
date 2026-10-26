/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_MIXED_JOINED_ROOT_LAYOUT_H
#define PIONEER_Q38_MIXED_JOINED_ROOT_LAYOUT_H
#include "PioneerQ38MixedProjectionRootLayout.h"
namespace PioneerQ38MixedJoinedRootLayout {
using namespace PioneerQ38MixedProjectionRootLayout;
enum {Magic=0x31524a51,Fault=0x46524a51,Code=0x40000,Outer=0x75000,JointSummary=0x75500,
 Recipes=0xb0000,RecipeCopies=0xb6000,RecipeStride=5120,RecipeBytes=4636,
 DenseCopies=0xbc000,RawCopy=0xc2000,Raw=0xc8000,RopeOffset=0x200000};
struct PrivateSeal {uint32_t boot[64],outer[64],frame[64],phase;};
static inline bool Shape(const uint32_t* d){return PioneerQ38MixedDenseRootLayout::Shape(d);}
static inline bool BootShape(const uint32_t* b){
 if(!b || b[0]!=Magic)return false;
 uint32_t old[64];for(unsigned i=0;i<64;i++)old[i]=b[i];old[0]=PioneerQ38MixedDenseRootLayout::Magic;
 return PioneerQ38MixedDenseRootLayout::BootShape(old);
}
static inline bool OuterShape(const uint32_t* d){
 if(!d || d[0]!=Magic || d[1]!=1 || d[2]!=64 || d[3]>=64 || d[3]%4!=3 || d[4]>=8192 || d[5]!=129
  || !d[6] || d[6]>=0x7ffffffb || d[7]>=7 || d[8]!=PioneerQ38ProjectionRoute(d[7]) || !d[16]
  || d[9]<0x04000000 || d[9]>0xc0000000 || (d[9]&4095) || d[10]!=5120 || d[11]!=5120
  || d[12]>=7 || d[14]>=7 || (d[13]&63) || (d[15]&63) || d[13]<0x04000000 || d[15]<0x04000000
  || d[13]>0xc3fffc00 || d[15]>0xc3fffc00
  || d[40]!=d[9]+RopeOffset || d[41]!=d[8] || d[42]!=8192 || d[43]!=64 || d[44]!=Recipes
  || d[45]!=RecipeCopies || d[46]!=DenseCopies || d[47]!=RawCopy || !d[48] || d[48]>=0x7fffffff
  || d[49]!=119 || d[50]!=Frames || d[51]!=Raw || d[52]>=248320 || d[53]!=5120 || d[54]!=67108864)return false;
 for(unsigned i=32;i<40;i+=2)if(!(d[i]|d[i+1]))return false;
 for(unsigned i=55;i<64;i++)if(d[i])return false;
 for(unsigned i=0;i<2;i++){
  unsigned bank=d[12+i*2],address=d[13+i*2];
  if(bank==d[7] && address<d[9]+67108864 && d[9]<address+1024)return false;
 }
 if(d[12]==d[14] && d[13]<d[15]+1024 && d[15]<d[13]+1024)return false;
 return true; // geometry only; original-source/init/native/commit still owed
}
}
#endif
