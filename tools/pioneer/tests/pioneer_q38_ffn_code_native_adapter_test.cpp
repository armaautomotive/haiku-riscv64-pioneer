/* SPDX-License-Identifier: MIT */
// ACTUAL extracted native mapping methods with simulated kernel primitives.
// NO physical mapping,card access or native scope/launch authority.
#include <array>
#include <cassert>
#include <cstdio>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38FfnGraphImages.h"
using uint32=uint32_t;using area_id=int;
static constexpr int B_OK=0,B_ANY_KERNEL_ADDRESS=1,B_KERNEL_READ_AREA=2,B_KERNEL_WRITE_AREA=4,B_UNCACHED_MEMORY=8;
static constexpr uint64_t Bar=UINT64_C(0x900000000);
static std::array<uint32,10240> prefix;
static unsigned maps=0,reads=0,writes=0;static bool badMap=false,nullMap=false;
static uint32 PioneerRead32(const volatile uint32* p){reads++;return *p;}
static void PioneerWrite32(volatile uint32* p,uint32 v){writes++;*p=v;}
static area_id map_physical_memory(const char*,uint64_t physical,unsigned bytes,int placement,unsigned flags,void** view){
 assert(physical==Bar+0x19064000 && bytes==40960 && placement==B_ANY_KERNEL_ADDRESS
  && flags==(B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA|B_UNCACHED_MEMORY));maps++;
 if(badMap)return -1;*view=nullMap?nullptr:prefix.data();return 23;
}
struct NativeAdapter {
 struct {uint64_t bar0_host_start=Bar;} profile;
 unsigned runtimeCore=0;bool fleetEnabled=true,runtimeMapped=true,panelStable=true;
 area_id q38FfnCodeArea=-1;volatile uint32* q38FfnCodeView=nullptr;bool q38FfnCodeMapAttempted=false;
 uint32 boot=0;std::array<uint32,4096> panel={};
 bool Q38PanelMappingStable(){return panelStable;}
 uint32 RuntimeCodeRead(unsigned i){assert(!i);return PioneerRead32(&boot);}
 void RuntimeCodeWrite(unsigned i,uint32 v){assert(!i);PioneerWrite32(&boot,v);}
 uint32 Q38PanelRead(uint32 a){assert(!(a&3) && a>=0x70000 && a<0x74000);return PioneerRead32(panel.data()+(a-0x70000)/4);}
 void Q38PanelWrite(uint32 a,uint32 v){assert(!(a&3) && a>=0x70000 && a<0x74000);PioneerWrite32(panel.data()+(a-0x70000)/4,v);}
#include PIONEER_FFN_NATIVE_ADAPTER_INCLUDE
};
int main(){unsigned refused=0,words=0;
 {NativeAdapter h;for(unsigned w:{0U,119U,120U,UINT32_MAX}){
  assert(h.FfnGraphProgramRead(w,0x64000)==UINT32_MAX && !h.FfnGraphProgramWrite(w,0x64000,1));refused++;}
  assert(!maps && !reads && !writes);
 }
 NativeAdapter h;assert(h.MapFfnGraphCodePrefix() && maps==1 && !reads && !writes);
 assert(h.MapFfnGraphCodePrefix() && maps==1 && h.FfnGraphCodeMappingStable(0));
 for(unsigned w=0;w<120;w++){
  h.runtimeCore=w;assert(h.FfnGraphCodeMappingStable(w));
  for(uint32 a:{0x6000U,0x64000U,0x68000U,0x70000U,0x72000U}){
   unsigned before=reads+writes;
   assert(h.FfnGraphProgramRead(120,a)==UINT32_MAX && !h.FfnGraphProgramWrite(120,a,0));
   assert(h.FfnGraphProgramRead((w+1)%120,a)==UINT32_MAX && !h.FfnGraphProgramWrite((w+1)%120,a,0));
   assert(reads+writes==before);refused+=2;
  }
  for(uint32 a=0x6000;a<0x74004;a+=4){
   bool valid=a==0x6000 || (a>=0x64000 && a<(w?0x67000U:0x6e000U)) || (a>=0x70000 && a<0x74000);
   unsigned before=reads+writes;
   if(!valid){assert(h.FfnGraphProgramRead(w,a)==UINT32_MAX && !h.FfnGraphProgramWrite(w,a,0));assert(reads+writes==before);refused++;continue;}
   uint32 v=0x12345678U^a^(w*0x9e3779b9U);
   assert(h.FfnGraphProgramWrite(w,a,v) && h.FfnGraphProgramRead(w,a)==v && reads+writes==before+2);words++;
   for(unsigned byte=1;byte<4;byte++){before=reads+writes;
    assert(h.FfnGraphProgramRead(w,a+byte)==UINT32_MAX && !h.FfnGraphProgramWrite(w,a+byte,v));assert(reads+writes==before);refused++;}
  }
  for(uint32 a:{0U,0x5ffcU,0x6004U,0x63ffcU,0x74000U,0xffb20000U,UINT32_MAX}){
   unsigned before=reads+writes;assert(h.FfnGraphProgramRead(w,a)==UINT32_MAX && !h.FfnGraphProgramWrite(w,a,0));assert(reads+writes==before);refused++;}
 }
 assert(words==867448 && reads==words && writes==words && maps==1);
 {NativeAdapter x;unsigned before=maps;nullMap=true;assert(!x.MapFfnGraphCodePrefix() && maps==before+1
  && x.q38FfnCodeArea==23 && x.q38FfnCodeMapAttempted && !x.q38FfnCodeView);nullMap=false;
  assert(!x.MapFfnGraphCodePrefix() && maps==before+1);refused++;
 }
 for(unsigned bad=0;bad<7;bad++){
  NativeAdapter x;unsigned before=maps;
  if(bad==0)x.runtimeCore=1;if(bad==1)x.fleetEnabled=false;if(bad==2)x.runtimeMapped=false;
  if(bad==3)x.panelStable=false;if(bad==4)x.q38FfnCodeMapAttempted=true;
  if(bad==5)x.q38FfnCodeView=prefix.data();if(bad==6)x.q38FfnCodeArea=23;
  if(bad==6){badMap=true;assert(!x.MapFfnGraphCodePrefix() && maps==before+1);badMap=false;before=maps;}
  assert(!x.MapFfnGraphCodePrefix() && maps==before);refused++;
 }
 for(unsigned bad=0;bad<6;bad++){
  NativeAdapter x;x.q38FfnCodeArea=23;x.q38FfnCodeView=prefix.data();
  if(bad==0)x.runtimeCore=1;if(bad==1)x.fleetEnabled=false;if(bad==2)x.runtimeMapped=false;
  if(bad==3)x.panelStable=false;if(bad==4)x.q38FfnCodeArea=-1;if(bad==5)x.q38FfnCodeView=nullptr;
  unsigned before=reads+writes;assert(!x.FfnGraphCodeMappingStable(0) && x.FfnGraphProgramRead(0,0x64000)==UINT32_MAX
   && !x.FfnGraphProgramWrite(0,0x64000,1) && reads+writes==before);refused++;
 }
 printf("PASS ACTUAL native FFN code mapping methods:words%u/refusals%u/ALL120 exact role footprints/map40KiB once/no stack/peer-gap/unaligned/wrong-owner IO/no retry after failed map;SIMULATED_KERNEL_NOT_NATIVE_SCOPE_OR_LAUNCH\n",words,refused);
}
