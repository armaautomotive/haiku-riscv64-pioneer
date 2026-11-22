/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_ROLE_SERVICE_TICK_H
#define PIONEER_QWEN35_DATAFLOW_ROLE_SERVICE_TICK_H
#include "PioneerQwen35DataflowRoleCommand.h"
namespace PioneerQwen35 {
enum DataflowRoleTickResult {
    DataflowRoleTickInvalid=0,
    DataflowRoleTickIdle=1,
    DataflowRoleTickComplete=2,
    DataflowRoleTickWaitingAck=3,
    DataflowRoleTickFault=4,
    DataflowRoleTickWaitingRecovery=5,
    DataflowRoleTickStopRequested=PIONEER_Q35_ROLE_STOP_REQUESTED
};
struct DataflowRoleTickState { bool stopObserved=false,started=false; };
static_assert(sizeof(DataflowRoleTickState)<=sizeof(uint32_t),"startup shim clears one word");
// One bounded decision, with no wait or implicit retry. The future service
// loop owns its poll deadline and must park after StopRequested. A stop receipt
// alone does not establish a parked PC or quiet NoC; the physical owner must
// audit those facts independently before restoration.
template<class Body> DataflowRoleTickResult DataflowRoleServiceTick(
    DataflowRoleMailbox& box,unsigned role,DataflowRoleTickState& state,Body& body)
{
    if (role>=DataflowRoleCount || !DataflowRoleMailboxHeaderValid(box))
        return DataflowRoleTickInvalid;
    if (!state.started) {
        if (!DataflowRoleMailboxPublishStarted(box,role)) return DataflowRoleTickInvalid;
        state.started=true;
    }
    if (state.stopObserved) return DataflowRoleTickStopRequested;
    if (DataflowLoadAcquire(box.stopRequest)) {
        if (!DataflowRoleMailboxObserveStop(box,role)) return DataflowRoleTickInvalid;
        state.stopObserved=true;
        return DataflowRoleTickStopRequested;
    }
    const uint32_t ack=DataflowLoadAcquire(box.acknowledged);
    const uint32_t command=DataflowLoadAcquire(box.command);
    if (ack!=DataflowLoadAcquire(box.acknowledged)
        || command!=DataflowLoadAcquire(box.command)) return DataflowRoleTickIdle;
    if (command==ack) return DataflowRoleTickIdle;
    if (ack>=DataflowWorkerSequenceLimit || command!=ack+1) return DataflowRoleTickInvalid;
    if (DataflowRoleMailboxReceipt(box,role,command,DataflowRoleComplete))
        return DataflowRoleTickWaitingAck;
    if (DataflowRoleMailboxReceipt(box,role,command,DataflowRoleFailed))
        return DataflowRoleTickWaitingRecovery;
    const auto result=DataflowRoleRunCommand(box,role,command,body);
    if (result==DataflowRoleCommandComplete) return DataflowRoleTickComplete;
    if (result==DataflowRoleCommandBodyFailed) return DataflowRoleTickFault;
    return DataflowRoleTickInvalid;
}
}
#endif
