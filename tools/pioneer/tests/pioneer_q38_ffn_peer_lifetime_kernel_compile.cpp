/* SPDX-License-Identifier: MIT */
// RISC-V kernel compile/stack witness ONLY,not a native factory or adapter.
// Deliberately NO NoC submit,GDDR/host numeric,PCI write,model/learned,
// clock restoration,END,unmap/free/retry APIs in this witness interface.
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38FfnPeerLifetimeOwner.h"
struct PeerLifetimeCompileIO {
 void QuarantineHostPage();bool SelectRuntimeCore(unsigned);bool Q38PanelMappingStable();bool FfnGraphCodeMappingStable(unsigned);
 bool FfnGraphProgramReadChecked(unsigned,uint32_t,uint32_t&);bool FfnGraphProgramWrite(unsigned,uint32_t,uint32_t);
 uint32_t RuntimeCodeRead(unsigned);uint32_t Q38PanelRead(uint32_t);void Q38PanelWrite(uint32_t,uint32_t);
 uint32_t SourceRead(unsigned);uint32_t ResetRead();uint32_t RuntimePcRead();void RuntimeResetWrite(uint32_t);
 uint32_t HostPciRead(unsigned,uint16_t,uint8_t);uint32_t ResidentNocRead(uint32_t);void ResidentNocWrite(uint32_t,uint32_t);
 void Pause();uint64_t Q38CohortNow();
};
extern "C" bool pioneer_q38_ffn_peer_lifetime_compile(PeerLifetimeCompileIO& io,const tt_dram_samples_result& proof,
 PioneerResidentBanks& banks,PioneerQ38DispatchLease& cohort,PioneerQ38FfnGraphCodeOwner& heapCode,
 PioneerQ38FfnPeerLifetimeOwner& owner,PioneerQ38FfnPeerLifetimeResult& result) {
 return owner.Qualify(io,proof,banks,cohort,heapCode,result);
}
