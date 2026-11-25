/* SPDX-License-Identifier: MIT */
// Exercise the exact extracted service-unsafe guard with simulated state.
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#ifdef TEST_RESIDENT_CLOSE_SCOPE
#define PIONEER_QWEN35_RESIDENT_SERVICE_WITNESS 1
#endif
#ifdef TEST_SNAPSHOT_SCRATCH_HOLD
#define PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS 1
#endif
#ifdef TEST_QUALIFICATION_CAPTURE
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS 1
#define PIONEER_QWEN35_RECOVERY_RUN_ENABLE 1
#endif
#include <cassert>
#include <cstdio>

struct Owner {
    bool active=false,retained=false,scratch=true;
    bool Active() const { return active; }
    bool Retained() const { return retained; }
    const void* Scratch() const { return scratch ? this : nullptr; }
};
struct Service {
    bool started=true,closed=false,ready=false,graph=false,fault=false,held=false,capture=false;
#ifdef TEST_RESIDENT_CLOSE_SCOPE
    const void* resident=nullptr;
    bool ResidentSessionIdle(const void* session) const { return session && session==resident; }
#endif
    bool Started() const { return started; }
    bool Closed() const { return closed; }
    bool Ready() const { return ready; }
    bool GraphRunning() const { return graph; }
    bool Faulted() const { return fault; }
    bool ScratchHeld() const { return held; }
    bool SharedRoleCapturePending() const { return capture; }
};
static bool Unsafe(Service* qwen35DataflowService,const Owner& qwen35QualificationOwner,
    bool qwen35QualificationInternalActive,bool controlledQ35Graph=false,bool controlledQ35Fault=false
#ifdef TEST_RESIDENT_CLOSE_SCOPE
    ,const void* controlledQ35ResidentSession=nullptr
#endif
#ifdef TEST_QUALIFICATION_CAPTURE
    ,const Service* controlledQ35HeldObservation=nullptr,bool hasBinding=true,bool hasMemory=true
#endif
    ) {
#ifndef TEST_SNAPSHOT_SCRATCH_HOLD
    (void)qwen35QualificationOwner;(void)qwen35QualificationInternalActive;
#endif
#ifdef TEST_QUALIFICATION_CAPTURE
    const void* qwen35SharedRoleBinding=hasBinding ? &qwen35QualificationOwner : nullptr;
    const void* qwen35DataflowMemory=hasMemory ? &qwen35QualificationOwner : nullptr;
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
#ifdef TEST_RESIDENT_CLOSE_SCOPE
    service.held=false;int session=0,wrong=0;service.resident=&session;
    assert(Unsafe(&service,owner,false) && !Unsafe(&service,owner,false,false,false,&session)
        && Unsafe(&service,owner,false,false,false,&wrong));
    service.resident=nullptr;assert(Unsafe(&service,owner,false,false,false,&session));
#endif
#ifdef TEST_QUALIFICATION_CAPTURE
    service.held=false;service.capture=true;Service other;
    assert(Unsafe(&service,owner,true));
#ifdef TEST_RESIDENT_CLOSE_SCOPE
    auto captureUnsafe=[&](const Service* selected,bool binding=true,bool memory=true) {
        return Unsafe(&service,owner,true,false,false,nullptr,selected,binding,memory);
    };
#else
    auto captureUnsafe=[&](const Service* selected,bool binding=true,bool memory=true) {
        return Unsafe(&service,owner,true,false,false,selected,binding,memory);
    };
#endif
    assert(!captureUnsafe(&service) && captureUnsafe(nullptr) && captureUnsafe(&other)
        && captureUnsafe(&service,false,true) && captureUnsafe(&service,true,false));
    owner.scratch=false;assert(captureUnsafe(&service));owner.scratch=true;
    owner.retained=true;assert(captureUnsafe(&service));owner.retained=false;
    owner.active=false;assert(captureUnsafe(&service));owner.active=true;
    service.capture=false;assert(captureUnsafe(&service));service.capture=true;
#endif
    service.closed=true;assert(!Unsafe(&service,owner,false));
    std::puts("PASS held scratch exception is limited to the active internal owner; SIMULATED ONLY");
}
