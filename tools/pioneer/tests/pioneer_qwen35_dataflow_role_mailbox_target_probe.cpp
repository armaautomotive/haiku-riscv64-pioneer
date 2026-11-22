/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowRoleServiceTick.h"
using namespace PioneerQwen35;
// Compile-only handoff; this is not a processor launcher or a device image.
bool PioneerQwen35RoleMailboxTargetProbe(DataflowRoleMailbox& mailbox,
    unsigned role,unsigned sequence,bool success)
{
    return DataflowRoleMailboxIssue(mailbox,sequence)
        && DataflowRoleMailboxBegin(mailbox,role,sequence)
        && DataflowRoleMailboxComplete(mailbox,role,sequence,success)
        && DataflowRoleMailboxAcknowledge(mailbox,sequence,true,true);
}
bool PioneerQwen35RoleMailboxStopTargetProbe(DataflowRoleMailbox& mailbox,
    unsigned role,bool fault)
{
    const bool requested=fault
        ?DataflowRoleMailboxRequestFaultStop(mailbox,true,true,true)
        :DataflowRoleMailboxRequestStop(mailbox);
    return requested && DataflowRoleMailboxObserveStop(mailbox,role)
        && DataflowRoleMailboxStopReceipts(mailbox);
}

struct RoleTickProbeBody { bool Run() { return false; } };
unsigned PioneerQwen35RoleTickTargetProbe(DataflowRoleMailbox& mailbox,
    unsigned role,DataflowRoleTickState& state)
{
    RoleTickProbeBody body;
    return DataflowRoleServiceTick(mailbox,role,state,body);
}
