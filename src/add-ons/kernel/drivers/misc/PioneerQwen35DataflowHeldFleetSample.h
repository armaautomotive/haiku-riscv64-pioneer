/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_HELD_FLEET_SAMPLE_H
#define PIONEER_QWEN35_DATAFLOW_HELD_FLEET_SAMPLE_H

#include "../../../../../tools/pioneer/PioneerQwen35DataflowNocProtocol.h"
#include "../../../../../tools/pioneer/PioneerQwen35DataflowBarrierLayout.h"
#include "PioneerQwen35DataflowNocSafetySample.h"

// Observe only fixed status registers on an already-selected tile. The scope
// admits this during the installing service's held-worker callback. A quiet
// sample does not exclude incoming traffic or a later reset/control write.
template<class IO,class Scope,class Guard>
bool PioneerQwen35SampleSelectedHeldTile(IO& io,Scope& scope,Guard& guard,unsigned core)
{
    using namespace PioneerQwen35;
    if (!scope.ProveRoleStatusInspectionAllowed(io) || !guard.ProveSelectedRoute(core)
        || !PioneerQwen35SampleNocFifosAndClock([&](unsigned offset) { return io.ResidentNocRead(offset); })
        || io.ResetRead()!=0x47800 || io.ResetRead()!=0x47800) return false;
    for (unsigned noc=0;noc<2;noc++) {
        const unsigned base=noc?0x10000:0;
        uint32_t first=0,second=0;
        if (!DataflowNocDecodeLocalRoute(io.ResidentNocRead(base+DataflowNocLogicalIdOffset),first)
            || !DataflowNocDecodeLocalRoute(io.ResidentNocRead(base+DataflowNocLogicalIdOffset),second)
            || first!=second || !DataflowNocRouteMatchesWorker(first,core)) return false;
        for (unsigned i=0;i<4;i++)
            if (io.ResidentNocRead(base+0x40+i*0x800)) return false;
        for (unsigned i=0;i<32;i++)
            if (io.ResidentNocRead(base+0x240+i*4)&0xffU) return false;
    }
    return PioneerQwen35SampleNocFifosAndClock([&](unsigned offset) { return io.ResidentNocRead(offset); })
        && io.ResetRead()==0x47800 && scope.ProveRoleStatusInspectionAllowed(io)
        && guard.ProveSelectedRoute(core);
}

// The selected route is reprogrammed only under the borrowed service lock.
// ProveSelectedRoute establishes the exact initial two TLB configurations;
// reselecting the original core reconstructs them and is verified afterward.
// Any uncertainty after route selection requires the caller to retain recovery
// state. A finite number of MMIO operations does not bound a stuck PCI read.
template<class IO,class Scope,class Guard>
bool PioneerQwen35SampleHeldFleet(IO& io,Scope& scope,Guard& guard,
    unsigned& failedCore,bool& restoreFailed)
{
    using namespace PioneerQwen35;
    failedCore=DataflowBarrierWorkers; restoreFailed=false;
    const unsigned original=io.runtimeCore;
    if (!scope.ProveRoleStatusInspectionAllowed(io) || original>=DataflowBarrierWorkers
        || !guard.ProveSelectedRoute(original)) return false;
    bool okay=true,touched=false;
    for (unsigned pass=0;pass<2 && okay;pass++) {
        for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
            failedCore=core;
            if (!scope.ProveRoleStatusInspectionAllowed(io)) { okay=false; break; }
            // Selection may partially program the two TLBs even on failure.
            touched=true;
            // A failed selection can leave either TLB partly programmed. Do
            // not issue another selection over an unconfirmed write; retain
            // the session so recovery can deal with the uncertain route.
            if (!io.SelectRuntimeCore(core) || !io.DataflowPanelMappingStable()) {
                restoreFailed=true;return false;
            }
            if (!PioneerQwen35SampleSelectedHeldTile(io,scope,guard,core)) {
                okay=false; break;
            }
            io.Pause();
        }
    }
    if (touched) {
        if (!scope.ProveRoleStatusInspectionAllowed(io) || !io.SelectRuntimeCore(original)
            || !io.DataflowPanelMappingStable() || !scope.ProveRoleStatusInspectionAllowed(io)
            || !guard.ProveSelectedRoute(original)) restoreFailed=true;
    }
    if (restoreFailed || !okay) return false;
    failedCore=DataflowBarrierWorkers; return true;
}
#endif
