/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_TEST_OBSERVED_TRACE 1
#include "PioneerQ38ObservedTrace.h"
#include <cassert>
static uint32_t Bits(float f){uint32_t b;std::memcpy(&b,&f,4);return b;}
static void Fill(PioneerQ38ObservedTrace& trace,bool wrongFeedback=false){
 std::array<uint32_t,5120> hidden={};std::vector<uint32_t> logits(248320);
 assert(trace.Embedding(0,248319,hidden.data(),5120));
 for(unsigned position=0;position<2;position++){
  for(unsigned i=0;i<hidden.size();i++)hidden[i]=Bits(float(position)+float(i%13)*.125f);
  for(unsigned layer=0;layer<64;layer++)assert(trace.Layer(position,layer,hidden.data(),5120));
  std::fill(logits.begin(),logits.end(),0);unsigned selected=position?PioneerQ38ObservedTrace::EosSecond:PioneerQ38ObservedTrace::EosFirst;
  logits[selected]=Bits(1.f);
  for(unsigned first=0;first<248320;first+=256)assert(trace.Vocabulary(position,first,logits.data()+first,std::min(256U,248320-first)));
  if(!position)assert(trace.Embedding(1,17,hidden.data(),5120));
  std::array<uint32_t,64> record={};
  const unsigned h[]={PioneerQ38ObservedTrace::TokenMagic,1,position,position?17U:248319U,selected,Bits(1.f),position+1,(position+1)&1,unsigned(!position),unsigned(!position),64,973};
  for(unsigned i=0;i<12;i++)record[i]=h[i];
  for(unsigned i=0;i<8;i++)record[12+i]=i%2?0:1+i/2;
  for(unsigned i=0;i<16;i++)record[20+i]=0x12340000+i;
  record[44]=position+1;record[45]=48;record[46]=119;
  if(wrongFeedback && position){record[9]=1;assert(!trace.Commit(position,record.data(),64));return;}
  assert(trace.Commit(position,record.data(),64));
 }
}
int main(int argc,char** argv){
 assert(argc==2);PioneerQ38ObservedTrace ready(true);Fill(ready);
 assert(ready.EosComplete() && !ready.Complete() && !ready.SavedComplete() && !ready.SavedEosComplete());
 std::array<uint32_t,64> terminal={};const unsigned h[]={0x31545451,1,2,2,1,119,64,973,0,1};
 for(unsigned i=0;i<10;i++)terminal[i]=h[i];
 for(unsigned i=0;i<8;i++)terminal[12+i]=i%2?0:1+i/2;
 for(unsigned i=0;i<16;i++)terminal[20+i]=0x12340000+i;
 for(unsigned i=0;i<8;i++)terminal[36+i]=0xfffffff0+i;
 unsigned faults=0;
 for(unsigned word=0;word<64;word++)if(word<36 || word>=44){
  auto bad=ready;auto changed=terminal;changed[word]^=1U<<(word*7%32);
  assert(!bad.SaveEos(argv[1],changed.data(),64) && !bad.EosComplete() && !bad.SavedEosComplete());faults++;
 }
 {auto bad=ready;assert(!bad.Save(argv[1]) && !bad.SavedComplete() && !bad.EosComplete());faults++;}
 {auto bad=ready;std::array<uint32_t,5120> hidden={};assert(!bad.Embedding(2,17,hidden.data(),5120) && !bad.EosComplete());faults++;}
 {auto bad=ready;assert(!bad.SaveEos(argv[1],nullptr,64) && !bad.EosComplete());faults++;}
 {auto bad=ready;assert(!bad.SaveEos(argv[1],terminal.data(),63) && !bad.EosComplete());faults++;}
 {PioneerQ38ObservedTrace partial(true);assert(!partial.SaveEos(argv[1],terminal.data(),64));faults++;}
 {PioneerQ38ObservedTrace bad(true);Fill(bad,true);assert(!bad.EosComplete() && !bad.SaveEos(argv[1],terminal.data(),64));faults++;}
 {PioneerQ38ObservedTrace normal;assert(!normal.EosComplete() && !normal.SaveEos(argv[1],terminal.data(),64));faults++;}
 assert(ready.SaveEos(argv[1],terminal.data(),64) && !ready.EosComplete() && ready.SavedEosComplete() && !ready.SavedComplete());
 assert(!ready.SaveEos(argv[1],terminal.data(),64) && !ready.SavedEosComplete());faults++;
 printf("PASS distinct EOS DATA recorder/two complete observations+terminal/exclusive EOS files/NO third embedding or normal trace promotion/%u terminal,identity,source,padding,partial,family,feedback,duplicate refusals;SYNTHETIC DATA only,NOT model/EOS execution/STOP/native/Work/TPS proof\n",faults);
}
