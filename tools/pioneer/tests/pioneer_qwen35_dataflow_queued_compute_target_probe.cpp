/* SPDX-License-Identifier: MIT */
// Compile-only separate-role adapter. This does not install or launch a RISC.
#define PIONEER_QWEN35_QUEUED_COMPUTE 1
#include "../PioneerQwen35DataflowBlackholeIO.h"
#include "../PioneerQwen35DataflowTensixOps.h"
using namespace PioneerQwen35;
bool PioneerQwen35QueuedComputeTargetProbe(const DataflowCoreInvocation& program,
    DataflowQueueView (&queues)[DataflowCircularBufferCount])
{
    DataflowBlackholeTensixOps compute;
    return PioneerQwen35DataflowCompute(program,queues[0],queues[1],queues[2],compute);
}
