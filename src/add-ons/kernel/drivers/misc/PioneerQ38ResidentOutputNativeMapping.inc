/* SPDX-License-Identifier: MIT */
// Class-body fragment for a DISTINCT genuine PioneerSessionIO native role.
// Included ONLY by the distinct OFFLINE resident-output role. Complete
// native factory/execution and physical qualification remain REQUIRED.
// Call Map ONLY after actual QPS1 STOP/restore, ALL120 held and exact
// initialized model/clock/cookie/thread/host/fleet lifetime is retained.
// No old projection map/ready flag can authorize this larger fixed view.
#include "PioneerQ38ResidentOutputOwnerRole.h"
area_id q38ResidentOutputMemoryArea[2]={-1,-1};
volatile uint32* q38ResidentOutputMemoryView[2]={NULL,NULL};
bool q38ResidentOutputMemoryMapAttempted=false;
uint64 q38ResidentOutputMemoryBar=0;
bool MapResidentOutputMemory() {
 if(!ResidentOutputMappingSourceScope() || runtimeCore || !fleetEnabled || !runtimeMapped || !Q38PanelMappingStable()
  || !runtimeEntry || !scratchData || payloadBytes!=8192 || TT_Q38_PANEL_VIEW_BASE!=0x6e000 || TT_Q38_PANEL_VIEW_BYTES<0x3e000
  || profile.bar0_host_start>UINT64_MAX-UINT64_C(0x1914e000))return false;
 if(q38ResidentOutputMemoryMapAttempted)return ResidentOutputMemoryMappingStable(0);
 for(unsigned i=0;i<2;i++)if(q38ResidentOutputMemoryArea[i]>=B_OK || q38ResidentOutputMemoryView[i])return false;
 q38ResidentOutputMemoryMapAttempted=true;q38ResidentOutputMemoryBar=profile.bar0_host_start;
 // Prefix40KiB plus the exact rounded ROM/manifests/shadows suffix. The
 // existing panel owns6e000..ac000 and runtime owns the separate trailer.
 //14da00..14e000 is MAPPED page padding, NEVER logical read/write authority.
 const uint32 offsets[]={0x19064000,0x190b8000},bytes[]={40960,614400};
 for(unsigned i=0;i<2;i++){
  void* view=NULL;
  q38ResidentOutputMemoryArea[i]=map_physical_memory("P100A complete resident output L1",profile.bar0_host_start+offsets[i],bytes[i],
   B_ANY_KERNEL_ADDRESS,B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA|B_UNCACHED_MEMORY,&view);
  if(q38ResidentOutputMemoryArea[i]<B_OK || !view)return false; // retain partial map;NO retry/unmap/fallback
  q38ResidentOutputMemoryView[i]=static_cast<volatile uint32*>(view);
 }
 return ResidentOutputMemoryMappingStable(0);
}
bool ResidentOutputMemoryMappingStable(unsigned w) {
 return w<120 && runtimeCore==w && fleetEnabled && runtimeMapped && runtimeEntry && scratchData && payloadBytes==8192
  && Q38PanelMappingStable() && q38ResidentOutputMemoryMapAttempted && q38ResidentOutputMemoryArea[0]>=B_OK
  && q38ResidentOutputMemoryArea[1]>=B_OK && q38ResidentOutputMemoryView[0] && q38ResidentOutputMemoryView[1]
  && profile.bar0_host_start==q38ResidentOutputMemoryBar && profile.bar0_host_start<=UINT64_MAX-UINT64_C(0x1914e000)
  && TT_Q38_PANEL_VIEW_BASE==0x6e000 && TT_Q38_PANEL_VIEW_BYTES>=0x3e000;
} // CPU alias/selected-core binding ONLY;NOT source/ownership/init/launch authority
bool ResidentOutputMemoryReadChecked(unsigned w,uint32 a,uint32& output) {
 if(!PioneerQ38ResidentOutputSnapshotContains(w,a) || !ResidentOutputMemoryMappingStable(w))return false;
 uint32 value;
 if(a==0x6000)value=RuntimeCodeRead(0);
 else if(a<0x6e000)value=PioneerRead32(q38ResidentOutputMemoryView[0]+(a-0x64000)/4);
 else if(a<0xac000)value=Q38PanelRead(a);
 else if(a>=0x17ffc0)value=ResidentL1Read(0x1fc0/4+(a-0x17ffc0)/4);
 else value=PioneerRead32(q38ResidentOutputMemoryView[1]+(a-0xb8000)/4);
 output=value;return true; // arbitrary UINT32_MAX original DATA is valid
}
bool ResidentOutputMemoryWrite(unsigned w,uint32 a,uint32 value) {
 if(!PioneerQ38ResidentOutputSnapshotContains(w,a) || !ResidentOutputMemoryMappingStable(w))return false;
 if(a==0x6000)RuntimeCodeWrite(0,value);
 else if(a<0x6e000)PioneerWrite32(q38ResidentOutputMemoryView[0]+(a-0x64000)/4,value);
 else if(a<0xac000)Q38PanelWrite(a,value);
 else if(a>=0x17ffc0)ResidentL1Write(0x1fc0/4+(a-0x17ffc0)/4,value);
 else PioneerWrite32(q38ResidentOutputMemoryView[1]+(a-0xb8000)/4,value);
 return true;
}
