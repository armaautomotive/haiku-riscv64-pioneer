/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowQueuedInput.h"
#include <cassert>
#include <cstdio>
using namespace PioneerQwen35;
int main()
{
    for (unsigned w=0;w<DataflowCircularBufferPages;w++)
        for (unsigned a=0;a<DataflowCircularBufferPages;a++)
            assert(DataflowQueuedInputPairValid(DataflowWeightBufferAddress+w*DataflowWeightBufferPageStride,
                DataflowInputBufferAddress+a*DataflowInputPageBytes)==(w==a));
    assert(!DataflowQueuedInputPairValid(0,0));
    assert(!DataflowQueuedInputPairValid(DataflowWeightBufferAddress+1,DataflowInputBufferAddress));
    assert(!DataflowQueuedInputPairValid(DataflowWeightBufferAddress,DataflowInputBufferAddress+1));
    assert(!DataflowQueuedInputPairValid(DataflowWeightBufferAddress,DataflowActivationCacheAddress));
    assert(!DataflowQueuedInputPairValid(DataflowWeightBufferAddress+2*DataflowWeightBufferPageStride,
        DataflowInputBufferAddress+2*DataflowInputPageBytes));
    std::puts("PASS queued Tensix input pages match the weight ticket and reject stale addresses; OFFLINE ONLY");
}
