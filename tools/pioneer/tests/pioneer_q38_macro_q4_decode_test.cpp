/* SPDX-License-Identifier: MIT */
#include <array>
#include <cassert>
#include <cstdio>
#include <stdexcept>
#include <vector>
#include "../PioneerQ38MacroQ4Decode.h"

// Independent cycle model of the small documented subset. In particular,
// stores read BEFORE a same-cycle scheduled simple result is written.
// Reject unsupported config,format,unit,delay,register or address choices.
struct MacroMachine {
 uint32_t lr[8][32]={},templates[4]={},sequence[4]={},misc=0;
 unsigned cycle=0,raw[256]={},result[256]={},written[256]={},moves=0,macros=0;
 unsigned ready[8]={};bool enabled=false,finished=false;
 struct Pending {unsigned at,reg;uint32_t op;};
 std::vector<Pending> pending;
 std::vector<uint32_t> issued;
 void Need(bool condition){if(!condition)throw std::runtime_error("macro schedule/ownership violation");}
 void Drain(){Need(pending.empty());cycle+=8;}
 void Op(uint32_t op) {
  issued.push_back(op);const unsigned code=op>>24;
  if(code==0x71) {
   unsigned reg=(op>>20)&15,mode=(op>>16)&15;Need(reg<8);
   for(unsigned l=0;l<32;l++) {
    if(mode==0)lr[reg][l]=(op&65535)<<16;
    else if(mode==10)lr[reg][l]=(lr[reg][l]&0xffff0000)|(op&65535);
    else {Need(mode==2);lr[reg][l]=op&65535;}
   }
   ready[reg]=cycle+1;
  } else if(code==0x91) {
   unsigned reg=(op>>4)&15,mod=op&15,value=op>>8&65535;
   Need(enabled && (mod==0 || mod==1) && reg<=8);
   if(!mod){Need(ready[0]<=cycle);value=lr[0][0];for(unsigned l=0;l<32;l++)Need(lr[0][l]==value);}
   if(reg<4)templates[reg]=value;
   else if(reg<8)sequence[reg-4]=value;
   else {Need(value==0);misc=value;}
  } else if(code==0x93) {
   unsigned index=(op>>22)&3,reg=(op>>20)&3,addr=op&1023;
   Need(enabled && !misc && ((op>>16)&15)==13 && !(op&0xfc01) && addr<16);
   unsigned s=sequence[index];Need(s==4 || s==5);
   uint32_t arithmetic=templates[(s&7)-4];
   Need(arithmetic==(s==4?0x7e004001U:0x7affc001U));
   for(const auto& p:pending)Need(p.reg!=reg && p.at!=cycle+1);
   for(unsigned l=0;l<32;l++)lr[reg][l]=raw[((addr&~3U)+l/8)*16+(l%8)*2+bool(addr&2)];
   ready[reg]=cycle+2;pending.push_back({cycle+1,reg,arithmetic});macros++;
  } else if(code==0x72) {
   unsigned reg=(op>>20)&15,addr=op&1023;
   Need(enabled && reg<2 && ready[reg]<=cycle && ((op>>16)&15)==5 && !(op&0xfc01));
   for(unsigned l=0;l<32;l++) {
    unsigned row=(addr&~3U)+l/8,col=(l%8)*2+bool(addr&2);
    Need((row>=128 && row<136) || (row>=144 && row<152));
    unsigned logical=row-128-(row>=144?8:0),at=logical*16+col;
    Need(!written[at] && lr[reg][l]<=15);result[at]=lr[reg][l];written[at]++;
   }
  } else if(code==8) {
   Need(op==uint32_t(0x08002040|(moves*4<<17)|(moves*4)) && pending.empty() && moves<4);
   for(unsigned i=moves*64;i<(moves+1)*64;i++)Need(written[i]==1);
   moves++;
  } else {Need(op==0x8a000002);enabled=true;}
  for(unsigned i=0;i<pending.size();) {
   auto p=pending[i];if(p.at!=cycle){i++;continue;}
   for(unsigned l=0;l<32;l++) {
    if(p.op==0x7e004001){Need(ready[4]<=cycle && lr[4][l]==15);lr[p.reg][l]&=lr[4][l];}
    else lr[p.reg][l]>>=4;
   }
   ready[p.reg]=cycle+1;pending.erase(pending.begin()+i);
  }
  cycle++;
 }
};
int main() {
 unsigned cases=0;
 for(unsigned seed=0;seed<256;seed++)for(unsigned panel=0;panel<16;panel++) {
  MacroMachine m;for(unsigned i=0;i<256;i++)m.raw[i]=(i+seed)&255;
  PioneerQ38MacroQ4Decode::Setup(m);unsigned before=m.issued.size();
  assert(PioneerQ38MacroQ4Decode::Decode(m,panel));m.Drain();
  assert(m.macros==8 && m.moves==4 && m.issued.size()-before==20+unsigned(!(panel&2)));
  for(unsigned i=0;i<256;i++)assert(m.written[i]==1 && m.result[i]==((m.raw[i]>>(panel&2?4:0))&15));
  PioneerQ38MacroQ4Decode::Finish(m);
  for(unsigned v:m.sequence)assert(!v);
  auto issued=m.issued;assert(!PioneerQ38MacroQ4Decode::Decode(m,16) && issued==m.issued);cases++;
 }
 unsigned negatives=0;
 // A store immediately after the load would read before its decode result.
 {MacroMachine m;PioneerQ38MacroQ4Decode::Setup(m);PioneerSfpuLoad32(m,4,15);m.Op(0x930d0000);
  try{m.Op(0x72050080);assert(false);}catch(const std::runtime_error&){negatives++;}}
 // No instruction-count waits or extra scheduled MAD/round/store are accepted.
 for(uint32_t sequence:{0x104U,0x404U,0x04000004U}) {
  MacroMachine m;PioneerQ38MacroQ4Decode::Setup(m);m.sequence[0]=sequence;
  try{m.Op(0x930d0000);assert(false);}catch(const std::runtime_error&){negatives++;}
 }
 printf("PASS_MACRO_Q4_DECODE exactCases%u all256bytes/all16panels/32lanes timed loads+simple+native stores;negativeSchedules%u;OFFLINE_ONLY_NO_NATIVE_OR_SPEED_GRANT\n",cases,negatives);
}
