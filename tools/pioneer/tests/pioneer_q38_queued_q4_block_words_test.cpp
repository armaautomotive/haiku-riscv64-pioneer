/* SPDX-License-Identifier: MIT */
// OFFBOARD word/transaction test,NOT a complete-worker or hardware test.
#define PIONEER_Q38_QUEUED_Q4_BLOCK_IMAGE 1
#define PIONEER_Q38_PREPARED_EXACT_Q4_IMAGE 1
#define PIONEER_Q38_PREPARED_TWO_PRODUCT_IMAGE 1
#define PIONEER_Q38_PREPARED_TWO_PRODUCT_REPLAY_IMAGE 1
#include "../PioneerQ38QueuedQ4Block.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <utility>
#include <vector>
#ifdef NDEBUG
#error Assertions required
#endif
using Q=PioneerQ38QueuedQ4Block;
struct Observer {
 std::array<unsigned,128> config{};std::vector<unsigned> words;
 unsigned reads=0,bad=~0U;bool lost=false;
 void Op(unsigned op){
  words.push_back(op);if(op==0xa2ff9816U)return;
  const unsigned code=op>>24,index=op&255,mask=(op>>16)&255,value=(op>>8)&255;
  assert(code>=0xb3 && code<=0xb6 && index<128 && mask==255);
  const unsigned shift=(code-0xb3)*8;
  config[index]=(config[index]&~(255U<<shift))|(value<<shift);
 }
 bool ReadConfig(unsigned index,unsigned& value){
  assert(index<128);const unsigned at=reads++;value=config[index];
  if(at==bad){if(lost)return false;value^=1;}
  return true;
 }
};
struct Stream {
 std::vector<std::pair<unsigned,unsigned>> events;unsigned masks=0,escapes=0,seals=0;
 void Op(unsigned op){events.push_back({0,op});}
 void Config(unsigned i,unsigned v){events.push_back({i+1,v});}
 void Drain(){events.push_back({~0U,0});}
 unsigned Mask(unsigned i){masks++;return 0xa53f00e1U^i;}
 void Escape(unsigned,unsigned){escapes++;}
 void SealData(){seals++;}
};
#ifndef PIONEER_Q38_QUEUED_Q4_WORD_MODEL_ONLY
int main(){
 static_assert(Q::WaitAll==0xa2ff9816U && ((Q::WaitAll>>15)&511)==511 && (Q::WaitAll&8191)==0x1816,"Blackhole all-class wait");
 static_assert(Q::EscapeBase==0x9c610 && Q::EscapeStride==80 && Q::EscapeBytes==16*80,"sixteen separate header+payload planes");
 const unsigned indices[]={1,50,64,65,72,73,76,84,86,124};unsigned valid=0,invalid=0,faults=0;
 for(unsigned index:indices)for(unsigned byte=0;byte<4;byte++)for(unsigned value=0;value<256;value++){
  Q q;Observer e;e.config.fill(0xa55ac33c);const auto before=e.config;
  assert(q.Begin() && !q.Begin());const unsigned desired=value<<(byte*8);
  assert(q.Config(e,index,desired) && e.words.size()==4);
  assert(e.words[0]==(0xb3ff0000U|((desired&255)<<8)|index));
  assert(e.words[1]==(0xb4ff0000U|(((desired>>8)&255)<<8)|index));
  assert(e.words[2]==(0xb5ff0000U|(((desired>>16)&255)<<8)|index));
  assert(e.words[3]==(0xb6ff0000U|((desired>>24)<<8)|index));
  for(unsigned at=0;at<128;at++)assert(e.config[at]==(at==index?desired:before[at]));
  Q::Wait(e);assert(e.words.size()==5 && e.words.back()==0xa2ff9816U);
  // Explicit synthetic completion only;this is not an actual GPU wait.
  assert(q.Verify(e) && e.reads==1 && q.phase==Q::Idle && !q.seen && !q.Verify(e));valid++;
 }
 for(unsigned index=0;index<256;index++){
  bool okay=false;for(unsigned good:indices)okay|=good==index;if(okay)continue;
  Q q;Observer e;assert(q.Begin() && !q.Config(e,index,0x12345678) && e.words.empty());
  assert(q.phase==Q::Failed && !q.Begin() && !q.Config(e,1,0) && !q.Verify(e) && !e.reads);invalid++;
 }
 for(unsigned at=0;at<Q::Fields;at++)for(bool lost:{false,true}){
  Q q;Observer e;assert(q.Begin());for(unsigned index:indices)assert(q.Config(e,index,index*0x1020408U));
  e.bad=at;e.lost=lost;assert(!q.Verify(e) && e.reads==at+1 && q.phase==Q::Failed);
  const auto words=e.words;assert(!q.Verify(e) && !q.Begin() && !q.Config(e,1,0));
  assert(e.reads==at+1 && e.words==words);faults++;
 }
 {
  Q q;Observer e;assert(q.Begin());
  for(unsigned index:indices)assert(q.Config(e,index,~index));
  for(unsigned index:indices)assert(q.Config(e,index,index));
  assert(q.Verify(e) && e.reads==10 && e.words.size()==80);
 }
 Stream old,next;assert(PioneerQ38ExactQ4TwoProductDot::Run(old) && Q::Dot(next));
 assert(old.events==next.events && old.masks==8 && old.escapes==16 && old.seals==1);
 assert(!next.masks && !next.escapes && next.seals==1);
 Stream oldMapped,nextMapped;
 PioneerQ38ExactQ4Affine::PanelEmitter<Stream> before(oldMapped);
 Q::PanelEmitter<Stream> after(nextMapped);
 assert(PioneerQ38ExactQ4TwoProductDot::Run(before) && Q::Dot(after));
 assert(oldMapped.events==nextMapped.events && oldMapped.escapes==16 && !nextMapped.escapes
  && oldMapped.seals==1 && nextMapped.seals==1);
 printf("PASS queued-Q4 block component:word cases%u unsupported-register refusals%u final-readback faults%u;exact original dot/control stream;1280B immutable correction workspace;NO complete-worker,native,device or speed result\n",valid,invalid,faults);
}
#endif
