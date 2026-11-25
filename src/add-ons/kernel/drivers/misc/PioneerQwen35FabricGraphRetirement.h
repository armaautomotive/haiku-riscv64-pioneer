/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_FABRIC_GRAPH_RETIREMENT_H
#define PIONEER_QWEN35_FABRIC_GRAPH_RETIREMENT_H
#include <stdint.h>

// Record that the retained graph and its bank claim can participate in final
// context destruction. Never free them separately: the bank's sticky owner
// points into the graph. The caller holds the lock and supplies a metadata-only
// guard for the exact ticket, reservation and continuing physical exclusion.
// An interrupted publication is retained and is not automatically replayed.
template<class Context,class Check>
bool PioneerQ35RecordFabricGraphRetirement(Context& context,uint64_t ticket,Check& check)
{
    auto* graph=context.qwen35GraphOwner;
    if (!graph || !ticket || context.recoveryFreshSession || context.qwen35SharedRoleBinding
        || context.recoveryFabricGraphRetirementAttempted || context.recoveryFabricGraphRetirementRecorded)
        return false;
    auto ready=[&]() {
        return context.qwen35GraphOwner==graph && !context.recoveryFreshSession
            && !context.qwen35SharedRoleBinding && graph->FabricContextRetirementReady(ticket);
    };
    if (!check() || !ready() || !check() || !ready()) return false;
    context.recoveryFabricGraphRetirementAttempted=true;
    if (!check() || !ready()) return false;
    context.recoveryFabricGraphRetirementRecorded=true;
    if (!check() || !ready()) {
        context.recoveryFabricGraphRetirementRecorded=false;return false;
    }
    return true;
}
#endif
