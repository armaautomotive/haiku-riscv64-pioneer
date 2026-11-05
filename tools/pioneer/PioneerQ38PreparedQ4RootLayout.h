/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_PREPARED_Q4_ROOT_LAYOUT_H
#define PIONEER_Q38_PREPARED_Q4_ROOT_LAYOUT_H
#include "PioneerQ38PreparedQ4Layout.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionInputShape.h"
// Immutable per-layer/FFN expected-read DATA. NOT native source ownership.
// The native owner must authenticate these against the SAME prepared index
// and initialized correction extents BEFORE any worker sees a descriptor.
struct PioneerQ38PreparedQ4RootLayout {
 using Word=PioneerQ38PreparedQ4Layout::Word;using L=PioneerQ38PreparedQ4Layout;
 enum:unsigned {Magic=0x31525150,Offset=0x70000,Stride=0x2800,Workers=119,
  HeaderBytes=64,DescriptorsBytes=Workers*64,LedgerOffset=HeaderBytes+DescriptorsBytes,
  Bytes=9600,RootTable=L::RootTable,RootLedger=RootTable+DescriptorsBytes,
  RootTableBytes=Bytes-HeaderBytes,RootHeader=RootTable+RootTableBytes,RootShadow=RootHeader+HeaderBytes,
  ShadowLedger=RootShadow+DescriptorsBytes,ShadowHeader=RootShadow+RootTableBytes,RootEnd=ShadowHeader+HeaderBytes};
 static unsigned PhaseOffset(unsigned role){return role>=2 && role<=4?Offset+(role-2)*Stride:0;}
 static bool Header(const volatile Word* h,unsigned layer,unsigned role){
  if(!h || layer>=64 || !PhaseOffset(role))return false;
  const unsigned expected[]={Magic,1,layer,role,role==4?17408U:5120U,Workers,16,Bytes,
   PhaseOffset(role),RootTable,RootLedger,1,0,0,0,0};
  for(unsigned i=0;i<16;i++)if(h[i]!=expected[i])return false;
  return true;
 }
 // A zero-request immutable template may be stamped ONCE for this job.
 // Rebuilding the identical job during gathering retains the SAME stamp;
 // a different sequence needs a freshly staged phase table,not adoption.
 static bool Bind(const volatile Word* header,unsigned layer,unsigned role,
  const volatile Word* descriptor,const volatile Word* ledger,const Word* job,
  unsigned firstRow,unsigned bank,volatile Word* output,unsigned& reads,unsigned& flits){
  if(!Header(header,layer,role) || !descriptor || !ledger || !job || !output || bank>=7 || (firstRow&15)
   || ledger[2]!=firstRow || ledger[3]!=bank || job[8]!=PioneerQ38ProjectionRoute(bank) || header[4]!=job[5]
   || firstRow>=(role==4?5120U:17408U) || job[7]*16>(role==4?5120U:17408U)-firstRow
   || (descriptor[2] && descriptor[2]!=job[12]))return false;
  Word stage[16];for(unsigned i=0;i<16;i++)stage[i]=descriptor[i];stage[2]=job[12];
  if(!L::Shape(stage,job))return false;
  const Word summary[]={L::SummaryMagic,job[11],job[12],job[5],job[7],ledger[0],ledger[1],0,0,0,0,0,0,0,0,0};
  unsigned count=0,units=0;if(!L::SummaryShape(summary,job,count,units))return false;
  for(unsigned i=0;i<16;i++)output[i]=stage[i];
  reads=count;flits=units;return true;
 }
};
static_assert(PioneerQ38PreparedQ4RootLayout::Offset+3*PioneerQ38PreparedQ4RootLayout::Stride<=0x78000
 && PioneerQ38PreparedQ4RootLayout::Bytes<=PioneerQ38PreparedQ4RootLayout::Stride
 && !(PioneerQ38PreparedQ4RootLayout::RootTableBytes&63)
 && PioneerQ38PreparedQ4RootLayout::RootEnd<0xb0000,"fixed disjoint prepared ROOT data geometry");
#endif
