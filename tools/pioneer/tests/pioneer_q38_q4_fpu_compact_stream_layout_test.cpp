/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38Q4FpuCompactStreamLayout.h"
#include <cassert>
#include <cstdio>
#include <set>
using L=PioneerQ38Q4FpuCompactStreamLayout;
int main(){
 unsigned cases=0,refused=0;
 for(unsigned width:{5120U,17408U})for(unsigned block:{0U,width/256-1})for(unsigned buffer:{0U,1U}){
  std::set<unsigned> source,destination,headers;unsigned flits=0;
  for(unsigned p=0;p<26;p++){
   L::Read r={};assert(L::Packet(width,block,buffer,p,r));
   assert(!(r.source&63) && !(r.destination&63) && !(r.bytes&63) && (r.bytes==64 || r.bytes==256));flits+=r.bytes/64;
   for(unsigned i=0;i<r.bytes;i++){assert(source.insert(r.source+i).second);assert(destination.insert(r.destination+i).second);}
   for(unsigned i=0;i<64;i++)assert(headers.insert(r.destination-64+i).second);
  }
  assert(flits==80 && source.size()==5120 && destination.size()==5120 && headers.size()==26*64);
  for(unsigned a:headers)assert(!destination.count(a));
  for(unsigned i=0;i<4608;i++)assert(source.count(0x04000000+block*4608+i));
  for(unsigned i=0;i<512;i++)assert(source.count(0x04100000+block*512+i));
  cases++;
 }
 for(unsigned field=0;field<4;field++)for(unsigned value:{0U,1U,2U,26U,68U,5121U,~0U}){
  const unsigned k=field==0?value:5120,b=field==1?value:0,slot=field==2?value:0,p=field==3?value:0;
  if(L::Width(k) && b<k/256 && slot<2 && p<26)continue;
  L::Read r={0x81234567,0x92345678,0xa3456789};assert(!L::Packet(k,b,slot,p,r));
  assert(r.source==0x81234567 && r.destination==0x92345678 && r.bytes==0xa3456789);refused++;
 }
 printf("PASS_COMPACT_STREAM_LAYOUT cases%u completeOriginal4608_plusInput512_26uniqueRequests80flits_noPayloadHeaderAlias_atomicRefusals%u DATA_ONLY_NOT_CAPACITY_OR_OWNERSHIP\n",cases,refused);
}
