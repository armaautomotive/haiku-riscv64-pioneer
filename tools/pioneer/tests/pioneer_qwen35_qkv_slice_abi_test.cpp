/* SPDX-License-Identifier: MIT */
// ABI used by the offline QKV slice auditor; no card access or authority.
#include <stddef.h>
#include "../PioneerQwen35DataflowDeviceStream.h"

using namespace PioneerQwen35;
static_assert(sizeof(DataflowStreamRecord)==2048 && offsetof(DataflowStreamRecord,payload)==64,
    "record payload must match the simulator slice reader");
static_assert(sizeof(RowSpace)==128 && offsetof(DataflowCoreInvocation,output)==440,
    "projection output must match the simulator slice reader");
int main() { return 0; }
