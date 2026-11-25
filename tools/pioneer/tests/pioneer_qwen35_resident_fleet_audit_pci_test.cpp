/* SPDX-License-Identifier: MIT */
// Real root host, PCI fleet adapter and two-pass audit over simulated IO.
#include "resident-host-fixture.inc"
#include "resident-fleet-fixture.inc"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35ResidentFleetAuditPci.h"

struct AuditRaw : ResidentRaw {
    FleetIO& fleet;
    int failSelect=-1;bool torn=false;
    explicit AuditRaw(FleetIO& samples):fleet(samples) {}
    bool SelectRuntimeCore(unsigned core) { selected=core;return core<DataflowCoreCount && int(core)!=failSelect; }
    bool MapQ38Panel() { return mapped; }
    bool ReadCoreMemory(unsigned core,unsigned address,void* output,unsigned bytes) {
        if (selected!=core || !mapped || core>=DataflowCoreCount) return false;
        auto& sample=fleet.sample[core];const void* source=nullptr;
        if (address==DataflowWorkerMailboxAddress && bytes==sizeof(sample.worker)) source=&sample.worker;
        if (address==DataflowRoleMailboxAddress && bytes==sizeof(sample.roles)) source=&sample.roles;
        if (address==DataflowQueueStateAddress && bytes==sizeof(sample.queues)) source=&sample.queues;
        if (!source) return false;
        memcpy(output,source,bytes);return true;
    }
    void Pause() {
        if (torn && selected==37) { fleet.sample[37].worker.failure=1;torn=false; }
    }
};
struct AuditScope {
    FleetIO& fleet;DataflowResidentFleetAudit& audit;
    bool retained=false,begin=true,finish=true,runtime=true;
    bool ResidentSessionAllowed(AuditRaw&,const DataflowResidentCommand&,unsigned) const { return !retained; }
    bool ResidentRootAllowed(AuditRaw& raw,const DataflowResidentCommand&,unsigned) const { return raw.selected==0; }
    bool ResidentAuditAllowed(AuditRaw&,const DataflowResidentCommand&) const { return !retained; }
    bool BeginResidentFleetAudit(AuditRaw&,const DataflowResidentCommand&) { return begin; }
    bool ResidentAuditReadAllowed(AuditRaw& raw,const DataflowResidentCommand&,unsigned core) const {
        return !retained && raw.selected==core && raw.mapped;
    }
    bool AuditResidentCoreRuntime(AuditRaw& raw,const DataflowResidentCommand& command,unsigned core) {
        raw.selected=core;return runtime && fleet.AuditResidentCoreRuntime(core,command);
    }
    bool FinishResidentFleetAudit(AuditRaw&,const DataflowResidentCommand&) { return finish; }
    bool ResidentAuditRootAllowed(AuditRaw& raw,const DataflowResidentCommand&) const { return raw.selected==0; }
    void RetainResidentAudit() { retained=true; }
    void RetainResident() { retained=true; }
    bool AuditResidentFleet(AuditRaw& raw,const DataflowResidentCommand& command) {
        PioneerQwen35ResidentFleetAuditPci<AuditRaw,AuditScope> adapter(raw,*this);
        return audit.Audit(command,adapter)==DataflowResidentFleetAuditComplete && audit.AuditedFor(command);
    }
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
    for (unsigned scenario=0;scenario<6;scenario++) {
        auto fleet=std::make_unique<FleetIO>(command);auto audit=std::make_unique<DataflowResidentFleetAudit>();
        auto raw=std::make_unique<AuditRaw>(*fleet);AuditScope scope{*fleet,*audit};
        PioneerQwen35ResidentHostPci<AuditRaw,AuditScope> host(*raw,scope);
        assert(host.Submit(command));PutCandidate(*raw,command);
        assert(host.PollCandidate()==decltype(host)::CandidateReady);
        if (scenario==1) raw->torn=true;
        if (scenario==2) raw->failSelect=37;
        if (scenario==3) scope.runtime=false;
        if (scenario==4) scope.finish=false;
        if (scenario==5) fleet->sample[37].worker.state=DataflowWorkerFatal;
        DataflowArgmaxResult output{};
        if (!scenario) {
            assert(host.AuditCandidate() && raw->selected==0 && audit->AuditedFor(command)
                && host.AcknowledgeCandidate() && host.Output(output) && output.token==42);
        } else {
            assert(!host.AuditCandidate() && audit->Retained() && scope.retained && host.IsRetained()
                && !host.AcknowledgeCandidate() && !host.Output(output));
        }
    }
    puts("PASS PCI fleet snapshots, torn reads, routes and host acknowledgement; SIMULATED ONLY");
}
