/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_ROLE_COMMAND_H
#define PIONEER_QWEN35_DATAFLOW_ROLE_COMMAND_H
#include "PioneerQwen35DataflowRoleMailbox.h"
namespace PioneerQwen35 {
enum DataflowRoleCommandResult {
    DataflowRoleCommandInvalid=0,
    DataflowRoleCommandComplete=1,
    DataflowRoleCommandBodyFailed=2,
    DataflowRoleCommandReceiptFailed=3
};
// A role calls this exactly once after seeing a command. Body is device-side;
// completion is published only after it returns. Failed or missing receipts
// are retained for the coordinator's separate settlement and recovery audit.
template<class Body> DataflowRoleCommandResult DataflowRoleRunCommand(
    DataflowRoleMailbox& box,unsigned role,uint32_t sequence,Body& body)
{
    if (!DataflowRoleMailboxBegin(box,role,sequence)) return DataflowRoleCommandInvalid;
    const bool success=body.Run();
    if (!DataflowRoleMailboxComplete(box,role,sequence,success))
        return DataflowRoleCommandReceiptFailed;
    return success?DataflowRoleCommandComplete:DataflowRoleCommandBodyFailed;
}
}
#endif
