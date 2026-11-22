/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_NOC_SAFETY_SAMPLE_H
#define PIONEER_QWEN35_DATAFLOW_NOC_SAFETY_SAMPLE_H
#include "../../../../../tools/pioneer/PioneerQwen35DataflowNocProtocol.h"

// The NIU registers remain readable with the tile clock disabled. With that
// clock disabled, touching L1 or a RISC register can wedge the NoC. FIFO
// requests are absent from the outstanding counters until they leave the
// FIFO, and an idle command register does not mean the FIFO is empty.
// The caller verifies the selected route and independently prevents another
// agent changing the clock/configuration after this observation.
// https://github.com/tenstorrent/tt-isa-documentation/blob/main/BlackholeA0/NoC/MemoryMap.md#niu-request-fifos
// This sample cannot prove absence of incoming traffic or counter overflow.
template<class Read>
bool PioneerQwen35SampleNocFifosAndClock(Read read)
{
    for (unsigned noc=0;noc<2;noc++) {
        const unsigned base=noc?0x10000:0;
        uint32_t observed=0;
        if (PioneerQwen35::DataflowNocCheckFifo(
                [&](unsigned offset) { return read(base+offset); },observed)
            !=PioneerQwen35::DataflowNocFifoReady) return false;
    }
    return true;
}
#endif
