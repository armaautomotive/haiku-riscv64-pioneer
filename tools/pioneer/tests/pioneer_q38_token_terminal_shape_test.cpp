/* SPDX-License-Identifier: MIT */
#include <cstdint>
#include "../PioneerQ38TokenTerminalShape.h"
#include <array>
#include <cassert>
#include <cstdio>
int main(){
 namespace T=PioneerQ38TokenLayout;
 std::array<uint32_t,64> boot={},base={};
 for(unsigned i=8;i<32;i++)boot[i]=0x31415900U+i*0x010203U;
 base[0]=T::TerminalMagic;base[1]=1;base[5]=119;base[9]=1;
 for(unsigned i=0;i<8;i++)base[12+i]=boot[8+i];
 for(unsigned i=0;i<16;i++)base[20+i]=boot[16+i];
 for(unsigned i=0;i<8;i++)base[36+i]=0xffffff00U+i;
 base[42]=base[38];unsigned accepted=0,refused=0;
 for(unsigned kind=0;kind<7;kind++){
  auto t=base;unsigned limit=kind==6?8192:kind==1?7:3,prompts=2;bool eos=kind==1,cancel=kind>=2 && kind<=5;
  t[2]=cancel?3:eos?2:1;t[3]=cancel?unsigned(kind==5):kind==6?8192:3;t[4]=cancel?0:t[3]-1;
  t[6]=kind==2?1:64;t[7]=kind==2 || kind==3?0:973;t[8]=t[3]&1;t[10]=kind==2 || kind==3?1:kind==4?2:kind==5?3:0;
  const std::array<uint32_t,3> progress={T::Magic,t[4],t[6]};
  auto shape=[&](const uint32_t* term,const uint32_t* identity,const uint32_t* p,unsigned count,unsigned input,bool stop,bool requested){
   return PioneerQ38TokenTerminalShape(term,identity,p,count,input,stop,requested);
  };
  assert(shape(t.data(),boot.data(),progress.data(),limit,prompts,eos,cancel));accepted++;
  for(unsigned word=0;word<64;word++)if(word<36 || word==38 || word==42 || word>=44)for(unsigned bit=0;bit<32;bit++){
   auto bad=t;bad[word]^=1U<<bit;assert(!shape(bad.data(),boot.data(),progress.data(),limit,prompts,eos,cancel));refused++;
  }
  for(unsigned word=0;word<3;word++)for(unsigned bit=0;bit<32;bit++){
   auto bad=progress;bad[word]^=1U<<bit;assert(!shape(t.data(),boot.data(),bad.data(),limit,prompts,eos,cancel));refused++;
  }
  for(unsigned word=8;word<32;word++){
   auto bad=boot;bad[word]^=1;assert(!shape(t.data(),bad.data(),progress.data(),limit,prompts,eos,cancel));refused++;
  }
  assert(!shape(nullptr,boot.data(),progress.data(),limit,prompts,eos,cancel)
   && !shape(t.data(),nullptr,progress.data(),limit,prompts,eos,cancel)
   && !shape(t.data(),boot.data(),nullptr,limit,prompts,eos,cancel));refused+=3;
  for(unsigned invalid:{0U,8193U,0xffffffffU}){assert(!shape(t.data(),boot.data(),progress.data(),invalid,prompts,eos,cancel));refused++;}
  for(unsigned invalid:{0U,limit+1,0xffffffffU}){assert(!shape(t.data(),boot.data(),progress.data(),limit,invalid,eos,cancel));refused++;}
  if(cancel){assert(!shape(t.data(),boot.data(),progress.data(),limit,prompts,eos,false));refused++;}
  if(eos){assert(!shape(t.data(),boot.data(),progress.data(),limit,prompts,false,cancel));refused++;}
 }
 printf("PASS terminal DATA grammar%u normal/EOS/cancel-stage/context-boundary cases/%u bit/progress/identity/config/null refusals;NO native owner/publication/STOP/restore/END/Work grant\n",accepted,refused);
}
