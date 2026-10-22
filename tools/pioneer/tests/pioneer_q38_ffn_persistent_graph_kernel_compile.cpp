/* SPDX-License-Identifier: MIT */
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38FfnPersistentGraphOwner.h"
struct PersistentGraphCompileIO {
 void QuarantineHostPage();void FfnGraphRetainHostOwners();
 bool FfnGraphScope(const PioneerQ38FfnPersistentGraphOwner&,const PioneerQ38GraphMutableBanks&,
  const PioneerQ38CanonicalModelPlan&,const PioneerQ38FfnIdentity&,const uint8_t*,unsigned);
 bool SelectRuntimeCore(unsigned);bool Q38PanelMappingStable();bool FfnGraphCodeMappingStable(unsigned);
 bool FfnGraphProgramReadChecked(unsigned,uint32_t,uint32_t&);bool FfnGraphProgramWrite(unsigned,uint32_t,uint32_t);
 uint32_t RuntimeCodeRead(unsigned);uint32_t Q38PanelRead(uint32_t);void Q38PanelWrite(uint32_t,uint32_t);
 uint32_t SourceRead(unsigned);uint32_t ResetRead();uint32_t RuntimePcRead();void RuntimeResetWrite(uint32_t);
 uint32_t RuntimeStatusRead(unsigned);void RuntimeStatusWrite(unsigned,uint32_t);
 uint32_t HostPciRead(unsigned,uint16_t,uint8_t);uint32_t ResidentNocRead(uint32_t);void ResidentNocWrite(uint32_t,uint32_t);
 void Pause();uint64_t Q38CohortNow();
};
extern "C" bool pioneer_q38_ffn_persistent_graph_compile(PersistentGraphCompileIO& io,const tt_dram_samples_result& proof,
 PioneerResidentBanks& banks,PioneerQ38DispatchLease& cohort,PioneerQ38FfnGraphCodeOwner& code,
 PioneerQ38GraphMutableBanks& graph,const PioneerQ38CanonicalModelPlan& plan,const PioneerQ38FfnIdentity& identity,
 const uint8_t* ledger,unsigned pages,const PioneerQ38FfnCanonicalJobs& jobs,
 const uint32_t (&input)[5120],uint32_t (&output)[5120],PioneerQ38FfnPersistentGraphOwner& owner,
 PioneerQ38FfnPersistentGraphResult& result) {
 return owner.Prepare(io,proof,banks,cohort,code,graph,plan,identity,ledger,pages,jobs,0,0,result)
  && owner.Run(io,jobs,0,0,input,result) && owner.CopyOutput(io,output,result) && owner.Close(io,result);
}
