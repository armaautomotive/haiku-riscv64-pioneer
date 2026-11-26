/* SPDX-License-Identifier: MIT */
// ABI used by the offline QKV slice auditor; no card access or authority.
#include <stddef.h>
#include "../PioneerQwen35DataflowDeviceStream.h"

using namespace PioneerQwen35;
static_assert(sizeof(DataflowStreamRecord)==2048 && offsetof(DataflowStreamRecord,payload)==64,
    "record payload must match the simulator slice reader");
static_assert(sizeof(RowSpace)==128 && offsetof(DataflowCoreInvocation,output)==440,
    "projection output must match the simulator slice reader");
static_assert(offsetof(DataflowConvInvocation,qkv)==84,
    "convolution in-place output must match the simulator slice reader");
static_assert(offsetof(DataflowDeltaShardInvocation,program)==28
    && offsetof(DataflowDeltaProgram,qkv)==64
    && offsetof(DataflowDeltaProgram,alpha)==320
    && offsetof(DataflowDeltaProgram,beta)==448,
    "recurrent inputs must match the simulator slice reader");
int main() { return 0; }
