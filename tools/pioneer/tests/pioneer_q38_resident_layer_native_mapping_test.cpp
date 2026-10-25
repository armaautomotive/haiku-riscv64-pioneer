/* SPDX-License-Identifier: MIT */
// ACTUAL new native class-body adapter; simulated OS aliases ONLY. No
// actual new native factory/source/clock/model/context/launch is supplied.
#include <stdint.h>
#include <assert.h>
#include <array>
#include <memory>
#include <vector>
#include <stdio.h>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentLayerFootprint.h"
using uint32=uint32_t;using uint64=uint64_t;using area_id=int;
static constexpr int B_OK=0,B_ANY_KERNEL_ADDRESS=1,B_KERNEL_READ_AREA=2,B_KERNEL_WRITE_AREA=4,B_UNCACHED_MEMORY=8;
enum {TT_Q38_PANEL_VIEW_BASE=0x6e000,TT_Q38_PANEL_VIEW_BYTES=0x42000};
struct MapEndpoint {
 static constexpr uint64 Bar=UINT64_C(0x900000000);
 std::vector<uint32> prefix=std::vector<uint32>(40960/4),suffix=std::vector<uint32>(811008/4);
 unsigned maps=0,mmio=0;int fail=-1,nullView=-1;
};
static MapEndpoint* endpoint;
static area_id map_physical_memory(const char*,uint64 physical,unsigned bytes,int placement,unsigned flags,void** view){
 assert(endpoint && endpoint->maps<2 && placement==B_ANY_KERNEL_ADDRESS && flags==(B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA|B_UNCACHED_MEMORY));
 unsigned i=endpoint->maps++;assert(physical==MapEndpoint::Bar+(i?0x190b8000:0x19064000) && bytes==(i?811008U:40960U));
 if(int(i)==endpoint->fail)return -1;
 *view=int(i)==endpoint->nullView?nullptr:i?endpoint->suffix.data():endpoint->prefix.data();return int(i)+31;
}
static uint32 PioneerRead32(const volatile uint32* p){endpoint->mmio++;return *p;}
static void PioneerWrite32(volatile uint32* p,uint32 v){endpoint->mmio++;*p=v;}
struct MappingIO:MapEndpoint {
 struct {uint64 bar0_host_start=Bar;}profile;
 unsigned runtimeCore=0,payloadBytes=8192;bool fleetEnabled=true,runtimeMapped=true,panelStable=true,sourceScope=true;
 uint32 entry=0x1250006f,scratch[2048]={};uint32* runtimeEntry=&entry;uint32* scratchData=scratch;
 std::vector<uint32> panel=std::vector<uint32>(TT_Q38_PANEL_VIEW_BYTES/4);
 unsigned direct=0;bool ResidentLayerMappingSourceScope(){return sourceScope;}
 bool Q38PanelMappingStable(){return panelStable;}
 uint32 RuntimeCodeRead(unsigned i){assert(!i);direct++;return entry;}
 void RuntimeCodeWrite(unsigned i,uint32 v){assert(!i);direct++;entry=v;}
 uint32 Q38PanelRead(uint32 a){assert(a>=0x6e000 && a<0xac000);direct++;return panel[(a-0x6e000)/4];}
 void Q38PanelWrite(uint32 a,uint32 v){assert(a>=0x6e000 && a<0xac000);direct++;panel[(a-0x6e000)/4]=v;}
 uint32 ResidentL1Read(unsigned i){assert(i>=0x1fc0/4 && i<0x1fc0/4+8);direct++;return scratch[i];}
 void ResidentL1Write(unsigned i,uint32 v){assert(i>=0x1fc0/4 && i<0x1fc0/4+8);direct++;scratch[i]=v;}
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentLayerNativeMapping.inc"
};
static unsigned passes=0,refusals=0;
int main(){
 setvbuf(stdout,nullptr,_IOLBF,0);
 {
  auto io=std::make_unique<MappingIO>();endpoint=io.get();uint32 output=0x12345678;
  // Old mapping presence is never used to obtain the NEW logical grant.
  assert(!io->ResidentLayerMemoryReadChecked(0,0x17dffc,output) && output==0x12345678 && !io->maps && !io->mmio && !io->direct);
  assert(io->MapResidentLayerMemory() && io->maps==2 && !io->mmio && !io->direct);
  assert(io->MapResidentLayerMemory() && io->maps==2);
  for(unsigned w=0;w<120;w++){
   io->runtimeCore=w;assert(io->ResidentLayerMemoryMappingStable(w));
   for(unsigned i=0;i<PioneerQ38ResidentLayerSnapshotWords(w);i++){
    auto a=PioneerQ38ResidentLayerSnapshotAddress(w,i);uint32 value=i&1?UINT32_MAX:0x9e3779b9U*(i+1);
    assert(io->ResidentLayerMemoryWrite(w,a,value) && io->ResidentLayerMemoryReadChecked(w,a,output) && output==value);passes++;
   }
   for(uint32 a:{0U,0x6001U,0x63ffcU,0xac000U,0xb0000U,0xb7ffcU,0x17e000U,0x17effcU,0x17f000U,0x17ffbcU,0x17ffe0U,UINT32_MAX}){
    output=0x12345678;unsigned n=io->mmio+io->direct;
    assert(!io->ResidentLayerMemoryReadChecked(w,a,output) && output==0x12345678 && !io->ResidentLayerMemoryWrite(w,a,0)
     && io->mmio+io->direct==n);refusals++;
   }
   if(w)for(uint32 a:{0xc8000U,0x14b600U,0x14c800U,0x17dffcU}){
    unsigned n=io->mmio+io->direct;assert(!io->ResidentLayerMemoryWrite(w,a,0) && !io->ResidentLayerMemoryReadChecked(w,a,output) && io->mmio+io->direct==n);refusals++;
   }
  }
  io->runtimeCore=119;for(unsigned kind=0;kind<8;kind++){
   auto saved=io->profile.bar0_host_start;auto* p=io->runtimeEntry;auto* s=io->scratchData;
   if(!kind)io->runtimeCore=120;if(kind==1)io->fleetEnabled=false;if(kind==2)io->runtimeMapped=false;if(kind==3)io->panelStable=false;
   if(kind==4)io->profile.bar0_host_start++;if(kind==5)io->runtimeEntry=nullptr;if(kind==6)io->scratchData=nullptr;if(kind==7)io->payloadBytes--;
   unsigned n=io->mmio+io->direct;output=0x12345678;assert(!io->ResidentLayerMemoryReadChecked(119,0xb8000,output)
    && !io->ResidentLayerMemoryWrite(119,0xb8000,0) && output==0x12345678 && io->mmio+io->direct==n);refusals++;
   io->runtimeCore=119;io->fleetEnabled=io->runtimeMapped=io->panelStable=true;io->profile.bar0_host_start=saved;io->runtimeEntry=p;io->scratchData=s;io->payloadBytes=8192;
  }
 }
 for(unsigned kind=0;kind<12;kind++){
  auto io=std::make_unique<MappingIO>();endpoint=io.get();
  if(!kind)io->sourceScope=false;if(kind==1)io->runtimeCore=1;if(kind==2)io->fleetEnabled=false;if(kind==3)io->runtimeMapped=false;
  if(kind==4)io->panelStable=false;if(kind==5)io->runtimeEntry=nullptr;if(kind==6)io->scratchData=nullptr;if(kind==7)io->payloadBytes--;
  if(kind==8)io->profile.bar0_host_start=UINT64_MAX-0x1917dfff;if(kind==9)io->q38ResidentLayerMemoryArea[0]=1;
  if(kind==10)io->q38ResidentLayerMemoryView[1]=io->suffix.data();if(kind==11)io->profile.bar0_host_start=UINT64_MAX;
  assert(!io->MapResidentLayerMemory() && !io->maps && !io->mmio && !io->direct && !io->q38ResidentLayerMemoryMapAttempted);refusals++;
 }
 for(unsigned kind=0;kind<4;kind++){
  auto io=std::make_unique<MappingIO>();endpoint=io.get();if(kind<2)io->fail=int(kind);else io->nullView=int(kind-2);
  assert(!io->MapResidentLayerMemory() && io->q38ResidentLayerMemoryMapAttempted);unsigned maps=io->maps;
  assert(!io->MapResidentLayerMemory() && io->maps==maps && !io->mmio && !io->direct);uint32 output=123;
  assert(!io->ResidentLayerMemoryReadChecked(0,0xb8000,output) && output==123 && !io->ResidentLayerMemoryWrite(0,0xb8000,0)
   && !io->mmio && !io->direct);refusals++;
 }
 printf("PASS ACTUAL distinct native mapping fragment/full logical words%u/inertRefusals%u/exact2physical aliases/ROOT17e000 exact suffix end/SAME6 peer ROMs/peer no ROOT grant/UINT32_MAX data/partial maps retained NO retry/MMIO;TEST OS aliases only,NO native factory/card/launch/speed grant\n",passes,refusals);
}
