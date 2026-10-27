/* SPDX-License-Identifier: MIT */
// Actual SDK template/frame witness ONLY,not a native IO implementation or
// loadable driver. Undefined IO hooks deliberately prevent a runtime alias.
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38RecurrentBlockMemory.h"
struct RecurrentMemoryKernelCompileIO {
 bool RecurrentBlockMemoryScope(const PioneerQ38RecurrentBlockMemory&,const PioneerQ38CanonicalModelPlan&,
  const PioneerQ38GdnIdentity&,const uint8_t*,unsigned);
 bool RecurrentBlockMemoryMappingStable(unsigned);
 bool RecurrentBlockMemoryReadChecked(unsigned,uint32_t,uint32_t&);
 bool RecurrentBlockMemoryWrite(unsigned,uint32_t,uint32_t);
 void RecurrentBlockRetainHostOwners();void QuarantineHostPage();
 bool SelectRuntimeCore(unsigned);bool Q38PanelMappingStable();
 uint32_t ResetRead();uint32_t SourceRead(unsigned);uint32_t ResidentNocRead(uint32_t);
 uint32_t HostPciRead(unsigned,uint16_t,uint8_t);uint32_t RuntimeCodeRead(unsigned);uint32_t Q38PanelRead(uint32_t);
};
extern "C" bool pioneer_q38_recurrent_memory_template_prepare(PioneerQ38RecurrentBlockMemory& heap,RecurrentMemoryKernelCompileIO& io,
 const tt_dram_samples_result& dram,PioneerResidentBanks& banks,PioneerQ38DispatchLease& cohort,const PioneerQ38CanonicalModelPlan& plan,
 const PioneerQ38GdnIdentity& id,const uint8_t* ledger,unsigned pages,PioneerQ38RecurrentBlockMemoryResult& result){
 return heap.PrepareHeld(io,dram,banks,cohort,plan,id,ledger,pages,result);
}
extern "C" bool pioneer_q38_recurrent_memory_template_close(PioneerQ38RecurrentBlockMemory& heap,RecurrentMemoryKernelCompileIO& io,
 PioneerQ38RecurrentBlockMemoryResult& result){return heap.CloseHeldNoLaunch(io,result);}
