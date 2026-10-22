/* SPDX-License-Identifier: MIT */
// ACTUAL native mapping/read/write methods. Kernel primitives simulated.
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38EmbeddingFootprint.h"
#include <array>
#include <cassert>
#include <cstdio>
using uint32=uint32_t;using uint64=uint64_t;using area_id=int;
static constexpr int B_OK=0,B_ANY_KERNEL_ADDRESS=1,B_KERNEL_READ_AREA=2,B_KERNEL_WRITE_AREA=4,B_UNCACHED_MEMORY=8;
static constexpr uint64 Bar=UINT64_C(0x900000000);
enum{TT_Q38_PANEL_VIEW_BASE=0x6e000,TT_Q38_PANEL_VIEW_BYTES=0x42000};
static std::array<uint32,10240> prefix;
static unsigned maps=0,reads=0,writes=0;static bool failMap=false,nullMap=false;
static uint32 PioneerRead32(const volatile uint32* p){reads++;return *p;}
static void PioneerWrite32(volatile uint32* p,uint32 value){writes++;*p=value;}
static area_id map_physical_memory(const char*,uint64 physical,unsigned bytes,int placement,unsigned flags,void** view){
 assert(physical==Bar+0x19064000 && bytes==40960 && placement==B_ANY_KERNEL_ADDRESS && flags==(B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA|B_UNCACHED_MEMORY));
 maps++;if(failMap)return -1;*view=nullMap?nullptr:prefix.data();return 23;
}
struct NativeEmbeddingMap {
 struct{uint64 bar0_host_start=Bar;}profile;
 unsigned runtimeCore=0;bool fleetEnabled=false,runtimeMapped=true,panelStable=true,sourceScope=true;
 uint32 boot=0;volatile uint32* runtimeEntry=&boot;std::array<uint32,2048> scratch={};volatile uint32* scratchData=scratch.data();unsigned payloadBytes=8192;
 std::array<uint32,TT_Q38_PANEL_VIEW_BYTES/4> panel={};
 area_id q38EmbeddingMemoryArea=-1;volatile uint32* q38EmbeddingMemoryView=nullptr;
 bool q38EmbeddingMemoryMapAttempted=false;uint64 q38EmbeddingMemoryBar=0;
 bool EmbeddingSourceScope(bool controller){assert(!controller);return sourceScope;}
 bool Q38PanelMappingStable(){return panelStable;}
 uint32 RuntimeCodeRead(unsigned i){assert(!i);return PioneerRead32(runtimeEntry);}
 void RuntimeCodeWrite(unsigned i,uint32 v){assert(!i);PioneerWrite32(runtimeEntry,v);}
 uint32 Q38PanelRead(uint32 a){assert(!(a&3) && a>=0x6e000 && a<0xb0000);return PioneerRead32(panel.data()+(a-0x6e000)/4);}
 void Q38PanelWrite(uint32 a,uint32 v){assert(!(a&3) && a>=0x6e000 && a<0xb0000);PioneerWrite32(panel.data()+(a-0x6e000)/4,v);}
 uint32 ResidentL1Read(unsigned i){assert(i<2048);return PioneerRead32(scratchData+i);}
 void ResidentL1Write(unsigned i,uint32 v){assert(i<2048);PioneerWrite32(scratchData+i,v);}
#include "embedding_native_mapping.inc"
};
int main(){
 NativeEmbeddingMap io;unsigned refused=0;
 auto deny=[&](NativeEmbeddingMap& h,uint32 a){unsigned count=reads+writes;uint32 out=0x12345678;
  assert(!h.EmbeddingReadChecked(a,out) && out==0x12345678 && !h.EmbeddingWrite(a,0) && reads+writes==count);refused++;};
 deny(io,0x6000);assert(!maps && io.MapEmbeddingMemory() && maps==1 && io.MapEmbeddingMemory() && maps==1);
 for(unsigned i=0;i<73737;i++){
  auto a=PioneerQ38EmbeddingSnapshotAddress(i);uint32 value=i%17?0x9e3779b9U*(i+1)^a:UINT32_MAX,out=0;
  assert(io.EmbeddingWrite(a,value) && io.EmbeddingReadChecked(a,out) && value==out);
 }
 assert(reads==73737 && writes==73737);
 for(uint32 a:{0U,0x5ffcU,0x6004U,0x63ffcU,0xac000U,0xafffcU,0xb0000U,0xc0000U,0x17ffbcU,0x17ffe0U,0xffb20000U,UINT32_MAX})deny(io,a);
 for(uint32 a:{0x6000U,0x64000U,0x6e000U,0x74000U,0xabffcU,0x17ffdcU})for(unsigned byte=1;byte<4;byte++)deny(io,a+byte);
 for(unsigned bad=0;bad<11;bad++){
  NativeEmbeddingMap h;
  if(bad==0)h.sourceScope=false;if(bad==1)h.runtimeCore=1;if(bad==2)h.fleetEnabled=true;if(bad==3)h.runtimeMapped=false;
  if(bad==4)h.panelStable=false;if(bad==5)h.runtimeEntry=nullptr;if(bad==6)h.scratchData=nullptr;if(bad==7)h.payloadBytes=4096;
  if(bad==8)h.profile.bar0_host_start=UINT64_MAX;if(bad==9)h.q38EmbeddingMemoryArea=1;if(bad==10)h.q38EmbeddingMemoryView=prefix.data();
  unsigned before=maps;assert(!h.MapEmbeddingMemory() && maps==before);deny(h,0x6000);
 }
 for(bool null:{false,true}){
  NativeEmbeddingMap h;failMap=!null;nullMap=null;unsigned before=maps;assert(!h.MapEmbeddingMemory() && h.q38EmbeddingMemoryMapAttempted && maps==before+1);
  failMap=nullMap=false;before=maps;assert(!h.MapEmbeddingMemory() && maps==before);deny(h,0x6000);
 }
 io.profile.bar0_host_start+=4096;deny(io,0x6000);
 printf("PASS ACTUAL embedding native mapping ALL73737words/refusals%u;single40KiB prefix/whole root stacks+scratch+trailers,UINT32_MAX DATA/unchanged failed output/holes+MMIO+QPR manifests excluded/partial null map sticky/changed BAR;SIMULATED_KERNEL_NOT_SCOPE_FACTORY_OR_CARD_EXECUTION\n",refused);
}
