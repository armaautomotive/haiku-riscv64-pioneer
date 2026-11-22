/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_ROLE_COORDINATOR_H
#define PIONEER_QWEN35_DATAFLOW_ROLE_COORDINATOR_H
#include "PioneerQwen35DataflowRoleServiceTick.h"
namespace PioneerQwen35 {
// Call only after both role bodies and compute have returned, with exclusive
// coordinator ownership. A failed precheck leaves every queue untouched;
// this is not a settlement or recovery proof for an active processor.
inline bool DataflowRoleCleanQueuesForReuse(DataflowQueueControl* controls)
{
    if (!controls) return false;
    for (unsigned i=0;i<DataflowCircularBufferCount;i++) {
        auto& c=controls[i];
        if (DataflowLoadAcquire(c.produced)!=DataflowLoadAcquire(c.consumed)
            || DataflowQueueFailure(c) || c.reserved[0] || c.reserved[1]) return false;
    }
    for (unsigned i=0;i<DataflowCircularBufferCount;i++) {
        DataflowStoreRelease(controls[i].produced,0);
        DataflowStoreRelease(controls[i].consumed,0);
    }
    DataflowFence();
    for (unsigned i=0;i<DataflowCircularBufferCount;i++) {
        auto& c=controls[i];
        if (DataflowLoadAcquire(c.produced) || DataflowLoadAcquire(c.consumed)
            || DataflowQueueFailure(c) || c.reserved[0] || c.reserved[1]) return false;
    }
    return true;
}
enum DataflowRoleCoordinatorResult {
    DataflowRoleCoordinatorInvalid=0,
    DataflowRoleCoordinatorComplete=1,
    DataflowRoleCoordinatorRetainedFault=2
};
// Offline contract: coordinator is the sole command/ack writer; Ops.RunCompute
// runs the device Tensix compute body, Ops.Pause is a bounded poll hint, and
// Ops.AuditAndResetCleanQueues verifies/rebases queues only after terminal
// role receipts. Physical owner, cache policy and all-fleet graph integration
// are not provided. Any failure retains the command unacknowledged.
template<class Ops> DataflowRoleCoordinatorResult DataflowRoleCoordinate(
    DataflowRoleMailbox& box,uint32_t sequence,unsigned polls,Ops& ops)
{
    if (!polls || polls>DataflowQueuePollLimit
        || !DataflowRoleMailboxIssue(box,sequence)) return DataflowRoleCoordinatorInvalid;
    const bool computeOkay=ops.RunCompute();
    for (unsigned attempt=0;attempt<polls;attempt++) {
        bool allTerminal=true,anyFailure=false;
        for (unsigned role=0;role<DataflowRoleCount;role++) {
            if (DataflowRoleMailboxReceipt(box,role,sequence,DataflowRoleFailed)) anyFailure=true;
            else if (!DataflowRoleMailboxReceipt(box,role,sequence,DataflowRoleComplete))
                allTerminal=false;
        }
        if (allTerminal) {
            if (!computeOkay || anyFailure || !ops.AuditAndResetCleanQueues())
                return DataflowRoleCoordinatorRetainedFault;
            return DataflowRoleMailboxAcknowledge(box,sequence,true,true)
                ?DataflowRoleCoordinatorComplete:DataflowRoleCoordinatorRetainedFault;
        }
        ops.Pause();
    }
    return DataflowRoleCoordinatorRetainedFault;
}
}
#endif
