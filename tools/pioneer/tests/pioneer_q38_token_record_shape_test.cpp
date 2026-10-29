/* SPDX-License-Identifier: MIT */
#include <cstdint>
#include "../PioneerQ38TokenRecordShape.h"
#include "../PioneerQ38TokenTerminalShape.h"
#include <array>
#include <cassert>
#include <cstdio>
int main(){
 namespace T=PioneerQ38TokenLayout;unsigned accepted=0,refused=0;
 std::array<uint32_t,64> boot={};for(unsigned i=8;i<32;i++)boot[i]=0x51430000U+i*0x10203U;
 for(unsigned kind=0;kind<11;kind++){
  const unsigned limit=kind==8?8192:kind==2 || kind==3 || kind==4 || kind==5 || kind==7 || kind==10?7:kind==9?1:3;
  const unsigned prompts=kind==9?1:2,position=kind==0 || kind==10?0:kind==3 || kind==4?1:kind==8?8191:kind==9?0:2;
  const bool stop=kind==2 || kind==7 || kind==10,cancel=kind>=3 && kind<=7;
  std::array<uint32_t,64> terminal={},r={};
  terminal[0]=T::TerminalMagic;terminal[1]=1;terminal[2]=cancel?3:stop && kind!=10?2:1;
  terminal[3]=kind==0 || kind==10?limit:position+1;terminal[4]=kind==0 || kind==10?limit-1:kind==3 || kind==4?position+1:position;
  terminal[5]=119;terminal[6]=kind==3?1:64;terminal[7]=kind==3?0:973;
  terminal[8]=terminal[3]&1;terminal[9]=1;terminal[10]=kind==3?1:kind==4?2:cancel?3:0;
  for(unsigned i=0;i<8;i++)terminal[12+i]=boot[8+i];
  for(unsigned i=0;i<16;i++)terminal[20+i]=boot[16+i];
  const std::array<uint32_t,3> progress={T::Magic,terminal[4],terminal[6]};
  assert(PioneerQ38TokenTerminalShape(terminal.data(),boot.data(),progress.data(),limit,prompts,stop,cancel));
  r[0]=T::TokenMagic;r[1]=1;r[2]=position;r[3]=17;r[4]=stop?42:55;r[5]=0xbf000000U;
  r[6]=position+1;r[7]=(position+1)&1;r[8]=unsigned(position+1<prompts);
  r[9]=unsigned(position+1<limit && !(stop && position+1>=prompts));r[10]=64;r[11]=973;r[44]=position+1;r[45]=48;r[46]=119;
  for(unsigned i=0;i<8;i++){r[12+i]=boot[8+i];r[36+i]=0xfffffff0U+i;}
  r[42]=r[38];for(unsigned i=0;i<16;i++)r[20+i]=boot[16+i];
  auto shape=[&](const uint32_t* record,const uint32_t* identity,const uint32_t* end,unsigned pos,unsigned input,unsigned count,unsigned prompt){
   return PioneerQ38TokenRecordShape(record,identity,end,pos,input,count,prompt,stop,42,43);
  };
  assert(shape(r.data(),boot.data(),terminal.data(),position,17,limit,prompts));accepted++;
  for(unsigned word=0;word<64;word++)if(word<4 || (word>=6 && word<36) || word==38 || word==42 || word>=44){
   for(unsigned bit=0;bit<32;bit++){
    auto bad=r;bad[word]^=1U<<bit;
    assert(!shape(bad.data(),boot.data(),terminal.data(),position,17,limit,prompts));refused++;
   }
  }
  for(unsigned word=8;word<32;word++){
   auto bad=boot;bad[word]^=1;assert(!shape(r.data(),bad.data(),terminal.data(),position,17,limit,prompts));refused++;
  }
  for(uint32_t badBits:{0x7f800000U,0xff800000U,0x7fc00001U,0xffffffffU}){
   auto bad=r;bad[5]=badBits;assert(!shape(bad.data(),boot.data(),terminal.data(),position,17,limit,prompts));refused++;
  }
  for(unsigned invalid:{248320U,0xffffffffU}){
   auto bad=r;bad[4]=invalid;assert(!shape(bad.data(),boot.data(),terminal.data(),position,17,limit,prompts));refused++;
  }
  for(unsigned invalid:{0U,8193U,0xffffffffU}){assert(!shape(r.data(),boot.data(),terminal.data(),position,17,invalid,prompts));refused++;}
  for(unsigned invalid:{0U,limit+1,0xffffffffU}){assert(!shape(r.data(),boot.data(),terminal.data(),position,17,limit,invalid));refused++;}
  assert(!shape(nullptr,boot.data(),terminal.data(),position,17,limit,prompts)
   && !shape(r.data(),nullptr,terminal.data(),position,17,limit,prompts)
   && !shape(r.data(),boot.data(),nullptr,position,17,limit,prompts)
   && !shape(r.data(),boot.data(),terminal.data(),position,18,limit,prompts));refused+=4;
 }
 printf("PASS committed-record DATA grammar%u prefill/final/EOS/cancel-stage/context-boundary cases/%u identity/position/flags/padding/nonfinite/config/null refusals;NO native owner/GDDR publication/NoC baseline/HOLD/END/Work grant\n",accepted,refused);
}
