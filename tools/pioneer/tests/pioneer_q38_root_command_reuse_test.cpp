/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_WORK_ROOT_COMMAND_REUSE
#include <cstdint>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include "../PioneerQ38RootCommandReuse.h"
struct IO{
 uint32_t words[10]={},seen=0;unsigned reads=0,writes=0,badWord=10;bool wrote[10]={};
 uint32_t Read(unsigned word){assert(word<10);reads++;seen|=1U<<word;return words[word]^(word==badWord&&wrote[word]?1U:0U);}
 void Write(unsigned word,uint32_t value){assert(word<10&&(seen&(1U<<word)));writes++;wrote[word]=true;words[word]=value;}
};
int main(){
 unsigned cases=0;
 for(unsigned worker=1;worker<120;worker++)for(bool write:{false,true})for(unsigned bytes:{4U,64U,320U,1280U}){
  uint32_t c[10]={write?0x75000U:0x83f00U,0,write?129U:worker,write?0x74000U:0x75000U,0,write?worker:129U,0,write?0x212U:0U,bytes,0};
  for(unsigned different=0;different<1024;different++){
   IO io;unsigned count=0;
   for(unsigned w=0;w<10;w++){bool change=bool(different&(1U<<w));io.words[w]=c[w]^(change?0x80000000U:0U);count+=change;}
   assert(PioneerQ38ProgramRootCommand(io,c,true)&&io.seen==1023&&io.writes==count&&io.reads==10+count&&!memcmp(c,io.words,sizeof(c)));cases++;
  }
  IO io;memcpy(io.words,c,sizeof(c));io.words[7]^=0xff000000U; // real hardware may change reserved control bits
  assert(PioneerQ38ProgramRootCommand(io,c,true)&&io.writes==1&&io.reads==11&&io.words[7]==c[7]);
  io.reads=io.writes=io.seen=0;assert(PioneerQ38ProgramRootCommand(io,c,true)&&io.writes==0&&io.reads==10&&io.seen==1023);
  IO pending;assert(!PioneerQ38ProgramRootCommand(pending,c,false)&&!pending.reads&&!pending.writes);
  for(unsigned fault=0;fault<10;fault++){
   IO bad;for(unsigned w=0;w<10;w++)bad.words[w]=c[w]^1U;bad.badWord=fault;
   assert(!PioneerQ38ProgramRootCommand(bad,c,true)&&bad.writes==fault+1&&bad.reads==2*(fault+1)&&bad.seen==((1U<<(fault+1))-1));
  }
 }
 printf("PASS %u exact register-difference patterns across119routes/read+write/4lengths;EVERY actual command field read,only changed fields written+readback;hardware-reserved control changes repaired,10 per-field failed readbacks stop before later writes;pending idle refuses BEFORE any IO;NO doorbell/source/native/speed authority\n",cases);
}
