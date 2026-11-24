/* SPDX-License-Identifier: MIT */
// Exercise the actual restricted ioctl branch with a simulated locked owner.
#define B_DEVICE_OP_CODES_END 1000
#define PIONEER_DRIVER_BUILD_TAG UINT64_C(0x2026112400000001)
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35ProjectionQualificationProtocol.h"
#include <cassert>
#include <cstdio>
#include <cstring>
using status_t=int;
constexpr status_t B_OK=0,B_BAD_VALUE=-1,B_NOT_ALLOWED=-2,B_DEV_NOT_READY=-3;
struct PioneerResidentBanks { static constexpr uint64_t Total=4096; };
struct tt_resident_session_result {};
struct QualificationOwner {
    bool active=false,retained=false;
    bool Active() const { return active; }
    void Retain() { if (active) retained=true; }
};
static unsigned prepareCalls=0,runCalls=0,retains=0,locks=0,unlocks=0;
static bool attachAllowed=true,detachAllowed=true,prepareAllowed=true,unsafeAfterRun=false;
static unsigned runMode=0;
struct PioneerStoreContext;
struct PioneerQ35ProjectionQualificationScope {
    explicit PioneerQ35ProjectionQualificationScope(PioneerStoreContext&) {}
};
struct PioneerStoreContext {
    bool bankStorageOnly=true,unsafe=false;
    uint64_t bytes=PioneerResidentBanks::Total;
    unsigned uploadedPages=0;
    void *q38Dispatch=nullptr,*qwen35DataflowService=nullptr,*qwen35DataflowMemory=nullptr;
    void *qwen35SharedRoleBinding=nullptr,*qwen35GraphOwner=nullptr;
    QualificationOwner qwen35QualificationOwner;
    bool Unsafe() const { return unsafe; }
    bool RunDataflowQualificationTransaction(PioneerQ35ProjectionQualificationScope&,
        uint32_t& stage,uint32_t& routeStage,uint32_t& routeCore,bool& output,bool& cleaned) {
        ++runCalls;stage=(runMode==0 || runMode==3)?14:runMode==1?0:runMode==4?2:6;routeStage=2;routeCore=7;
        output=runMode==0;cleaned=runMode==0 || runMode==3 || runMode==4;
        qwen35QualificationOwner.active=runMode==2;
        unsafe=unsafeAfterRun;
        return runMode==0;
    }
};
struct PioneerProbeCookie { PioneerStoreContext* context=nullptr; };
static PioneerProbeCookie* sRuntimeOwner=nullptr;
static uint64_t sReloadGeneration=3;
static bool sColdRecoveryRequired=false;
static int sLock=1;
static status_t user_memcpy(void* out,const void* in,size_t n) { std::memcpy(out,in,n);return B_OK; }
static status_t AcquireProbeLock(PioneerProbeCookie*,bool) { ++locks;return B_OK; }
static status_t release_sem(int) { ++unlocks;return B_OK; }
struct PioneerQwen35RoleBorrowedScopeWitness {
    PioneerProbeCookie* cookie;PioneerStoreContext* context;
    PioneerQwen35RoleBorrowedScopeWitness(PioneerProbeCookie* c,PioneerStoreContext* p):cookie(c),context(p) {}
    bool Attach() { return attachAllowed && cookie && context; }
    bool Detach() { return detachAllowed; }
};
static bool PrepareQ38Dispatch(PioneerStoreContext& c) {
    ++prepareCalls;if (prepareAllowed) c.q38Dispatch=&c;return prepareAllowed;
}
static void RetainQ38DispatchOwner(PioneerStoreContext&,tt_resident_session_result&) {
    ++retains;sColdRecoveryRequired=true;
}
static status_t Control(PioneerProbeCookie* cookie,uint32_t operation,void* data,size_t length) {
    status_t status=B_OK;
#include "pioneer_q35_projection_control.inc"
    return B_NOT_ALLOWED;
}
static tt_q35_projection_qualification Request() {
    tt_q35_projection_qualification r{};
    r.version=1;r.expected_tag=PIONEER_DRIVER_BUILD_TAG;r.expected_generation=sReloadGeneration;
    return r;
}
static void Reset(PioneerProbeCookie& cookie,PioneerStoreContext& context) {
    cookie.context=&context;sRuntimeOwner=&cookie;sColdRecoveryRequired=false;
    prepareCalls=runCalls=retains=locks=unlocks=0;
    attachAllowed=detachAllowed=prepareAllowed=true;unsafeAfterRun=false;runMode=0;
}
int main() {
    {
        PioneerStoreContext c;PioneerProbeCookie cookie;Reset(cookie,c);
        auto r=Request();r.reserved=1;
        assert(Control(&cookie,TT_PCI_PROBE_Q35_PROJECTION_QUALIFY,&r,sizeof(r))==B_BAD_VALUE && !locks);
        r=Request();r.expected_tag++;
        assert(Control(&cookie,TT_PCI_PROBE_Q35_PROJECTION_QUALIFY,&r,sizeof(r))==B_OK
            && r.operation_status==B_BAD_VALUE && !prepareCalls && locks==1 && unlocks==1);
    }
    {
        PioneerStoreContext c;PioneerProbeCookie cookie;Reset(cookie,c);c.uploadedPages=1;
        auto r=Request();
        assert(Control(&cookie,TT_PCI_PROBE_Q35_PROJECTION_QUALIFY,&r,sizeof(r))==B_OK
            && r.operation_status==B_NOT_ALLOWED && !prepareCalls && !sColdRecoveryRequired);
    }
    {
        PioneerStoreContext c;PioneerProbeCookie cookie;Reset(cookie,c);attachAllowed=false;
        auto r=Request();
        assert(Control(&cookie,TT_PCI_PROBE_Q35_PROJECTION_QUALIFY,&r,sizeof(r))==B_OK
            && r.operation_status==B_NOT_ALLOWED && !prepareCalls && !retains);
    }
    {
        PioneerStoreContext c;PioneerProbeCookie cookie;Reset(cookie,c);prepareAllowed=false;
        auto r=Request();
        assert(Control(&cookie,TT_PCI_PROBE_Q35_PROJECTION_QUALIFY,&r,sizeof(r))==B_OK
            && r.operation_status==B_DEV_NOT_READY && r.stage==1 && retains==1 && sColdRecoveryRequired);
    }
    for (unsigned mode=0;mode<=4;mode++) {
        PioneerStoreContext c;PioneerProbeCookie cookie;Reset(cookie,c);runMode=mode;
        auto r=Request();
        assert(Control(&cookie,TT_PCI_PROBE_Q35_PROJECTION_QUALIFY,&r,sizeof(r))==B_OK
            && prepareCalls==1 && runCalls==1 && locks==1 && unlocks==1
            && r.build_tag==PIONEER_DRIVER_BUILD_TAG && r.generation==3);
        assert((r.operation_status==B_OK)==(mode==0));
        assert(r.output_verified==(mode==0) && r.cleanup_complete==(mode==0 || mode==3 || mode==4));
        if (mode==4) assert(r.stage==3 && r.operation_status==B_DEV_NOT_READY);
        assert((retains==1)==(mode==2) && sColdRecoveryRequired==(mode==2));
        if (mode==2) assert(c.qwen35QualificationOwner.retained);
    }
    {
        PioneerStoreContext c;PioneerProbeCookie cookie;Reset(cookie,c);runMode=1;unsafeAfterRun=true;
        auto r=Request();
        assert(Control(&cookie,TT_PCI_PROBE_Q35_PROJECTION_QUALIFY,&r,sizeof(r))==B_OK
            && r.operation_status==B_DEV_NOT_READY && retains==1 && sColdRecoveryRequired);
    }
    {
        PioneerStoreContext c;PioneerProbeCookie cookie;Reset(cookie,c);runMode=4;unsafeAfterRun=true;
        auto r=Request();
        assert(Control(&cookie,TT_PCI_PROBE_Q35_PROJECTION_QUALIFY,&r,sizeof(r))==B_OK
            && r.operation_status==B_DEV_NOT_READY && !r.cleanup_complete && retains==1 && sColdRecoveryRequired);
    }
    {
        PioneerStoreContext c;PioneerProbeCookie cookie;Reset(cookie,c);unsafeAfterRun=true;
        auto r=Request();
        assert(Control(&cookie,TT_PCI_PROBE_Q35_PROJECTION_QUALIFY,&r,sizeof(r))==B_OK
            && r.operation_status==B_DEV_NOT_READY && r.output_verified && !r.cleanup_complete
            && retains==1 && sColdRecoveryRequired);
    }
    {
        PioneerStoreContext c;PioneerProbeCookie cookie;Reset(cookie,c);detachAllowed=false;
        auto r=Request();
        assert(Control(&cookie,TT_PCI_PROBE_Q35_PROJECTION_QUALIFY,&r,sizeof(r))==B_NOT_ALLOWED
            && retains==1 && sColdRecoveryRequired && unlocks==1);
    }
    std::puts("PASS actual fixed projection control: admission, verified result and retained faults; SIMULATED ONLY");
}
