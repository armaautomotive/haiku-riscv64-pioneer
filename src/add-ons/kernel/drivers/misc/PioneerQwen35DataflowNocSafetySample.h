/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_NOC_SAFETY_SAMPLE_H
#define PIONEER_QWEN35_DATAFLOW_NOC_SAFETY_SAMPLE_H
#include "../../../../../tools/pioneer/PioneerQwen35DataflowNocProtocol.h"

// Execute a selected worker's first RISC/L1 audit only after both NIUs show
// the expected route and an enabled tile clock. NIU registers themselves
// remain accessible while the attached tile is clock-disabled. The caller
// must serialize TLB selection and prevent concurrent clock changes.
// https://github.com/tenstorrent/tt-isa-documentation/blob/main/BlackholeA0/NoC/MemoryMap.md#tile-clock-disable
template<class IO,class Work>
bool PioneerQwen35WithSelectedClock(IO& io,unsigned core,Work work)
{
    using namespace PioneerQwen35;
    if (core>=120 || io.runtimeCore!=core || !io.DataflowPanelMappingStable()) return false;
    for (unsigned noc=0;noc<2;noc++) {
        const unsigned base=noc?0x10000:0;
        uint32_t first=0,second=0;
        if (!DataflowNocDecodeLocalRoute(io.ResidentNocRead(base+DataflowNocLogicalIdOffset),first)
            || !DataflowNocRouteMatchesWorker(first,core)) return false;
        const uint32_t config=io.ResidentNocRead(base+0x100);
        if (config==UINT32_MAX || (config&(1U<<12))) return false;
        io.Pause();
        const uint32_t repeat=io.ResidentNocRead(base+0x100);
        if (repeat==UINT32_MAX || (repeat&(1U<<12))
            || !DataflowNocDecodeLocalRoute(io.ResidentNocRead(base+DataflowNocLogicalIdOffset),second)
            || first!=second) return false;
    }
    if (io.runtimeCore!=core || !io.DataflowPanelMappingStable()) return false;
    return work();
}

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
