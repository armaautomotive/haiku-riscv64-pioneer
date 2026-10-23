/* SPDX-License-Identifier: MIT */
// Private template COMPILE ONLY,NOT an actual native adapter or selector.
#define PIONEER_Q38_GDN_MUTABLE_STATE
#define PIONEER_Q38_EMBEDDING_NATIVE 1
#define PIONEER_Q38_LEARNED_RMS_NATIVE 1
#define PIONEER_Q38_PROJECTION_EXECUTION_OWNER 1
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionGraphOwner.h"
struct ProjectionExecutionCompileIO {
 void QuarantineHostPage();void ProjectionGraphRetainHostOwners();
 bool ProjectionGraphMemoryScope(const PioneerQ38ProjectionGraphMemory&,const PioneerQ38CanonicalModelPlan&,const PioneerQ38GdnIdentity&,const uint8_t*,unsigned);
 bool ProjectionGraphExecutionScope(const PioneerQ38ProjectionGraphOwner&,const PioneerQ38ProjectionGraphMemory&,const PioneerQ38ProjectionMutableBanks&,
  const PioneerQ38CanonicalModelPlan&,const PioneerQ38LearnedRmsController&,const PioneerQ38ProjectionIdentity&,const uint8_t*,unsigned);
 bool SelectRuntimeCore(unsigned);bool Q38PanelMappingStable();bool ProjectionGraphMemoryMappingStable(unsigned);
 bool ProjectionGraphMemoryReadChecked(unsigned,uint32_t,uint32_t&);bool ProjectionGraphMemoryWrite(unsigned,uint32_t,uint32_t);
 uint32_t RuntimeCodeRead(unsigned);uint32_t Q38PanelRead(uint32_t);
 uint32_t SourceRead(unsigned);uint32_t ResetRead();uint32_t RuntimePcRead();void RuntimeResetWrite(uint32_t);
 uint32_t RuntimeStatusRead(unsigned);void RuntimeStatusWrite(unsigned,uint32_t);
 uint32_t HostPciRead(unsigned,uint16_t,uint8_t);uint32_t ResidentNocRead(uint32_t);void ResidentNocWrite(uint32_t,uint32_t);
 void Pause();uint64_t Q38CohortNow();
};
extern "C" bool pioneer_projection_execution_compile(ProjectionExecutionCompileIO& io,const tt_dram_samples_result& proof,
 PioneerResidentBanks& banks,PioneerQ38DispatchLease& cohort,PioneerQ38ProjectionGraphMemory& memory,
 PioneerQ38ProjectionMutableBanks& graph,const PioneerQ38CanonicalModelPlan& plan,const PioneerQ38LearnedRmsController& producer,
 const PioneerQ38ProjectionIdentity& id,const uint8_t* ledger,unsigned pages,PioneerQ38ProjectionGraphOwner& owner,PioneerQ38ProjectionGraphResult& result){
 return owner.Prepare(io,proof,banks,cohort,memory,graph,plan,producer,id,ledger,pages,0,0,0,10,result)
  && owner.Run(io,10,result) && owner.Run(io,11,result) && owner.Close(io,result);
}
