// SPDX-License-Identifier: MIT
#include <cassert>
#include <cstddef>
#include <cstdint>

using uint64=uint64_t;
using thread_id=int;
using team_id=int;
constexpr int B_OK=0;
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS 1
#define PIONEER_QWEN35_RECOVERY_RUN_ENABLE 1
struct thread_info { team_id team; };
static thread_id currentThread=10;
static team_id currentTeam=20;
static thread_id find_thread(const char*) { return currentThread; }
static int get_thread_info(thread_id,thread_info* info) { info->team=currentTeam;return B_OK; }
namespace PioneerResidentBanks { constexpr uint64 Total=4096; }
struct PioneerPostResetMapping {
    bool ready=true,firmwareReady=true,topologyReady=true,savedValid=true;
    bool queueAttempted=false,dmaModified=false,fleetAudited=false,dmaAudited=false;
};
struct PioneerQ35FabricArcCompileOwner {
    enum Phase { Fresh,ClockForced,RiscsHeld,TensixToggled,NocReinitialized,
        TilesHeld,RiscsReleased,ClockReleased,Failed,Retired };
    uint64 ticket=42;Phase phase=Fresh;bool clockRestored=false;
    uint64 Ticket() const { return ticket; }
    Phase CurrentPhase() const { return phase; }
    bool ClockRestored() const { return clockRestored; }
};
struct PioneerStoreContext {
    PioneerPostResetMapping* recoveryMapping=nullptr;
    PioneerQ35FabricArcCompileOwner* recoveryFabricArcOwner=nullptr;
    bool bankStorageOnly=true,dispatchOwnerRetained=true;
    bool q35PciReservationHeld=true,q35PciReleaseAttempted=false;
    uint64 bytes=PioneerResidentBanks::Total;
};
struct PioneerProbeCookie {
    uint32_t magic=0x50313030;
    team_id team=20;
    PioneerStoreContext* context=nullptr;
    bool finalFreeRetained=false;
};
static PioneerProbeCookie* sRuntimeOwner=nullptr;
static PioneerProbeCookie* sQwen35ExclusiveCookie=nullptr;
static uint64 sReloadGeneration=1;
static unsigned sOpenCookies=1;
static bool sColdRecoveryRequired=true,sBankStorageOnly=true,sReloadQuiesced=false;
static bool ticketValid=true;
static int validate(uint64 ticket) { return ticketValid && ticket==42?B_OK:-1; }
struct pioneer_driver_recovery_guard_v2_info { unsigned version=2;int (*validate)(uint64)=::validate; };
struct PioneerQ35FabricArcCompileScope {};
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35FabricHostScope.inc"

int main() {
    PioneerPostResetMapping mapping,otherMapping;
    PioneerStoreContext context;
    context.recoveryMapping=&mapping;
    PioneerProbeCookie owner,inspector;
    owner.context=&context;
    sRuntimeOwner=&owner;sQwen35ExclusiveCookie=&owner;
    pioneer_driver_recovery_guard_v2_info guard;
    PioneerQ35FabricHostScope live(&owner,&owner,&context,&guard,42,10,20);
    assert(live.HostOwnershipValid());
    sReloadGeneration++;assert(!live.HostOwnershipValid());sReloadGeneration--;
    sOpenCookies++;assert(!live.HostOwnershipValid());sOpenCookies--;
    sReloadQuiesced=true;assert(!live.HostOwnershipValid());sReloadQuiesced=false;
    sColdRecoveryRequired=false;assert(!live.HostOwnershipValid());sColdRecoveryRequired=true;
    sBankStorageOnly=false;assert(!live.HostOwnershipValid());sBankStorageOnly=true;
    context.bankStorageOnly=false;assert(!live.HostOwnershipValid());context.bankStorageOnly=true;
    sRuntimeOwner=&inspector;assert(!live.HostOwnershipValid());sRuntimeOwner=&owner;
    sQwen35ExclusiveCookie=&inspector;assert(!live.HostOwnershipValid());sQwen35ExclusiveCookie=&owner;
    owner.context=nullptr;assert(!live.HostOwnershipValid());owner.context=&context;
    context.q35PciReservationHeld=false;assert(!live.HostOwnershipValid());context.q35PciReservationHeld=true;
    context.q35PciReleaseAttempted=true;assert(!live.HostOwnershipValid());context.q35PciReleaseAttempted=false;
    context.recoveryMapping=&otherMapping;assert(!live.HostOwnershipValid());context.recoveryMapping=&mapping;
    context.dispatchOwnerRetained=false;assert(!live.HostOwnershipValid());context.dispatchOwnerRetained=true;
    context.bytes--;assert(!live.HostOwnershipValid());context.bytes++;
    currentThread++;assert(!live.HostOwnershipValid());currentThread--;
    currentTeam++;assert(!live.HostOwnershipValid());currentTeam--;
    owner.magic=0;assert(!live.HostOwnershipValid());owner.magic=0x50313030;
    ticketValid=false;assert(!live.HostOwnershipValid());ticketValid=true;
    guard.version=1;assert(!live.HostOwnershipValid());guard.version=2;
    guard.validate=nullptr;assert(!live.HostOwnershipValid());guard.validate=validate;
    PioneerQ35FabricHostScope wrongTicket(&owner,&owner,&context,&guard,41,10,20);
    assert(!wrongTicket.HostOwnershipValid());
    owner.finalFreeRetained=true;assert(!live.HostOwnershipValid());
    sQwen35ExclusiveCookie=&inspector;
    PioneerQ35FabricHostScope retained(&inspector,&owner,&context,&guard,42,10,20);
    assert(retained.HostOwnershipValid());
    inspector.context=&context;assert(!retained.HostOwnershipValid());inspector.context=nullptr;
    owner.finalFreeRetained=false;assert(!retained.HostOwnershipValid());

    // The only admitted sequence is bound to the same retained owner, ticket,
    // mapping and monotonically advancing transaction stage. This models
    // host ownership; it does not model external firmware or a real reset.
    sQwen35ExclusiveCookie=&owner;
    PioneerQ35FabricHostScope sequence(&owner,&owner,&context,&guard,42,10,20);
    mapping.firmwareReady=false;
    assert(!sequence.FabricRecoveryAdmission(context,&guard,42));
    mapping.firmwareReady=true;
    assert(sequence.FabricRecoveryAdmission(context,&guard,42));
    assert(!sequence.FabricRecoveryAdmission(context,&guard,42));
    PioneerQ35FabricArcCompileOwner arc,otherArc;
    assert(!sequence.FabricRecoveryStageAllowed(context,arc,&guard,42,1));
    context.recoveryFabricArcOwner=&arc;
    assert(!sequence.FabricRecoveryStageAllowed(context,arc,&guard,42,2));
    assert(sequence.FabricRecoveryStageAllowed(context,arc,&guard,42,1));
    assert(sequence.FabricArcAllowed(arc,mapping,42,arc.phase));
    assert(!sequence.FabricArcAllowed(otherArc,mapping,42,otherArc.phase));
    assert(!sequence.FabricArcAllowed(arc,otherMapping,42,arc.phase));
    assert(!sequence.FabricArcAllowed(arc,mapping,41,arc.phase));
    assert(!sequence.FabricClockQueryAllowed(arc,mapping,42,arc.phase));
    assert(sequence.FabricRecoveryStageAllowed(context,arc,&guard,42,2));
    assert(sequence.FabricClockQueryAllowed(arc,mapping,42,arc.phase));
    assert(!sequence.FabricRecoveryStageAllowed(context,arc,&guard,42,4));
    assert(sequence.FabricRecoveryStageAllowed(context,arc,&guard,42,3));
    assert(sequence.FabricArcPhaseReady(arc,mapping,42,arc.phase));
    ticketValid=false;assert(!sequence.FabricArcAllowed(arc,mapping,42,arc.phase));ticketValid=true;
    arc.phase=arc.ClockForced;
    assert(sequence.FabricRecoveryStageAllowed(context,arc,&guard,42,3));
    assert(sequence.FabricRecoveryStageAllowed(context,arc,&guard,42,4));
    assert(sequence.FabricClockQueryAllowed(arc,mapping,42,arc.phase));
    assert(sequence.FabricRecoveryStageAllowed(context,arc,&guard,42,5));
    assert(sequence.FabricResetUnitAllowed(arc,mapping,42,arc.phase,true));
    assert(!sequence.FabricResetUnitAllowed(arc,mapping,42,arc.phase,false));
    arc.phase=arc.RiscsHeld;
    assert(sequence.FabricRecoveryStageAllowed(context,arc,&guard,42,5));
    assert(sequence.FabricRecoveryStageAllowed(context,arc,&guard,42,6));
    assert(sequence.FabricArcPhaseReady(arc,mapping,42,arc.phase));
    arc.phase=arc.TensixToggled;
    assert(sequence.FabricRecoveryStageAllowed(context,arc,&guard,42,6));
    assert(sequence.FabricRecoveryStageAllowed(context,arc,&guard,42,7));
    assert(sequence.FabricArcPhaseReady(arc,mapping,42,arc.phase));
    arc.phase=arc.NocReinitialized;
    assert(sequence.FabricRecoveryStageAllowed(context,arc,&guard,42,7));
    assert(sequence.FabricRecoveryStageAllowed(context,arc,&guard,42,8));
    assert(sequence.FabricTileResetAllowed(arc,mapping,42,119));
    assert(!sequence.FabricTileResetAllowed(arc,mapping,42,120));
    arc.phase=arc.TilesHeld;
    assert(sequence.FabricRecoveryStageAllowed(context,arc,&guard,42,8));
    assert(sequence.FabricRecoveryStageAllowed(context,arc,&guard,42,9));
    assert(sequence.FabricResetUnitAllowed(arc,mapping,42,arc.phase,false));
    arc.phase=arc.RiscsReleased;
    assert(sequence.FabricRecoveryStageAllowed(context,arc,&guard,42,9));
    assert(sequence.FabricRecoveryStageAllowed(context,arc,&guard,42,10));
    assert(sequence.FabricArcPhaseReady(arc,mapping,42,arc.phase));
    arc.phase=arc.ClockReleased;
    assert(sequence.FabricRecoveryStageAllowed(context,arc,&guard,42,10));
    assert(sequence.FabricRecoveryStageAllowed(context,arc,&guard,42,11));
    assert(sequence.FabricClockQueryAllowed(arc,mapping,42,arc.phase));
    for (unsigned stage=12;stage<=15;stage++)
        assert(sequence.FabricRecoveryStageAllowed(context,arc,&guard,42,stage));
    assert(!sequence.FabricRetirementAllowed(arc,mapping,42));
    mapping.fleetAudited=mapping.dmaAudited=true;arc.clockRestored=true;
    assert(sequence.FabricRetirementAllowed(arc,mapping,42));
    arc.phase=arc.Retired;
    assert(sequence.FabricRecoveryStageAllowed(context,arc,&guard,42,15));
    assert(sequence.FabricRecoveryStageAllowed(context,arc,&guard,42,16));
    assert(sequence.FabricRecoveryStageAllowed(context,arc,&guard,42,17));
    assert(!sequence.FabricRecoveryStageAllowed(context,arc,&guard,42,16));
    return 0;
}
