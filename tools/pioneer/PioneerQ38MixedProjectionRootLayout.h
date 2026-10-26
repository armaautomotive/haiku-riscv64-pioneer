/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_MIXED_PROJECTION_ROOT_LAYOUT_H
#define PIONEER_Q38_MIXED_PROJECTION_ROOT_LAYOUT_H
#include "PioneerQ38MixedDenseRootLayout.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38DenseProjectionShape.h"
namespace PioneerQ38MixedProjectionRootLayout {
// Shared DATA extents, never a QMR1/native ownership alias.
using namespace PioneerQ38MixedDenseRootLayout;
enum {Magic=0x31504d51,Fault=0x46504d51,Code=0x58000,Parts=0x75800,Rows=0x76800,
 Banks=0x76a00,MatrixHeader=0x76b00,RecipeShadow=0x76c00,HeaderShadow=0x76d00,
 BankShadow=0x76d40,RowsShadow=0x76e00,PartsShadow=0x77000,QuantizedChunk=0x7e000,
 Projected=0x80000,Native=0xa0000,Scales=0xab000,NativeShadow=0x150000,ScaleShadow=0x154000};
static inline bool Shape(const uint32_t* d){
 unsigned type=0,k=0,n=0;
 if(!d || d[0]!=Magic || !PioneerQ38DenseProjectionGeometry(d[42],d[43],type,k,n)
  || !d[48] || d[48]>=0x7fffffff || d[49]!=119 || d[50]!=Parts || d[51]!=MatrixHeader || d[52]!=Banks
  || d[13]!=Rows || d[53]!=(d[43]==10?Output:Input) || d[54]!=Projected || d[55]!=PeerJobs
  || d[56]!=Nonces || d[57]!=PeerBoots)return false;
 for(unsigned i=58;i<64;i++)if(d[i])return false;
 return true; // COMPLETE canonical recipe grammar is checked separately
}
static inline bool BootShape(const uint32_t* b){
 if(!b || b[0]!=Magic)return false;
 uint32_t old[64];for(unsigned i=0;i<64;i++)old[i]=b[i];old[0]=PioneerQ38MixedDenseRootLayout::Magic;
 return PioneerQ38MixedDenseRootLayout::BootShape(old);
}
}
#endif
