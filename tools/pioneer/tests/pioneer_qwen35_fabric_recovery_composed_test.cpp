/* SPDX-License-Identifier: MIT */
// Runs the actual retained owner and context transaction together against
// simulated ARC, mapping, topology, reset and guard facilities.
#define main q35_fabric_arc_owner_test_unused_main
#include "pioneer_qwen35_fabric_arc_owner_test.cpp"
#undef main
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define PIONEER_QWEN35_RECOVERY_RUN_ENABLE 1
#define PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS 1
using pioneer_driver_recovery_guard_v2_info=Guard;
struct Context;
struct Cookie { Context* context=nullptr; };
static Cookie* sRuntimeOwner=nullptr;
static bool sColdRecoveryRequired=true;
struct Context {
    PioneerPostResetMapping* recoveryMapping=nullptr;
    Owner* recoveryFabricArcOwner=nullptr;
    bool dispatchOwnerRetained=true;
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35FabricRecoveryTransaction.inc"
    ~Context() { delete recoveryFabricArcOwner; }
};
struct TransactionScope : Scope {
    unsigned deniedStage=0,deniedCall=1,stageCalls[18]{};
    bool FabricRecoveryAdmission(Context& context,Guard* guard,uint64 ticket) {
        return guard && ticket==123 && context.recoveryMapping==mapping && allowed;
    }
    bool FabricRecoveryStageAllowed(Context& context,const Owner& owner,Guard*,uint64 ticket,unsigned stage) {
        if (!expected) expected=const_cast<Owner*>(&owner);
        assert(stage<=17);const unsigned call=++stageCalls[stage];
        return (stage!=deniedStage || call!=deniedCall) && context.recoveryFabricArcOwner==&owner
            && context.recoveryMapping==mapping && ticket==123 && allowed;
    }
};
struct TransactionFixture {
    FabricQueue queue{0,0,0};PioneerPostResetMapping mapping;Context context;Cookie cookie;
    TransactionScope scope;Guard guard{Validate};
    tt_arc_scratch_result arc=FabricQueue::Arc();tt_arc_queue_result reply{};
    PioneerPostResetAuditResult fleet;PioneerQ35PostResetL2Result l2;unsigned stage=0;
    TransactionFixture() {
        device=&queue;mapCount=deleteCount=0;failMap=failDelete=loseTicketOnDelete=-1;
        changeQueueOnRead=loseTicketOnRead=-1;ticketValid=sColdRecoveryRequired=true;ResetUnit();
        context.recoveryMapping=&mapping;cookie.context=&context;sRuntimeOwner=&cookie;scope.mapping=&mapping;
    }
    bool Run() { return context.RunQ35FabricRecovery(&guard,123,arc,reply,fleet,l2,scope,stage); }
};
int main()
{
    {
        TransactionFixture f;assert(f.Run() && f.stage==17 && f.context.recoveryFabricArcOwner && f.mapping.q35CloseComplete
            && f.context.recoveryFabricArcOwner->CurrentPhase()==Owner::Retired
            && sColdRecoveryRequired && !f.mapping.queueUnsafe && deleteCount==4 && f.queue.notifications==7);
        for (unsigned core=0;core<120;core++) assert(tileReset[core]==0x47800);
        for (unsigned word=0;word<8;word++) assert(resetPage[0x40/4+word]==UINT32_MAX);
        const unsigned before=f.queue.io;assert(!f.Run() && f.queue.io==before);
    }
    {
        TransactionFixture f;dropReset=4;
        assert(!f.Run() && f.stage==5 && f.context.recoveryFabricArcOwner
            && f.context.recoveryFabricArcOwner->CurrentPhase()==Owner::Failed
            && f.mapping.queueUnsafe && sColdRecoveryRequired);
        const unsigned before=f.queue.io;assert(!f.Run() && f.queue.io==before);
    }
    {
        TransactionFixture f;f.scope.deniedStage=13;
        assert(!f.Run() && f.stage==13 && f.context.recoveryFabricArcOwner
            && f.mapping.queueUnsafe && sColdRecoveryRequired);
        const unsigned before=f.queue.io;assert(!f.Run() && f.queue.io==before);
    }
    {
        TransactionFixture f;f.scope.deniedStage=15;f.scope.deniedCall=2;
        assert(!f.Run() && f.stage==15 && f.context.recoveryFabricArcOwner
            && f.context.recoveryFabricArcOwner->CurrentPhase()==Owner::Retired
            && !f.mapping.queueUnsafe && sColdRecoveryRequired);
        const unsigned before=f.queue.io;assert(!f.Run() && f.queue.io==before);
    }
    puts("PASS retained context and real fabric owner: full reset, audits, retirement and failure retention; SIMULATED ONLY");
}
