/* SPDX-License-Identifier: MIT */
// Real host transaction and two-pass audit over simulated panel/fleet IO.
#include "resident-host-fixture.inc"
#include "resident-fleet-fixture.inc"

struct ComposedScope {
    FleetIO& fleet;DataflowResidentFleetAudit& audit;bool retained=false;
    bool ResidentSessionAllowed(ResidentRaw&,const DataflowResidentCommand&,unsigned) const { return !retained; }
    bool ResidentRootAllowed(ResidentRaw& io,const DataflowResidentCommand&,unsigned) const { return io.selected==0; }
    bool AuditResidentFleet(ResidentRaw& raw,const DataflowResidentCommand& command) {
        struct Adapter {
            ResidentRaw& raw;FleetIO& fleet;
            bool ResidentAuditAllowed(const DataflowResidentCommand& c) { return fleet.ResidentAuditAllowed(c); }
            bool BeginResidentAudit(const DataflowResidentCommand& c) { return fleet.BeginResidentAudit(c); }
            bool FenceResidentAudit(const DataflowResidentCommand& c) { return fleet.FenceResidentAudit(c); }
            bool AuditResidentCoreRuntime(unsigned core,const DataflowResidentCommand& c) {
                raw.selected=core;return fleet.AuditResidentCoreRuntime(core,c);
            }
            bool ReadStableResidentCore(unsigned core,DataflowResidentFleetSample& s) {
                return raw.selected==core && fleet.ReadStableResidentCore(core,s);
            }
            bool EndResidentAudit(const DataflowResidentCommand& c) {
                if (!fleet.EndResidentAudit(c)) return false;
                raw.selected=0;return true;
            }
            void RetainResidentAudit() { fleet.RetainResidentAudit(); }
        } adapter{raw,fleet};
        return audit.Audit(command,adapter)==DataflowResidentFleetAuditComplete && audit.AuditedFor(command);
    }
    void RetainResident() { retained=true; }
};
int main() {
    (void)&q35_pci_fixture_main;(void)&q35_publisher_fixture_main;
    auto f=std::make_unique<Fixture>();auto plan=std::make_unique<DataflowStreamPlan>();
    assert(DataflowStreamPlanBuilder::Build(f->packed,f->manifest,f->workspace,f->rope,f->routes,
        99,DataflowGraphDecodeSteps,*plan));
    DataflowDynamicTokenContext token{};assert(DataflowDynamicTokenContext::Build(f->manifest,12,1,token));
    DataflowGraphRunDescriptor root{};
    assert(DataflowGraphRunBuild(plan->binding[0],plan->lease[0],f->routes,token,
        {6,DataflowGraphPromptSteps+1,DataflowGraphDecodeSteps},2,100,0,root));
    DataflowResidentCommand command{};assert(DataflowResidentCommandBuild(root,command));
    for (unsigned scenario=0;scenario<4;scenario++) {
        auto raw=std::make_unique<ResidentRaw>();auto fleet=std::make_unique<FleetIO>(command);
        auto audit=std::make_unique<DataflowResidentFleetAudit>();ComposedScope scope{*fleet,*audit};
        PioneerQwen35ResidentHostPci<ResidentRaw,ComposedScope> host(*raw,scope);
        if (scenario==1) fleet->sample[37].worker.state=DataflowWorkerFatal;
        if (scenario==2) fleet->mutate=true;
        if (scenario==3) fleet->end=false;
        assert(host.Submit(command));PutCandidate(*raw,command);
        assert(host.PollCandidate()==decltype(host)::CandidateReady);
        DataflowArgmaxResult output{};
        if (!scenario) {
            assert(host.AuditCandidate() && raw->selected==0 && audit->AuditedFor(command)
                && host.AcknowledgeCandidate() && host.Output(output) && output.token==42);
        } else {
            assert(!host.AuditCandidate() && host.IsRetained() && scope.retained
                && !host.AcknowledgeCandidate() && !host.Output(output));
        }
    }
    puts("PASS resident host acknowledges only after two-pass fleet audit and root restoration; SIMULATED ONLY");
}
