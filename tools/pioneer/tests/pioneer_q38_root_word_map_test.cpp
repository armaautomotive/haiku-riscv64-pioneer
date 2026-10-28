/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_TEST_FAST_ROOT_MEMORY 1
#include "PioneerQ38RootWordMap.h"
#include <cassert>
#include <cstdio>
#include <memory>
int main(){
 auto fast=std::make_unique<PioneerQ38RootWordMap>();std::map<uint32_t,uint32_t> slow;uint32_t random=0x1743ba91;
 assert(fast->empty() && fast->find(0)==fast->end());unsigned writes=0,reads=0,erases=0,missing=0;
 for(unsigned n=0;n<1000000;n++){
  random^=random<<13;random^=random>>17;random^=random<<5;
  uint32_t address=n%7==0?random:n%7==1?(random&0x3ffffU)*4:n%7==2?0x17fff8+(random&15):0x6e000+(random&2047U)*4;
  switch(n%5){
   case 0:case 1:{uint32_t value=random^n;(*fast)[address]=value;slow[address]=value;writes++;break;}
   case 2:assert(fast->erase(address)==slow.erase(address));erases++;break;
   case 3:{assert(fast->count(address)==slow.count(address));auto a=fast->find(address);auto b=slow.find(address);
    assert((a==fast->end())==(b==slow.end()));if(b!=slow.end())assert(a->first==b->first && a->second==b->second);reads++;break;}
   case 4:if(slow.count(address))assert(fast->at(address)==slow.at(address));else{
    bool threw=false;try{(void)fast->at(address);}catch(const std::out_of_range&){threw=true;}assert(threw);missing++;
   }break;
  }
  assert(fast->size()==slow.size());
 }
 for(const auto& word:slow)assert(fast->count(word.first) && fast->at(word.first)==word.second);
 fast->clear();slow.clear();assert(fast->empty() && fast->find(0x6e000)==fast->end());
 (*fast)[0x6e000]=42;assert(fast->erase(0x6e000)==1 && !fast->count(0x6e000) && (*fast)[0x6e000]==0);
 printf("PASS exact sparse ROOT word map/1000000 operations/writes%u/finds%u/erases%u/missing-at exceptions%u/all final entries vs std::map/unaligned+high keys+boundary/default insert/erase/reinsert/clear;NO ROOT instruction or model execution\n",writes,reads,erases,missing);
}
