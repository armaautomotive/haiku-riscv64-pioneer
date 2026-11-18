/* SPDX-License-Identifier: MIT */
// Command/transport model only. The real producer and mixed-format worker
// will still need connectivity, receipt, restore and device qualification.
#include "../PioneerQ38TwinNativeQ8.h"
#include "../PioneerQ38NativeQ8Data.h"
#include "../PioneerQwen38Panel.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>

namespace N=PioneerQ38TwinNativeQ8;
using Format=PioneerQ38NativeQ8Data;

struct Engine {
 std::array<unsigned,N::PanelBytes> operand={};
 const uint8_t* tile;
 unsigned begin,base=0,c0=0x4231,c1=0xa03c8000,c73=0x13;
 unsigned ops=0,configs=0,drains=0,unpacks=0,pattern=0;
 bool drained=true,lanes=false;
 Engine(const uint8_t* data,unsigned address):tile(data),begin(address){}
 void Drain(){drained=true;drains++;}
 void Config(unsigned reg,unsigned value){
  assert(drained);configs++;
  switch(reg){
   case 0:assert(value==0x4231 || value==0x423e);c0=value;break;
   case 1:assert(value==0xa03c8000 || value==0xa03c0000);c1=value;break;
   case 73:assert(value==3 || value==0x13);c73=value;break;
   case 76:assert(value==(begin+N::Header-16)/16 || value==(begin+N::Header+N::PanelBytes-16)/16);base=(value+1)*16;break;
   default:assert(false);
  }
 }
 void Op(unsigned op){
  ops++;drained=false;
  assert(c0==0x423e && c1==0xa03c0000 && c73==3);
  if(op==0x8a000002){assert(!lanes && !pattern);lanes=true;return;}
  assert(lanes);
  constexpr unsigned seq[]={0x3700000f,0x5e23fc00,0x5120000a,0x5420000f,0x420000c1};
  assert(op==seq[pattern]);
  if(++pattern<5)return;
  pattern=0;
  assert(base>=begin+N::Header && base+N::PanelBytes<=begin+Format::QuantEnd);
  for(unsigned i=0;i<N::PanelBytes;i++){
   uint8_t tc=0;assert(Format::Decode(tile[base-begin+i],tc));
   const int value=int(tc)-int((tc&128)*2);
   operand[i]=PioneerInt8Operand(value);
  }
  unpacks++;
 }
};

int main(){
 std::array<uint8_t,Format::PageBytes> source={},wire={};
 unsigned positive=0,bad=0;
 for(unsigned batch:{unsigned(N::T::Buffer0),unsigned(N::T::Buffer1)})for(unsigned t=0;t<4;t++){
  const unsigned address=batch+t*N::Tile;
  for(unsigned i=0;i<32;i++)source[i]=i<16?uint8_t(i*13):uint8_t(i&1?0x38:0xb8);
  for(unsigned i=0;i<2*N::PanelBytes;i++)source[N::Header+i]=uint8_t(int((i*37+t*97)%255)-127);
  assert(Format::PreparePage(source.data(),0,N::Tile,wire.data(),wire.size()));
  assert(!memcmp(source.data(),wire.data(),N::Header) &&
   !memcmp(source.data()+Format::QuantEnd,wire.data()+Format::QuantEnd,source.size()-Format::QuantEnd));
  Engine e(wire.data(),address);N::Enter(e);
  for(unsigned panel=0;panel<2;panel++){
   PioneerQ38Panel expected={};assert(PioneerQ38UnpackPanel(8,source.data(),N::Tile,panel,&expected));
   const auto before=e;
   assert(N::Unpack(e,8,address,panel) && e.configs==before.configs+1 && e.drains==before.drains+2
    && e.ops==before.ops+5 && e.unpacks==before.unpacks+1 && e.drained);
   for(unsigned i=0;i<N::PanelBytes;i++)assert(e.operand[i]==((expected.operand[i/2]>>(16*(i&1)))&65535));
   positive++;
  }
  const auto before=e;
  assert(!N::Unpack(e,8,address,2) && !N::Unpack(e,12,address,0) && !N::Unpack(e,14,address,0));
  assert(e.ops==before.ops && e.configs==before.configs && e.drains==before.drains && e.operand==before.operand);
  bad+=3;
  N::Restore(e);
  assert(e.drained && e.c0==0x4231 && e.c1==0xa03c8000 && e.c73==0x13 && e.configs==8);
 }
 // Exhaust the entire allocation, not just the happy path's eight cells.
 unsigned seen=0;
 for(unsigned at=N::T::Buffer0-1;at<=N::T::BufferLimit;at++){
  unsigned data=0x12345678;
  const bool fit=N::PanelAddress(at,0,data);
  if(fit){assert(data==at+N::Header);seen++;}else assert(data==0x12345678);
 }
 assert(seen==8);
 Engine e(wire.data(),N::T::Buffer0);N::Enter(e);
 for(unsigned wrong:{0x90000U,0x91000U,0x94000U,0xa0000U,0xffffffffU}){
  const auto before=e;assert(!N::Unpack(e,8,wrong,0));
  assert(e.ops==before.ops && e.configs==before.configs && e.drains==before.drains);bad++;
 }
 auto unchanged=wire;source[N::Header+2*N::PanelBytes-1]=0x80;
 assert(!Format::PreparePage(source.data(),0,N::Tile,wire.data(),wire.size()) && wire==unchanged);
 printf("PASS twin native Q8: %u exact panels and %u inert refusals, exhaustive eight slots, late -128 refusal; synthetic unpack, no physical timing\n",
  positive,bad);
}
