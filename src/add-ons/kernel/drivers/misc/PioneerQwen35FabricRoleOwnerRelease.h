/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_FABRIC_ROLE_OWNER_RELEASE_H
#define PIONEER_QWEN35_FABRIC_ROLE_OWNER_RELEASE_H
#include <stdint.h>

// CPU-memory teardown only. The caller holds the recovery lock; check is a
// metadata-only proof of the exact guard, retained PCI reservation and
// continuing device/firmware exclusion. A successful release does not
// release the graph, service, bank claim, reservation or cold latch.
// After any failure the caller retains the context and must not replay an
// attempted destruction. If the final check fails, the role is already gone.
template<class Context,class Check>
bool PioneerQ35ReleaseFabricRoleOwner(Context& context,uint64_t ticket,Check& check)
{
    auto* role=context.qwen35SharedRoleBinding;
    auto* graph=context.qwen35GraphOwner;
    if (!role || !graph || !ticket || context.recoveryFreshSession) return false;
    auto before=[&]() {
        return context.qwen35SharedRoleBinding==role && context.qwen35GraphOwner==graph
            && !context.recoveryFreshSession && check()
            && context.qwen35SharedRoleBinding==role && context.qwen35GraphOwner==graph
            && !context.recoveryFreshSession;
    };
    if (!before() || !role->BeginFabricDestroy(ticket,before) || !before()) return false;
    delete role;
    context.qwen35SharedRoleBinding=nullptr;
    return !context.recoveryFreshSession && context.qwen35GraphOwner==graph
        && check() && !context.recoveryFreshSession && context.qwen35GraphOwner==graph
        && !context.qwen35SharedRoleBinding;
}
#endif
