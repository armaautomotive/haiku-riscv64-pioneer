/* SPDX-License-Identifier: MIT */
// Template/stack witness,NOT genuine native IO or a loadable driver.
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenMemory.h"
struct TokenMemoryKernelCompileIO {
 bool TokenMemoryScope(const PioneerQ38TokenMemory&,const PioneerQ38CanonicalModelPlan&,const PioneerQ38TokenStartupPlan&,
  const PioneerQ38GdnIdentity&,const uint8_t*,unsigned);
 bool TokenMemoryMappingStable(unsigned);bool TokenMemoryReadChecked(unsigned,uint32_t,uint32_t&);
 bool TokenMemoryWrite(unsigned,uint32_t,uint32_t);void TokenMemoryRetainHostOwners();void QuarantineHostPage();
 bool SelectRuntimeCore(unsigned);bool Q38PanelMappingStable();
 uint32_t ResetRead();uint32_t SourceRead(unsigned);uint32_t ResidentNocRead(uint32_t);
 uint32_t HostPciRead(unsigned,uint16_t,uint8_t);uint32_t RuntimeCodeRead(unsigned);uint32_t Q38PanelRead(uint32_t);
};
extern "C" bool pioneer_q38_token_memory_template_prepare(PioneerQ38TokenMemory& heap,TokenMemoryKernelCompileIO& io,
 const tt_dram_samples_result& dram,PioneerResidentBanks& banks,PioneerQ38DispatchLease& cohort,const PioneerQ38CanonicalModelPlan& plan,
 const PioneerQ38TokenStartupPlan& startup,const PioneerQ38GdnIdentity& id,const uint8_t* ledger,unsigned pages,PioneerQ38TokenMemoryResult& result){
 return heap.PrepareHeld(io,dram,banks,cohort,plan,startup,id,ledger,pages,result);
}
extern "C" bool pioneer_q38_token_memory_template_close(PioneerQ38TokenMemory& heap,TokenMemoryKernelCompileIO& io,
 PioneerQ38TokenMemoryResult& result){return heap.CloseHeldNoLaunch(io,result);}
