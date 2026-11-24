/* SPDX-License-Identifier: MIT */
// Exercise the exact extracted service-unsafe guard with simulated state.
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#ifdef TEST_SNAPSHOT_SCRATCH_HOLD
#define PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS 1
#endif
#include <cassert>
#include <cstdio>

struct Owner {
    bool active=false,retained=false;
    bool Active() const { return active; }
    bool Retained() const { return retained; }
};
struct Service {
    bool started=true,closed=false,ready=false,graph=false,fault=false,held=false;
    bool Started() const { return started; }
    bool Closed() const { return closed; }
    bool Ready() const { return ready; }
    bool GraphRunning() const { return graph; }
    bool Faulted() const { return fault; }
    bool ScratchHeld() const { return held; }
};
static bool Unsafe(Service* qwen35DataflowService,const Owner& qwen35QualificationOwner,
    bool qwen35QualificationInternalActive,bool controlledQ35Graph=false,bool controlledQ35Fault=false) {
#ifndef TEST_SNAPSHOT_SCRATCH_HOLD
    (void)qwen35QualificationOwner;(void)qwen35QualificationInternalActive;
#endif
#include "pioneer_q35_projection_service_unsafe_guard.inc"
    return false;
}
int main() {
    Owner owner;Service service;
    assert(!Unsafe(nullptr,owner,false) && Unsafe(&service,owner,false));
    service.ready=true;assert(Unsafe(&service,owner,false) && !Unsafe(&service,owner,false,true));
    service.ready=false;service.fault=true;assert(!Unsafe(&service,owner,false,false,true));
    service.fault=false;service.held=true;
    assert(Unsafe(&service,owner,false) && Unsafe(&service,owner,true));
    owner.active=true;
#ifdef TEST_SNAPSHOT_SCRATCH_HOLD
    assert(Unsafe(&service,owner,false) && !Unsafe(&service,owner,true));
    owner.retained=true;assert(Unsafe(&service,owner,true));owner.retained=false;
    service.held=false;assert(Unsafe(&service,owner,true));service.held=true;
#else
    assert(Unsafe(&service,owner,true));
#endif
    service.closed=true;assert(!Unsafe(&service,owner,false));
    std::puts("PASS held scratch exception is limited to the active internal owner; SIMULATED ONLY");
}
