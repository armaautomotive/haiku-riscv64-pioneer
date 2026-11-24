/* SPDX-License-Identifier: MIT */
// RV32 compile-only composition of the real worker mailbox, NoC transport
// and resident lifecycle. This is not a runnable graph or physical authority.
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS 1
#include "../PioneerQwen35DataflowResidentWorker.h"
extern "C" unsigned qwen35_resident_lifecycle_target_probe(unsigned core,
    PioneerQwen35::DataflowWorkerMailbox& box,const PioneerQwen35::DataflowRoleMailbox& roles,
    PioneerQwen35::DataflowQueueControl* queues,const PioneerQwen35::DataflowResidentCommand& command,
    const PioneerQwen35::DataflowGraphRunDescriptor& installed,
    PioneerQwen35::DataflowGraphRunDescriptor& expanded) {
    using namespace PioneerQwen35;
    DataflowResidentWorkerIO io(core,box,roles,queues);
    DataflowResidentLifecycle<DataflowResidentWorkerIO> lifecycle(io);
    if (lifecycle.Start(core,command,installed,box.completedSequence,0,0,expanded)
        !=DataflowResidentLifecycleRun) return DataflowResidentLifecycleRetained;
    if (lifecycle.GraphDone(true)!=DataflowResidentLifecyclePending) return DataflowResidentLifecycleRetained;
    for (unsigned poll=0;poll<=command.pollLimit;poll++) {
        const auto state=lifecycle.Tick();
        if (state!=DataflowResidentLifecyclePending) return state;
    }
    return DataflowResidentLifecycleRetained;
}
