/* SPDX-License-Identifier: MIT */
// ACTUAL native class-body adapter,TEST OS aliases only. Not a source,
// native context or execution qualification; genuine kernel scope is separate.
#define PIONEER_Q38_MIXED_LIVE_NATIVE_CONTEXT 1
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38MixedLiveFootprint.h"
#include <cassert>
#include <cstdio>
#include <memory>
#include <vector>
using uint32=uint32_t;using uint64=uint64_t;using area_id=int;
class PioneerQ38MixedLiveMemory;class PioneerQ38CanonicalModelPlan;struct PioneerQ38GdnIdentity;
static constexpr int B_OK=0,B_ANY_KERNEL_ADDRESS=1,B_KERNEL_READ_AREA=2,B_KERNEL_WRITE_AREA=4,B_UNCACHED_MEMORY=8;
enum {TT_Q38_PANEL_VIEW_BASE=0x6e000,TT_Q38_PANEL_VIEW_BYTES=0x42000};
static const unsigned offsets[]={0x19020000,0x19040000,0x19060000,0x190ac000},sizes[]={8192,131072,57344,868352};
struct Endpoint {
 static constexpr uint64 Bar=UINT64_C(0x900000000);
 std::vector<uint32> region[4];unsigned maps=0,mmio=0,direct=0;int fail=-1,nullView=-1;
 Endpoint(){for(unsigned i=0;i<4;i++)region[i].resize(sizes[i]/4);}
};
static Endpoint* endpoint;
static area_id map_physical_memory(const char*,uint64 physical,unsigned bytes,int placement,unsigned flags,void** view){
 assert(endpoint && endpoint->maps<4 && placement==B_ANY_KERNEL_ADDRESS && flags==(B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA|B_UNCACHED_MEMORY));
 unsigned i=endpoint->maps++;assert(physical==Endpoint::Bar+offsets[i] && bytes==sizes[i]);
 if(int(i)==endpoint->fail)return -1;
 *view=int(i)==endpoint->nullView?nullptr:endpoint->region[i].data();return 31+int(i);
}
static uint32 PioneerRead32(const volatile uint32* p){endpoint->mmio++;return *p;}
static void PioneerWrite32(volatile uint32* p,uint32 v){endpoint->mmio++;*p=v;}
struct MappingIO:Endpoint {
 struct {uint64 bar0_host_start=Bar;}profile;
 unsigned runtimeCore=0,payloadBytes=8192;bool fleetEnabled=true,runtimeMapped=true,panelStable=true,sourceScope=true;
 uint32 entry=0,scratch[2048]={};uint32* runtimeEntry=&entry;uint32* scratchData=scratch;
 std::vector<uint32> panel=std::vector<uint32>(TT_Q38_PANEL_VIEW_BYTES/4);
 bool Q38PanelMappingStable(){return panelStable;}
 uint32 RuntimeCodeRead(unsigned i){assert(!i);direct++;return entry;}
 void RuntimeCodeWrite(unsigned i,uint32 v){assert(!i);direct++;entry=v;}
 uint32 Q38PanelRead(uint32 a){assert(a>=0x6e000 && a<0xac000);direct++;return panel[(a-0x6e000)/4];}
 void Q38PanelWrite(uint32 a,uint32 v){assert(a>=0x6e000 && a<0xac000);direct++;panel[(a-0x6e000)/4]=v;}
 uint32 ResidentL1Read(unsigned i){assert(i>=0x1fc0/4 && i<0x1fc0/4+8);direct++;return scratch[i];}
 void ResidentL1Write(unsigned i,uint32 v){assert(i>=0x1fc0/4 && i<0x1fc0/4+8);direct++;scratch[i]=v;}
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38MixedLiveNativeMapping.inc"
};
bool MappingIO::MixedLiveMappingSourceScope(){return sourceScope;} // TEST callback only;not a native permission
int main(){
 unsigned words=0,refused=0;
 {
  auto io=std::make_unique<MappingIO>();endpoint=io.get();uint32 out=123;
  assert(!io->MixedLiveMemoryReadChecked(0,0x40000,out) && out==123 && !io->maps && !io->mmio && !io->direct);
  assert(io->MapMixedLiveMemory() && io->maps==4 && !io->mmio && !io->direct);
  assert(!io->MapMixedLiveMemory() && io->maps==4); // one attempt,not an idempotent map/retry
  for(unsigned w=0;w<120;w++){
   io->runtimeCore=w;
   for(unsigned i=0;i<PioneerQ38MixedLiveSnapshotWords(w);i++){
    unsigned a=PioneerQ38MixedLiveSnapshotAddress(w,i);uint32 value=i&1?UINT32_MAX:0x9e3779b9U*(i+1);
    assert(io->MixedLiveMemoryWrite(w,a,value) && io->MixedLiveMemoryReadChecked(w,a,out) && out==value);words++;
   }
   for(unsigned a:{0U,0x6001U,0x1fffcU,0x22000U,0x60000U,0xb0000U,0x17f800U,0x17ffe0U,UINT32_MAX}){
    if(PioneerQ38MixedLiveSnapshotContains(w,a))continue;
    unsigned n=io->mmio+io->direct;out=123;
    assert(!io->MixedLiveMemoryReadChecked(w,a,out) && out==123 && !io->MixedLiveMemoryWrite(w,a,0) && io->mmio+io->direct==n);refused++;
   }
  }
  for(unsigned kind=0;kind<8;kind++){
   io->runtimeCore=119;auto bar=io->profile.bar0_host_start;auto* entry=io->runtimeEntry;auto* scratch=io->scratchData;
   if(kind==0)io->runtimeCore=118;if(kind==1)io->fleetEnabled=false;if(kind==2)io->runtimeMapped=false;if(kind==3)io->panelStable=false;
   if(kind==4)io->profile.bar0_host_start++;if(kind==5)io->runtimeEntry=nullptr;if(kind==6)io->scratchData=nullptr;if(kind==7)io->payloadBytes--;
   unsigned n=io->mmio+io->direct;out=123;
   assert(!io->MixedLiveMemoryReadChecked(119,0x20000,out) && out==123 && !io->MixedLiveMemoryWrite(119,0x20000,0) && io->mmio+io->direct==n);refused++;
   io->runtimeCore=119;io->fleetEnabled=io->runtimeMapped=io->panelStable=true;io->profile.bar0_host_start=bar;io->runtimeEntry=entry;io->scratchData=scratch;io->payloadBytes=8192;
  }
 }
 for(unsigned kind=0;kind<12;kind++){
  auto io=std::make_unique<MappingIO>();endpoint=io.get();
  if(kind==0)io->sourceScope=false;if(kind==1)io->runtimeCore=1;if(kind==2)io->fleetEnabled=false;if(kind==3)io->runtimeMapped=false;
  if(kind==4)io->panelStable=false;if(kind==5)io->runtimeEntry=nullptr;if(kind==6)io->scratchData=nullptr;if(kind==7)io->payloadBytes--;
  if(kind==8)io->profile.bar0_host_start=UINT64_MAX-0x1917ffff;if(kind==9)io->q38MixedLiveMemoryArea[0]=1;
  if(kind==10)io->q38MixedLiveMemoryView[3]=io->region[3].data();if(kind==11)io->profile.bar0_host_start=UINT64_MAX;
  assert(!io->MapMixedLiveMemory() && !io->maps && !io->mmio && !io->direct && !io->q38MixedLiveMemoryMapAttempted);refused++;
 }
 for(unsigned kind=0;kind<8;kind++){
  auto io=std::make_unique<MappingIO>();endpoint=io.get();if(kind<4)io->fail=int(kind);else io->nullView=int(kind-4);
  assert(!io->MapMixedLiveMemory() && io->q38MixedLiveMemoryMapAttempted);unsigned maps=io->maps;uint32 out=123;
  assert(!io->MapMixedLiveMemory() && io->maps==maps && !io->mmio && !io->direct);
  assert(!io->MixedLiveMemoryReadChecked(0,0x40000,out) && out==123 && !io->MixedLiveMemoryWrite(0,0x40000,0));refused++;
 }
 assert(words==TT_Q38_MIXED_LIVE_SAVED_WORDS);
 printf("PASS ACTUAL new native mapping adapter/all120 words%u/inert refusals%u/exact4aliases/selected-core+BAR+logical ROOT/peer bounds/UINT32_MAX/partial maps retained with NO retry;TEST OS endpoints only,NO native source/execution/speed grant\n",words,refused);
}
