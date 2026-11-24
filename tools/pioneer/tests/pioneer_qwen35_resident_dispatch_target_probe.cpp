/* SPDX-License-Identifier: MIT */
// Compile the real Blackhole NoC fanout and descriptor expansion for RV32.
// This object is not linked into a runnable service until its lifecycle and
// host submission path are connected and qualified.
#include "../PioneerQwen35DataflowResidentDispatch.h"
extern "C" bool qwen35_resident_dispatch_target_probe(unsigned core,
    const PioneerQwen35::DataflowResidentCommand& command,
    const PioneerQwen35::DataflowGraphRunDescriptor& installed,
    unsigned completed,unsigned priorEpoch,unsigned priorLastStep,
    PioneerQwen35::DataflowGraphRunDescriptor& expanded) {
    using namespace PioneerQwen35;
    if (!DataflowResidentCommandExpand(command,installed,core,completed,priorEpoch,priorLastStep,expanded))
        return false;
    DataflowBlackholeBarrierIO io(core);DataflowResidentFanout fanout;
    return fanout.Send(core,command,io) && fanout.Complete();
}
