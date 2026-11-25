/* SPDX-License-Identifier: MIT */
// Exercise the actual transaction coordinator with retained-session doubles.
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define PIONEER_QWEN35_RECOVERY_RUN_ENABLE 1
#define PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS 1
#define PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS 1
#define PIONEER_QWEN35_FABRIC_GUARD_WITNESS 1
#include <cassert>
#include <cstdio>
#include <new>
#include <vector>
using status_t=int;
constexpr status_t B_OK=0,B_NOT_ALLOWED=-1,B_NO_MEMORY=-2,B_DEV_NOT_READY=-3;
struct Banks {};
struct Owner {
    bool eligible=true,active=true,retired=false;
    bool ModifiedResetEligible(const Banks&) const { return eligible; }
    bool Active() const { return active; }
    bool Retired() const { return retired; }
};
struct PioneerStoreContext {
    void* recoveryFreshSession=nullptr;
    void* qwen35SharedRoleBinding=nullptr;
    void* qwen35GraphOwner=nullptr;
    Owner qwen35QualificationOwner;
    Banks banks;
};
struct PioneerProbeCookie { PioneerStoreContext* context=nullptr; };
struct pioneer_driver_recovery_guard_v2_info {};
static unsigned failure=0,allocations=0,discards=0,retains=0;static bool usedFabric=false;
static std::vector<unsigned> calls;
struct Scope {
    bool allowed=true;
    bool RecoveryTransactionAdmission(PioneerProbeCookie*,PioneerStoreContext&,
        pioneer_driver_recovery_guard_v2_info*) { return allowed; }
    bool FabricRecoveryTransactionAdmission(PioneerProbeCookie*,PioneerStoreContext&,
        pioneer_driver_recovery_guard_v2_info*) { return allowed; }
};
struct FakeSession {
    PioneerStoreContext* old=nullptr;
    bool attached=false;
    static FakeSession* Allocate() {
        allocations++;
        return failure==1?nullptr:new(std::nothrow) FakeSession;
    }
    bool DiscardUnstarted() {
        if (attached) return false;
        discards++;delete this;return true;
    }
    void RetainUncertain(Scope&) {
        retains++;
        if (old && !old->recoveryFreshSession) old->recoveryFreshSession=this;
    }
    bool Prepare(PioneerProbeCookie*,PioneerStoreContext& c,pioneer_driver_recovery_guard_v2_info*,Scope&) {
        calls.push_back(1);old=&c;
        if (failure==2) return false;
        attached=true;c.recoveryFreshSession=this;
        if (failure==3) return false;
        if (failure==4) c.recoveryFreshSession=nullptr;
        return true;
    }
    bool PrepareFabric(PioneerProbeCookie* owner,PioneerStoreContext& c,pioneer_driver_recovery_guard_v2_info* guard,Scope& scope) {
        usedFabric=true;return Prepare(owner,c,guard,scope);
    }
    bool RestoreScratch(pioneer_driver_recovery_guard_v2_info*,Scope&) {
        calls.push_back(2);
        if (failure==5) { old->recoveryFreshSession=nullptr;return false; }
        return true;
    }
    bool CloseTransport(pioneer_driver_recovery_guard_v2_info*,Scope&) { calls.push_back(3);return failure!=6; }
    bool Retire(pioneer_driver_recovery_guard_v2_info*,Scope&) {
        calls.push_back(4);
        if (failure==7) return false;
        old->recoveryFreshSession=nullptr;old->qwen35QualificationOwner.active=false;
        old->qwen35QualificationOwner.retired=failure!=8;
        delete this;return true;
    }
};
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35ModifiedRecoveryTransaction.inc"

int main() {
    pioneer_driver_recovery_guard_v2_info guard;
    for (unsigned fabric=0;fabric<2;fabric++) for (failure=0;failure<=8;failure++) {
        allocations=discards=retains=0;usedFabric=false;calls.clear();PioneerStoreContext context;
        PioneerProbeCookie owner{&context};Scope scope;
        const int result=fabric?PioneerQ35RunModifiedFabricRecoveryTransaction<FakeSession>(&owner,&context,&guard,scope)
            :PioneerQ35RunModifiedRecoveryTransaction<FakeSession>(&owner,&context,&guard,scope);
        assert(usedFabric==(fabric && failure!=1));
        if (!failure) {
            assert(result==B_OK && calls==std::vector<unsigned>({1,2,3,4})
                && !context.recoveryFreshSession && context.qwen35QualificationOwner.retired);
        } else if (failure==1) assert(result==B_NO_MEMORY && calls.empty());
        else if (failure==2) assert(result==B_DEV_NOT_READY && discards==1 && !context.recoveryFreshSession);
        else if (failure==8) assert(result==B_DEV_NOT_READY && !context.recoveryFreshSession);
        else {
            assert(result==B_DEV_NOT_READY && context.recoveryFreshSession);
            assert(retains==1);
            // Test doubles have no live mappings; production retains these.
            delete static_cast<FakeSession*>(context.recoveryFreshSession);
        }
    }
    failure=0;allocations=0;PioneerStoreContext context;PioneerProbeCookie owner{&context};Scope scope;
    scope.allowed=false;
    assert(PioneerQ35RunModifiedRecoveryTransaction<FakeSession>(&owner,&context,&guard,scope)==B_NOT_ALLOWED && !allocations);
    assert(PioneerQ35RunModifiedFabricRecoveryTransaction<FakeSession>(&owner,&context,&guard,scope)==B_NOT_ALLOWED && !allocations);
    scope.allowed=true;context.qwen35SharedRoleBinding=&context;
    assert(PioneerQ35RunModifiedRecoveryTransaction<FakeSession>(&owner,&context,&guard,scope)==B_NOT_ALLOWED && !allocations);
    context.qwen35SharedRoleBinding=nullptr;context.qwen35GraphOwner=&context;
    assert(PioneerQ35RunModifiedRecoveryTransaction<FakeSession>(&owner,&context,&guard,scope)==B_NOT_ALLOWED && !allocations);
    std::puts("PASS modified recovery transaction order, mapping exclusion and retained failures; SIMULATED ONLY");
}
