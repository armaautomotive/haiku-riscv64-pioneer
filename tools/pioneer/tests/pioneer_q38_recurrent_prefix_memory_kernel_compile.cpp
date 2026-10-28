/* SPDX-License-Identifier: MIT */
// Actual SDK template/frame witness ONLY,not a native IO implementation or
// loadable driver. Undefined IO hooks deliberately prevent a runtime alias.
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38RecurrentPrefixMemory.h"
struct PrefixMemoryKernelCompileIO {
 bool RecurrentPrefixMemoryScope(const PioneerQ38RecurrentPrefixMemory&,const PioneerQ38CanonicalModelPlan&,
  const PioneerQ38GdnIdentity&,const uint8_t*,unsigned);
 bool RecurrentPrefixMemoryMappingStable(unsigned);
 bool RecurrentPrefixMemoryReadChecked(unsigned,uint32_t,uint32_t&);
 bool RecurrentPrefixMemoryWrite(unsigned,uint32_t,uint32_t);
 void RecurrentPrefixRetainHostOwners();void QuarantineHostPage();
 bool SelectRuntimeCore(unsigned);bool Q38PanelMappingStable();
 uint32_t ResetRead();uint32_t SourceRead(unsigned);uint32_t ResidentNocRead(uint32_t);
 uint32_t HostPciRead(unsigned,uint16_t,uint8_t);uint32_t RuntimeCodeRead(unsigned);uint32_t Q38PanelRead(uint32_t);
};
extern "C" bool pioneer_q38_recurrent_prefix_memory_template_prepare(PioneerQ38RecurrentPrefixMemory& heap,PrefixMemoryKernelCompileIO& io,
 const tt_dram_samples_result& dram,PioneerResidentBanks& banks,PioneerQ38DispatchLease& cohort,const PioneerQ38CanonicalModelPlan& plan,
 const PioneerQ38GdnIdentity& id,const uint8_t* ledger,unsigned pages,PioneerQ38RecurrentPrefixMemoryResult& result){
 return heap.PrepareHeld(io,dram,banks,cohort,plan,id,ledger,pages,result);
}
extern "C" bool pioneer_q38_recurrent_prefix_memory_template_close(PioneerQ38RecurrentPrefixMemory& heap,PrefixMemoryKernelCompileIO& io,
 PioneerQ38RecurrentPrefixMemoryResult& result){return heap.CloseHeldNoLaunch(io,result);}
