/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_TOKEN_NATIVE_CONTEXT
#include "PioneerQ38TokenNativeRole.h"
bool TokenMemoryMappingSourceScope();bool TokenMemoryAccessScope();
bool TokenMemoryScope(const PioneerQ38TokenMemory&,const PioneerQ38CanonicalModelPlan&,const PioneerQ38TokenStartupPlan&,
 const PioneerQ38GdnIdentity&,const uint8_t*,unsigned);
void TokenMemoryRetainHostOwners();
area_id q38TokenMemoryArea[4]={-1,-1,-1,-1};
volatile uint32* q38TokenMemoryView[4]={NULL,NULL,NULL,NULL};
bool q38TokenMemoryMapAttempted=false;uint64 q38TokenMemoryBar=0;
bool TokenMemoryMappingStable(unsigned worker){
 if(worker>=120 || runtimeCore!=worker || !fleetEnabled || !runtimeMapped || !runtimeEntry || !scratchData || payloadBytes!=8192
  || !Q38PanelMappingStable() || !q38TokenMemoryMapAttempted || profile.bar0_host_start!=q38TokenMemoryBar
  || profile.bar0_host_start>UINT64_MAX-UINT64_C(0x19180000) || TT_Q38_PANEL_VIEW_BASE!=0x6e000 || TT_Q38_PANEL_VIEW_BYTES<0x3e000)return false;
 for(unsigned i=0;i<4;i++)if(q38TokenMemoryArea[i]<B_OK || !q38TokenMemoryView[i])return false;
 return true; // VM aliases only;not code/source/launch/STOP or cleanup authority
}
bool MapTokenMemory(){
 if(!TokenMemoryMappingSourceScope() || runtimeCore || !fleetEnabled || !runtimeMapped || !runtimeEntry || !scratchData
  || payloadBytes!=8192 || !Q38PanelMappingStable() || profile.bar0_host_start>UINT64_MAX-UINT64_C(0x19180000)
  || q38TokenMemoryMapAttempted)return false;
 for(unsigned i=0;i<4;i++)if(q38TokenMemoryArea[i]>=B_OK || q38TokenMemoryView[i])return false;
 q38TokenMemoryMapAttempted=true;q38TokenMemoryBar=profile.bar0_host_start;
 const uint32 offsets[]={0x19020000,0x19040000,0x19060000,0x190ac000},bytes[]={8192,131072,57344,868352};
 for(unsigned i=0;i<4;i++){
  void* view=NULL;
  q38TokenMemoryArea[i]=map_physical_memory("P100A full token retained L1",profile.bar0_host_start+offsets[i],bytes[i],
   B_ANY_KERNEL_ADDRESS,B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA|B_UNCACHED_MEMORY,&view);
  if(q38TokenMemoryArea[i]<B_OK || !view)return false; // retain EVERY partial alias;no repair/retry/unmap
  q38TokenMemoryView[i]=static_cast<volatile uint32*>(view);
 }
 return TokenMemoryMappingStable(0);
}
bool TokenMemoryReadChecked(unsigned worker,uint32 address,uint32& output){
 if(!PioneerQ38TokenL1::SnapshotContains(worker,address) || !TokenMemoryAccessScope() || !TokenMemoryMappingStable(worker))return false;
 uint32 value;
 if(address==0x6000)value=RuntimeCodeRead(0);
 else if(address<0x40000)value=PioneerRead32(q38TokenMemoryView[0]+(address-0x20000)/4);
 else if(address<0x60000)value=PioneerRead32(q38TokenMemoryView[1]+(address-0x40000)/4);
 else if(address<0x6e000)value=PioneerRead32(q38TokenMemoryView[2]+(address-0x60000)/4); // includes ROOT60000+60004
 else if(address<0xac000)value=Q38PanelRead(address);
 else if(address>=0x17ffc0)value=ResidentL1Read(0x1fc0/4+(address-0x17ffc0)/4);
 else value=PioneerRead32(q38TokenMemoryView[3]+(address-0xac000)/4);
 output=value;return true; // UINT32_MAX is legitimate original DATA,never an error sentinel
}
bool TokenMemoryWrite(unsigned worker,uint32 address,uint32 value){
 if(!PioneerQ38TokenL1::SnapshotContains(worker,address) || !TokenMemoryAccessScope() || !TokenMemoryMappingStable(worker))return false;
 if(address==0x6000)RuntimeCodeWrite(0,value);
 else if(address<0x40000)PioneerWrite32(q38TokenMemoryView[0]+(address-0x20000)/4,value);
 else if(address<0x60000)PioneerWrite32(q38TokenMemoryView[1]+(address-0x40000)/4,value);
 else if(address<0x6e000)PioneerWrite32(q38TokenMemoryView[2]+(address-0x60000)/4,value);
 else if(address<0xac000)Q38PanelWrite(address,value);
 else if(address>=0x17ffc0)ResidentL1Write(0x1fc0/4+(address-0x17ffc0)/4,value);
 else PioneerWrite32(q38TokenMemoryView[3]+(address-0xac000)/4,value);
 return true;
}
#endif
