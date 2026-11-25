/* SPDX-License-Identifier: MIT */
// Actual host and PCI adapter with simulated tree proof and worker state.
#define PIONEER_QWEN35_TREE_SETTLEMENT 1
#include "resident-pci-audit-fixture.inc"
#include "../PioneerQwen35DataflowResidentTreeAudit.h"
struct TreeRaw : AuditRaw {
    unsigned rootReads=0,nonRootReads=0,nonRootSelects=0;
    explicit TreeRaw(FleetIO& fleet):AuditRaw(fleet) {}
    bool SelectRuntimeCore(unsigned core) {
        if (core) ++nonRootSelects;
        return AuditRaw::SelectRuntimeCore(core);
    }
    bool ReadCoreMemory(unsigned core,unsigned address,void* output,unsigned bytes) {
        if (core) ++nonRootReads;else ++rootReads;
        return AuditRaw::ReadCoreMemory(core,address,output,bytes);
    }
};
struct TreeScope {
    FleetIO& fleet;DataflowResidentTreeAudit& audit;
    bool retained=false,treeProof=true,runtime=true,finish=true;
    bool ResidentSessionAllowed(TreeRaw&,const DataflowResidentCommand&,unsigned) const { return !retained; }
    bool ResidentRootAllowed(TreeRaw& raw,const DataflowResidentCommand&,unsigned) const { return raw.selected==0; }
    bool ResidentAuditAllowed(TreeRaw&,const DataflowResidentCommand&) const { return !retained; }
    bool ResidentTreeAuditAllowed(TreeRaw&,const DataflowResidentCommand&) const { return !retained && treeProof; }
    bool BeginResidentFleetAudit(TreeRaw&,const DataflowResidentCommand&) { return true; }
    bool ResidentAuditReadAllowed(TreeRaw& raw,const DataflowResidentCommand&,unsigned core) const {
        return !retained && raw.selected==core && raw.mapped;
    }
    bool AuditResidentCoreRuntime(TreeRaw& raw,const DataflowResidentCommand& command,unsigned core) {
        return runtime && raw.selected==core && fleet.AuditResidentCoreRuntime(core,command);
    }
    bool FinishResidentFleetAudit(TreeRaw&,const DataflowResidentCommand&) { return finish; }
    bool ResidentAuditRootAllowed(TreeRaw& raw,const DataflowResidentCommand&) const { return raw.selected==0; }
    void RetainResidentAudit() { retained=true; }
    void RetainResident() { retained=true; }
    bool AuditResidentFleet(TreeRaw& raw,const DataflowResidentCommand& command) {
        PioneerQwen35ResidentFleetAuditPci<TreeRaw,TreeScope> adapter(raw,*this);
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
    for (unsigned scenario=0;scenario<5;scenario++) {
        auto fleet=std::make_unique<FleetIO>(command);auto audit=std::make_unique<DataflowResidentTreeAudit>();
        auto raw=std::make_unique<TreeRaw>(*fleet);TreeScope scope{*fleet,*audit};
        PioneerQwen35ResidentHostPci<TreeRaw,TreeScope> host(*raw,scope);
        assert(host.Submit(command));PutCandidate(*raw,command);
        if (scenario==3) {
            auto legacy=DataflowResidentCandidateBuild(command);legacy.version=1;
            legacy.checksum=DataflowResidentCandidateChecksum(legacy);
            uint32_t words[8];memcpy(words,&legacy,sizeof(words));
            for (unsigned i=0;i<8;i++) raw->panel[(DataflowResidentCandidateAddress-DataflowPciL1ViewBase)/4+i]=words[i];
            assert(host.PollCandidate()==decltype(host)::CandidateRetained && host.IsRetained()
                && !host.AcknowledgeCandidate() && !raw->rootReads && !raw->nonRootSelects);
            continue;
        }
        assert(host.PollCandidate()==decltype(host)::CandidateReady);
        if (scenario==1) scope.treeProof=false;
        if (scenario==2) fleet->sample[0].sender.checksum^=1;
        if (scenario==4) scope.finish=false;
        DataflowArgmaxResult output{};
        if (!scenario) {
            assert(host.AuditCandidate() && audit->AuditedFor(command) && raw->selected==0
                && raw->rootReads==32 && !raw->nonRootReads && !raw->nonRootSelects
                && host.AcknowledgeCandidate() && host.Output(output) && output.token==42);
        } else {
            assert(!host.AuditCandidate() && host.IsRetained() && scope.retained
                && !host.AcknowledgeCandidate() && !host.Output(output) && !raw->nonRootSelects);
            if (scenario==1) assert(!raw->rootReads && !audit->Retained());
            else assert(audit->Retained());
        }
    }
    puts("PASS root-only PCI audit rejects unproven tree and legacy receipt; SIMULATED ONLY");
}
