/* SPDX-License-Identifier: MIT */
// Freestanding compile probe only; does not install or launch a device image.
#include "../PioneerQwen35DataflowRmsSave.h"
extern "C" bool q35_rms_save_probe(const PioneerQwen35::DataflowRmsSaveInvocation* invocation,
    unsigned sequence,float* cache,float* input,float* weight,float* output) {
    if (!invocation) return false;
    PioneerQwen35::DataflowBlackholeRmsSaveOps ops;
    return ops.Bind(*invocation,sequence)
        && PioneerQwen35::PioneerQwen35DataflowRmsSaveWorker(*invocation,sequence,
            cache,input,weight,output,ops);
}
