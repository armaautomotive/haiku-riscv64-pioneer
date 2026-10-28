/* SPDX-License-Identifier: MIT */
// Template/frame witness only. Undefined PREFIX hooks, no native IO or ABI.
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38RecurrentPrefixOwner.h"
struct PrefixOwnerKernelCompileIO {
 bool RecurrentPrefixExecutionScope(const PioneerQ38RecurrentPrefixOwner&,const PioneerQ38RecurrentPrefixMemory&,
  const PioneerQ38CanonicalModelPlan&,const PioneerQ38RecurrentPrefixPlan&,const PioneerQ38GdnIdentity&,const uint8_t*,unsigned);
 bool RecurrentPrefixMemoryScope(const PioneerQ38RecurrentPrefixMemory&,const PioneerQ38CanonicalModelPlan&,const PioneerQ38GdnIdentity&,const uint8_t*,unsigned);
 bool RecurrentPrefixMemoryMappingStable(unsigned);
 bool RecurrentPrefixMemoryReadChecked(unsigned,uint32_t,uint32_t&);
 bool RecurrentPrefixMemoryWrite(unsigned,uint32_t,uint32_t);
 void RecurrentPrefixRetainHostOwners();void QuarantineHostPage();
 bool SelectRuntimeCore(unsigned);bool Q38PanelMappingStable();
 uint32_t ResetRead();uint32_t SourceRead(unsigned);uint32_t ResidentNocRead(uint32_t);
 uint32_t HostPciRead(unsigned,uint16_t,uint8_t);uint32_t RuntimeCodeRead(unsigned);uint32_t Q38PanelRead(uint32_t);uint32_t RuntimePcRead();
 void RuntimeResetWrite(uint32_t);void ResidentNocWrite(uint32_t,uint32_t);void Pause();uint64_t Q38CohortNow();
};
extern "C" bool pioneer_q38_prefix_owner_template_prepare(PioneerQ38RecurrentPrefixOwner& owner,PrefixOwnerKernelCompileIO& io,
 const tt_dram_samples_result& dram,PioneerResidentBanks& banks,PioneerQ38DispatchLease& cohort,PioneerQ38RecurrentPrefixMemory& memory,
 const PioneerQ38CanonicalModelPlan& c,const PioneerQ38RecurrentPrefixPlan& plan,const PioneerQ38GdnIdentity& id,const uint8_t* ledger,
 unsigned pages,PioneerQ38RecurrentPrefixResult& result){return owner.Prepare(io,dram,banks,cohort,memory,c,plan,id,ledger,pages,result);}
extern "C" bool pioneer_q38_prefix_owner_template_run(PioneerQ38RecurrentPrefixOwner& owner,PrefixOwnerKernelCompileIO& io,
 PioneerQ38RecurrentPrefixResult& result){return owner.Run(io,result);}
extern "C" bool pioneer_q38_prefix_owner_template_close(PioneerQ38RecurrentPrefixOwner& owner,PrefixOwnerKernelCompileIO& io,
 PioneerQ38RecurrentPrefixResult& result,PioneerQ38RecurrentPrefixMemoryResult& memory){return owner.Close(io,result,memory);}
