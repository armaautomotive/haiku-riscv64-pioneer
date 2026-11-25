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
struct MockFabricProof {
    uint64 fabric_generation=kTicket,firmware_generation=kTicket;
    bool valid=true;
    bool ReadyForScratchRestore() const { return valid; }
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
};
struct PioneerHostDmaAuditIO { bool AfterNodeId(unsigned) { return Step(2); } };
struct MockIO : PioneerHostDmaAuditIO {
    bool postResetDisposeComplete=false,fabricOldMmioDisposeComplete=false,quarantined=false;
    void QuarantineHostPage() { quarantined=true; }
    int ReleaseHostPage() { return Step(17)?B_OK:-1; }
};
class PioneerQ35FreshRecoverySession;
struct PioneerStoreContext {
    uint64 bytes;
    unsigned profile=123;
    uint8_t* initialized;
    uint32_t* hashes;
    bool bankStorageOnly=true,recoveryAttempted=true,recoveryFinishPending=true;
    uint64 recoveryFinishTicket=kTicket;
    bool recoveryFabricEvidenceRecorded=false,recoveryFabricPciIdleAttempted=false,recoveryFabricPciIdle=false;
    bool recoveryFabricAuditReleaseAttempted=false,recoveryFabricAuditReleased=false;
    bool q35PciReservationHeld=true,q35PciReleaseAttempted=false;
    uint64 recoveryFabricStartTicket=kTicket;
    void* recoveryFabricArcOwner=nullptr;
    void* qwen35SharedRoleBinding=nullptr;void* qwen35GraphOwner=nullptr;
    PioneerPostResetMapping* recoveryMapping=nullptr;
    MockProof recoveryFinishProof;
    MockFabricProof recoveryFabricEvidence;
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
static bool PioneerResidentSessionIdle(MockIO&,MockNoc&,unsigned&) { return Step(3); }
static bool PioneerPrepareHostPage(MockIO&,MockHost& host) { if (!Step(5)) return false;host.state=TT_HOST_PAGE_READY;host.allocated=1;host.released=0;return true; }
static bool PioneerResidentLeaseBegin(MockIO&,MockNoc&,MockLease& lease) {
    if (!Step(6)) return false;
    lease.ready=true;return true;
}
static bool PioneerResidentLeaseEnd(MockIO&,MockNoc&,MockLease& lease) {
    if (!Step(8)) return false;
    lease.ready=false;return true;
}
static bool PioneerQ38DispatchMmioEnd(MockIO&,MockPci& pci,MockTransaction& t) {
    if (!Step(15)) return false;
    pci.restored=1;t.config_attempted=false;return true;
}
struct PioneerQ35ProjectionResetRecovery {
    bool restored=false;
    template<class Scope,class Proof>
    bool Restore(MockScratch&,uint64 ticket,const Proof&,MockIO&,Scope&,
        PioneerResidentBanks&,const MockDram&,PioneerResidentBanks&,const MockDram&,
        const MockNoc&,const MockHost&,MockLease&,const uint8_t*,unsigned pages) {
        if (ticket!=kTicket || pages!=PioneerResidentBanks::Pages || !Step(7)) return false;
        restored=true;return true;
    }
    bool VerifiedFor(const MockScratch&,uint64 ticket,const PioneerResidentBanks&,
        const PioneerResidentBanks& fresh) const { return restored && ticket==kTicket && !fresh.quarantined; }
};
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35FreshRecoverySession.inc"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35ModifiedRecoveryTransaction.inc"

struct Scope {
    bool admission=true,retained=false,closeAdmission=true,retirementAdmission=true,ownerAdmission=true;
    bool quarantineAtRetirement=false,loseClaimAfterRetirement=false;
    bool dropAttachment=false;
    unsigned failBoundary=0;
    bool RecoveryTransactionAdmission(PioneerProbeCookie*,PioneerStoreContext&,pioneer_driver_recovery_guard_v2_info*) { return admission; }
    bool FabricRecoveryTransactionAdmission(PioneerProbeCookie*,PioneerStoreContext&,pioneer_driver_recovery_guard_v2_info*) { return admission; }
    bool FreshRecoveryAdmission(PioneerQ35FreshRecoverySession&,PioneerProbeCookie*,PioneerStoreContext&,uint64 ticket) {
        return admission && ticket==kTicket;
    }
    bool FreshRecoveryBoundary(PioneerQ35FreshRecoverySession& session,PioneerStoreContext& old,
        PioneerStoreContext& fresh,unsigned stage) {
        assert(old.recoveryFreshSession==&session && session.Fresh()==&fresh && session.Ticket()==kTicket);
        assert(fresh.profile==old.profile && !memcmp(fresh.initialized,old.initialized,PioneerResidentBanks::Pages));
        if (stage==failBoundary && dropAttachment) old.recoveryFreshSession=nullptr;
        return stage!=failBoundary;
    }
    bool FreshRecoveryRestoreAdmission(PioneerQ35FreshRecoverySession&,PioneerStoreContext&,
        PioneerStoreContext&,const PioneerQ35ProjectionResetRecovery&) { return true; }
    bool FreshRecoveryRestoreComplete(PioneerQ35FreshRecoverySession&,PioneerStoreContext&,
        PioneerStoreContext&,const PioneerQ35ProjectionResetRecovery&) { return true; }
    bool FreshRecoveryCloseAdmission(PioneerQ35FreshRecoverySession&,PioneerStoreContext&,
        PioneerStoreContext&,const PioneerQ35ProjectionResetRecovery&) { return closeAdmission; }
    bool FreshRecoveryPostTransferAudit(PioneerQ35FreshRecoverySession&,PioneerStoreContext&,
        PioneerStoreContext&,const PioneerQ35ProjectionResetRecovery&) { return true; }
    bool FreshRecoveryPciIdle(PioneerQ35FreshRecoverySession&,PioneerStoreContext&,
        PioneerStoreContext&,const PioneerQ35ProjectionResetRecovery&) { return Step(16); }
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
            && session->RestoreScratch(&guard,scope) && session->CloseTransport(&guard,scope));
        assert(session->Retire(&guard,scope) && !old.recoveryFreshSession && old.qwen35QualificationOwner.RetiredModifiedIdentity(
            &old.qwen35QualificationOwner.scratch,&old.qwen35QualificationOwner.snapshot)
            && old.io.fabricOldMmioDisposeComplete && !old.io.postResetDisposeComplete && !scope.retained);
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
