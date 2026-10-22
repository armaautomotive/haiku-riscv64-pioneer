/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_FFN_GRAPH_FIXTURE_H
#define PIONEER_Q38_FFN_GRAPH_FIXTURE_H
#include <array>
#include <cassert>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38FfnGraphShape.h"
// TEST DATA ONLY,not a canonical model/native manifest factory or authority.
struct FfnGraphFixture {
 std::array<uint32_t,TT_Q38_FFN_GRAPH_WORDS> manifest={};
 std::array<uint32_t,119*16> source={};
 FfnGraphFixture(unsigned bank=0,bool edge=false,bool wide=true,bool reversed=false,unsigned types=0) {
  auto& d=manifest;d[0]=TT_Q38_FFN_GRAPH_MARKER;d[1]=TT_Q38_FFN_GRAPH_VERSION;d[2]=67108864;d[3]=bank;d[4]=edge?0xc0000000:0x04000000;
  d[5]=1;d[6]=0;d[7]=3;d[8]=PioneerQ38SpanRoute(bank);d[9]=129;d[10]=TT_Q38_FFN_GRAPH_MANIFEST+256;
  d[11]=4352;d[12]=TT_Q38_FFN_GRAPH_MANIFEST+768;d[13]=5;d[14]=1;
  for(unsigned i=0;i<16;i++)d[16+i]=i?0x12340000+i:0x16005;
  for(unsigned i=32;i<40;i++)d[i]=0x13570000+i;d[40]=123;d[41]=7;d[42]=bank*9+unsigned(edge);
  for(unsigned w=0;w<119;w++)for(unsigned i=0;i<16;i++)source[w*16+i]=i?0x12340000+i:0x16005;
  for(unsigned role=0;role<3;role++) {
   uint32_t* m=d.data()+PioneerQ38FfnGraphRoleOffset(role);m[0]=role;m[1]=(types&(1U<<role))?14:12;
   m[2]=wide?119:role==2?20:68;m[3]=role==2?17408:5120;m[4]=role==2?5120:17408;
   unsigned groups=m[4]/16,row=0;
   std::array<unsigned,119> first={},count={};
   for(unsigned w=0;w<m[2];w++){first[w]=row;count[w]=groups/m[2]+unsigned(w<groups%m[2]);row+=count[w]*16;}
   for(unsigned w=0;w<m[2];w++) {
    unsigned index=reversed?m[2]-1-w:w;uint32_t* p=m+16+w*8;
    p[0]=0x10000040+role*0x10000000+(w/7)*0x00800000;p[1]=w%7;p[2]=count[index];
    p[4]=PioneerQ38SpanRoute(p[1]);p[5]=source[w*16];m[16+119*8+w]=first[index];
   }
  }
  assert(PioneerQ38FfnGraphManifest(d.data(),source.data()));
 }
};
#endif
