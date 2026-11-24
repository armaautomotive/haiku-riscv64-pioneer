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
static bool failFreshAllocation=false;
static bool sColdRecoveryRequired=true,sReloadQuiesced=false,sBankStorageOnly=true;
static unsigned sOpenCookies=1;
static std::vector<unsigned> operations;
static bool Step(unsigned operation) { operations.push_back(operation);return failedOperation!=operation; }
struct MockDram { unsigned route=77; };
struct MockNoc {};
struct MockRoot { struct A { struct B { struct C { MockDram dram; } baseline;MockNoc noc; } baseline; } baseline; };
struct MockPci { unsigned node_id[3]={2,2,2}; };
struct MockProof {
    uint64 reset_generation=kTicket,firmware_generation=kTicket;
    bool valid=true;
    bool ReadyToRetireOneOwner() const { return valid; }
};
struct MockScratch {};
struct MockQualificationOwner {
    MockScratch scratch;
    bool active=true,retained=false;
    bool Active() const { return active; }
    MockScratch* Scratch() { return active?&scratch:nullptr; }
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
    void* recoveryMapping=nullptr;
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
    ~PioneerStoreContext() { free(initialized);free(hashes); }
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
static bool PioneerPrepareHostPage(MockIO&,unsigned&) { return Step(5); }
static bool PioneerResidentLeaseBegin(MockIO&,MockNoc&,MockLease& lease) {
    if (!Step(6)) return false;
    lease.ready=true;return true;
}
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35FreshRecoverySession.inc"

struct Scope {
    bool admission=true,retained=false;
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
    void RetainFreshRecovery(PioneerQ35FreshRecoverySession&) { retained=true; }
};
static void Reset() {
    failedOperation=guardCalls=failGuardAt=0;failFreshAllocation=false;operations.clear();
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
    for (unsigned invalid=0;invalid<10;invalid++) {
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
