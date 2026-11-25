/* SPDX-License-Identifier: MIT */
// Exercise the actual host scope, context transaction and fabric owner
// together with simulated PCI, ARC, guard and reset registers.
#define main q35_fabric_arc_owner_test_unused_main
#include "pioneer_qwen35_fabric_arc_owner_test.cpp"
#undef main
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define PIONEER_QWEN35_RECOVERY_RUN_ENABLE 1
#define PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS 1
#define PIONEER_QWEN35_FABRIC_GUARD_WITNESS 1
#include "../../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadState.h"
using pioneer_driver_recovery_guard_v2_info=Guard;
using PioneerQ35FabricArcCompileOwner=Owner;
using thread_id=int;using team_id=int;using status_t=int;
enum { B_NOT_ALLOWED=-1,B_NO_MEMORY=-2,B_DEV_NOT_READY=-3,B_DEVICE_OP_CODES_END=1000 };
struct thread_info { team_id team; };
static thread_id find_thread(const char*) { return 10; }
static int get_thread_info(thread_id,thread_info* info) { info->team=20;return B_OK; }
namespace PioneerResidentBanks { constexpr uint64 Total=4096; }
struct PioneerStoreContext;
struct PioneerProbeCookie {
    uint32 magic=0x50313030;team_id team=20;
    PioneerStoreContext* context=nullptr;bool finalFreeRetained=false;
};
static PioneerProbeCookie* sRuntimeOwner=nullptr;
static PioneerProbeCookie* sQwen35ExclusiveCookie=nullptr;
static uint64 sReloadGeneration=1;
static unsigned sOpenCookies=1;
static bool sColdRecoveryRequired=true,sBankStorageOnly=true,sReloadQuiesced=false;
struct PioneerStoreContext {
    Profile profile;struct { void* qwen35BorrowedRoleScope=nullptr; } io;
    bool recoveryFabricStartAttempted=false,recoveryAttempted=false;uint64 recoveryFabricStartTicket=0;
    void* recoveryFreshSession=nullptr;
    PioneerPostResetMapping* recoveryMapping=nullptr;
    Owner* recoveryFabricArcOwner=nullptr;
    PioneerDriverFabricRecoveryProof recoveryFabricEvidence{};
    bool recoveryFabricEvidenceRecorded=false;
    bool dispatchOwnerRetained=true,q35PciReservationHeld=true,q35PciReleaseAttempted=false;
    bool bankStorageOnly=true;uint64 bytes=PioneerResidentBanks::Total;
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35FabricRecoveryTransaction.inc"
    ~PioneerStoreContext() { delete recoveryFabricArcOwner; }
};
struct PioneerQ35FabricArcCompileScope {};
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35FabricHostScope.inc"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35FabricRecoveryStart.inc"

struct Fixture {
    FabricQueue queue{0,0,0};PioneerPostResetMapping mapping;PioneerStoreContext context;
    PioneerProbeCookie cookie;Guard guard{Validate};
    PioneerQ35FabricHostScope* scope=nullptr;
    tt_arc_scratch_result firmware=FabricQueue::Arc();tt_arc_queue_result reply{};
    PioneerPostResetAuditResult fleet{};PioneerQ35PostResetL2Result l2{};unsigned stage=0;
    Fixture() {
        device=&queue;mapCount=deleteCount=0;failMap=failDelete=loseTicketOnDelete=-1;
        changeQueueOnRead=loseTicketOnRead=-1;ticketValid=sColdRecoveryRequired=true;ResetUnit();
        context.recoveryMapping=&mapping;cookie.context=&context;
        sRuntimeOwner=sQwen35ExclusiveCookie=&cookie;sReloadQuiesced=false;sOpenCookies=1;
        scope=new PioneerQ35FabricHostScope(&cookie,&cookie,&context,&guard,123,10,20);
    }
    ~Fixture() { delete scope; }
    bool Run() { return context.RunQ35FabricRecovery(&guard,123,firmware,reply,fleet,l2,*scope,stage); }
};
int main() {
    {
        FabricQueue queue{0,0,0};PioneerStoreContext context;PioneerProbeCookie cookie;Guard guard{Validate};
        device=&queue;mapCount=deleteCount=0;failMap=failDelete=loseTicketOnDelete=-1;
        changeQueueOnRead=loseTicketOnRead=-1;ticketValid=sColdRecoveryRequired=true;ResetUnit();
        cookie.context=&context;sRuntimeOwner=sQwen35ExclusiveCookie=&cookie;sReloadQuiesced=false;sOpenCookies=1;
        tt_pioneer_recovery_run result{};
        assert(Qwen35FabricRecoveryStartLocked(&cookie,&cookie,&context,&guard,123,10,20,result)==B_DEV_NOT_READY
            && result.stage==PIONEER_RECOVERY_STAGE_FABRIC_SEQUENCE
            && result.detail==PIONEER_RECOVERY_DETAIL_FABRIC_AUDITED
            && context.recoveryFabricStartAttempted && context.recoveryFabricStartTicket==123
            && context.recoveryFabricEvidenceRecorded && context.recoveryFabricArcOwner
            && context.recoveryMapping && context.recoveryMapping->q35CloseComplete
            && queue.notifications==7 && sColdRecoveryRequired);
        const unsigned before=queue.io;
        assert(Qwen35FabricRecoveryStartLocked(&cookie,&cookie,&context,&guard,123,10,20,result)==B_NOT_ALLOWED
            && queue.io==before);
        delete context.recoveryMapping;context.recoveryMapping=nullptr;
    }
    {
        // An old faulted owner may have closed its fd; a newly admitted
        // inspector must still operate on that exact retained context.
        FabricQueue queue{0,0,0};PioneerStoreContext context;PioneerProbeCookie owner,inspector;Guard guard{Validate};
        device=&queue;mapCount=deleteCount=0;failMap=failDelete=loseTicketOnDelete=-1;
        changeQueueOnRead=loseTicketOnRead=-1;ticketValid=sColdRecoveryRequired=true;ResetUnit();
        owner.context=&context;owner.finalFreeRetained=true;
        sRuntimeOwner=&owner;sQwen35ExclusiveCookie=&inspector;sReloadQuiesced=false;sOpenCookies=1;
        tt_pioneer_recovery_run result{};
        assert(Qwen35FabricRecoveryStartLocked(&inspector,&owner,&context,&guard,123,10,20,result)==B_DEV_NOT_READY
            && result.detail==PIONEER_RECOVERY_DETAIL_FABRIC_AUDITED && context.recoveryFabricEvidenceRecorded
            && context.recoveryMapping && context.recoveryMapping->q35CloseComplete && sColdRecoveryRequired);
        delete context.recoveryMapping;context.recoveryMapping=nullptr;
    }
    {
        FabricQueue queue{0,0,0};PioneerStoreContext context;PioneerProbeCookie cookie;Guard guard{Validate};
        device=&queue;mapCount=deleteCount=0;failMap=failDelete=loseTicketOnDelete=-1;
        changeQueueOnRead=loseTicketOnRead=-1;ticketValid=sColdRecoveryRequired=true;ResetUnit();dropReset=4;
        cookie.context=&context;sRuntimeOwner=sQwen35ExclusiveCookie=&cookie;sReloadQuiesced=false;sOpenCookies=1;
        tt_pioneer_recovery_run result{};
        assert(Qwen35FabricRecoveryStartLocked(&cookie,&cookie,&context,&guard,123,10,20,result)==B_DEV_NOT_READY
            && result.stage==PIONEER_RECOVERY_STAGE_FABRIC_SEQUENCE && result.detail==5
            && context.recoveryFabricStartAttempted && context.recoveryFabricStartTicket==123
            && context.recoveryFabricArcOwner && context.recoveryFabricArcOwner->CurrentPhase()==Owner::Failed
            && context.recoveryMapping && !context.recoveryFabricEvidenceRecorded && sColdRecoveryRequired);
        const unsigned before=queue.io;
        assert(Qwen35FabricRecoveryStartLocked(&cookie,&cookie,&context,&guard,123,10,20,result)==B_NOT_ALLOWED
            && queue.io==before);
        delete context.recoveryMapping;context.recoveryMapping=nullptr;
    }
    {
        Fixture f;
        const bool result=f.Run();
        if (!result) fprintf(stderr,"fabric host composed failed stage=%u phase=%u\n",f.stage,
            f.context.recoveryFabricArcOwner?unsigned(f.context.recoveryFabricArcOwner->CurrentPhase()):99U);
        assert(result && f.stage==17 && f.mapping.q35CloseComplete && f.context.recoveryFabricArcOwner
            && f.context.recoveryFabricArcOwner->CurrentPhase()==Owner::Retired
            && sColdRecoveryRequired && !f.mapping.queueUnsafe && f.queue.notifications==7);
        const auto& evidence=f.context.recoveryFabricEvidence;
        assert(f.context.recoveryFabricEvidenceRecorded && evidence.fabric_generation==123
            && evidence.firmware_generation==123 && evidence.retained_owners==1
            && evidence.core_count==120 && evidence.flags==(PIONEER_FABRIC_PROOF_RESET_SEQUENCE
                | PIONEER_FABRIC_PROOF_CLOCK_RESTORED | PIONEER_FABRIC_PROOF_FIRMWARE_READY | PIONEER_FABRIC_PROOF_ALL_CORES_HELD
                | PIONEER_FABRIC_PROOF_NOC_IDLE | PIONEER_FABRIC_PROOF_HOST_DMA_IDLE)
            && !evidence.Complete() && !evidence.ReadyToRetireOneOwner());
        PioneerDriverReloadState guardState;uint64 generation=0,ticket=0,next=0;uint32 faultEpoch=0;
        assert(guardState.Attach(generation));guardState.Fault(generation);guardState.nextRecoveryTicket=122;
        assert(guardState.BeginRecovery(generation,3,ticket) && ticket==123);
        assert(!guardState.CompleteFabricRecovery(ticket,3,evidence,faultEpoch,next)
            && !next && guardState.cold);
        for (unsigned core=0;core<120;core++) assert(tileReset[core]==0x47800);
        const unsigned before=f.queue.io;assert(!f.Run() && f.queue.io==before);
    }
    {
        Fixture f;dropReset=4;
        assert(!f.Run() && f.stage==5 && f.context.recoveryFabricArcOwner
            && f.context.recoveryFabricArcOwner->CurrentPhase()==Owner::Failed
            && f.mapping.queueUnsafe && sColdRecoveryRequired && !f.context.recoveryFabricEvidenceRecorded);
        const unsigned before=f.queue.io;assert(!f.Run() && f.queue.io==before);
    }
    {
        Fixture f;dropTile=57;
        assert(!f.Run() && f.stage==8 && f.context.recoveryFabricArcOwner
            && f.context.recoveryFabricArcOwner->CurrentPhase()==Owner::Failed
            && f.context.recoveryFabricArcOwner->FailedTile()==57 && sColdRecoveryRequired
            && !f.context.recoveryFabricEvidenceRecorded);
        const unsigned before=f.queue.io;assert(!f.Run() && f.queue.io==before);
    }
    {
        Fixture f;f.mapping.queueAttempted=true;
        assert(!f.Run() && f.stage==0 && !f.context.recoveryFabricArcOwner && !f.queue.io);
    }
    {
        Fixture f;f.mapping.corruptFirmwareResult=true;
        assert(!f.Run() && f.stage==14 && f.context.recoveryFabricArcOwner
            && !f.mapping.q35FirmwareRevalidated && !f.context.recoveryFabricEvidenceRecorded
            && sColdRecoveryRequired);
        const unsigned before=f.queue.io;assert(!f.Run() && f.queue.io==before);
    }
    for (unsigned fault=0;fault<2;fault++) {
        Fixture f;f.mapping.corruptAuditResult=fault==0;f.mapping.corruptL2Result=fault==1;
        assert(!f.Run() && f.stage==17 && f.context.recoveryFabricArcOwner
            && f.context.recoveryFabricArcOwner->CurrentPhase()==Owner::Retired
            && f.mapping.q35CloseComplete && sColdRecoveryRequired
            && !f.context.recoveryFabricEvidenceRecorded);
    }
    {
        Fixture f;loseTicketOnDelete=0;
        assert(!f.Run() && f.stage==15 && f.context.recoveryFabricArcOwner
            && f.context.recoveryFabricArcOwner->CurrentPhase()==Owner::Failed && sColdRecoveryRequired);
        const unsigned before=f.queue.io;assert(!f.Run() && f.queue.io==before);
    }
    puts("PASS actual host scope, fabric transaction and owner; SIMULATED ONLY");
}
