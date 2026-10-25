/* SPDX-License-Identifier: MIT */
// Class-body fragment for a DISTINCT genuine PioneerSessionIO native role.
// Included ONLY by the distinct OFFLINE resident-layer role. Complete
// native factory/execution and physical qualification remain REQUIRED.
// Call Map ONLY after actual QPS1 STOP/restore, ALL120 held and exact
// initialized model/clock/cookie/thread/host/fleet lifetime is retained.
// No old projection map/ready flag can authorize this larger fixed view.
#include "PioneerQ38ResidentLayerNativeRole.h"
area_id q38ResidentLayerMemoryArea[2]={-1,-1};
volatile uint32* q38ResidentLayerMemoryView[2]={NULL,NULL};
bool q38ResidentLayerMemoryMapAttempted=false;
uint64 q38ResidentLayerMemoryBar=0;
bool MapResidentLayerMemory() {
 if(!ResidentLayerMappingSourceScope() || runtimeCore || !fleetEnabled || !runtimeMapped || !Q38PanelMappingStable()
  || !runtimeEntry || !scratchData || payloadBytes!=8192 || TT_Q38_PANEL_VIEW_BASE!=0x6e000 || TT_Q38_PANEL_VIEW_BYTES<0x3e000
  || profile.bar0_host_start>UINT64_MAX-UINT64_C(0x1917e000))return false;
 if(q38ResidentLayerMemoryMapAttempted)return ResidentLayerMemoryMappingStable(0);
 for(unsigned i=0;i<2;i++)if(q38ResidentLayerMemoryArea[i]>=B_OK || q38ResidentLayerMemoryView[i])return false;
 q38ResidentLayerMemoryMapAttempted=true;q38ResidentLayerMemoryBar=profile.bar0_host_start;
 // Prefix40KiB plus complete NEW ROM/manifests/shadows suffix. The
 // existing panel owns6e000..ac000 and runtime owns the separate trailer.
 //NEW ROOT suffix extends through17e000;peers STILL have no ROOT-only
 //manifest/norm/FFN logical grant. Nothing past17e000 is mapped here.
 const uint32 offsets[]={0x19064000,0x190b8000},bytes[]={40960,811008};
 for(unsigned i=0;i<2;i++){
  void* view=NULL;
  q38ResidentLayerMemoryArea[i]=map_physical_memory("P100A complete recurrent layer L1",profile.bar0_host_start+offsets[i],bytes[i],
   B_ANY_KERNEL_ADDRESS,B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA|B_UNCACHED_MEMORY,&view);
  if(q38ResidentLayerMemoryArea[i]<B_OK || !view)return false; // retain partial map;NO retry/unmap/fallback
  q38ResidentLayerMemoryView[i]=static_cast<volatile uint32*>(view);
 }
 return ResidentLayerMemoryMappingStable(0);
}
bool ResidentLayerMemoryMappingStable(unsigned w) {
 return w<120 && runtimeCore==w && fleetEnabled && runtimeMapped && runtimeEntry && scratchData && payloadBytes==8192
  && Q38PanelMappingStable() && q38ResidentLayerMemoryMapAttempted && q38ResidentLayerMemoryArea[0]>=B_OK
  && q38ResidentLayerMemoryArea[1]>=B_OK && q38ResidentLayerMemoryView[0] && q38ResidentLayerMemoryView[1]
  && profile.bar0_host_start==q38ResidentLayerMemoryBar && profile.bar0_host_start<=UINT64_MAX-UINT64_C(0x1917e000)
  && TT_Q38_PANEL_VIEW_BASE==0x6e000 && TT_Q38_PANEL_VIEW_BYTES>=0x3e000;
} // CPU alias/selected-core binding ONLY;NOT source/ownership/init/launch authority
bool ResidentLayerMemoryReadChecked(unsigned w,uint32 a,uint32& output) {
 if(!PioneerQ38ResidentLayerSnapshotContains(w,a) || !ResidentLayerMemoryMappingStable(w))return false;
 uint32 value;
 if(a==0x6000)value=RuntimeCodeRead(0);
 else if(a<0x6e000)value=PioneerRead32(q38ResidentLayerMemoryView[0]+(a-0x64000)/4);
 else if(a<0xac000)value=Q38PanelRead(a);
 else if(a>=0x17ffc0)value=ResidentL1Read(0x1fc0/4+(a-0x17ffc0)/4);
 else value=PioneerRead32(q38ResidentLayerMemoryView[1]+(a-0xb8000)/4);
 output=value;return true; // arbitrary UINT32_MAX original DATA is valid
}
bool ResidentLayerMemoryWrite(unsigned w,uint32 a,uint32 value) {
 if(!PioneerQ38ResidentLayerSnapshotContains(w,a) || !ResidentLayerMemoryMappingStable(w))return false;
 if(a==0x6000)RuntimeCodeWrite(0,value);
 else if(a<0x6e000)PioneerWrite32(q38ResidentLayerMemoryView[0]+(a-0x64000)/4,value);
 else if(a<0xac000)Q38PanelWrite(a,value);
 else if(a>=0x17ffc0)ResidentL1Write(0x1fc0/4+(a-0x17ffc0)/4,value);
 else PioneerWrite32(q38ResidentLayerMemoryView[1]+(a-0xb8000)/4,value);
 return true;
}
