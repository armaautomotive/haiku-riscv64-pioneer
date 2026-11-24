/* SPDX-License-Identifier: MIT */
// Type-check resident metadata promotion against the actual host PCI class.
// This is never linked into a live driver and grants no device authority.
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35ResidentHostPci.h"
#include "../PioneerQwen35DataflowResidentModelTransaction.h"
struct ResidentModelProbeRaw;
struct ResidentModelProbeScope;
using ResidentModelProbeHost=PioneerQwen35ResidentHostPci<ResidentModelProbeRaw,ResidentModelProbeScope>;
bool PioneerQ35ResidentModelPrepareProbe(PioneerQwen35::DataflowResidentModelTransaction& transaction,
    PioneerQwen35::DataflowProjectionAuthority*& live,PioneerQwen35::DataflowProjectionAuthority& candidate,
    const PioneerQwen35::DataflowResidentCommand& command,ResidentModelProbeHost& host,
    PioneerQwen35::DataflowProjectionProgram& scratch,unsigned& stage) {
    return transaction.Prepare(live,candidate,command,host,scratch,stage);
}
bool PioneerQ35ResidentModelCommitProbe(PioneerQwen35::DataflowResidentModelTransaction& transaction,
    const ResidentModelProbeHost& host,PioneerQwen35::DataflowArgmaxResult& output) {
    return transaction.Commit(host,output);
}
