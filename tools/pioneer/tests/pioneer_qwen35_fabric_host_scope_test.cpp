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
struct PioneerPostResetMapping {};
struct PioneerStoreContext {
    PioneerPostResetMapping* recoveryMapping=nullptr;
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
    assert(!live.FabricRecoveryAdmission(context,&guard,42));
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
    assert(!retained.FabricRecoveryAdmission(context,&guard,42));
    inspector.context=&context;assert(!retained.HostOwnershipValid());inspector.context=nullptr;
    owner.finalFreeRetained=false;assert(!retained.HostOwnershipValid());
    return 0;
}
