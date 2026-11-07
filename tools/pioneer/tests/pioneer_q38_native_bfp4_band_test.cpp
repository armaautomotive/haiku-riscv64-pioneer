/* SPDX-License-Identifier: MIT */
// Instruction-producer contracts only; no model, ISA simulator or device.
#include "../PioneerQ38NativeBfp4Band.h"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include <vector>
struct Event {unsigned kind,index,value;};
struct Trace {
 std::vector<Event> events;
 void Op(unsigned word){events.push_back({0,0,word});}
 void Config(unsigned index,unsigned value){events.push_back({1,index,value});}
 void Drain(){events.push_back({2,0,0});}
};
int main(){
 using B=PioneerQ38NativeBfp4Band;unsigned valid=0,rejected=0;
 for(unsigned k:{5120U,6144U,17408U}){
  Trace t;assert(B::Run(t,k));const unsigned tiles=k/32;
  assert(B::Tiles(k)==tiles && B::WeightEnd(k)==B::Weight-16+tiles*592);
  assert(B::InputEnd(k)==B::Input-16+tiles*80);
  unsigned weightConfigs=0,inputConfigs=0,matrices=0,releases=0,unpack0=0,unpack1=0,clears=0;
  for(const auto& e:t.events){
   if(e.kind==1 && e.index==76){
    assert(e.value==(B::Weight/16-1)+(weightConfigs?((weightConfigs-1)*37):0));weightConfigs++;
   }
   if(e.kind==1 && e.index==124){
    assert(e.value==(inputConfigs?B::Input/16-1+(inputConfigs-1)*5:PioneerQ38NativeBfpMatvec::InputAddress/16-1));inputConfigs++;
   }
   if(e.kind==0 && (e.value>>24)==0x26){
    const unsigned face=(matrices/2)%4,term=matrices%2;
    const unsigned expected=0x26084000|(face<2?32:40)|((face==3 && term==1)?0x00c00000:0);
    assert(e.value==expected);matrices++;if(e.value&0x00c00000)releases++;
   }
   if(e.kind==0 && e.value==0x420000c1)unpack0++;
   if(e.kind==0 && e.value==0x428000c1)unpack1++;
   if(e.kind==0 && e.value==0x10180000)clears++;
  }
  assert(weightConfigs==tiles+1 && inputConfigs==tiles+1 && matrices==tiles*8 && releases==tiles);
  assert(unpack0==tiles && unpack1==tiles && clears==1);
  assert(std::count_if(t.events.begin(),t.events.end(),[](const Event& e){return e.kind==2;})==2*tiles+4);
  ++valid;
 }
 for(unsigned k:{0U,32U,5119U,5121U,17409U,~0U}){
  Trace t;assert(!B::Run(t,k) && t.events.empty());++rejected;
 }
 std::printf("PASS BFP4 full-K producer: %u widths, %u invalid widths; two fidelity terms and bounded tile/header addresses; NO device, numerical, model-quality or speed qualification\n",valid,rejected);
}
