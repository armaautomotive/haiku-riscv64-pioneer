/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_TOKEN_NATIVE_CONTEXT
#include "PioneerQ38TokenNativeRole.h"
bool TokenMemoryMappingSourceScope();bool TokenMemoryAccessScope();
bool TokenDispatchImageAccessScope(unsigned);
bool TokenPeerStoreSelectionScope();bool TokenPeerStoreAccessScope(unsigned);
bool TokenRootStoreAccessScope();
bool TokenSharedTlbScope();
bool TokenClockReceiptScope();
bool TokenHardwareSealScope();bool TokenFinalOwnershipScope();bool TokenFinalLifetimeScope();bool TokenViewsScope();bool TokenViewCaptureScope();
bool TokenPciCaptureScope();bool TokenPciScope();
bool TokenPciLeaseIdentity(const PioneerQ38TokenPciLease&)const;
const void* TokenPciTransport();uint64 TokenPciEndpoint();
bool TokenPciReadChecked(bool,unsigned,unsigned,uint32&);bool TokenPciWriteChecked(bool,unsigned,unsigned,uint32);
bool TokenInitializationFifoEmpty(unsigned);bool TokenInitializationFifoReadChecked(unsigned,bool,uint32&);
bool PrepareTokenNativeParticipant(unsigned);
bool TokenMemoryScope(const PioneerQ38TokenMemory&,const PioneerQ38CanonicalModelPlan&,const PioneerQ38TokenStartupPlan&,
 const PioneerQ38GdnIdentity&,const uint8_t*,unsigned);
void TokenMemoryRetainHostOwners();
bool TokenRuntimeScope(const PioneerQ38TokenNativeOwner&,const PioneerQ38TokenMemory&,const PioneerQ38CanonicalModelPlan&,
 const PioneerQ38TokenStartupPlan&,const PioneerQ38GdnIdentity&,const uint8_t*,unsigned);
area_id q38TokenMemoryArea[4]={-1,-1,-1,-1};
volatile uint32* q38TokenMemoryView[4]={NULL,NULL,NULL,NULL};
bool q38TokenMemoryMapAttempted=false;uint64 q38TokenMemoryBar=0;
#include "PioneerQ38TokenNativeViewsMapping.inc"
bool TokenMemoryMappingStable(unsigned worker){
 if(worker>=120 || runtimeCore!=worker || !fleetEnabled || !runtimeMapped || !runtimeEntry || !runtimePc || !scratchData || payloadBytes!=8192
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
 // QTP2 adds the complete0x30000..0x3ffff capsule journal. Keep four
 // aliases,but cover that whole first aperture;SnapshotContains STILL
 // excludes the0x22000..0x2ffff hole and every other unowned address.
 const uint32 offsets[]={0x19020000,0x19040000,0x19060000,0x190ac000},bytes[]={PioneerQ38TokenL1::Precise?131072U:8192U,131072,57344,868352};
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
__attribute__((noinline)) bool TokenDispatchImageReadChecked(unsigned worker,uint32 address,uint32& output){
 if((address&3) || (address!=0x6000 && (address<TT_Q38_PANEL_VIEW_BASE
  || address-TT_Q38_PANEL_VIEW_BASE>=TT_Q38_PANEL_VIEW_BYTES))
  || !TokenDispatchImageAccessScope(worker) || !TokenMemoryMappingStable(worker))return false;
 output=address==0x6000?RuntimeCodeRead(0):Q38PanelRead(address);return true;
} // complete existing panel alias;UINT32_MAX is valid original DATA
__attribute__((noinline)) bool TokenDispatchImageWrite(unsigned worker,uint32 address,uint32 value){
 if((address&3) || (address!=0x6000 && (address<TT_Q38_PANEL_VIEW_BASE
  || address-TT_Q38_PANEL_VIEW_BASE>=TT_Q38_PANEL_VIEW_BYTES))
  || !TokenDispatchImageAccessScope(worker) || !TokenMemoryMappingStable(worker))return false;
 if(address==0x6000)RuntimeCodeWrite(0,value);else Q38PanelWrite(address,value);
 return true;
} // fixed dispatch originals only;does NOT widen token snapshot geometry
__attribute__((noinline)) bool TokenPeerStoreSelect(unsigned worker){
 if(worker>=120 || !TokenPeerStoreSelectionScope())return false;
 runtimeCore=worker; // intended view ONLY;no alias access until BOTH routes verify
 for(unsigned s=0;s<2;s++)for(unsigned i=0;i<3;i++){
  const uint32_t expected=BlackholeDramFixedTlbWord(s?0xffa00000U:0U,PioneerWorkerX(worker),PioneerWorkerY(worker),false,i);
  if(!TokenPeerStoreSelectionScope())return false;
  DramTlbWrite(s,i,expected);
  if(!TokenPeerStoreSelectionScope() || DramTlbRead(s,i)!=expected)return false;
 }
 return TokenPeerStoreSelectionScope() && TokenMemoryMappingStable(worker);
} // fixed existing200/201 routes,each write checked;NO final shared-TLB restoration
__attribute__((noinline)) bool TokenPeerNiuReadChecked(unsigned worker,uint32 address,uint32& output){
 if((address&3) || (address>=40 && address!=0x100) || !TokenPeerStoreAccessScope(worker) || !TokenMemoryMappingStable(worker))return false;
 output=ResidentNocRead(address);return true;
}
__attribute__((noinline)) bool TokenPeerNiuWrite(unsigned worker,uint32 address,uint32 value){
 if((address&3) || (address>=40 && address!=0x100) || !TokenPeerStoreAccessScope(worker) || !TokenMemoryMappingStable(worker))return false;
 ResidentNocWrite(address,value);return true;
} //4-byte original command/config words ONLY;0x40 START is NEVER accepted
__attribute__((noinline)) bool TokenPeerScratchReadChecked(unsigned worker,unsigned word,uint32& output){
 if(word>=2048 || !TokenPeerStoreAccessScope(worker) || !TokenMemoryMappingStable(worker))return false;
 output=ResidentL1Read(word);return true;
}
__attribute__((noinline)) bool TokenPeerScratchWrite(unsigned worker,unsigned word,uint32 value){
 if(word>=2048 || !TokenPeerStoreAccessScope(worker) || !TokenMemoryMappingStable(worker))return false;
 ResidentL1Write(word,value);return true;
} // existing fixed8KiB alias;arbitrary original UINT32_MAX remains valid data
__attribute__((noinline)) bool TokenRootNiuReadChecked(uint32 address,uint32& output){
 if((address&3) || (address>=40 && address!=0x100) || !TokenRootStoreAccessScope() || !TokenMemoryMappingStable(0))return false;
 output=ResidentNocRead(address);return true;
}
__attribute__((noinline)) bool TokenRootNiuWrite(uint32 address,uint32 value){
 if((address&3) || (address>=40 && address!=0x100) || !TokenRootStoreAccessScope() || !TokenMemoryMappingStable(0))return false;
 ResidentNocWrite(address,value);return true;
} // original commands/config ONLY;never START/counter clear or peer access
__attribute__((noinline)) bool TokenRootScratchReadChecked(unsigned word,uint32& output){
 if(word>=2048 || !TokenRootStoreAccessScope() || !TokenMemoryMappingStable(0))return false;
 output=ResidentL1Read(word);return true;
}
__attribute__((noinline)) bool TokenRootScratchWrite(unsigned word,uint32 value){
 if(word>=2048 || !TokenRootStoreAccessScope() || !TokenMemoryMappingStable(0))return false;
 ResidentL1Write(word,value);return true;
} // fixed ROOT8KiB only;UINT32_MAX remains original DATA
__attribute__((noinline)) bool TokenSharedRootCommandReadChecked(unsigned word,uint32& output){
 if(word>=10 || !TokenMemoryAccessScope() || !TokenSharedTlbScope())return false;
 output=ResidentNocRead(word*4);return true;
} // pre-change ONLY;alias veto takes effect BEFORE the first shared TLB write
__attribute__((noinline)) bool TokenSharedTlbReadChecked(unsigned slot,unsigned word,uint32& output){
 if(slot>=2 || word>=3 || !TokenSharedTlbScope())return false;
 output=DramTlbRead(slot,word);return true;
}
__attribute__((noinline)) bool TokenSharedTlbWrite(unsigned slot,unsigned word,uint32 value){
 if(slot>=2 || word>=3 || !TokenSharedTlbScope())return false;
 DramTlbWrite(slot,word,value);return true;
} // direct existing200/201 control aperture ONLY;not the mapped NoC/L1 aliases
__attribute__((noinline)) bool TokenClockTlbReadChecked(unsigned slot,unsigned word,uint32& output){
 if(slot>=2 || word>=3 || !TokenClockReceiptScope())return false;
 output=DramTlbRead(slot,word);return true;
} // read-only original-map audit after clock return;NO source alias or map write
__attribute__((noinline)) bool TokenHardwareSealTlbReadChecked(unsigned slot,unsigned word,uint32& output){
 if(slot>=2 || word>=3 || !TokenHardwareSealScope())return false;
 output=DramTlbRead(slot,word);return true;
} // fresh direct-control read before CPU-only hardware-lease retirement
__attribute__((noinline)) bool TokenFifoReadChecked(unsigned worker,bool overflow,uint32& output){
 if(worker>=120 || !TokenMemoryAccessScope() || !TokenMemoryMappingStable(worker))return false;
 output=TokenFifoStatusWord(overflow?0x68U:0x64U);return true;
} // scoped fixed read-only FIFO registers;no counter clear,START or broader MMIO range
#endif
