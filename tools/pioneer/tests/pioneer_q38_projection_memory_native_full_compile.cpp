/* SPDX-License-Identifier: MIT */
// COMPILE ONLY:instantiate the new memory component with the ACTUAL
// complete driver IO/context/scope/retention/END code. No new ioctl or
// linked/installable candidate is produced by this fixture.
#define PIONEER_Q38_PROJECTION_GRAPH_MEMORY 1
#include "../../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp"
extern "C" bool pioneer_projection_memory_kernel_prepare(PioneerStoreContext& c,PioneerQ38ProjectionGraphMemoryResult& result){
 if(!c.q38ProjectionMemory || !c.q38Dispatch || !c.q38FfnModelPlan)return false;
 return c.q38ProjectionMemory->PrepareHeld(c.io,c.baseline.baseline.baseline.baseline.dram,c.banks,*c.q38Dispatch,
  *c.q38FfnModelPlan,c.q38FfnModelIdentity,c.initialized,PioneerResidentBanks::Pages,result);
}
extern "C" bool pioneer_projection_memory_kernel_close_held(PioneerStoreContext& c,PioneerQ38ProjectionGraphMemoryResult& result){
 return c.q38ProjectionMemory && c.q38ProjectionMemory->CloseHeldNoLaunch(c.io,result);
}
