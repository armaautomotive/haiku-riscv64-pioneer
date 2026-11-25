/* SPDX-License-Identifier: MIT */
// Exercise the actual post-reset session ordering using simulated resources.
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define PIONEER_QWEN35_RECOVERY_RUN_ENABLE 1
#define PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS 1
#define PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS 1
#define PIONEER_QWEN35_FRESH_RECOVERY_TEST 1
#define PIONEER_QWEN35_FABRIC_GUARD_WITNESS 1
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <vector>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadState.h"
using uint64=uint64_t;
using status_t=int;
constexpr int B_OK=0,B_NOT_ALLOWED=-1,B_NO_MEMORY=-2,B_DEV_NOT_READY=-3;
constexpr unsigned TT_HOST_PAGE_READY=1;
constexpr uint64 kTicket=37;
namespace PioneerQwen35 { enum { DataflowCoreCount=120 }; }

static unsigned failedOperation=0,guardCalls=0,failGuardAt=0;
static bool failFreshAllocation=false,auditSourceMismatch=false;
static bool sColdRecoveryRequired=true,sReloadQuiesced=false,sBankStorageOnly=true;
static unsigned sOpenCookies=1;
static std::vector<unsigned> operations;
static bool Step(unsigned operation) { operations.push_back(operation);return failedOperation!=operation; }
constexpr unsigned TT_DRAM_SOURCE_WORDS=16;
struct MockDram { unsigned route=77,source[TT_DRAM_SOURCE_WORDS]{}; };
struct MockNoc {};
struct MockRoot { struct A { struct B { struct C { MockDram dram; } baseline;MockNoc noc; } baseline; } baseline; };
struct MockPci { unsigned node_id[3]={2,2,2};unsigned restored=0; };
struct MockProof {
    uint64 reset_generation=kTicket,firmware_generation=kTicket;
    bool valid=true;
    bool ReadyToRetireOneOwner() const { return valid; }
};
struct MockFabricProof : PioneerDriverFabricRecoveryProof {
    bool valid=true;
    MockFabricProof() {
        flags=PIONEER_FABRIC_PROOF_BEFORE_MATRIX & ~(PIONEER_FABRIC_PROOF_OLD_MAPPINGS_DISPOSED
            | PIONEER_FABRIC_PROOF_OWNERS_RELEASED | PIONEER_FABRIC_PROOF_PCI_RESERVATION_RELEASED);
        core_count=120;retained_owners=1;fabric_generation=firmware_generation=kTicket;
    }
    bool ReadyForScratchRestore() const { return valid && PioneerDriverFabricRecoveryProof::ReadyForScratchRestore(); }
    bool ReadyToRetireOneOwner() const { return valid && PioneerDriverFabricRecoveryProof::ReadyToRetireOneOwner(); }
};
struct PioneerResidentBanks;
struct MockScratch {
    bool eligible=true;
    bool ResetRestoreEligible(const uint32_t* p) const { return eligible && p; }
    bool ResetRestoreClaimMatches(const PioneerResidentBanks&,const uint32_t*) const;
};
using PioneerQ35ProjectionScratch=MockScratch;
struct MockQualificationOwner {
    MockScratch scratch;
    uint32_t snapshot=1;
    bool active=true,retained=true,retiredFlag=false;
    bool Active() const { return active && !retiredFlag; }
    bool Retired() const { return retiredFlag; }
    MockScratch* Scratch() { return Active()?&scratch:nullptr; }
    uint32_t* Snapshot() { return Active()?&snapshot:nullptr; }
    void Retain() { retained=true; }
    bool ModifiedResetEligible(const PioneerResidentBanks&) const;
    bool RetiredModifiedIdentity(const MockScratch* expected,const uint32_t* saved) const {
        return active && retiredFlag && retained && expected==&scratch && saved==&snapshot;
    }
    template<class Proof,class Receipt,class Banks,class Session,class Scope>
    bool RetireModifiedAfterReset(const Proof&,uint64 ticket,const Receipt& receipt,
        const Banks& old,const Banks& fresh,const Session& session,Scope& scope) {
        if (!Active() || !receipt.VerifiedFor(scratch,ticket,old,fresh)
            || !scope.QualificationModifiedResetRetirementAllowed(*this,scratch,receipt,session)) return false;
        retained=true;retiredFlag=true;return true;
    }
};
struct PioneerResidentBanks {
    static constexpr uint64 Total=128*4096;
    static constexpr unsigned Pages=128;
    bool quarantined=false,claimed=true;
    bool Q35StreamClaimIdentity(const MockScratch* owner,unsigned count) const { return owner && claimed && count==120; }
    void Quarantine() { quarantined=true; }
    bool Bind(const MockDram&) { return Step(4); }
    bool Q35RecoveryRoutesMatch(const MockDram& old,const PioneerResidentBanks&,const MockDram& fresh) const {
        return old.route==fresh.route;
    }
};
bool MockScratch::ResetRestoreClaimMatches(const PioneerResidentBanks& banks,const uint32_t* p) const {
    return ResetRestoreEligible(p) && banks.Q35StreamClaimIdentity(this,120);
}
bool MockQualificationOwner::ModifiedResetEligible(const PioneerResidentBanks& banks) const {
    return Active() && retained && scratch.ResetRestoreClaimMatches(banks,&snapshot);
}
struct MockLease { bool recovery_required=false,ready=false,pending=false,l1_restored=true,niu_restored=true,tlb_restored=true; };
struct MockHost { unsigned state=0,allocated=0,released=1;int release_status=0; };
struct MockTransaction { bool config_attempted=false; };
struct tt_arc_scratch_result {};
struct PioneerPostResetAuditResult {};
struct PioneerQ35PostResetL2Result {};
struct PioneerPostResetMapping {
    MockDram topology;
    explicit PioneerPostResetMapping(unsigned) { if (auditSourceMismatch) topology.source[0]=1; }
    bool Open() { return Step(9); }
    bool ProbeFirmware(unsigned,tt_arc_scratch_result&) { return Step(10); }
    bool Audit(PioneerPostResetAuditResult&) { return Step(11); }
    bool AuditQ35L2CpuHeld(PioneerQ35PostResetL2Result&) { return Step(12); }
    bool AuditHostDma() { return Step(13); }
    bool Close() { return Step(14); }
    template<class Check> bool OpenQ35Guarded(Check& check) { return check() && Open() && check(); }
    template<class Check> bool ProbeQ35FabricFirmwareGuarded(unsigned limit,tt_arc_scratch_result& result,Check& check) {
        return check() && ProbeFirmware(limit,result) && check();
    }
    template<class Check> bool AuditQ35Fabric(PioneerPostResetAuditResult& result,Check& check) {
        return check() && Audit(result) && check();
    }
    template<class Check> bool AuditQ35L2CpuHeldGuarded(PioneerQ35PostResetL2Result& result,Check& check) {
        return check() && AuditQ35L2CpuHeld(result) && check();
    }
    template<class Check> bool AuditQ35HostDmaGuarded(Check& check) { return check() && AuditHostDma() && check(); }
    template<class Check> bool RevalidateQ35FabricFirmware(const tt_arc_scratch_result&,Check& check) {
        return check() && Step(19) && check();
    }
    template<class Check> bool AuditQ35RootAerGuarded(Check& check) { return check() && Step(20) && check(); }
    template<class Check> bool CloseQ35Guarded(Check& check) { return check() && Close() && check(); }
};
struct PioneerHostDmaAuditIO { bool AfterNodeId(unsigned) { return Step(2); } };
struct MockIO : PioneerHostDmaAuditIO {
    bool postResetDisposeComplete=false,fabricOldMmioDisposeComplete=false,quarantined=false;
    bool fabricHostDisposeAttempted=false,fabricHostDisposeComplete=false;
    bool fleetEnabled=false;
    unsigned runtimeCore=0;
    bool freshMmioReady=true,freshMmioDisposed=false;
    bool freshHostReleaseReady=true,freshHostReleaseAttempted=false,freshHostReleased=false;
    template<class Check> bool GuardedRecoveryBaseline(unsigned node,Check& check) {
        return check() && AfterNodeId(node) && check();
    }
    void QuarantineHostPage() { quarantined=true; }
    int ReleaseHostPage() { freshHostReleaseAttempted=true;freshHostReleased=Step(17);return freshHostReleased?B_OK:-1; }
    bool FreshRecoveryHostReleaseReady() const {
        return freshMmioDisposed && freshHostReleaseReady && !freshHostReleaseAttempted && !quarantined;
    }
    bool FreshRecoveryHostReleased() const { return freshMmioDisposed && freshHostReleased && !quarantined; }
    bool FreshRecoveryMmioUnmapReady() const { return freshMmioReady && !freshMmioDisposed; }
    bool ReferencesRecoveryHostArea(int32_t area) const { return area==77; }
    template<class Check,class Protected> int DisposeFabricOldMmio(Check& check,Protected& protectedArea) {
        if (!check() || protectedArea(91) || !check()) return B_DEV_NOT_READY;
        freshMmioDisposed=fabricOldMmioDisposeComplete=true;return B_OK;
    }
    template<class Check,class Protected> int DisposeFabricHostBuffers(Check& check,Protected& protectedArea) {
        if (fabricHostDisposeAttempted || !fabricOldMmioDisposeComplete || !check()
            || protectedArea(92) || !check()) return B_DEV_NOT_READY;
        fabricHostDisposeAttempted=true;
        if (!Step(22) || !check()) return B_DEV_NOT_READY;
        fabricHostDisposeComplete=true;return B_OK;
    }
};
class PioneerQ35FreshRecoverySession;
struct MockMatrixReceipt {
    template<class... Args> bool Seal(Args&&...) { return false; }
};
struct PioneerStoreContext {
    uint64 bytes;
    unsigned profile=123;
    uint8_t* initialized;
    uint32_t* hashes;
    bool bankStorageOnly=true,recoveryAttempted=true,recoveryFinishPending=true;
    uint64 recoveryFinishTicket=kTicket;
    bool recoveryFabricEvidenceRecorded=false,recoveryFabricPciIdleAttempted=false,recoveryFabricPciIdle=false;
    bool recoveryFabricAuditReleaseAttempted=false,recoveryFabricAuditReleased=false;
    bool recoveryFabricRoleViewsDisposed=false;
    bool fabricRoleRestorationReady=true;
    bool q35PciReservationHeld=true,q35PciReleaseAttempted=false;
    uint64 recoveryFabricStartTicket=kTicket;
    void* recoveryFabricArcOwner=nullptr;
    void* qwen35SharedRoleBinding=nullptr;void* qwen35GraphOwner=nullptr;
    PioneerPostResetMapping* recoveryMapping=nullptr;
    MockProof recoveryFinishProof;
    MockFabricProof recoveryFabricEvidence;
    MockMatrixReceipt recoveryFabricMatrixReceipt;
    PioneerQ35FreshRecoverySession* recoveryFreshSession=nullptr;
    MockQualificationOwner qwen35QualificationOwner;
    PioneerResidentBanks banks;
    MockRoot baseline;
    MockIO io;
    MockPci pci;
    MockTransaction transaction;
    unsigned idle=0;
    MockHost host;
    MockLease lease;
    explicit PioneerStoreContext(uint64 capacity):bytes(capacity),
        initialized(failFreshAllocation?nullptr:static_cast<uint8_t*>(calloc(PioneerResidentBanks::Pages,1))),
        hashes(static_cast<uint32_t*>(calloc(PioneerResidentBanks::Pages,sizeof(uint32_t)))) {}
    ~PioneerStoreContext() { delete recoveryMapping;free(initialized);free(hashes); }
    MockPci& Pci() { return pci; }
};
struct PioneerProbeCookie { PioneerStoreContext* context=nullptr; };
static PioneerProbeCookie* sRuntimeOwner=nullptr;
static int Validate(uint64 ticket) {
    ++guardCalls;return ticket==kTicket && guardCalls!=failGuardAt?B_OK:-1;
}
struct pioneer_driver_recovery_guard_v2_info { int (*validate)(uint64)=Validate; };
static bool BlackholePioneerMmioBegin(MockIO&,MockPci&,MockTransaction& t) { if (!Step(1)) return false;t.config_attempted=true;return true; }
template<class Check> static bool PioneerQ35GuardedMmioBegin(MockIO& io,MockPci& pci,MockTransaction& t,Check& check) {
    return check() && BlackholePioneerMmioBegin(io,pci,t) && check();
}
static bool PioneerResidentSessionIdle(MockIO&,MockNoc&,unsigned&) { return Step(3); }
static bool PioneerPrepareHostPage(MockIO&,MockHost& host) { if (!Step(5)) return false;host.state=TT_HOST_PAGE_READY;host.allocated=1;host.released=0;return true; }
template<class Check> static bool PioneerQ35GuardedPrepareHostPage(MockIO& io,MockHost& host,Check& check) {
    return check() && PioneerPrepareHostPage(io,host) && check();
}
static bool PioneerResidentLeaseBegin(MockIO&,MockNoc&,MockLease& lease) {
    if (!Step(6)) return false;
    lease.ready=true;return true;
}
template<class Check> static bool PioneerQ35GuardedSessionIdle(MockIO& io,MockNoc& noc,unsigned& idle,Check& check) {
    return check() && PioneerResidentSessionIdle(io,noc,idle) && check();
}
template<class Check> static bool PioneerQ35GuardedLeaseBegin(MockIO& io,MockNoc& noc,MockLease& lease,Check& check) {
    if (!check() || !PioneerResidentLeaseBegin(io,noc,lease) || !check()) { lease.recovery_required=true;return false; }
    return true;
}
template<class Check> static bool PioneerQ35GuardedSelectRoleRoot(MockIO& io,Check& check) {
    if (!check() || !io.fleetEnabled || !Step(21) || !check()) return false;
    io.runtimeCore=0;return true;
}
static bool PioneerResidentLeaseEnd(MockIO&,MockNoc&,MockLease& lease) {
    if (!Step(8)) return false;
    lease.ready=false;return true;
}
template<class Check> static bool PioneerQ35GuardedLeaseEnd(MockIO& io,MockNoc& noc,MockLease& lease,Check& check) {
    if (!check() || !PioneerResidentLeaseEnd(io,noc,lease) || !check()) {
        lease.recovery_required=true;return false;
    }
    return true;
}
static bool PioneerQ38DispatchMmioEnd(MockIO&,MockPci& pci,MockTransaction& t) {
    if (!Step(15)) return false;
    pci.restored=1;t.config_attempted=false;return true;
}
template<class Check,class Unmap> static bool PioneerQ35GuardedMmioEnd(MockIO& io,MockPci& pci,
    MockTransaction& t,Check& check,Unmap& unmap) {
    if (!check() || !unmap()) { t.config_attempted=false;return false; }
    const bool okay=PioneerQ38DispatchMmioEnd(io,pci,t);
    if (!check()) { pci.restored=0;return false; }
    return okay;
}
struct PioneerQ35ProjectionResetRecovery {
    bool restored=false;
    template<class Check> static bool TicketCheck(const MockProof&,Check&) { return true; }
    template<class Check> static bool TicketCheck(const MockFabricProof&,Check& check) { return check(); }
    template<class Scope,class Proof,class Check>
    bool Restore(MockScratch&,uint64 ticket,const Proof& proof,MockIO&,Scope&,
        PioneerResidentBanks&,const MockDram&,PioneerResidentBanks&,const MockDram&,
        const MockNoc&,const MockHost&,MockLease&,const uint8_t*,unsigned pages,Check& check) {
        if (ticket!=kTicket || pages!=PioneerResidentBanks::Pages || !TicketCheck(proof,check) || !Step(7)) return false;
        restored=true;return true;
    }
    bool VerifiedFor(const MockScratch&,uint64 ticket,const PioneerResidentBanks&,
        const PioneerResidentBanks& fresh) const { return restored && ticket==kTicket && !fresh.quarantined; }
};
static bool PioneerQ35GraphOwnerReleaseAfterFabric(void* graph,const MockFabricProof& proof,uint64 ticket,
    pioneer_driver_recovery_guard_v2_info* guard) {
    return graph && proof.ReadyToRetireOneOwner() && ticket==kTicket && guard
        && guard->validate(ticket)==B_OK && Step(23) && guard->validate(ticket)==B_OK;
}
static bool PioneerQ35FabricRoleRestorationMatches(const PioneerStoreContext& old,uint64 ticket) {
    return old.fabricRoleRestorationReady && old.recoveryFreshSession && old.qwen35SharedRoleBinding
        && old.qwen35GraphOwner && ticket==kTicket;
}
static bool roleRetirementSucceeds=true,roleRetirementRecorded=false;
template<class Check> static bool PioneerQ35RecordFabricRoleRetirement(PioneerStoreContext&,uint64 ticket,Check& check) {
    if (ticket!=kTicket || !check() || !roleRetirementSucceeds || !Step(24) || !check()) return false;
    roleRetirementRecorded=true;return true;
}
static bool PioneerQ35FabricRoleRetirementMatches(const PioneerStoreContext& old,uint64 ticket) {
    return roleRetirementRecorded && ticket==kTicket && old.recoveryFreshSession
        && old.qwen35SharedRoleBinding && old.qwen35GraphOwner;
}
static bool roleDetachPrepared=false;
template<class Check> static bool PioneerQ35PrepareFabricRoleSessionDetach(PioneerStoreContext& old,uint64 ticket,Check& check) {
    if (roleDetachPrepared || !PioneerQ35FabricRoleRetirementMatches(old,ticket) || !check()) return false;
    roleDetachPrepared=true;return check();
}
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35FreshRecoverySession.inc"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35ModifiedRecoveryTransaction.inc"

struct Scope {
    bool admission=true,retained=false,closeAdmission=true,retirementAdmission=true,ownerAdmission=true;
    bool quarantineAtRetirement=false,loseClaimAfterRetirement=false;
    bool dropAttachment=false,roleAdmission=true,roleRestored=true,loseRoleEvidence=false;
    bool roleCloseAdmission=true,rolePostLease=true,roleHostAdmission=true,roleHostComplete=true,protectOldHost=false;
    bool clearHostCompletion=false;
    bool roleMappingsAdmission=true,roleMappingsComplete=true;
    bool roleGraphAdmission=true,roleGraphComplete=true;
    bool roleFreshHostAdmission=true,roleFreshHostComplete=true;
    bool roleOwnerRetirementAllowed=true;
    bool roleSessionRetirementAllowed=true,dropRoleRetirement=false;
    bool replaceRoleAtAdmission=false,replaceGraphAtAdmission=false;
    unsigned auditChecks=0,failAuditAt=0,leaseChecks=0,failLeaseAt=0;
    unsigned setupChecks=0,failSetupAt=0,pciChecks=0,failPciAt=0;
    bool protectFreshArea=false,poisonLeaseBeforePci=false;
    unsigned postAuditCalls=0;
    unsigned failBoundary=0,replaceRoleAtBoundary=0;
    bool RecoveryTransactionAdmission(PioneerProbeCookie*,PioneerStoreContext&,pioneer_driver_recovery_guard_v2_info*) { return admission; }
    bool FabricRecoveryTransactionAdmission(PioneerProbeCookie*,PioneerStoreContext&,pioneer_driver_recovery_guard_v2_info*) { return admission; }
    bool FreshRoleRecoveryAdmission(PioneerQ35FreshRecoverySession&,PioneerProbeCookie*,PioneerStoreContext& old,uint64 ticket) {
        if (replaceRoleAtAdmission) old.qwen35SharedRoleBinding=&old;
        if (replaceGraphAtAdmission) old.qwen35GraphOwner=&old;
        return roleAdmission && ticket==kTicket;
    }
    bool FreshRoleRecoveryRestored(PioneerQ35FreshRecoverySession&,PioneerStoreContext& old,PioneerStoreContext&) {
        if (loseRoleEvidence) old.fabricRoleRestorationReady=false;
        return roleRestored;
    }
    template<class Receipt> bool FreshRoleRecoveryCloseAdmission(PioneerQ35FreshRecoverySession&,
        PioneerStoreContext&,PioneerStoreContext&,const Receipt&) { return roleCloseAdmission; }
    template<class Receipt> bool FreshRoleRecoveryPostLeaseAudit(PioneerQ35FreshRecoverySession&,
        PioneerStoreContext&,PioneerStoreContext&,const Receipt&) { return rolePostLease; }
    template<class Receipt> bool FreshRoleOldHostDisposeAdmission(PioneerQ35FreshRecoverySession&,
        PioneerStoreContext&,PioneerStoreContext&,const Receipt&) { return roleHostAdmission; }
    template<class Receipt> bool FreshRoleOldHostAccessAllowed(PioneerQ35FreshRecoverySession&,
        PioneerStoreContext&,PioneerStoreContext&,const Receipt&) { return true; }
    template<class Receipt> bool FreshRoleOldHostAreaProtected(PioneerQ35FreshRecoverySession&,
        PioneerStoreContext&,PioneerStoreContext&,const Receipt&,int32_t area) { return protectOldHost && area==92; }
    template<class Receipt> bool FreshRoleOldHostDisposeComplete(PioneerQ35FreshRecoverySession&,
        PioneerStoreContext& old,PioneerStoreContext&,const Receipt&) {
        if (clearHostCompletion) old.io.fabricHostDisposeComplete=false;
        return roleHostComplete;
    }
    template<class Receipt> bool FreshRoleMappingsDisposedAdmission(PioneerQ35FreshRecoverySession&,
        PioneerStoreContext&,PioneerStoreContext&,const Receipt&) { return roleMappingsAdmission; }
    template<class Receipt> bool FreshRoleMappingsDisposedComplete(PioneerQ35FreshRecoverySession&,
        PioneerStoreContext&,PioneerStoreContext&,const Receipt&) { return roleMappingsComplete; }
    template<class Receipt> bool FreshRoleGraphRetirementAdmission(PioneerQ35FreshRecoverySession&,
        PioneerStoreContext&,PioneerStoreContext&,const Receipt&) { return roleGraphAdmission; }
    template<class Receipt> bool FreshRoleGraphRetirementComplete(PioneerQ35FreshRecoverySession&,
        PioneerStoreContext&,PioneerStoreContext&,const Receipt&) { return roleGraphComplete; }
    template<class Receipt> bool FreshRoleHostReleaseAdmission(PioneerQ35FreshRecoverySession&,
        PioneerStoreContext&,PioneerStoreContext&,const Receipt&) { return roleFreshHostAdmission; }
    template<class Receipt> bool FreshRoleHostReleaseComplete(PioneerQ35FreshRecoverySession&,
        PioneerStoreContext&,PioneerStoreContext&,const Receipt&) { return roleFreshHostComplete; }
    template<class Receipt> bool FreshRoleOwnerRetirementAllowed(PioneerQ35FreshRecoverySession&,
        PioneerStoreContext&,PioneerStoreContext&,const Receipt&) { return roleOwnerRetirementAllowed; }
    template<class Receipt> bool FreshRoleSessionRetirementAllowed(PioneerQ35FreshRecoverySession&,
        PioneerStoreContext&,PioneerStoreContext&,const Receipt&) {
        if (dropRoleRetirement) roleRetirementRecorded=false;
        return roleSessionRetirementAllowed;
    }
    bool FreshRecoverySetupAccessAllowed(PioneerQ35FreshRecoverySession&,PioneerStoreContext&,
        PioneerStoreContext&,unsigned) { return ++setupChecks!=failSetupAt; }
    bool FreshRecoveryAdmission(PioneerQ35FreshRecoverySession&,PioneerProbeCookie*,PioneerStoreContext&,uint64 ticket) {
        return admission && ticket==kTicket;
    }
    bool FreshRecoveryBoundary(PioneerQ35FreshRecoverySession& session,PioneerStoreContext& old,
        PioneerStoreContext& fresh,unsigned stage) {
        assert(old.recoveryFreshSession==&session && session.Fresh()==&fresh && session.Ticket()==kTicket);
        assert(fresh.profile==old.profile && !memcmp(fresh.initialized,old.initialized,PioneerResidentBanks::Pages));
        if (stage==failBoundary && dropAttachment) old.recoveryFreshSession=nullptr;
        if (stage==replaceRoleAtBoundary) old.qwen35SharedRoleBinding=&fresh;
        return stage!=failBoundary;
    }
    bool FreshRecoveryRestoreAdmission(PioneerQ35FreshRecoverySession&,PioneerStoreContext&,
        PioneerStoreContext&,const PioneerQ35ProjectionResetRecovery&) { return true; }
    bool FreshRecoveryRestoreComplete(PioneerQ35FreshRecoverySession&,PioneerStoreContext&,
        PioneerStoreContext&,const PioneerQ35ProjectionResetRecovery&) { return true; }
    template<class Receipt> bool FreshRecoveryCloseAdmission(PioneerQ35FreshRecoverySession&,PioneerStoreContext&,
        PioneerStoreContext&,const Receipt&) { return closeAdmission; }
    template<class Receipt> bool FreshRecoveryPostTransferAudit(PioneerQ35FreshRecoverySession&,PioneerStoreContext&,
        PioneerStoreContext& fresh,const Receipt&) {
        if (++postAuditCalls==2 && poisonLeaseBeforePci) fresh.lease.pending=true;
        return true;
    }
    template<class Receipt> bool FreshRecoveryAuditAccessAllowed(PioneerQ35FreshRecoverySession&,PioneerStoreContext&,
        PioneerStoreContext&,const Receipt&) { return ++auditChecks!=failAuditAt; }
    template<class Receipt> bool FreshRecoveryPciAccessAllowed(PioneerQ35FreshRecoverySession&,PioneerStoreContext&,
        PioneerStoreContext&,const Receipt&) { return ++pciChecks!=failPciAt; }
    template<class Receipt> bool FreshRecoveryAreaProtected(PioneerQ35FreshRecoverySession&,PioneerStoreContext&,
        PioneerStoreContext&,const Receipt&,int32_t area) { return protectFreshArea && area==91; }
    template<class Receipt> bool FreshRecoveryLeaseAccessAllowed(PioneerQ35FreshRecoverySession&,PioneerStoreContext&,
        PioneerStoreContext&,const Receipt&) { return ++leaseChecks!=failLeaseAt; }
    template<class Receipt> bool FreshRecoveryPciIdle(PioneerQ35FreshRecoverySession&,PioneerStoreContext&,
        PioneerStoreContext&,const Receipt&) { return Step(16); }
    bool FreshRecoveryRetirementAdmission(PioneerQ35FreshRecoverySession&,PioneerStoreContext&,
        PioneerStoreContext& fresh,const PioneerQ35ProjectionResetRecovery&) {
        if (quarantineAtRetirement) fresh.banks.Quarantine();
        return retirementAdmission;
    }
    bool FreshRecoveryAfterOwnerRetired(PioneerQ35FreshRecoverySession&,PioneerStoreContext& old,
        PioneerStoreContext&,const PioneerQ35ProjectionResetRecovery&) {
        if (!Step(18)) return false;
        if (loseClaimAfterRetirement) old.banks.claimed=false;
        return true;
    }
    bool QualificationModifiedResetRetirementAllowed(const MockQualificationOwner&,const MockScratch&,
        const PioneerQ35ProjectionResetRecovery&,const PioneerQ35FreshRecoverySession&) { return ownerAdmission; }
    void RetainFreshRecovery(PioneerQ35FreshRecoverySession&) { retained=true; }
};
static void Reset() {
    failedOperation=guardCalls=failGuardAt=0;failFreshAllocation=false;auditSourceMismatch=false;operations.clear();
    roleRetirementSucceeds=true;roleRetirementRecorded=false;roleDetachPrepared=false;
    sColdRecoveryRequired=true;sReloadQuiesced=false;sBankStorageOnly=true;sOpenCookies=1;
}
static void CheckRetained(PioneerQ35FreshRecoverySession* session,PioneerStoreContext& old,Scope& scope) {
    assert(session->CurrentStage()==PioneerQ35FreshRecoverySession::Retained && scope.retained
        && old.recoveryFreshSession==session && old.qwen35QualificationOwner.retained && old.banks.quarantined
        && session->Fresh() && session->Fresh()->banks.quarantined && session->Fresh()->lease.recovery_required
        && session->Fresh()->io.quarantined && sColdRecoveryRequired && !session->DiscardUnstarted());
}
int main() {
    pioneer_driver_recovery_guard_v2_info guard;
    unsigned fabricAuditChecks=0,fabricCloseGuardChecks=0,fabricPciChecks=0;
    for (unsigned invalid=0;invalid<7;invalid++) {
        Reset();PioneerStoreContext old(PioneerResidentBanks::Total);PioneerProbeCookie owner{&old};sRuntimeOwner=&owner;
        int role=1,graph=2;old.qwen35SharedRoleBinding=&role;old.qwen35GraphOwner=&graph;
        old.qwen35QualificationOwner.active=false;old.qwen35QualificationOwner.retained=false;
        old.io.fabricOldMmioDisposeComplete=true;
        old.recoveryFabricEvidenceRecorded=old.recoveryFabricPciIdleAttempted=old.recoveryFabricPciIdle=true;
        old.recoveryFabricAuditReleaseAttempted=old.recoveryFabricAuditReleased=true;
        old.recoveryFabricRoleViewsDisposed=true;
        Scope scope;
        if (invalid==0) old.qwen35SharedRoleBinding=nullptr;
        if (invalid==1) old.qwen35GraphOwner=nullptr;
        if (invalid==2) scope.roleAdmission=false;
        if (invalid==3) scope.replaceRoleAtAdmission=true;
        if (invalid==4) scope.replaceGraphAtAdmission=true;
        if (invalid==5) old.recoveryFabricEvidence.valid=false;
        if (invalid==6) old.recoveryFabricRoleViewsDisposed=false;
        auto* session=PioneerQ35FreshRecoverySession::Allocate();assert(session);
        assert(!session->PrepareFabricRole(&owner,old,&guard,scope) && operations.empty()
            && !old.recoveryFreshSession && !old.qwen35QualificationOwner.retained && session->DiscardUnstarted());
    }
    for (unsigned scenario=0;scenario<8;scenario++) {
        Reset();PioneerStoreContext old(PioneerResidentBanks::Total);PioneerProbeCookie owner{&old};sRuntimeOwner=&owner;
        int role=1,graph=2,replacement=3;old.qwen35SharedRoleBinding=&role;old.qwen35GraphOwner=&graph;
        old.qwen35QualificationOwner.active=false;old.qwen35QualificationOwner.retained=false;
        old.io.fabricOldMmioDisposeComplete=true;
        old.recoveryFabricEvidenceRecorded=old.recoveryFabricPciIdleAttempted=old.recoveryFabricPciIdle=true;
        old.recoveryFabricAuditReleaseAttempted=old.recoveryFabricAuditReleased=true;
        old.recoveryFabricRoleViewsDisposed=true;
        Scope scope;auto* session=PioneerQ35FreshRecoverySession::Allocate();assert(session);
        assert(session->PrepareFabricRole(&owner,old,&guard,scope) && session->RoleReady()
            && session->Fresh() && session->Fresh()->io.fleetEnabled && !old.io.fleetEnabled
            && !old.qwen35QualificationOwner.Active());
        session->Fresh()->io.fleetEnabled=false;
        assert(!session->RoleReady());
        session->Fresh()->io.fleetEnabled=true;
        assert(!session->RestoreScratch(&guard,scope) && !session->CloseTransport(&guard,scope)
            && !session->Retire(&guard,scope));
        if (scenario==1) old.qwen35SharedRoleBinding=&replacement;
        if (scenario==2) old.qwen35GraphOwner=&replacement;
        if (scenario==3) scope.roleRestored=false;
        if (scenario==4) failGuardAt=guardCalls+1;
        if (scenario==5) old.recoveryFabricRoleViewsDisposed=false;
        if (scenario==6) old.fabricRoleRestorationReady=false;
        if (scenario==7) scope.loseRoleEvidence=true;
        const bool verified=session->VerifyRoleRestoration(&guard,scope);
        if (!scenario) {
            assert(verified && !session->RoleReady() && session->CurrentStage()==PioneerQ35FreshRecoverySession::RoleVerified
                && old.recoveryFreshSession==session && old.qwen35SharedRoleBinding==&role
                && old.qwen35GraphOwner==&graph && !scope.retained && !old.qwen35QualificationOwner.retained);
        } else {
            assert(!verified && session->CurrentStage()==PioneerQ35FreshRecoverySession::Retained && scope.retained
                && old.banks.quarantined && session->Fresh()->banks.quarantined
                && session->Fresh()->io.quarantined && !old.qwen35QualificationOwner.retained);
        }
        PioneerQ35FreshRecoverySession::DisposeSimulated(session);
    }
    // A role session keeps both owners and host pages after closing only its
    // fresh transport. Selection must return to root before lease cleanup.
    for (unsigned scenario=0;scenario<10;scenario++) {
        Reset();PioneerStoreContext old(PioneerResidentBanks::Total);PioneerProbeCookie owner{&old};sRuntimeOwner=&owner;
        int role=1,graph=2,replacement=3;old.qwen35SharedRoleBinding=&role;old.qwen35GraphOwner=&graph;
        old.qwen35QualificationOwner.active=false;old.qwen35QualificationOwner.retained=false;
        old.io.fabricOldMmioDisposeComplete=true;
        old.recoveryFabricEvidenceRecorded=old.recoveryFabricPciIdleAttempted=old.recoveryFabricPciIdle=true;
        old.recoveryFabricAuditReleaseAttempted=old.recoveryFabricAuditReleased=old.recoveryFabricRoleViewsDisposed=true;
        Scope scope;auto* session=PioneerQ35FreshRecoverySession::Allocate();assert(session);
        assert(session->PrepareFabricRole(&owner,old,&guard,scope) && session->RoleReady());
        assert(!session->CloseRoleTransport(&guard,scope)); // role and graph receipt absent
        assert(session->VerifyRoleRestoration(&guard,scope));
        auto* fresh=session->Fresh();fresh->io.runtimeCore=119;
        if (scenario==1) scope.closeAdmission=false;
        if (scenario==2) failedOperation=21; // root selection
        if (scenario==3) failedOperation=8;  // lease close
        if (scenario==4) scope.protectFreshArea=true;
        if (scenario==5) scope.poisonLeaseBeforePci=true;
        if (scenario==6) old.qwen35SharedRoleBinding=&replacement;
        if (scenario==7) failGuardAt=guardCalls+1;
        if (scenario==8) scope.roleCloseAdmission=false;
        if (scenario==9) scope.rolePostLease=false;
        const bool closed=session->CloseRoleTransport(&guard,scope);
        if (!scenario) {
            assert(closed && fresh->io.runtimeCore==0 && fresh->io.freshMmioDisposed
                && session->CurrentStage()==PioneerQ35FreshRecoverySession::TransportClosed
                && old.recoveryFreshSession==session && old.qwen35SharedRoleBinding==&role
                && old.qwen35GraphOwner==&graph && old.q35PciReservationHeld
                && fresh->host.allocated && !fresh->host.released && !scope.retained);
            const unsigned count=operations.size();
            assert(!session->CloseRoleTransport(&guard,scope) && !session->Retire(&guard,scope)
                && operations.size()==count);
            assert(session->DisposeRoleOldHostBuffers(&guard,scope)
                && session->CurrentStage()==PioneerQ35FreshRecoverySession::RoleHostDisposed
                && old.io.fabricHostDisposeComplete && !old.qwen35QualificationOwner.retained
                && old.qwen35SharedRoleBinding==&role && old.qwen35GraphOwner==&graph
                && fresh->host.allocated && !fresh->host.released && old.q35PciReservationHeld);
            assert(session->PublishRoleMappingsDisposed(&guard,scope)
                && session->CurrentStage()==PioneerQ35FreshRecoverySession::RoleMappingsProved
                && old.recoveryFabricEvidence.ReadyToRetireOneOwner()
                && old.recoveryFreshSession==session && old.q35PciReservationHeld);
        } else {
            assert(!closed && session->CurrentStage()==PioneerQ35FreshRecoverySession::Retained
                && scope.retained && old.recoveryFreshSession==session && old.banks.quarantined
                && fresh->banks.quarantined && fresh->io.quarantined && old.q35PciReservationHeld
                && !old.qwen35QualificationOwner.retained);
        }
        PioneerQ35FreshRecoverySession::DisposeSimulated(session);
    }
    for (unsigned scenario=1;scenario<=7;scenario++) {
        Reset();PioneerStoreContext old(PioneerResidentBanks::Total);PioneerProbeCookie owner{&old};sRuntimeOwner=&owner;
        int role=1,graph=2,replacement=3;old.qwen35SharedRoleBinding=&role;old.qwen35GraphOwner=&graph;
        old.qwen35QualificationOwner.active=false;old.qwen35QualificationOwner.retained=false;
        old.io.fabricOldMmioDisposeComplete=true;
        old.recoveryFabricEvidenceRecorded=old.recoveryFabricPciIdleAttempted=old.recoveryFabricPciIdle=true;
        old.recoveryFabricAuditReleaseAttempted=old.recoveryFabricAuditReleased=old.recoveryFabricRoleViewsDisposed=true;
        Scope scope;auto* session=PioneerQ35FreshRecoverySession::Allocate();assert(session);
        assert(session->PrepareFabricRole(&owner,old,&guard,scope) && session->VerifyRoleRestoration(&guard,scope)
            && session->CloseRoleTransport(&guard,scope));
        if (scenario==1) scope.roleHostAdmission=false;
        if (scenario==2) scope.protectOldHost=true;
        if (scenario==3) failedOperation=22;
        if (scenario==4) scope.roleHostComplete=false;
        if (scenario==5) failGuardAt=guardCalls+1;
        if (scenario==6) old.qwen35GraphOwner=&replacement;
        if (scenario==7) scope.clearHostCompletion=true;
        assert(!session->DisposeRoleOldHostBuffers(&guard,scope)
            && session->CurrentStage()==PioneerQ35FreshRecoverySession::Retained && scope.retained
            && old.recoveryFreshSession==session && old.io.quarantined && old.banks.quarantined
            && session->Fresh()->io.quarantined && old.q35PciReservationHeld);
        const unsigned count=operations.size();
        assert(!session->DisposeRoleOldHostBuffers(&guard,scope) && operations.size()==count);
        PioneerQ35FreshRecoverySession::DisposeSimulated(session);
    }
    unsigned mappingsChecks=0;
    for (unsigned failure=0;;failure++) {
        if (failure && failure>mappingsChecks) break;
        Reset();PioneerStoreContext old(PioneerResidentBanks::Total);PioneerProbeCookie owner{&old};sRuntimeOwner=&owner;
        int role=1,graph=2;old.qwen35SharedRoleBinding=&role;old.qwen35GraphOwner=&graph;
        old.qwen35QualificationOwner.active=false;old.qwen35QualificationOwner.retained=false;
        old.io.fabricOldMmioDisposeComplete=true;
        old.recoveryFabricEvidenceRecorded=old.recoveryFabricPciIdleAttempted=old.recoveryFabricPciIdle=true;
        old.recoveryFabricAuditReleaseAttempted=old.recoveryFabricAuditReleased=old.recoveryFabricRoleViewsDisposed=true;
        Scope scope;auto* session=PioneerQ35FreshRecoverySession::Allocate();assert(session);
        assert(session->PrepareFabricRole(&owner,old,&guard,scope) && session->VerifyRoleRestoration(&guard,scope)
            && session->CloseRoleTransport(&guard,scope) && session->DisposeRoleOldHostBuffers(&guard,scope));
        const unsigned before=guardCalls;if (failure) failGuardAt=before+failure;
        const bool published=session->PublishRoleMappingsDisposed(&guard,scope);
        if (!failure) {
            assert(published && old.recoveryFabricEvidence.ReadyToRetireOneOwner());
            mappingsChecks=guardCalls-before;assert(mappingsChecks>=4);
        } else {
            assert(!published && scope.retained && old.recoveryFabricEvidence.ReadyForScratchRestore()
                && !old.recoveryFabricEvidence.ReadyToRetireOneOwner()
                && session->CurrentStage()==PioneerQ35FreshRecoverySession::Retained);
        }
        PioneerQ35FreshRecoverySession::DisposeSimulated(session);
    }
    std::printf("PASS old-mapping evidence: %u simulated ticket-loss boundaries\n",mappingsChecks);
    for (unsigned denied=0;denied<2;denied++) {
        Reset();PioneerStoreContext old(PioneerResidentBanks::Total);PioneerProbeCookie owner{&old};sRuntimeOwner=&owner;
        int role=1,graph=2;old.qwen35SharedRoleBinding=&role;old.qwen35GraphOwner=&graph;
        old.qwen35QualificationOwner.active=false;old.qwen35QualificationOwner.retained=false;
        old.io.fabricOldMmioDisposeComplete=true;
        old.recoveryFabricEvidenceRecorded=old.recoveryFabricPciIdleAttempted=old.recoveryFabricPciIdle=true;
        old.recoveryFabricAuditReleaseAttempted=old.recoveryFabricAuditReleased=old.recoveryFabricRoleViewsDisposed=true;
        Scope scope;auto* session=PioneerQ35FreshRecoverySession::Allocate();assert(session);
        assert(session->PrepareFabricRole(&owner,old,&guard,scope) && session->VerifyRoleRestoration(&guard,scope)
            && session->CloseRoleTransport(&guard,scope) && session->DisposeRoleOldHostBuffers(&guard,scope));
        if (denied) scope.roleMappingsComplete=false;else scope.roleMappingsAdmission=false;
        assert(!session->PublishRoleMappingsDisposed(&guard,scope) && scope.retained
            && old.recoveryFabricEvidence.ReadyForScratchRestore()
            && !old.recoveryFabricEvidence.ReadyToRetireOneOwner()
            && session->CurrentStage()==PioneerQ35FreshRecoverySession::Retained);
        PioneerQ35FreshRecoverySession::DisposeSimulated(session);
    }
    unsigned graphChecks=0;
    for (unsigned failure=0;;failure++) {
        if (failure && failure>graphChecks) break;
        Reset();PioneerStoreContext old(PioneerResidentBanks::Total);PioneerProbeCookie owner{&old};sRuntimeOwner=&owner;
        int role=1,graph=2;old.qwen35SharedRoleBinding=&role;old.qwen35GraphOwner=&graph;
        old.qwen35QualificationOwner.active=false;old.qwen35QualificationOwner.retained=false;
        old.io.fabricOldMmioDisposeComplete=true;
        old.recoveryFabricEvidenceRecorded=old.recoveryFabricPciIdleAttempted=old.recoveryFabricPciIdle=true;
        old.recoveryFabricAuditReleaseAttempted=old.recoveryFabricAuditReleased=old.recoveryFabricRoleViewsDisposed=true;
        Scope scope;auto* session=PioneerQ35FreshRecoverySession::Allocate();assert(session);
        assert(session->PrepareFabricRole(&owner,old,&guard,scope) && session->VerifyRoleRestoration(&guard,scope)
            && session->CloseRoleTransport(&guard,scope) && session->DisposeRoleOldHostBuffers(&guard,scope)
            && session->PublishRoleMappingsDisposed(&guard,scope));
        const unsigned before=guardCalls;if (failure) failGuardAt=before+failure;
        const bool retired=session->RetireRoleGraph(&guard,scope);
        if (!failure) {
            assert(retired && session->CurrentStage()==PioneerQ35FreshRecoverySession::RoleGraphRetired
                && old.recoveryFreshSession==session && old.qwen35SharedRoleBinding==&role
                && old.qwen35GraphOwner==&graph && old.q35PciReservationHeld
                && session->Fresh()->host.allocated && !session->Fresh()->host.released);
            graphChecks=guardCalls-before;assert(graphChecks>=6);
        } else {
            assert(!retired && scope.retained && old.recoveryFreshSession==session
                && old.q35PciReservationHeld && session->CurrentStage()==PioneerQ35FreshRecoverySession::Retained);
        }
        const unsigned count=operations.size();
        assert(!session->RetireRoleGraph(&guard,scope) && operations.size()==count);
        PioneerQ35FreshRecoverySession::DisposeSimulated(session);
    }
    for (unsigned denied=0;denied<3;denied++) {
        Reset();PioneerStoreContext old(PioneerResidentBanks::Total);PioneerProbeCookie owner{&old};sRuntimeOwner=&owner;
        int role=1,graph=2;old.qwen35SharedRoleBinding=&role;old.qwen35GraphOwner=&graph;
        old.qwen35QualificationOwner.active=false;old.qwen35QualificationOwner.retained=false;
        old.io.fabricOldMmioDisposeComplete=true;
        old.recoveryFabricEvidenceRecorded=old.recoveryFabricPciIdleAttempted=old.recoveryFabricPciIdle=true;
        old.recoveryFabricAuditReleaseAttempted=old.recoveryFabricAuditReleased=old.recoveryFabricRoleViewsDisposed=true;
        Scope scope;auto* session=PioneerQ35FreshRecoverySession::Allocate();assert(session);
        assert(session->PrepareFabricRole(&owner,old,&guard,scope) && session->VerifyRoleRestoration(&guard,scope)
            && session->CloseRoleTransport(&guard,scope) && session->DisposeRoleOldHostBuffers(&guard,scope)
            && session->PublishRoleMappingsDisposed(&guard,scope));
        if (denied==0) scope.roleGraphAdmission=false;
        if (denied==1) failedOperation=23;
        if (denied==2) scope.roleGraphComplete=false;
        assert(!session->RetireRoleGraph(&guard,scope) && scope.retained
            && old.recoveryFreshSession==session && old.q35PciReservationHeld
            && session->CurrentStage()==PioneerQ35FreshRecoverySession::Retained);
        const unsigned count=operations.size();
        assert(!session->RetireRoleGraph(&guard,scope) && operations.size()==count);
        PioneerQ35FreshRecoverySession::DisposeSimulated(session);
    }
    std::printf("PASS graph retirement: %u simulated ticket-loss boundaries, one-shot retention\n",graphChecks);
    unsigned freshHostChecks=0;
    for (unsigned failure=0;;failure++) {
        if (failure && failure>freshHostChecks) break;
        Reset();PioneerStoreContext old(PioneerResidentBanks::Total);PioneerProbeCookie owner{&old};sRuntimeOwner=&owner;
        int role=1,graph=2;old.qwen35SharedRoleBinding=&role;old.qwen35GraphOwner=&graph;
        old.qwen35QualificationOwner.active=false;old.qwen35QualificationOwner.retained=false;
        old.io.fabricOldMmioDisposeComplete=true;
        old.recoveryFabricEvidenceRecorded=old.recoveryFabricPciIdleAttempted=old.recoveryFabricPciIdle=true;
        old.recoveryFabricAuditReleaseAttempted=old.recoveryFabricAuditReleased=old.recoveryFabricRoleViewsDisposed=true;
        Scope scope;auto* session=PioneerQ35FreshRecoverySession::Allocate();assert(session);
        assert(session->PrepareFabricRole(&owner,old,&guard,scope) && session->VerifyRoleRestoration(&guard,scope)
            && session->CloseRoleTransport(&guard,scope) && session->DisposeRoleOldHostBuffers(&guard,scope)
            && session->PublishRoleMappingsDisposed(&guard,scope) && session->RetireRoleGraph(&guard,scope));
        auto* fresh=session->Fresh();const unsigned before=guardCalls;if (failure) failGuardAt=before+failure;
        const bool released=session->ReleaseRoleFreshHostPage(&guard,scope);
        if (!failure) {
            assert(released && session->CurrentStage()==PioneerQ35FreshRecoverySession::RoleFreshHostReleased
                && fresh->host.released && fresh->io.freshHostReleaseAttempted && !scope.retained
                && old.recoveryFreshSession==session && old.q35PciReservationHeld);
            freshHostChecks=guardCalls-before;assert(freshHostChecks>=4);
        } else {
            assert(!released && scope.retained && old.recoveryFreshSession==session
                && old.q35PciReservationHeld && session->CurrentStage()==PioneerQ35FreshRecoverySession::Retained);
            assert(bool(fresh->host.released)==fresh->io.freshHostReleaseAttempted);
        }
        const unsigned count=operations.size();
        assert(!session->ReleaseRoleFreshHostPage(&guard,scope) && operations.size()==count);
        PioneerQ35FreshRecoverySession::DisposeSimulated(session);
    }
    for (unsigned denied=0;denied<4;denied++) {
        Reset();PioneerStoreContext old(PioneerResidentBanks::Total);PioneerProbeCookie owner{&old};sRuntimeOwner=&owner;
        int role=1,graph=2;old.qwen35SharedRoleBinding=&role;old.qwen35GraphOwner=&graph;
        old.qwen35QualificationOwner.active=false;old.qwen35QualificationOwner.retained=false;
        old.io.fabricOldMmioDisposeComplete=true;
        old.recoveryFabricEvidenceRecorded=old.recoveryFabricPciIdleAttempted=old.recoveryFabricPciIdle=true;
        old.recoveryFabricAuditReleaseAttempted=old.recoveryFabricAuditReleased=old.recoveryFabricRoleViewsDisposed=true;
        Scope scope;auto* session=PioneerQ35FreshRecoverySession::Allocate();assert(session);
        assert(session->PrepareFabricRole(&owner,old,&guard,scope) && session->VerifyRoleRestoration(&guard,scope)
            && session->CloseRoleTransport(&guard,scope) && session->DisposeRoleOldHostBuffers(&guard,scope)
            && session->PublishRoleMappingsDisposed(&guard,scope) && session->RetireRoleGraph(&guard,scope));
        auto* fresh=session->Fresh();
        if (denied==0) scope.roleFreshHostAdmission=false;
        if (denied==1) failedOperation=17;
        if (denied==2) scope.roleFreshHostComplete=false;
        if (denied==3) fresh->io.freshHostReleaseReady=false;
        assert(!session->ReleaseRoleFreshHostPage(&guard,scope) && scope.retained
            && old.recoveryFreshSession==session && old.q35PciReservationHeld
            && session->CurrentStage()==PioneerQ35FreshRecoverySession::Retained);
        assert(bool(fresh->host.released)==(denied==2));
        const unsigned count=operations.size();
        assert(!session->ReleaseRoleFreshHostPage(&guard,scope) && operations.size()==count);
        PioneerQ35FreshRecoverySession::DisposeSimulated(session);
    }
    std::printf("PASS fresh host release: %u simulated ticket-loss boundaries, one-shot retention\n",freshHostChecks);
    unsigned roleOwnerChecks=0;
    for (unsigned failure=0;;failure++) {
        if (failure && failure>roleOwnerChecks) break;
        Reset();PioneerStoreContext old(PioneerResidentBanks::Total);PioneerProbeCookie owner{&old};sRuntimeOwner=&owner;
        int role=1,graph=2;old.qwen35SharedRoleBinding=&role;old.qwen35GraphOwner=&graph;
        old.qwen35QualificationOwner.active=false;old.qwen35QualificationOwner.retained=false;
        old.io.fabricOldMmioDisposeComplete=true;
        old.recoveryFabricEvidenceRecorded=old.recoveryFabricPciIdleAttempted=old.recoveryFabricPciIdle=true;
        old.recoveryFabricAuditReleaseAttempted=old.recoveryFabricAuditReleased=old.recoveryFabricRoleViewsDisposed=true;
        Scope scope;auto* session=PioneerQ35FreshRecoverySession::Allocate();assert(session);
        assert(session->PrepareFabricRole(&owner,old,&guard,scope) && session->VerifyRoleRestoration(&guard,scope)
            && session->CloseRoleTransport(&guard,scope) && session->DisposeRoleOldHostBuffers(&guard,scope)
            && session->PublishRoleMappingsDisposed(&guard,scope) && session->RetireRoleGraph(&guard,scope)
            && session->ReleaseRoleFreshHostPage(&guard,scope) && session->RoleOwnerRetirementReady(kTicket));
        const unsigned before=guardCalls;if (failure) failGuardAt=before+failure;
        const bool recorded=session->RecordRoleOwnerRetirement(&guard,scope);
        if (!failure) {
            assert(recorded && session->CurrentStage()==PioneerQ35FreshRecoverySession::RoleOwnerRetired
                && old.recoveryFreshSession==session && old.qwen35SharedRoleBinding==&role
                && old.qwen35GraphOwner==&graph && old.q35PciReservationHeld && !scope.retained);
            roleOwnerChecks=guardCalls-before;assert(roleOwnerChecks>=6);
        } else assert(!recorded && scope.retained && old.recoveryFreshSession==session
            && old.q35PciReservationHeld && session->CurrentStage()==PioneerQ35FreshRecoverySession::Retained);
        const unsigned count=operations.size();
        assert(!session->RecordRoleOwnerRetirement(&guard,scope) && operations.size()==count);
        PioneerQ35FreshRecoverySession::DisposeSimulated(session);
    }
    for (unsigned denial=0;denial<3;denial++) {
        Reset();PioneerStoreContext old(PioneerResidentBanks::Total);PioneerProbeCookie owner{&old};sRuntimeOwner=&owner;
        int role=1,graph=2;old.qwen35SharedRoleBinding=&role;old.qwen35GraphOwner=&graph;
        old.qwen35QualificationOwner.active=false;old.qwen35QualificationOwner.retained=false;
        old.io.fabricOldMmioDisposeComplete=true;
        old.recoveryFabricEvidenceRecorded=old.recoveryFabricPciIdleAttempted=old.recoveryFabricPciIdle=true;
        old.recoveryFabricAuditReleaseAttempted=old.recoveryFabricAuditReleased=old.recoveryFabricRoleViewsDisposed=true;
        Scope scope;auto* session=PioneerQ35FreshRecoverySession::Allocate();assert(session);
        assert(session->PrepareFabricRole(&owner,old,&guard,scope) && session->VerifyRoleRestoration(&guard,scope)
            && session->CloseRoleTransport(&guard,scope) && session->DisposeRoleOldHostBuffers(&guard,scope)
            && session->PublishRoleMappingsDisposed(&guard,scope) && session->RetireRoleGraph(&guard,scope)
            && session->ReleaseRoleFreshHostPage(&guard,scope));
        if (denial==0) scope.roleOwnerRetirementAllowed=false;
        if (denial==1) roleRetirementSucceeds=false;
        if (denial==2) failedOperation=24;
        assert(!session->RecordRoleOwnerRetirement(&guard,scope) && scope.retained
            && old.recoveryFreshSession==session && old.q35PciReservationHeld
            && session->CurrentStage()==PioneerQ35FreshRecoverySession::Retained);
        PioneerQ35FreshRecoverySession::DisposeSimulated(session);
    }
    std::printf("PASS role-owner receipt: %u simulated ticket-loss boundaries, ownership retained\n",roleOwnerChecks);
    unsigned roleSessionChecks=0;
    for (unsigned failure=0;;failure++) {
        if (failure && failure>roleSessionChecks) break;
        Reset();PioneerStoreContext old(PioneerResidentBanks::Total);PioneerProbeCookie owner{&old};sRuntimeOwner=&owner;
        int role=1,graph=2;old.qwen35SharedRoleBinding=&role;old.qwen35GraphOwner=&graph;
        old.qwen35QualificationOwner.active=false;old.qwen35QualificationOwner.retained=false;
        old.io.fabricOldMmioDisposeComplete=true;
        old.recoveryFabricEvidenceRecorded=old.recoveryFabricPciIdleAttempted=old.recoveryFabricPciIdle=true;
        old.recoveryFabricAuditReleaseAttempted=old.recoveryFabricAuditReleased=old.recoveryFabricRoleViewsDisposed=true;
        Scope scope;auto* session=PioneerQ35FreshRecoverySession::Allocate();assert(session);
        assert(session->PrepareFabricRole(&owner,old,&guard,scope) && session->VerifyRoleRestoration(&guard,scope)
            && session->CloseRoleTransport(&guard,scope) && session->DisposeRoleOldHostBuffers(&guard,scope)
            && session->PublishRoleMappingsDisposed(&guard,scope) && session->RetireRoleGraph(&guard,scope)
            && session->ReleaseRoleFreshHostPage(&guard,scope) && session->RecordRoleOwnerRetirement(&guard,scope)
            && session->RoleSessionRetirementReady(kTicket));
        const unsigned before=guardCalls;if (failure) failGuardAt=before+failure;
        const bool retired=session->RetireRoleSession(&guard,scope);
        if (!failure) {
            assert(retired && roleDetachPrepared && !old.recoveryFreshSession && old.qwen35SharedRoleBinding==&role
                && old.qwen35GraphOwner==&graph && old.q35PciReservationHeld && !scope.retained);
            roleSessionChecks=guardCalls-before;assert(roleSessionChecks>=2);
        } else {
            assert(!retired && scope.retained && old.recoveryFreshSession==session && old.q35PciReservationHeld
                && session->CurrentStage()==PioneerQ35FreshRecoverySession::Retained);
            assert(!session->RetireRoleSession(&guard,scope));
            PioneerQ35FreshRecoverySession::DisposeSimulated(session);
        }
    }
    for (unsigned denial=0;denial<2;denial++) {
        Reset();PioneerStoreContext old(PioneerResidentBanks::Total);PioneerProbeCookie owner{&old};sRuntimeOwner=&owner;
        int role=1,graph=2;old.qwen35SharedRoleBinding=&role;old.qwen35GraphOwner=&graph;
        old.qwen35QualificationOwner.active=false;old.qwen35QualificationOwner.retained=false;
        old.io.fabricOldMmioDisposeComplete=true;
        old.recoveryFabricEvidenceRecorded=old.recoveryFabricPciIdleAttempted=old.recoveryFabricPciIdle=true;
        old.recoveryFabricAuditReleaseAttempted=old.recoveryFabricAuditReleased=old.recoveryFabricRoleViewsDisposed=true;
        Scope scope;auto* session=PioneerQ35FreshRecoverySession::Allocate();assert(session);
        assert(session->PrepareFabricRole(&owner,old,&guard,scope) && session->VerifyRoleRestoration(&guard,scope)
            && session->CloseRoleTransport(&guard,scope) && session->DisposeRoleOldHostBuffers(&guard,scope)
            && session->PublishRoleMappingsDisposed(&guard,scope) && session->RetireRoleGraph(&guard,scope)
            && session->ReleaseRoleFreshHostPage(&guard,scope) && session->RecordRoleOwnerRetirement(&guard,scope));
        if (denial==0) scope.roleSessionRetirementAllowed=false;else scope.dropRoleRetirement=true;
        assert(!session->RetireRoleSession(&guard,scope) && scope.retained
            && old.recoveryFreshSession==session && old.q35PciReservationHeld
            && session->CurrentStage()==PioneerQ35FreshRecoverySession::Retained);
        PioneerQ35FreshRecoverySession::DisposeSimulated(session);
    }
    std::printf("PASS fresh role-session retirement: %u simulated ticket-loss boundaries; owners retained\n",roleSessionChecks);
    unsigned roleCloseGuardChecks=0;
    for (unsigned failAt=0;;failAt++) {
        if (failAt && failAt>roleCloseGuardChecks) break;
        Reset();PioneerStoreContext old(PioneerResidentBanks::Total);PioneerProbeCookie owner{&old};sRuntimeOwner=&owner;
        int role=1,graph=2;old.qwen35SharedRoleBinding=&role;old.qwen35GraphOwner=&graph;
        old.qwen35QualificationOwner.active=false;old.qwen35QualificationOwner.retained=false;
        old.io.fabricOldMmioDisposeComplete=true;
        old.recoveryFabricEvidenceRecorded=old.recoveryFabricPciIdleAttempted=old.recoveryFabricPciIdle=true;
        old.recoveryFabricAuditReleaseAttempted=old.recoveryFabricAuditReleased=old.recoveryFabricRoleViewsDisposed=true;
        Scope scope;auto* session=PioneerQ35FreshRecoverySession::Allocate();assert(session);
        assert(session->PrepareFabricRole(&owner,old,&guard,scope) && session->VerifyRoleRestoration(&guard,scope));
        session->Fresh()->io.runtimeCore=119;
        const unsigned before=guardCalls;
        if (failAt) failGuardAt=before+failAt;
        const bool closed=session->CloseRoleTransport(&guard,scope);
        if (!failAt) {
            assert(closed);roleCloseGuardChecks=guardCalls-before;assert(roleCloseGuardChecks>20);
        } else {
            assert(!closed && scope.retained && session->CurrentStage()==PioneerQ35FreshRecoverySession::Retained
                && old.recoveryFreshSession==session && old.banks.quarantined && session->Fresh()->io.quarantined);
            const unsigned count=operations.size();
            assert(!session->CloseRoleTransport(&guard,scope) && operations.size()==count);
        }
        PioneerQ35FreshRecoverySession::DisposeSimulated(session);
    }
    std::printf("PASS fresh role transport: %u simulated ticket-loss boundaries\n",roleCloseGuardChecks);
    for (unsigned boundary=1;boundary<=6;boundary++) {
        Reset();PioneerStoreContext old(PioneerResidentBanks::Total);PioneerProbeCookie owner{&old};sRuntimeOwner=&owner;
        int role=1,graph=2;old.qwen35SharedRoleBinding=&role;old.qwen35GraphOwner=&graph;
        old.qwen35QualificationOwner.active=false;old.qwen35QualificationOwner.retained=false;
        old.io.fabricOldMmioDisposeComplete=true;
        old.recoveryFabricEvidenceRecorded=old.recoveryFabricPciIdleAttempted=old.recoveryFabricPciIdle=true;
        old.recoveryFabricAuditReleaseAttempted=old.recoveryFabricAuditReleased=old.recoveryFabricRoleViewsDisposed=true;
        Scope scope;scope.replaceRoleAtBoundary=boundary;
        auto* session=PioneerQ35FreshRecoverySession::Allocate();assert(session);
        assert(!session->PrepareFabricRole(&owner,old,&guard,scope)
            && session->CurrentStage()==PioneerQ35FreshRecoverySession::Retained && session->FailureStage()==boundary
            && old.recoveryFreshSession==session && old.banks.quarantined && session->Fresh()->banks.quarantined
            && session->Fresh()->io.quarantined && scope.retained && !old.qwen35QualificationOwner.retained);
        PioneerQ35FreshRecoverySession::DisposeSimulated(session);
    }
    for (unsigned failure=1;failure<=10;failure++) {
        Reset();PioneerStoreContext old(PioneerResidentBanks::Total);PioneerProbeCookie owner{&old};sRuntimeOwner=&owner;
        int role=1,graph=2;old.qwen35SharedRoleBinding=&role;old.qwen35GraphOwner=&graph;
        old.qwen35QualificationOwner.active=false;old.qwen35QualificationOwner.retained=false;
        old.io.fabricOldMmioDisposeComplete=true;
        old.recoveryFabricEvidenceRecorded=old.recoveryFabricPciIdleAttempted=old.recoveryFabricPciIdle=true;
        old.recoveryFabricAuditReleaseAttempted=old.recoveryFabricAuditReleased=old.recoveryFabricRoleViewsDisposed=true;
        Scope scope;scope.failSetupAt=failure;
        auto* session=PioneerQ35FreshRecoverySession::Allocate();assert(session);
        assert(!session->PrepareFabricRole(&owner,old,&guard,scope) && scope.setupChecks==failure
            && session->CurrentStage()==PioneerQ35FreshRecoverySession::Retained
            && session->FailureStage()==(failure<=2?PioneerQ35FreshRecoverySession::Mapped
                :failure<=6?PioneerQ35FreshRecoverySession::Baseline
                :failure<=8?PioneerQ35FreshRecoverySession::Host:PioneerQ35FreshRecoverySession::Lease)
            && old.recoveryFreshSession==session && old.qwen35SharedRoleBinding==&role && old.qwen35GraphOwner==&graph
            && old.banks.quarantined && session->Fresh()->io.quarantined && scope.retained
            && !old.qwen35QualificationOwner.retained);
        const auto count=operations.size();
        assert(!session->PrepareFabricRole(&owner,old,&guard,scope) && operations.size()==count);
        PioneerQ35FreshRecoverySession::DisposeSimulated(session);
    }
    for (unsigned invalid=0;invalid<8;invalid++) {
        Reset();PioneerStoreContext old(PioneerResidentBanks::Total);PioneerProbeCookie owner{&old};sRuntimeOwner=&owner;
        old.io.fabricOldMmioDisposeComplete=true;
        old.recoveryFabricEvidenceRecorded=old.recoveryFabricPciIdleAttempted=old.recoveryFabricPciIdle=true;
        old.recoveryFabricAuditReleaseAttempted=old.recoveryFabricAuditReleased=true;
        if (invalid==0) old.recoveryFabricEvidence.valid=false;
        if (invalid==1) old.recoveryFabricEvidence.fabric_generation++;
        if (invalid==2) old.recoveryFabricEvidence.firmware_generation++;
        if (invalid==3) old.io.fabricOldMmioDisposeComplete=false;
        if (invalid==4) old.io.postResetDisposeComplete=true;
        if (invalid==5) old.recoveryFabricAuditReleased=false;
        if (invalid==6) old.q35PciReservationHeld=false;
        if (invalid==7) old.recoveryFabricArcOwner=&old;
        Scope scope;auto* session=PioneerQ35FreshRecoverySession::Allocate();assert(session);
        assert(!session->PrepareFabric(&owner,old,&guard,scope) && operations.empty()
            && !old.recoveryFreshSession && session->DiscardUnstarted());
    }
    {
        Reset();PioneerStoreContext old(PioneerResidentBanks::Total);PioneerProbeCookie owner{&old};sRuntimeOwner=&owner;
        old.io.fabricOldMmioDisposeComplete=true;
        old.recoveryFabricEvidenceRecorded=old.recoveryFabricPciIdleAttempted=old.recoveryFabricPciIdle=true;
        old.recoveryFabricAuditReleaseAttempted=old.recoveryFabricAuditReleased=true;
        Scope scope;auto* session=PioneerQ35FreshRecoverySession::Allocate();assert(session);
        assert(!session->Prepare(&owner,old,&guard,scope) && operations.empty());
        assert(session->PrepareFabric(&owner,old,&guard,scope)
            && session->RestoreScratch(&guard,scope));
        const unsigned guardsBeforeClose=guardCalls;
        assert(session->CloseTransport(&guard,scope));
        fabricAuditChecks=scope.auditChecks;fabricCloseGuardChecks=guardCalls-guardsBeforeClose;
        fabricPciChecks=scope.pciChecks;
        assert(fabricAuditChecks>16 && fabricCloseGuardChecks>fabricAuditChecks
            && fabricPciChecks>=4 && session->Fresh()->io.freshMmioDisposed);
        assert(session->Retire(&guard,scope) && !old.recoveryFreshSession && old.qwen35QualificationOwner.RetiredModifiedIdentity(
            &old.qwen35QualificationOwner.scratch,&old.qwen35QualificationOwner.snapshot)
            && old.io.fabricOldMmioDisposeComplete && !old.io.postResetDisposeComplete && !scope.retained);
    }
    for (unsigned failedCheck=1;failedCheck<=fabricPciChecks;failedCheck++) {
        Reset();PioneerStoreContext old(PioneerResidentBanks::Total);PioneerProbeCookie owner{&old};sRuntimeOwner=&owner;
        old.io.fabricOldMmioDisposeComplete=true;
        old.recoveryFabricEvidenceRecorded=old.recoveryFabricPciIdleAttempted=old.recoveryFabricPciIdle=true;
        old.recoveryFabricAuditReleaseAttempted=old.recoveryFabricAuditReleased=true;
        Scope scope;auto* session=PioneerQ35FreshRecoverySession::Allocate();assert(session);
        assert(session->PrepareFabric(&owner,old,&guard,scope) && session->RestoreScratch(&guard,scope));
        scope.failPciAt=failedCheck;
        assert(!session->CloseTransport(&guard,scope));CheckRetained(session,old,scope);
        assert(scope.pciChecks==failedCheck && session->FailureStage()==PioneerQ35FreshRecoverySession::PciClosed);
        const auto count=operations.size();
        assert(!session->CloseTransport(&guard,scope) && operations.size()==count);
        PioneerQ35FreshRecoverySession::DisposeSimulated(session);
    }
    for (unsigned refusal=0;refusal<3;refusal++) {
        Reset();PioneerStoreContext old(PioneerResidentBanks::Total);PioneerProbeCookie owner{&old};sRuntimeOwner=&owner;
        old.io.fabricOldMmioDisposeComplete=true;
        old.recoveryFabricEvidenceRecorded=old.recoveryFabricPciIdleAttempted=old.recoveryFabricPciIdle=true;
        old.recoveryFabricAuditReleaseAttempted=old.recoveryFabricAuditReleased=true;
        Scope scope;auto* session=PioneerQ35FreshRecoverySession::Allocate();assert(session);
        assert(session->PrepareFabric(&owner,old,&guard,scope) && session->RestoreScratch(&guard,scope));
        if (refusal==0) session->Fresh()->io.freshMmioReady=false;
        if (refusal==1) scope.protectFreshArea=true;
        if (refusal==2) scope.poisonLeaseBeforePci=true;
        assert(!session->CloseTransport(&guard,scope));CheckRetained(session,old,scope);
        assert(session->FailureStage()==PioneerQ35FreshRecoverySession::PciClosed
            && !session->Fresh()->io.freshMmioDisposed);
        PioneerQ35FreshRecoverySession::DisposeSimulated(session);
    }
    for (unsigned failedCheck=1;failedCheck<=fabricAuditChecks;failedCheck++) {
        Reset();PioneerStoreContext old(PioneerResidentBanks::Total);PioneerProbeCookie owner{&old};sRuntimeOwner=&owner;
        old.io.fabricOldMmioDisposeComplete=true;
        old.recoveryFabricEvidenceRecorded=old.recoveryFabricPciIdleAttempted=old.recoveryFabricPciIdle=true;
        old.recoveryFabricAuditReleaseAttempted=old.recoveryFabricAuditReleased=true;
        Scope scope;auto* session=PioneerQ35FreshRecoverySession::Allocate();assert(session);
        assert(session->PrepareFabric(&owner,old,&guard,scope) && session->RestoreScratch(&guard,scope));
        scope.failAuditAt=failedCheck;
        assert(!session->CloseTransport(&guard,scope));CheckRetained(session,old,scope);
        assert(scope.auditChecks==failedCheck && session->Fresh()->recoveryMapping);
        const auto count=operations.size();
        assert(!session->CloseTransport(&guard,scope) && operations.size()==count);
        PioneerQ35FreshRecoverySession::DisposeSimulated(session);
    }
    for (unsigned failedCheck=1;failedCheck<=2;failedCheck++) {
        Reset();PioneerStoreContext old(PioneerResidentBanks::Total);PioneerProbeCookie owner{&old};sRuntimeOwner=&owner;
        old.io.fabricOldMmioDisposeComplete=true;
        old.recoveryFabricEvidenceRecorded=old.recoveryFabricPciIdleAttempted=old.recoveryFabricPciIdle=true;
        old.recoveryFabricAuditReleaseAttempted=old.recoveryFabricAuditReleased=true;
        Scope scope;auto* session=PioneerQ35FreshRecoverySession::Allocate();assert(session);
        assert(session->PrepareFabric(&owner,old,&guard,scope) && session->RestoreScratch(&guard,scope));
        scope.failLeaseAt=failedCheck;
        assert(!session->CloseTransport(&guard,scope));CheckRetained(session,old,scope);
        assert(scope.leaseChecks==failedCheck && !scope.auditChecks && !session->Fresh()->recoveryMapping);
        const auto count=operations.size();
        assert(!session->CloseTransport(&guard,scope) && operations.size()==count);
        PioneerQ35FreshRecoverySession::DisposeSimulated(session);
    }
    for (unsigned failedCheck=1;failedCheck<=fabricCloseGuardChecks;failedCheck++) {
        Reset();PioneerStoreContext old(PioneerResidentBanks::Total);PioneerProbeCookie owner{&old};sRuntimeOwner=&owner;
        old.io.fabricOldMmioDisposeComplete=true;
        old.recoveryFabricEvidenceRecorded=old.recoveryFabricPciIdleAttempted=old.recoveryFabricPciIdle=true;
        old.recoveryFabricAuditReleaseAttempted=old.recoveryFabricAuditReleased=true;
        Scope scope;auto* session=PioneerQ35FreshRecoverySession::Allocate();assert(session);
        assert(session->PrepareFabric(&owner,old,&guard,scope) && session->RestoreScratch(&guard,scope));
        failGuardAt=guardCalls+failedCheck;
        assert(!session->CloseTransport(&guard,scope));CheckRetained(session,old,scope);
        assert(guardCalls==failGuardAt);
        const auto count=operations.size();
        assert(!session->CloseTransport(&guard,scope) && operations.size()==count);
        PioneerQ35FreshRecoverySession::DisposeSimulated(session);
    }
    for (unsigned failure : {9u,10u,11u,12u,13u,19u,20u,14u}) {
        Reset();PioneerStoreContext old(PioneerResidentBanks::Total);PioneerProbeCookie owner{&old};sRuntimeOwner=&owner;
        old.io.fabricOldMmioDisposeComplete=true;
        old.recoveryFabricEvidenceRecorded=old.recoveryFabricPciIdleAttempted=old.recoveryFabricPciIdle=true;
        old.recoveryFabricAuditReleaseAttempted=old.recoveryFabricAuditReleased=true;
        Scope scope;auto* session=PioneerQ35FreshRecoverySession::Allocate();assert(session);
        assert(session->PrepareFabric(&owner,old,&guard,scope) && session->RestoreScratch(&guard,scope));
        failedOperation=failure;
        assert(!session->CloseTransport(&guard,scope));CheckRetained(session,old,scope);
        assert(session->Fresh()->recoveryMapping && operations.back()==failure);
        PioneerQ35FreshRecoverySession::DisposeSimulated(session);
    }
    {
        Reset();PioneerStoreContext old(PioneerResidentBanks::Total);PioneerProbeCookie owner{&old};sRuntimeOwner=&owner;
        old.io.fabricOldMmioDisposeComplete=true;
        old.recoveryFabricEvidenceRecorded=old.recoveryFabricPciIdleAttempted=old.recoveryFabricPciIdle=true;
        old.recoveryFabricAuditReleaseAttempted=old.recoveryFabricAuditReleased=true;
        Scope scope;auto* session=PioneerQ35FreshRecoverySession::Allocate();assert(session);
        assert(session->PrepareFabric(&owner,old,&guard,scope));
        old.recoveryFabricEvidence.valid=false;
        assert(!session->RestoreScratch(&guard,scope));CheckRetained(session,old,scope);
        PioneerQ35FreshRecoverySession::DisposeSimulated(session);
    }
    {
        // Actual transaction coordinator and fresh session together; all
        // hardware, proof and transfer methods here are simulated.
        Reset();PioneerStoreContext old(PioneerResidentBanks::Total);PioneerProbeCookie owner{&old};sRuntimeOwner=&owner;
        old.io.fabricOldMmioDisposeComplete=true;
        old.recoveryFabricEvidenceRecorded=old.recoveryFabricPciIdleAttempted=old.recoveryFabricPciIdle=true;
        old.recoveryFabricAuditReleaseAttempted=old.recoveryFabricAuditReleased=true;
        Scope scope;old.recoveryFabricEvidence.valid=false;
        assert(PioneerQ35RunModifiedFabricRecoveryTransaction<PioneerQ35FreshRecoverySession>(&owner,&old,&guard,scope)==B_DEV_NOT_READY
            && !old.recoveryFreshSession && operations.empty());
        old.recoveryFabricEvidence.valid=true;
        assert(PioneerQ35RunModifiedFabricRecoveryTransaction<PioneerQ35FreshRecoverySession>(&owner,&old,&guard,scope)==B_OK
            && !old.recoveryFreshSession && old.qwen35QualificationOwner.Retired() && !scope.retained);
    }
    for (unsigned failure=0;failure<=6;failure++) {
        Reset();PioneerStoreContext old(PioneerResidentBanks::Total);old.io.postResetDisposeComplete=true;
        old.initialized[7]=9;PioneerProbeCookie owner{&old};sRuntimeOwner=&owner;Scope scope;
        auto* session=PioneerQ35FreshRecoverySession::Allocate();assert(session);
        failedOperation=failure;
        const bool ready=session->Prepare(&owner,old,&guard,scope);
        if (!failure) {
            assert(ready && !scope.retained && session->CurrentStage()==PioneerQ35FreshRecoverySession::Ready
                && session->Fresh()->lease.ready && operations==std::vector<unsigned>({1,2,3,4,5,6}));
        } else {
            assert(!ready);CheckRetained(session,old,scope);
            assert(session->FailureStage()==(failure==1?2:failure<=3?3:failure==4?4:failure==5?5:6));
            assert(operations.back()==failure);
            const auto count=operations.size();
            assert(!session->Prepare(&owner,old,&guard,scope) && operations.size()==count);
        }
        PioneerQ35FreshRecoverySession::DisposeSimulated(session);
    }
    for (unsigned boundary=1;boundary<=6;boundary++) {
        Reset();PioneerStoreContext old(PioneerResidentBanks::Total);old.io.postResetDisposeComplete=true;
        PioneerProbeCookie owner{&old};sRuntimeOwner=&owner;Scope scope;scope.failBoundary=boundary;
        scope.dropAttachment=boundary==PioneerQ35FreshRecoverySession::Mapped;
        auto* session=PioneerQ35FreshRecoverySession::Allocate();assert(session);
        assert(!session->Prepare(&owner,old,&guard,scope));CheckRetained(session,old,scope);
        assert(session->FailureStage()==boundary);
        PioneerQ35FreshRecoverySession::DisposeSimulated(session);
    }
    for (unsigned failure=0;failure<=11;failure++) {
        Reset();PioneerStoreContext old(PioneerResidentBanks::Total);old.io.postResetDisposeComplete=true;
        PioneerProbeCookie owner{&old};sRuntimeOwner=&owner;Scope scope;
        auto* session=PioneerQ35FreshRecoverySession::Allocate();assert(session);
        assert(session->Prepare(&owner,old,&guard,scope));
        assert(!session->CloseTransport(&guard,scope));
        if (failure && failure<=10) failedOperation=6+failure;
        if (failure==11) scope.closeAdmission=false;
        const bool restored=session->RestoreScratch(&guard,scope);
        const bool closed=restored && session->CloseTransport(&guard,scope);
        if (!failure) {
            assert(closed && !scope.retained && session->CurrentStage()==PioneerQ35FreshRecoverySession::TransportClosed
                && !session->Fresh()->lease.ready && session->Fresh()->host.allocated && !session->Fresh()->host.released
                && old.recoveryFreshSession==session && !session->Fresh()->recoveryMapping
                && operations==std::vector<unsigned>({1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16}));
        } else {
            assert(!closed);CheckRetained(session,old,scope);
            assert(session->FailureStage()==(failure==1?PioneerQ35FreshRecoverySession::ScratchRestoring
                :failure==2?PioneerQ35FreshRecoverySession::LeaseClosed
                :failure>=3 && failure<=8?PioneerQ35FreshRecoverySession::FleetAudited
                :failure==9?PioneerQ35FreshRecoverySession::PciClosed
                :failure==10?PioneerQ35FreshRecoverySession::TransportClosed
                :PioneerQ35FreshRecoverySession::Closing));
            if (failure>=3 && failure<=8) assert(session->Fresh()->recoveryMapping);
            const auto count=operations.size();
            assert(!session->RestoreScratch(&guard,scope) && !session->CloseTransport(&guard,scope)
                && operations.size()==count);
        }
        PioneerQ35FreshRecoverySession::DisposeSimulated(session);
    }
    for (unsigned mutation=0;mutation<2;mutation++) {
        Reset();PioneerStoreContext old(PioneerResidentBanks::Total);old.io.postResetDisposeComplete=true;
        PioneerProbeCookie owner{&old};sRuntimeOwner=&owner;Scope scope;
        auto* session=PioneerQ35FreshRecoverySession::Allocate();assert(session);
        assert(session->Prepare(&owner,old,&guard,scope) && session->RestoreScratch(&guard,scope)
            && session->CloseTransport(&guard,scope));
        if (!mutation) scope.quarantineAtRetirement=true;
        else scope.loseClaimAfterRetirement=true;
        assert(!session->Retire(&guard,scope));CheckRetained(session,old,scope);
        if (!mutation) assert(session->FailureStage()==PioneerQ35FreshRecoverySession::Retiring && operations.back()==16);
        else assert(session->FailureStage()==PioneerQ35FreshRecoverySession::OwnerRetired && operations.back()==18);
        PioneerQ35FreshRecoverySession::DisposeSimulated(session);
    }
    for (unsigned failure=0;failure<=10;failure++) {
        Reset();PioneerStoreContext old(PioneerResidentBanks::Total);old.io.postResetDisposeComplete=true;
        PioneerProbeCookie owner{&old};sRuntimeOwner=&owner;Scope scope;
        auto* session=PioneerQ35FreshRecoverySession::Allocate();assert(session);
        assert(session->Prepare(&owner,old,&guard,scope));
        assert(!session->Retire(&guard,scope));
        assert(session->RestoreScratch(&guard,scope) && session->CloseTransport(&guard,scope));
        auto* scratch=old.qwen35QualificationOwner.Scratch();auto* snapshot=old.qwen35QualificationOwner.Snapshot();
        if (failure==1) scope.retirementAdmission=false;
        if (failure==2) failedOperation=17;
        if (failure==3) scope.ownerAdmission=false;
        if (failure==4) failedOperation=18;
        if (failure==5) failGuardAt=guardCalls+1;
        if (failure==6) failGuardAt=guardCalls+3;
        if (failure==7) failGuardAt=guardCalls+5;
        if (failure==8) failGuardAt=guardCalls+2;
        if (failure==9) failGuardAt=guardCalls+4;
        if (failure==10) failGuardAt=guardCalls+6;
        const bool retired=session->Retire(&guard,scope);
        if (!failure) {
            assert(retired && !scope.retained && !old.recoveryFreshSession
                && old.qwen35QualificationOwner.RetiredModifiedIdentity(scratch,snapshot)
                && old.banks.Q35StreamClaimIdentity(scratch,120) && sColdRecoveryRequired
                && operations==std::vector<unsigned>({1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18}));
        } else {
            assert(!retired);CheckRetained(session,old,scope);
            assert(session->FailureStage()==(failure==1 || failure==5 || failure==8?PioneerQ35FreshRecoverySession::Retiring
                :failure==2 || failure==6 || failure==9?PioneerQ35FreshRecoverySession::HostReleased
                :PioneerQ35FreshRecoverySession::OwnerRetired));
            const auto count=operations.size();assert(!session->Retire(&guard,scope) && operations.size()==count);
            PioneerQ35FreshRecoverySession::DisposeSimulated(session);
        }
    }
    {
        Reset();PioneerStoreContext old(PioneerResidentBanks::Total);old.io.postResetDisposeComplete=true;
        PioneerProbeCookie owner{&old};sRuntimeOwner=&owner;Scope scope;
        auto* session=PioneerQ35FreshRecoverySession::Allocate();assert(session);
        assert(session->Prepare(&owner,old,&guard,scope) && session->RestoreScratch(&guard,scope));
        auditSourceMismatch=true;
        assert(!session->CloseTransport(&guard,scope));CheckRetained(session,old,scope);
        assert(session->FailureStage()==PioneerQ35FreshRecoverySession::FleetAudited
            && session->Fresh()->recoveryMapping && operations.back()==10);
        PioneerQ35FreshRecoverySession::DisposeSimulated(session);
    }
    for (unsigned invalidGuard=15;invalidGuard<=31;invalidGuard++) {
        Reset();PioneerStoreContext old(PioneerResidentBanks::Total);old.io.postResetDisposeComplete=true;
        PioneerProbeCookie owner{&old};sRuntimeOwner=&owner;Scope scope;
        auto* session=PioneerQ35FreshRecoverySession::Allocate();assert(session);
        assert(session->Prepare(&owner,old,&guard,scope));failGuardAt=invalidGuard;
        const bool restored=session->RestoreScratch(&guard,scope);
        assert(!(restored && session->CloseTransport(&guard,scope)));
        CheckRetained(session,old,scope);assert(guardCalls==invalidGuard);
        const auto count=operations.size();
        assert(!session->RestoreScratch(&guard,scope) && !session->CloseTransport(&guard,scope)
            && operations.size()==count);
        PioneerQ35FreshRecoverySession::DisposeSimulated(session);
    }
    {
        Reset();PioneerStoreContext old(PioneerResidentBanks::Total);old.io.postResetDisposeComplete=true;
        old.baseline.baseline.baseline.baseline.dram.route=78;
        PioneerProbeCookie owner{&old};sRuntimeOwner=&owner;Scope scope;
        auto* session=PioneerQ35FreshRecoverySession::Allocate();assert(session);
        assert(!session->Prepare(&owner,old,&guard,scope));CheckRetained(session,old,scope);
        assert(session->FailureStage()==PioneerQ35FreshRecoverySession::Banks
            && operations==std::vector<unsigned>({1,2,3,4}));
        PioneerQ35FreshRecoverySession::DisposeSimulated(session);
    }
    for (unsigned invalidGuard=3;invalidGuard<=14;invalidGuard++) {
        Reset();PioneerStoreContext old(PioneerResidentBanks::Total);old.io.postResetDisposeComplete=true;
        PioneerProbeCookie owner{&old};sRuntimeOwner=&owner;Scope scope;failGuardAt=invalidGuard;
        auto* session=PioneerQ35FreshRecoverySession::Allocate();assert(session);
        assert(!session->Prepare(&owner,old,&guard,scope));CheckRetained(session,old,scope);
        assert(session->FailureStage()==1+(invalidGuard-3)/2 && guardCalls==invalidGuard);
        PioneerQ35FreshRecoverySession::DisposeSimulated(session);
    }
    {
        Reset();PioneerStoreContext old(PioneerResidentBanks::Total);old.io.postResetDisposeComplete=true;
        PioneerProbeCookie owner{&old};sRuntimeOwner=&owner;Scope scope;scope.admission=false;
        auto* session=PioneerQ35FreshRecoverySession::Allocate();assert(session);
        assert(!session->Prepare(&owner,old,&guard,scope) && operations.empty()
            && !old.recoveryFreshSession && !scope.retained && session->DiscardUnstarted());
    }
    for (unsigned invalid=0;invalid<11;invalid++) {
        Reset();PioneerStoreContext old(PioneerResidentBanks::Total);old.io.postResetDisposeComplete=true;
        PioneerProbeCookie owner{&old};sRuntimeOwner=&owner;Scope scope;
        if (invalid==0) old.io.postResetDisposeComplete=false;
        if (invalid==1) old.recoveryFinishProof.valid=false;
        if (invalid==2) old.recoveryFinishProof.reset_generation++;
        if (invalid==3) old.recoveryFinishProof.firmware_generation++;
        if (invalid==4) old.banks.claimed=false;
        if (invalid==5) old.qwen35QualificationOwner.active=false;
        if (invalid==6) sColdRecoveryRequired=false;
        if (invalid==7) sOpenCookies=2;
        if (invalid==8) sReloadQuiesced=true;
        if (invalid==9) failGuardAt=2;
        if (invalid==10) old.qwen35QualificationOwner.scratch.eligible=false;
        auto* session=PioneerQ35FreshRecoverySession::Allocate();assert(session);
        assert(!session->Prepare(&owner,old,&guard,scope) && operations.empty()
            && !old.recoveryFreshSession && !scope.retained && session->DiscardUnstarted());
    }
    {
        Reset();PioneerStoreContext old(PioneerResidentBanks::Total);old.io.postResetDisposeComplete=true;
        PioneerProbeCookie owner{&old};sRuntimeOwner=&owner;Scope scope;failFreshAllocation=true;
        auto* session=PioneerQ35FreshRecoverySession::Allocate();assert(session);
        assert(!session->Prepare(&owner,old,&guard,scope) && operations.empty()
            && !old.recoveryFreshSession && !scope.retained && session->DiscardUnstarted());
    }
    std::puts("PASS fresh recovery session ordering and retained failures; SIMULATED ONLY");
}
