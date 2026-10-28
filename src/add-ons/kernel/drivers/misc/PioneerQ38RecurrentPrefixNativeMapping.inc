/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_RECURRENT_PREFIX_NATIVE_CONTEXT
#include "PioneerQ38RecurrentPrefixNativeRole.h"
bool RecurrentPrefixCallScopeMatches();bool RecurrentPrefixBaseScope();bool RecurrentPrefixMappingSourceScope();bool RecurrentPrefixMemoryAccessScope();
bool RecurrentPrefixInitializationScope(bool history,bool beginning);bool RecurrentPrefixInitializationOtherCoresHeld(bool history);
bool ConvHistoryZeroBeforeCohort();bool ConvHistoryZeroOtherCoresHeld();
bool RecurrentPrefixMemoryScope(const PioneerQ38RecurrentPrefixMemory&,const PioneerQ38CanonicalModelPlan&,const PioneerQ38GdnIdentity&,const uint8_t*,unsigned);
bool RecurrentPrefixExecutionScope(const PioneerQ38RecurrentPrefixOwner&,const PioneerQ38RecurrentPrefixMemory&,
 const PioneerQ38CanonicalModelPlan&,const PioneerQ38RecurrentPrefixPlan&,const PioneerQ38GdnIdentity&,const uint8_t*,unsigned);
void RecurrentPrefixRetainHostOwners();
area_id q38RecurrentPrefixMemoryArea[4]={-1,-1,-1,-1};
volatile uint32* q38RecurrentPrefixMemoryView[4]={NULL,NULL,NULL,NULL};
bool q38RecurrentPrefixMemoryMapAttempted=false;uint64 q38RecurrentPrefixMemoryBar=0;
#ifdef PIONEER_Q38_RECURRENT_PREFIX_FINAL_RELEASE
#include "PioneerQ38RecurrentPrefixFinalUnmap.inc"
#endif
bool RecurrentPrefixMemoryMappingStable(unsigned w){
 if(w>=120 || runtimeCore!=w || !fleetEnabled || !runtimeMapped || !runtimeEntry || !scratchData || payloadBytes!=8192
  || !Q38PanelMappingStable() || !q38RecurrentPrefixMemoryMapAttempted || profile.bar0_host_start!=q38RecurrentPrefixMemoryBar
  || profile.bar0_host_start>UINT64_MAX-UINT64_C(0x19180000) || TT_Q38_PANEL_VIEW_BASE!=0x6e000 || TT_Q38_PANEL_VIEW_BYTES<0x3e000)return false;
 for(unsigned i=0;i<4;i++)if(q38RecurrentPrefixMemoryArea[i]<B_OK || !q38RecurrentPrefixMemoryView[i])return false;
 return true; // CPU aliases only;no source/execution grant
}
bool MapRecurrentPrefixMemory(){
 if(!RecurrentPrefixMappingSourceScope() || runtimeCore || !fleetEnabled || !runtimeMapped || !runtimeEntry || !scratchData
  || payloadBytes!=8192 || !Q38PanelMappingStable() || profile.bar0_host_start>UINT64_MAX-UINT64_C(0x19180000)
  || q38RecurrentPrefixMemoryMapAttempted)return false;
 for(unsigned i=0;i<4;i++)if(q38RecurrentPrefixMemoryArea[i]>=B_OK || q38RecurrentPrefixMemoryView[i])return false;
 q38RecurrentPrefixMemoryMapAttempted=true;q38RecurrentPrefixMemoryBar=profile.bar0_host_start;
 const uint32 offsets[]={0x19020000,0x19040000,0x19060000,0x190ac000},bytes[]={8192,131072,57344,868352};
 for(unsigned i=0;i<4;i++){
  void* view=NULL;
  q38RecurrentPrefixMemoryArea[i]=map_physical_memory("P100A autonomous prefix retained L1",profile.bar0_host_start+offsets[i],bytes[i],
   B_ANY_KERNEL_ADDRESS,B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA|B_UNCACHED_MEMORY,&view);
  if(q38RecurrentPrefixMemoryArea[i]<B_OK || !view)return false; // retain every partial alias;no retry/repair
  q38RecurrentPrefixMemoryView[i]=static_cast<volatile uint32*>(view);
 }
 return RecurrentPrefixMemoryMappingStable(0);
}
bool RecurrentPrefixMemoryReadChecked(unsigned w,uint32 a,uint32& out){
 if(!PioneerQ38RecurrentPrefixSnapshotContains(w,a) || !RecurrentPrefixMemoryAccessScope() || !RecurrentPrefixMemoryMappingStable(w))return false;
 uint32 value;
 if(a==0x6000)value=RuntimeCodeRead(0);
 else if(a<0x40000)value=PioneerRead32(q38RecurrentPrefixMemoryView[0]+(a-0x20000)/4);
 else if(a<0x60000)value=PioneerRead32(q38RecurrentPrefixMemoryView[1]+(a-0x40000)/4);
 else if(a<0x6e000)value=PioneerRead32(q38RecurrentPrefixMemoryView[2]+(a-0x60000)/4);
 else if(a<0xac000)value=Q38PanelRead(a);
 else if(a>=0x17ffc0)value=ResidentL1Read(0x1fc0/4+(a-0x17ffc0)/4);
 else value=PioneerRead32(q38RecurrentPrefixMemoryView[3]+(a-0xac000)/4);
 out=value;return true; // UINT32_MAX is legitimate original DATA
}
bool RecurrentPrefixMemoryWrite(unsigned w,uint32 a,uint32 value){
 if(!PioneerQ38RecurrentPrefixSnapshotContains(w,a) || !RecurrentPrefixMemoryAccessScope() || !RecurrentPrefixMemoryMappingStable(w))return false;
 if(a==0x6000)RuntimeCodeWrite(0,value);
 else if(a<0x40000)PioneerWrite32(q38RecurrentPrefixMemoryView[0]+(a-0x20000)/4,value);
 else if(a<0x60000)PioneerWrite32(q38RecurrentPrefixMemoryView[1]+(a-0x40000)/4,value);
 else if(a<0x6e000)PioneerWrite32(q38RecurrentPrefixMemoryView[2]+(a-0x60000)/4,value);
 else if(a<0xac000)Q38PanelWrite(a,value);
 else if(a>=0x17ffc0)ResidentL1Write(0x1fc0/4+(a-0x17ffc0)/4,value);
 else PioneerWrite32(q38RecurrentPrefixMemoryView[3]+(a-0xac000)/4,value);
 return true;
}
#endif
