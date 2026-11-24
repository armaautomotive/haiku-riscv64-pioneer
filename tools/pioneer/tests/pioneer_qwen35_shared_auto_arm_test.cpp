/* SPDX-License-Identifier: MIT */
// Compile the actual binding method against controlled owner and scope doubles.
// This is a host simulation: no PCI mapping or device release is performed.
#define PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS 1
#include <cassert>
#include <cstdio>
#include <vector>
namespace PioneerQwen35 { enum : unsigned { DataflowBarrierWorkers=120 }; }
struct IO { unsigned runtimeCore=0; };
struct Context { IO io; };
struct ScopeStub {
    bool safe=false;
    unsigned checks=0;
    bool ProveRoleReleaseSafe(IO&,unsigned core) { ++checks;return safe && core<120; }
};
struct Views { bool opened=true;bool Opened() const { return opened; } };
static std::vector<unsigned> calls;
struct Adapter {
    Adapter(IO&,ScopeStub&,Views&) {}
};
struct Owner {
    bool mode=true,preserve=true,prepare=true,begin=true;
    bool AutoRestoreMode() const { return mode; }
    bool ComputeLocalRamPreservationSelected() const { return preserve; }
    bool PreparePossibleRelease(Adapter&) { calls.push_back(1);return prepare; }
    bool BeginReleaseAttempt(Adapter&) { calls.push_back(2);return begin; }
};
struct Order {
    bool okay=true;
    bool Arm() { calls.push_back(3);return okay; }
};
struct Binding {
    Context fContext;
    ScopeStub scope;
    Views fViews;
    Owner fOwner;
    Order fStartOrder;
    bool fAutoRestoreOnStop=true,fReleaseAuthorized=false,scopeAttached=true;
    ScopeStub* Scope() { return scopeAttached?&scope:nullptr; }
#include "pioneer_q35_shared_auto_arm.inc"
};
int main() {
    for (unsigned missing=0;missing<6;missing++) {
        Binding binding;calls.clear();
        binding.scope.safe=true;
        if (missing==0) binding.scopeAttached=false;
        if (missing==1) binding.fViews.opened=false;
        if (missing==2) binding.fOwner.mode=false;
        if (missing==3) binding.fOwner.preserve=false;
        if (missing==4) binding.fContext.io.runtimeCore=120;
        if (missing==5) binding.scope.safe=false;
        assert(!binding.ArmAndBeginRelease() && !binding.fReleaseAuthorized && calls.empty());
    }
    for (unsigned fail=1;fail<=3;fail++) {
        Binding binding;calls.clear();binding.scope.safe=true;
        binding.fOwner.prepare=fail!=1;binding.fOwner.begin=fail!=2;
        binding.fStartOrder.okay=fail!=3;
        assert(!binding.ArmAndBeginRelease() && !binding.fReleaseAuthorized && calls.size()==fail);
    }
    Binding good;calls.clear();good.scope.safe=true;
    assert(good.ArmAndBeginRelease() && good.fReleaseAuthorized && good.scope.checks==1
        && calls==std::vector<unsigned>({1,2,3}));
    Binding ordinary;calls.clear();ordinary.fAutoRestoreOnStop=false;
    assert(ordinary.ArmAndBeginRelease() && ordinary.fReleaseAuthorized
        && ordinary.scope.checks==0 && calls==std::vector<unsigned>({1,2,3}));
    std::puts("PASS actual shared auto-restore arm prerequisites and ordering; SIMULATED ONLY");
}
