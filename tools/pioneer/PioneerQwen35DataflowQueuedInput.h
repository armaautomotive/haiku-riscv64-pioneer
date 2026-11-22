/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_QUEUED_INPUT_H
#define PIONEER_QWEN35_DATAFLOW_QUEUED_INPUT_H

#include "PioneerQwen35DataflowProgram.h"
#include <stdint.h>

namespace PioneerQwen35 {
// Reader and compute exchange equally numbered tickets in separate queues.
// Each ticket owns the same page index in both buffers until compute pops it.
// Address validation does not itself prove publication, completion or safety.
inline bool DataflowQueuedInputPairValid(uintptr_t weight,uintptr_t input)
{
    for (unsigned page=0;page<DataflowCircularBufferPages;page++) {
        if (weight==DataflowWeightBufferAddress+page*DataflowWeightBufferPageStride
            && input==DataflowInputBufferAddress+page*DataflowInputPageBytes)
            return true;
    }
    return false;
}
}
#endif
