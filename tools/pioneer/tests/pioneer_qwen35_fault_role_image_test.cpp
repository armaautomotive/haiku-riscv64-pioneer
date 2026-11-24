/* SPDX-License-Identifier: MIT */
// Exercise the real binding's fault audit order using controlled observations.
#include <cassert>
#include <cstdint>
#include <cstdio>
namespace PioneerQwen35 {
enum { DataflowBarrierWorkers=120 };
struct DataflowWorkerSnapshot {};
[[maybe_unused]] static const uint32_t DataflowServiceTextWords[]={1};
[[maybe_unused]] static const uint32_t DataflowServiceDispatchWords[]={2};
}
struct IO {};
struct FakeScope { bool active=true; bool RetainedFaultAuditActive(IO&) const { return active; } };
struct Context { IO io; };
struct Views { bool opened=true; bool Opened() const { return opened; } };
struct Adapter {
    static bool settled,images,pcs;
    static unsigned settledCalls,imageCalls,pcCalls;
    Adapter(IO&,FakeScope&,Views&) {}
    bool FaultSettled(unsigned,const PioneerQwen35::DataflowWorkerSnapshot&,unsigned,unsigned) {
        ++settledCalls;return settled;
    }
    bool InspectRunningRoleImages(unsigned,int) { ++imageCalls;return images; }
    bool RunningRolePcs(unsigned,int,bool fault) { ++pcCalls;return fault && pcs; }
};
bool Adapter::settled=true,Adapter::images=true,Adapter::pcs=true;
unsigned Adapter::settledCalls=0,Adapter::imageCalls=0,Adapter::pcCalls=0;
struct Binding {
    Context fContext; Views fViews; FakeScope scope;
    bool fReleaseConfirmed[PioneerQwen35::DataflowBarrierWorkers]{};
    bool fStackReleaseConfirmed[PioneerQwen35::DataflowBarrierWorkers]{};
    bool fReleaseAuthorized=true;
    FakeScope* Scope() { return &scope; }
    int RoleImage() const { return 1; }
    int RoleBounds() const { return 2; }
#include "pioneer_q35_fault_core_settled.inc"
};
int main() {
    Binding binding;PioneerQwen35::DataflowWorkerSnapshot snapshot{};
    constexpr unsigned core=7;
    binding.fReleaseConfirmed[core]=binding.fStackReleaseConfirmed[core]=true;
    assert(binding.FaultCoreSettled(core,snapshot));
    assert(Adapter::settledCalls==1 && Adapter::imageCalls==1 && Adapter::pcCalls==1);
    Adapter::settledCalls=Adapter::imageCalls=Adapter::pcCalls=0;Adapter::settled=false;
    assert(!binding.FaultCoreSettled(core,snapshot) && Adapter::settledCalls==1
        && !Adapter::imageCalls && !Adapter::pcCalls);
    Adapter::settled=true;Adapter::images=false;
    assert(!binding.FaultCoreSettled(core,snapshot) && Adapter::imageCalls==1 && !Adapter::pcCalls);
    Adapter::images=true;Adapter::pcs=false;
    assert(!binding.FaultCoreSettled(core,snapshot) && Adapter::pcCalls==1);
    binding.scope.active=false;Adapter::settledCalls=0;
    assert(!binding.FaultCoreSettled(core,snapshot) && !Adapter::settledCalls);
    std::puts("PASS faulted worker requires settled state, exact role images and PCs; SIMULATED ONLY");
}
