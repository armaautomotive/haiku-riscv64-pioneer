/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38PreparedQ4Mask.h"
#include "../PioneerQ38Q4EscapeTiles.h"
#include <array>
#include <cassert>
#include <cstring>
#include <cstdio>
#ifdef NDEBUG
#error Prepared-source checks must remain enabled
#endif
using Layout=PioneerQ38PreparedQ4Layout;
using Codec=PioneerQ38Q4EscapeTiles;
struct MaskEndpoint {
 std::array<uint8_t,1024> payload{};std::array<unsigned,128> dense{};
 unsigned reads=0,writes=0,fences=0;
 unsigned PayloadWord(unsigned at){assert(!(at&3) && at+4<=payload.size());unsigned value;memcpy(&value,payload.data()+at,4);reads++;return value;}
 void MaskStore(unsigned i,unsigned v){assert(i<128);dense[i]=v;writes++;}
 void SealData(){fences++;}
};
int main(){
 unsigned expansions=0,geometry=0,refusals=0;
 for(unsigned count=0;count<=256;count++)for(unsigned shift:{0U,16U,32U,48U}){
  MaskEndpoint e;std::array<uint8_t,512> expected{},packed{};Codec::Mask description;
  for(unsigned row=0;row<count;row++){const unsigned value=(1U<<(row&15))|((row*71+3)&65535);expected[row*2]=uint8_t(value);expected[row*2+1]=uint8_t(value>>8);}
  assert(Codec::PackMask(expected.data(),512,packed.data(),packed.size(),description));
  memcpy(e.payload.data()+shift,packed.data(),description.bytes);
  const Layout::Window group={64,1024};const unsigned index=(description.kind<<30)|(count?64+shift:0);
  assert(PioneerQ38PreparedQ4Mask::Expand(e,index,group) && e.writes==128 && e.fences==1
   && !memcmp(expected.data(),e.dense.data(),512));expansions++;
  if(!count)assert(!e.reads);
 }
 for(unsigned k:{5120U,6144U,17408U})for(unsigned groups:{1U,3U,16U})for(unsigned worker:{1U,69U,119U}){
  const unsigned tiles=13+groups*(k>>8)+7,indexBytes=(tiles*4+15)&~15U;
  std::array<unsigned,16> j{},d{};j[1]=12;j[2]=2304;j[5]=k;j[6]=0x4000000+13*2304;j[7]=groups;j[8]=0x47;j[11]=worker;j[12]=17;j[14]=1;
  d={Layout::Magic,worker,17,12,k,groups,j[6],j[8],0x20000000,indexBytes+tiles*512,13,tiles,0x4000000,tiles*2304,1,0};
  assert(Layout::Shape(d.data(),j.data()));geometry++;
  std::array<unsigned,128> index{};
  for(unsigned group=0;group<groups;group++){
   index.fill(0x80000000);const unsigned first=Layout::IndexWordsBefore(d.data(),group),start=13+group*(k>>8);
   for(unsigned block=0;block<(k>>8);block++)index[first+block]=indexBytes+(start+block)*512;
   Layout::Window window;assert(Layout::GroupWindow(d.data(),group,index.data(),window));
   assert(!(Layout::IndexSource(d.data(),group)&63) && !(Layout::IndexReadBytes(d.data(),group)&63)
    && Layout::IndexReadBytes(d.data(),group)<=Layout::IndexCapacity && !(window.start&63) && !(window.bytes&63)
    && window.bytes<=Layout::PayloadCapacity && window.start+window.bytes<=Layout::Allocated(d[9]));
  }
  for(unsigned field=0;field<16;field++){
   auto bad=d;bad[field]^=field==10?1U:0x80000001U;assert(!Layout::Shape(bad.data(),j.data()));refusals++;
  }
  std::array<unsigned,16> summary={Layout::SummaryMagic,worker,17,k,groups,1+groups,1+groups,0,0,0,0,0,0,0,0,0};unsigned reads=0,flits=0;
  assert(Layout::SummaryShape(summary.data(),j.data(),reads,flits) && reads==1+groups && flits==1+groups);
 }
 for(unsigned word=0;word<8704;word++){
  const unsigned at=Layout::Input(word);assert(!(at&3) && ((word<1152 && at>=0x96200 && at+4<=0x97400)
   || (word>=1152 && at>=0xa8900 && at+4<=0xaff00)));
 }
 {MaskEndpoint e;assert(!PioneerQ38PreparedQ4Mask::Expand(e,0xc0000040,{64,1024}) && !e.writes && !e.reads);refusals++;}
 {MaskEndpoint e;assert(!PioneerQ38PreparedQ4Mask::Expand(e,0x80000001,{0,0}) && !e.writes && !e.reads);refusals++;}
 {MaskEndpoint e;e.payload[0]=1;assert(!PioneerQ38PreparedQ4Mask::Expand(e,0x40000040,{64,1024}) && !e.fences);refusals++;}
 printf("PASS_PREPARED_Q4_DATA exactMaskExpansions%u nativeGeometries%u refusals%u all8704InputWordsDisjoint;NO_ROM_NATIVE_MODEL_OR_TPS\n",expansions,geometry,refusals);
}
