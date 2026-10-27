/* SPDX-License-Identifier: MIT */
// Actual PioneerSessionIO class-body integration,not a mock IO adapter.
#ifdef PIONEER_Q38_MIXED_LIVE_NATIVE_CONTEXT
bool MixedLiveCallScopeMatches();bool MixedLiveBaseScope();bool MixedLiveMappingSourceScope();
bool MixedLiveMemoryScope(const PioneerQ38MixedLiveMemory&,const PioneerQ38CanonicalModelPlan&,
 const PioneerQ38GdnIdentity&,const uint8_t*,unsigned);
void MixedLiveRetainHostOwners();
bool MixedLiveExecutionScope(const PioneerQ38MixedLiveOwner&,const PioneerQ38MixedLiveMemory&,const PioneerQ38CanonicalModelPlan&,
 const PioneerQ38MixedLiveBlockPlan&,const PioneerQ38GdnIdentity&,const uint8_t*,unsigned);
area_id q38MixedLiveMemoryArea[4]={-1,-1,-1,-1};
volatile uint32* q38MixedLiveMemoryView[4]={NULL,NULL,NULL,NULL};
bool q38MixedLiveMemoryMapAttempted=false;
uint64 q38MixedLiveMemoryBar=0;
bool MixedLiveMemoryMappingStable(unsigned w){
#ifdef PIONEER_Q38_MIXED_LIVE_FINAL_RELEASE
 if(q38MixedLiveFinalUnmapAttempted)return false; // never reuse ANY alias after release begins,even on a partial failure
#endif
 if(w>=120 || runtimeCore!=w || !fleetEnabled || !runtimeMapped || !runtimeEntry || !scratchData || payloadBytes!=8192
  || !Q38PanelMappingStable() || !q38MixedLiveMemoryMapAttempted || profile.bar0_host_start!=q38MixedLiveMemoryBar
  || profile.bar0_host_start>UINT64_MAX-UINT64_C(0x19180000) || TT_Q38_PANEL_VIEW_BASE!=0x6e000 || TT_Q38_PANEL_VIEW_BYTES<0x3e000)return false;
 for(unsigned i=0;i<4;i++)if(q38MixedLiveMemoryArea[i]<B_OK || !q38MixedLiveMemoryView[i])return false;
 return true; // CPU aliases only;never source,execution or foreign-word permission
}
bool MapMixedLiveMemory(){
 if(!MixedLiveMappingSourceScope() || runtimeCore || !fleetEnabled || !runtimeMapped || !runtimeEntry || !scratchData
  || payloadBytes!=8192 || !Q38PanelMappingStable() || profile.bar0_host_start>UINT64_MAX-UINT64_C(0x19180000))return false;
 if(q38MixedLiveMemoryMapAttempted)return false; // no retry or partial-map promotion
 for(unsigned i=0;i<4;i++)if(q38MixedLiveMemoryArea[i]>=B_OK || q38MixedLiveMemoryView[i])return false;
 q38MixedLiveMemoryMapAttempted=true;q38MixedLiveMemoryBar=profile.bar0_host_start;
 const uint32 offsets[]={0x19020000,0x19040000,0x19060000,0x190ac000},bytes[]={8192,131072,57344,868352};
 for(unsigned i=0;i<4;i++){
  void* view=NULL;
  q38MixedLiveMemoryArea[i]=map_physical_memory("P100A complete mixed lifecycle L1",profile.bar0_host_start+offsets[i],bytes[i],
   B_ANY_KERNEL_ADDRESS,B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA|B_UNCACHED_MEMORY,&view);
  if(q38MixedLiveMemoryArea[i]<B_OK || !view)return false; // caller retains ALL partial aliases on failure
  q38MixedLiveMemoryView[i]=static_cast<volatile uint32*>(view);
 }
 return MixedLiveMemoryMappingStable(0);
}
bool MixedLiveMemoryReadChecked(unsigned w,uint32 a,uint32& out){
 if(!PioneerQ38MixedLiveSnapshotContains(w,a) || !MixedLiveMemoryMappingStable(w))return false;
 uint32 value;
 if(a==0x6000)value=RuntimeCodeRead(0);
 else if(a<0x40000)value=PioneerRead32(q38MixedLiveMemoryView[0]+(a-0x20000)/4);
 else if(a<0x60000)value=PioneerRead32(q38MixedLiveMemoryView[1]+(a-0x40000)/4);
 else if(a<0x6e000)value=PioneerRead32(q38MixedLiveMemoryView[2]+(a-0x60000)/4);
 else if(a<0xac000)value=Q38PanelRead(a);
 else if(a>=0x17ffc0)value=ResidentL1Read(0x1fc0/4+(a-0x17ffc0)/4);
 else value=PioneerRead32(q38MixedLiveMemoryView[3]+(a-0xac000)/4);
 out=value;return true; // UINT32_MAX is valid original DATA
}
bool MixedLiveMemoryWrite(unsigned w,uint32 a,uint32 value){
 if(!PioneerQ38MixedLiveSnapshotContains(w,a) || !MixedLiveMemoryMappingStable(w))return false;
 if(a==0x6000)RuntimeCodeWrite(0,value);
 else if(a<0x40000)PioneerWrite32(q38MixedLiveMemoryView[0]+(a-0x20000)/4,value);
 else if(a<0x60000)PioneerWrite32(q38MixedLiveMemoryView[1]+(a-0x40000)/4,value);
 else if(a<0x6e000)PioneerWrite32(q38MixedLiveMemoryView[2]+(a-0x60000)/4,value);
 else if(a<0xac000)Q38PanelWrite(a,value);
 else if(a>=0x17ffc0)ResidentL1Write(0x1fc0/4+(a-0x17ffc0)/4,value);
 else PioneerWrite32(q38MixedLiveMemoryView[3]+(a-0xac000)/4,value);
 return true;
}
#ifdef PIONEER_Q38_MIXED_LIVE_FINAL_RELEASE
#include "PioneerQ38MixedLiveFinalUnmap.inc"
#endif
#endif
// Only these two fixed coordinates exist. Switching the owned PCIe TLB
