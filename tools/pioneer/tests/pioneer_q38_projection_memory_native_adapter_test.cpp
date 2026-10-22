/* SPDX-License-Identifier: MIT */
// ACTUAL extracted kernel mapping/read/write methods;simulated primitives.
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionGraphFootprint.h"
#include <array>
#include <cassert>
#include <cstdio>
using uint32=uint32_t;using uint64=uint64_t;using area_id=int;
static constexpr int B_OK=0,B_ANY_KERNEL_ADDRESS=1,B_KERNEL_READ_AREA=2,B_KERNEL_WRITE_AREA=4,B_UNCACHED_MEMORY=8;
static constexpr uint64 Bar=UINT64_C(0x900000000);
enum {TT_Q38_PANEL_VIEW_BASE=0x6e000,TT_Q38_PANEL_VIEW_BYTES=0x42000};
static std::array<uint32,10240> prefix;
static std::array<uint32,9216> primary,shadow;
static unsigned maps=0,reads=0,writes=0;static int failMap=-1,nullMap=-1;
static uint32 PioneerRead32(const volatile uint32* p){reads++;return *p;}
static void PioneerWrite32(volatile uint32* p,uint32 value){writes++;*p=value;}
static area_id map_physical_memory(const char*,uint64 physical,unsigned bytes,int placement,unsigned flags,void** view){
 const uint32 offset[]={0x19064000,0x190b0000,0x190c0000},size[]={40960,36864,36864};
 unsigned i=0;while(i<3 && physical!=Bar+offset[i])i++;
 assert(i<3 && bytes==size[i] && placement==B_ANY_KERNEL_ADDRESS && flags==(B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA|B_UNCACHED_MEMORY));maps++;
 if(int(i)==failMap)return -1;*view=int(i)==nullMap?nullptr:i==0?prefix.data():i==1?primary.data():shadow.data();return 23+i;
}
struct NativeProjectionMemoryAdapter {
 struct {uint64 bar0_host_start=Bar;} profile;
 unsigned runtimeCore=0;bool fleetEnabled=true,runtimeMapped=true,panelStable=true;
 uint32 boot=0;volatile uint32* runtimeEntry=&boot;std::array<uint32,2048> scratch={};volatile uint32* scratchData=scratch.data();unsigned payloadBytes=8192;
 std::array<uint32,TT_Q38_PANEL_VIEW_BYTES/4> panel={};
 area_id q38ProjectionMemoryArea[3]={-1,-1,-1};volatile uint32* q38ProjectionMemoryView[3]={nullptr,nullptr,nullptr};
 bool q38ProjectionMemoryMapAttempted=false;uint64 q38ProjectionMemoryBar=0;
 bool Q38PanelMappingStable(){return panelStable;}
 uint32 RuntimeCodeRead(unsigned i){assert(!i);return PioneerRead32(runtimeEntry);}
 void RuntimeCodeWrite(unsigned i,uint32 value){assert(!i);PioneerWrite32(runtimeEntry,value);}
 uint32 Q38PanelRead(uint32 a){assert(!(a&3) && a>=0x6e000 && a<0xb0000);return PioneerRead32(panel.data()+(a-0x6e000)/4);}
 void Q38PanelWrite(uint32 a,uint32 value){assert(!(a&3) && a>=0x6e000 && a<0xb0000);PioneerWrite32(panel.data()+(a-0x6e000)/4,value);}
 uint32 ResidentL1Read(unsigned i){assert(i<2048);return PioneerRead32(scratchData+i);}
 void ResidentL1Write(unsigned i,uint32 value){assert(i<2048);PioneerWrite32(scratchData+i,value);}
#include PIONEER_PROJECTION_MEMORY_ADAPTER_INCLUDE
 uint32& ExpectedSlot(uint32 a){
  if(a==0x6000)return boot;
  if(a>=0x64000 && a<0x6e000)return prefix[(a-0x64000)/4];
  if(a>=0x6e000 && a<0xac000)return panel[(a-0x6e000)/4];
  if(a>=0x17ffc0 && a<0x17ffe0)return scratch[0x1fc0/4+(a-0x17ffc0)/4];
  if(a>=0xb0000 && a<0xb8300)return primary[(a-0xb0000)/4];
  assert(a>=0xc0000 && a<0xc8300);return shadow[(a-0xc0000)/4];
 }
};
int main(){
 unsigned words=0,refused=0;NativeProjectionMemoryAdapter h;
 auto deny=[&](NativeProjectionMemoryAdapter& io,unsigned w,uint32 a){
  unsigned before=reads+writes;uint32 out=0x12345678;
  assert(!io.ProjectionGraphMemoryReadChecked(w,a,out) && out==0x12345678 && !io.ProjectionGraphMemoryWrite(w,a,0) && before==reads+writes);refused++;
 };
 deny(h,0,0x6000);assert(!maps && h.MapProjectionGraphMemory() && maps==3 && h.MapProjectionGraphMemory() && maps==3);
 for(unsigned w=0;w<120;w++){
  h.runtimeCore=w;assert(h.ProjectionGraphMemoryMappingStable(w));
  for(unsigned i=0;i<PioneerQ38ProjectionGraphSnapshotWords(w);i++){
   uint32 a=PioneerQ38ProjectionGraphSnapshotAddress(w,i),value=i%17?0x9e3779b9U*(i+1)^a^(w*0x1234567U):UINT32_MAX,out=0;
   assert(h.ProjectionGraphMemoryWrite(w,a,value) && h.ExpectedSlot(a)==value && h.ProjectionGraphMemoryReadChecked(w,a,out) && out==value);words++;
  }
  for(uint32 a:{0x6000U,0x67000U,0x6e000U,0x74000U,0xabffcU,0xb0000U,0xc0000U,0x17ffdcU}){
   deny(h,120,a);deny(h,(w+1)%120,a);for(unsigned byte=1;byte<4;byte++)deny(h,w,a+byte);
  }
  for(uint32 a:{0U,0x5ffcU,0x6004U,0x63ffcU,0xac000U,0xafffcU,0xb8300U,0xb8ffcU,0xc8300U,0xc8ffcU,0x17ffbcU,0x17ffe0U,0xffb20000U,UINT32_MAX})deny(h,w,a);
  if(w){deny(h,w,0x68000);deny(h,w,0xb0000);deny(h,w,0xc0000);}
 }
 assert(words==8134072 && reads==words && writes==words && maps==3);
 for(int bad=0;bad<3;bad++)for(bool null:{false,true}){
  NativeProjectionMemoryAdapter io;unsigned before=maps;if(null)nullMap=bad;else failMap=bad;
  assert(!io.MapProjectionGraphMemory() && maps==before+unsigned(bad)+1 && io.q38ProjectionMemoryMapAttempted);
  nullMap=failMap=-1;before=maps;assert(!io.MapProjectionGraphMemory() && maps==before);deny(io,0,0x6000);
 }
 for(unsigned bad=0;bad<10;bad++){
  NativeProjectionMemoryAdapter io;
  if(bad==0)io.runtimeCore=1;if(bad==1)io.fleetEnabled=false;if(bad==2)io.runtimeMapped=false;if(bad==3)io.panelStable=false;
  if(bad==4)io.runtimeEntry=nullptr;if(bad==5)io.scratchData=nullptr;if(bad==6)io.payloadBytes=4096;
  if(bad==7)io.profile.bar0_host_start=UINT64_MAX;if(bad==8)io.q38ProjectionMemoryArea[0]=1;if(bad==9)io.q38ProjectionMemoryView[1]=primary.data();
  unsigned before=maps;assert(!io.MapProjectionGraphMemory() && maps==before);deny(io,0,0x6000);
 }
 h.runtimeCore=0;h.profile.bar0_host_start+=4096;assert(!h.ProjectionGraphMemoryMappingStable(0));deny(h,0,0x6000);
 printf("PASS ACTUAL native projection memory adapter ALL120/words%u/refusals%u;exact three fixed maps,full16KiB peer+ALL stacks/trailers/runtime/ROOT primary+shadow,UINT32_MAX valid/failed output UNCHANGED,wrong-role/hole/MMIO/page padding excluded,partial/null-map sticky no retry/changed BAR refused;SIMULATED_KERNEL_NOT_NATIVE_SCOPE_OR_EXECUTION\n",words,refused);
}
