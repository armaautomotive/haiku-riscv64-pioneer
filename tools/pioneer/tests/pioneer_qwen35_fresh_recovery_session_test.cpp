/* SPDX-License-Identifier: MIT */
// Exercise the actual post-reset session ordering using simulated resources.
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define PIONEER_QWEN35_RECOVERY_RUN_ENABLE 1
#define PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS 1
#define PIONEER_QWEN35_FRESH_RECOVERY_TEST 1
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <vector>
using uint64=uint64_t;
constexpr int B_OK=0;
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
struct MockPci { unsigned node_id[3]={2,2,2}; };
struct MockProof {
    uint64 reset_generation=kTicket,firmware_generation=kTicket;
    bool valid=true;
    bool ReadyToRetireOneOwner() const { return valid; }
};
struct MockScratch { bool eligible=true; bool ResetRestoreEligible(const uint32_t* p) const { return eligible && p; } };
struct MockQualificationOwner {
    MockScratch scratch;
    uint32_t snapshot=1;
    bool active=true,retained=false;
    bool Active() const { return active; }
    MockScratch* Scratch() { return active?&scratch:nullptr; }
    uint32_t* Snapshot() { return active?&snapshot:nullptr; }
    void Retain() { retained=true; }
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
struct MockLease { bool recovery_required=false,ready=false; };
struct tt_arc_scratch_result {};
struct PioneerPostResetAuditResult {};
struct PioneerPostResetMapping {
    MockDram topology;
    explicit PioneerPostResetMapping(unsigned) { if (auditSourceMismatch) topology.source[0]=1; }
    bool Open() { return Step(9); }
    bool ProbeFirmware(unsigned,tt_arc_scratch_result&) { return Step(10); }
    bool Audit(PioneerPostResetAuditResult&) { return Step(11); }
    bool AuditHostDma() { return Step(12); }
    bool Close() { return Step(13); }
};
struct PioneerHostDmaAuditIO { bool AfterNodeId(unsigned) { return Step(2); } };
struct MockIO : PioneerHostDmaAuditIO {
    bool postResetDisposeComplete=false,quarantined=false;
    void QuarantineHostPage() { quarantined=true; }
};
class PioneerQ35FreshRecoverySession;
struct PioneerStoreContext {
    uint64 bytes;
    unsigned profile=123;
    uint8_t* initialized;
    uint32_t* hashes;
    bool bankStorageOnly=true,recoveryAttempted=true,recoveryFinishPending=true;
    uint64 recoveryFinishTicket=kTicket;
    PioneerPostResetMapping* recoveryMapping=nullptr;
    MockProof recoveryFinishProof;
    PioneerQ35FreshRecoverySession* recoveryFreshSession=nullptr;
    MockQualificationOwner qwen35QualificationOwner;
    PioneerResidentBanks banks;
    MockRoot baseline;
    MockIO io;
    MockPci pci;
    unsigned transaction=0,idle=0,host=0;
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
static bool BlackholePioneerMmioBegin(MockIO&,MockPci&,unsigned&) { return Step(1); }
static bool PioneerResidentSessionIdle(MockIO&,MockNoc&,unsigned&) { return Step(3); }
static bool PioneerPrepareHostPage(MockIO&,unsigned& host) { if (!Step(5)) return false;host=1;return true; }
static bool PioneerResidentLeaseBegin(MockIO&,MockNoc&,MockLease& lease) {
    if (!Step(6)) return false;
    lease.ready=true;return true;
}
static bool PioneerResidentLeaseEnd(MockIO&,MockNoc&,MockLease& lease) {
    if (!Step(8)) return false;
    lease.ready=false;return true;
}
static bool PioneerQ38DispatchMmioEnd(MockIO&,MockPci&,unsigned&) { return Step(14); }
struct PioneerQ35ProjectionResetRecovery {
    bool restored=false;
    template<class Scope>
    bool Restore(MockScratch&,uint64 ticket,const MockProof&,MockIO&,Scope&,
        PioneerResidentBanks&,const MockDram&,PioneerResidentBanks&,const MockDram&,
        const MockNoc&,const unsigned&,MockLease&,const uint8_t*,unsigned pages) {
        if (ticket!=kTicket || pages!=PioneerResidentBanks::Pages || !Step(7)) return false;
        restored=true;return true;
    }
    bool VerifiedFor(const MockScratch&,uint64 ticket,const PioneerResidentBanks&,
        const PioneerResidentBanks& fresh) const { return restored && ticket==kTicket && !fresh.quarantined; }
};
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35FreshRecoverySession.inc"

struct Scope {
    bool admission=true,retained=false,closeAdmission=true;
    unsigned failBoundary=0;
    bool FreshRecoveryAdmission(PioneerQ35FreshRecoverySession&,PioneerProbeCookie*,PioneerStoreContext&,uint64 ticket) {
        return admission && ticket==kTicket;
    }
    bool FreshRecoveryBoundary(PioneerQ35FreshRecoverySession& session,PioneerStoreContext& old,
        PioneerStoreContext& fresh,unsigned stage) {
        assert(old.recoveryFreshSession==&session && session.Fresh()==&fresh && session.Ticket()==kTicket);
        assert(fresh.profile==old.profile && !memcmp(fresh.initialized,old.initialized,PioneerResidentBanks::Pages));
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
        PioneerStoreContext&,const PioneerQ35ProjectionResetRecovery&) { return Step(15); }
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
        auto* session=PioneerQ35FreshRecoverySession::Allocate();assert(session);
        assert(!session->Prepare(&owner,old,&guard,scope));CheckRetained(session,old,scope);
        assert(session->FailureStage()==boundary);
        PioneerQ35FreshRecoverySession::DisposeSimulated(session);
    }
    for (unsigned failure=0;failure<=10;failure++) {
        Reset();PioneerStoreContext old(PioneerResidentBanks::Total);old.io.postResetDisposeComplete=true;
        PioneerProbeCookie owner{&old};sRuntimeOwner=&owner;Scope scope;
        auto* session=PioneerQ35FreshRecoverySession::Allocate();assert(session);
        assert(session->Prepare(&owner,old,&guard,scope));
        assert(!session->CloseTransport(&guard,scope));
        if (failure && failure<=9) failedOperation=6+failure;
        if (failure==10) scope.closeAdmission=false;
        const bool restored=session->RestoreScratch(&guard,scope);
        const bool closed=restored && session->CloseTransport(&guard,scope);
        if (!failure) {
            assert(closed && !scope.retained && session->CurrentStage()==PioneerQ35FreshRecoverySession::TransportClosed
                && !session->Fresh()->lease.ready && session->Fresh()->host==1
                && old.recoveryFreshSession==session && !session->Fresh()->recoveryMapping
                && operations==std::vector<unsigned>({1,2,3,4,5,6,7,8,9,10,11,12,13,14,15}));
        } else {
            assert(!closed);CheckRetained(session,old,scope);
            assert(session->FailureStage()==(failure==1?PioneerQ35FreshRecoverySession::ScratchRestoring
                :failure==2?PioneerQ35FreshRecoverySession::LeaseClosed
                :failure>=3 && failure<=7?PioneerQ35FreshRecoverySession::FleetAudited
                :failure==8?PioneerQ35FreshRecoverySession::PciClosed
                :failure==9?PioneerQ35FreshRecoverySession::TransportClosed
                :PioneerQ35FreshRecoverySession::Closing));
            if (failure>=3 && failure<=7) assert(session->Fresh()->recoveryMapping);
            const auto count=operations.size();
            assert(!session->RestoreScratch(&guard,scope) && !session->CloseTransport(&guard,scope)
                && operations.size()==count);
        }
        PioneerQ35FreshRecoverySession::DisposeSimulated(session);
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
    for (unsigned invalidGuard=15;invalidGuard<=30;invalidGuard++) {
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
