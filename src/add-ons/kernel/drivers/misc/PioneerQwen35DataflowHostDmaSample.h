/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_HOST_DMA_SAMPLE_H
#define PIONEER_QWEN35_DATAFLOW_HOST_DMA_SAMPLE_H
#include "BlackholeHostDmaAudit.h"
#include "PioneerQwen35DataflowHeldFleetSample.h"

// Point-in-time observation of firmware HDMA channel 0 after the service has
// held the fleet. This does not exclude later ARC commands, other NoC writers
// or a firmware reset, and does not authorize role memory writes or release.
// The borrowed scope owns the session lock. TLB200 is temporarily redirected
// to the fixed read-only PCIe HDMA aperture; every attempted change must be
// restored and checked before returning, or the owner must remain quarantined.
template<class IO,class Scope,class Guard>
bool PioneerQwen35SampleHostDma(IO& io,Scope& scope,Guard& guard,bool& restoreFailed)
{
    using namespace PioneerQwen35;
    restoreFailed=false;
    const unsigned original=io.runtimeCore;
    if (!scope.ProveRoleStatusInspectionAllowed(io) || !io.SharedHostDmaMapped()
        || original>=DataflowBarrierWorkers || !guard.ProveSelectedRoute(original)) return false;
    bool changedRoute=false,modified=false,sampled=false;
    uint32_t saved[2][3]{};
    if (original!=0) {
        changedRoute=true;
        if (!io.SelectRuntimeCore(0) || !io.DataflowPanelMappingStable()
            || !guard.ProveSelectedRoute(0)) {
            // The selection may have changed only one TLB, or the selected
            // route may no longer match its readback. Never overwrite that
            // uncertain state with a second selection in this transaction.
            restoreFailed=true;return false;
        }
    }
    if (!PioneerQwen35SampleSelectedHeldTile(io,scope,guard,0)
        || !io.SharedHostDmaPciSourceStable()) goto restore;
    for (unsigned slot=0;slot<2;slot++) for (unsigned word=0;word<3;word++)
        saved[slot][word]=io.DramTlbRead(slot,word);
    if (!guard.ProveSelectedRoute(0) || !io.SharedHostDmaPciSourceStable()) goto restore;
    modified=true; // even the first failed TLB write may have taken effect
    for (unsigned word=0;word<3;word++) {
        if (!scope.ProveRoleStatusInspectionAllowed(io) || io.runtimeCore!=0) goto restore;
        io.DramTlbWrite(0,word,BlackholeHostDbiTlbWord(word));
    }
    {
        auto stable=[&]() {
            if (!scope.ProveRoleStatusInspectionAllowed(io) || io.runtimeCore!=0
                || !io.DataflowPanelMappingStable() || !io.SharedHostDmaMapped()
                || !guard.ProvePinnedBar(io.profile.bar0_host_start,io.profile.bar0_host_end)) return false;
            for (unsigned word=0;word<3;word++)
                if (io.DramTlbRead(0,word)!=BlackholeHostDbiTlbWord(word)
                    || io.DramTlbRead(1,word)!=saved[1][word]) return false;
            return io.SharedHostDmaPciSourceStable();
        };
        uint32_t first[4]{};sampled=true;
        for (unsigned pass=0;pass<3 && sampled;pass++) {
            if (!stable()) { sampled=false;break; }
            for (unsigned word=0;word<4;word++) {
                const uint32_t value=io.HostHdmaRead(word);
                if ((word%2==0 && value!=0) || (word%2==1 && value!=0 && value!=3)
                    || (pass && value!=first[word])) sampled=false;
                if (!pass) first[word]=value;
            }
            if (!stable()) sampled=false;
            if (pass<2) io.Pause();
        }
    }
restore:
    if (modified) {
        if (!scope.ProveRoleStatusInspectionAllowed(io) || io.runtimeCore!=0) {
            restoreFailed=true;return false;
        }
        for (unsigned word=0;word<3;word++) {
            if (!scope.ProveRoleStatusInspectionAllowed(io) || io.runtimeCore!=0) {
                restoreFailed=true;return false;
            }
            io.DramTlbWrite(0,word,saved[0][word]);
        }
        for (unsigned slot=0;slot<2;slot++) for (unsigned word=0;word<3;word++)
            if (io.DramTlbRead(slot,word)!=saved[slot][word]) restoreFailed=true;
        if (!guard.ProveSelectedRoute(0)) restoreFailed=true;
    }
    if (changedRoute && !restoreFailed) {
        if (!scope.ProveRoleStatusInspectionAllowed(io) || !io.SelectRuntimeCore(original)
            || !io.DataflowPanelMappingStable() || !guard.ProveSelectedRoute(original))
            restoreFailed=true;
    }
    return sampled && !restoreFailed;
}
#endif
