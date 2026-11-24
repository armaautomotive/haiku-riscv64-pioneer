/* SPDX-License-Identifier: MIT */
// Exercises the real context transaction with a simulated owner and mapping.
#include <cassert>
#include <cstdio>
#include <new>
#include <vector>
#include <cstdint>

#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define PIONEER_QWEN35_RECOVERY_RUN_ENABLE 1
#define PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS 1
using uint64=uint64_t;
enum { B_OK=0 };
struct tt_arc_scratch_result {};
struct tt_arc_queue_result {};
struct PioneerPostResetAuditResult {};
struct PioneerQ35PostResetL2Result {};
static unsigned failOperation=0,validations=0,failValidation=0;
static std::vector<unsigned> trace;
static bool Step(unsigned value) { trace.push_back(value);return failOperation!=value; }
static int Validate(uint64 ticket) {
    ++validations;return ticket==123 && validations!=failValidation?B_OK:-1;
}
struct pioneer_driver_recovery_guard_v2_info { int (*validate)(uint64); };
struct PioneerPostResetMapping {
    bool q35CloseComplete=false;
    bool Audit(PioneerPostResetAuditResult&) { return Step(12); }
    template<class Check> bool AuditQ35Fabric(PioneerPostResetAuditResult& result,Check&) { return Audit(result); }
    bool AuditQ35L2CpuHeld(PioneerQ35PostResetL2Result&) { return Step(13); }
    template<class Check> bool AuditQ35L2CpuHeldGuarded(PioneerQ35PostResetL2Result& result,Check&) {
        return AuditQ35L2CpuHeld(result);
    }
    bool AuditHostDma() { return Step(14); }
    template<class Check> bool AuditQ35HostDmaGuarded(Check&) { return AuditHostDma(); }
    template<class Check> bool CloseQ35Guarded(Check&) { return q35CloseComplete=Step(16); }
};
template<class Guard> class PioneerQ35FabricArcOwner {
public:
    enum Phase { Fresh,Retired };
    Phase phase=Fresh;unsigned clock=0,send=0,reset=0;
    PioneerQ35FabricArcOwner(PioneerPostResetMapping&,uint64 ticket) { assert(ticket==123); }
    template<class Scope> bool Open(Guard*,Scope&) { return Step(1); }
    template<class Scope> bool CheckClock(Guard*,Scope&,const tt_arc_scratch_result&,tt_arc_queue_result&) {
        const unsigned stages[]={2,4,11};assert(clock<3);return Step(stages[clock++]);
    }
    template<class Scope> bool SendNext(Guard*,Scope&,const tt_arc_scratch_result&,tt_arc_queue_result&) {
        const unsigned stages[]={3,6,7,10};assert(send<4);return Step(stages[send++]);
    }
    template<class Scope> bool SetRiscReset(Guard*,Scope&,bool hold) {
        assert(hold==(reset==0));return Step(reset++?9:5);
    }
    template<class Scope> bool HoldAllTiles(Guard*,Scope&) { return Step(8); }
    template<class Scope> bool Retire(Guard*,Scope&) { if (!Step(15)) return false;phase=Retired;return true; }
    Phase CurrentPhase() const { return phase; }
};
struct Context;
struct Cookie { Context* context=nullptr; };
static Cookie* sRuntimeOwner=nullptr;
static bool sColdRecoveryRequired=true;
struct Context {
    PioneerPostResetMapping* recoveryMapping=nullptr;
    PioneerQ35FabricArcOwner<pioneer_driver_recovery_guard_v2_info>* recoveryFabricArcOwner=nullptr;
    bool dispatchOwnerRetained=true,q35PciReservationHeld=true,q35PciReleaseAttempted=false;
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35FabricRecoveryTransaction.inc"
    ~Context() { delete recoveryFabricArcOwner; }
};
struct Scope {
    bool admission=true;unsigned admissionCalls=0,deniedStage=0,deniedCall=0,loseOwnerStage=0,losePciStage=0,calls[18]{};
    bool FabricRecoveryAdmission(Context&,pioneer_driver_recovery_guard_v2_info*,uint64) { ++admissionCalls;return admission; }
    bool FabricRecoveryStageAllowed(Context& context,const PioneerQ35FabricArcOwner<pioneer_driver_recovery_guard_v2_info>&,
        pioneer_driver_recovery_guard_v2_info*,uint64,unsigned stage) {
        assert(stage<=17);if (stage==loseOwnerStage) context.dispatchOwnerRetained=false;
        if (stage==losePciStage) context.q35PciReservationHeld=false;
        return ++calls[stage]!=deniedCall || stage!=deniedStage;
    }
};
struct Fixture {
    PioneerPostResetMapping mapping;Context context;Cookie cookie;Scope scope;
    pioneer_driver_recovery_guard_v2_info guard{Validate};
    tt_arc_scratch_result firmware;tt_arc_queue_result queue;
    PioneerPostResetAuditResult fleet;PioneerQ35PostResetL2Result l2;
    unsigned stage=99;
    Fixture() {
        trace.clear();failOperation=validations=failValidation=0;sColdRecoveryRequired=true;
        context.recoveryMapping=&mapping;cookie.context=&context;sRuntimeOwner=&cookie;
    }
    bool Run(uint64 ticket=123) {
        return context.RunQ35FabricRecovery(&guard,ticket,firmware,queue,fleet,l2,scope,stage);
    }
};
int main()
{
    {
        Fixture f;assert(f.Run() && f.stage==17 && f.context.recoveryFabricArcOwner && f.mapping.q35CloseComplete
            && f.context.recoveryFabricArcOwner->CurrentPhase()==PioneerQ35FabricArcOwner<pioneer_driver_recovery_guard_v2_info>::Retired
            && sColdRecoveryRequired && f.context.recoveryMapping==&f.mapping);
        for (unsigned i=1;i<=16;i++) assert(trace[i-1]==i);
        assert(trace.size()==16 && !f.Run() && trace.size()==16);
    }
    for (unsigned failure=1;failure<=16;failure++) {
        Fixture f;failOperation=failure;
        assert(!f.Run() && f.stage==failure && f.context.recoveryFabricArcOwner && sColdRecoveryRequired);
        assert(trace.size()==failure && trace.back()==failure);
        assert(!f.Run() && trace.size()==failure);
    }
    for (unsigned stage=1;stage<=17;stage++) for (unsigned call=1;call<=2;call++) {
        if (stage==17 && call==2) continue;
        Fixture f;f.scope.deniedStage=stage;f.scope.deniedCall=call;
        assert(!f.Run() && f.stage==stage && f.context.recoveryFabricArcOwner && sColdRecoveryRequired);
        const unsigned expected=stage==17?16:stage-1+(call==2);
        assert(trace.size()==expected && !f.Run() && trace.size()==expected);
    }
    {
        Fixture f;f.scope.admission=false;assert(!f.Run() && !f.context.recoveryFabricArcOwner && trace.empty());
        f.scope.admission=true;const unsigned admissions=f.scope.admissionCalls;
        assert(!f.Run(124) && !f.context.recoveryFabricArcOwner && trace.empty()
            && f.scope.admissionCalls==admissions);
        sColdRecoveryRequired=false;assert(!f.Run() && !f.context.recoveryFabricArcOwner && trace.empty());
    }
    {
        Fixture f;failValidation=4;
        assert(!f.Run() && f.context.recoveryFabricArcOwner && sColdRecoveryRequired);
        const auto calls=trace.size();assert(!f.Run() && trace.size()==calls);
    }
    {
        Fixture f;f.scope.loseOwnerStage=3;
        assert(!f.Run() && f.stage==3 && f.context.recoveryFabricArcOwner && trace.size()==2);
        f.context.dispatchOwnerRetained=true;
        assert(!f.Run() && trace.size()==2);
    }
    {
        Fixture f;f.scope.losePciStage=3;
        assert(!f.Run() && f.stage==3 && f.context.recoveryFabricArcOwner && trace.size()==2);
        f.context.q35PciReservationHeld=true;assert(!f.Run() && trace.size()==2);
    }
    {
        Fixture f;f.context.q35PciReleaseAttempted=true;
        assert(!f.Run() && !f.context.recoveryFabricArcOwner && trace.empty() && !f.scope.admissionCalls);
    }
    puts("PASS retained fabric recovery transaction: exact sequencing, every stage failure, lost scope and ticket; SIMULATED ONLY");
}
