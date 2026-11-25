/* SPDX-License-Identifier: MIT */
// Actual host candidate polling over simulated PCI; no device inference.
#define PIONEER_QWEN35_TREE_SETTLEMENT 1
#include "resident-host-fixture.inc"
struct PollRaw : ResidentRaw {
    unsigned reads=0;bool tear=false;
    uint32_t Q38PanelRead(unsigned address) { ++reads;return ResidentRaw::Q38PanelRead(address); }
    void Pause() {
        if (tear) {
            panel[(DataflowResidentCandidateAddress-DataflowPciL1ViewBase)/4+3]^=1;
            tear=false;
        }
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
    {
        auto raw=std::make_unique<PollRaw>();ResidentScope scope;
        PioneerQwen35ResidentHostPci<PollRaw,ResidentScope> host(*raw,scope);
        assert(host.Submit(command));raw->reads=0;
        assert(host.PollCandidate()==decltype(host)::CandidateWaiting && raw->reads==1);
        auto* marker=raw->panel+(DataflowResidentCandidateAddress-DataflowPciL1ViewBase)/4+7;
        *marker=command.trigger-1;raw->reads=0;
        assert(host.PollCandidate()==decltype(host)::CandidateWaiting && raw->reads==1);
        PutCandidate(*raw,command);raw->reads=0;
        assert(host.PollCandidate()==decltype(host)::CandidateReady && raw->reads==17);
        DataflowArgmaxResult out{};
        assert(host.AuditCandidate() && host.AcknowledgeCandidate() && host.Output(out) && out.token==42);
    }
    {
        auto raw=std::make_unique<PollRaw>();ResidentScope scope;
        PioneerQwen35ResidentHostPci<PollRaw,ResidentScope> host(*raw,scope);
        assert(host.Submit(command));raw->reads=0;
        raw->panel[(DataflowResidentCandidateAddress-DataflowPciL1ViewBase)/4+7]=command.trigger+1;
        assert(host.PollCandidate()==decltype(host)::CandidateRetained && raw->reads==1
            && host.IsRetained() && scope.retained && !host.AcknowledgeCandidate());
    }
    {
        auto raw=std::make_unique<PollRaw>();ResidentScope scope;
        PioneerQwen35ResidentHostPci<PollRaw,ResidentScope> host(*raw,scope);
        assert(host.Submit(command));PutCandidate(*raw,command);raw->reads=0;raw->tear=true;
        assert(host.PollCandidate()==decltype(host)::CandidateWaiting && raw->reads==17 && !host.IsRetained());
        PutCandidate(*raw,command);raw->reads=0;
        assert(host.PollCandidate()==decltype(host)::CandidateReady && raw->reads==17);
    }
    {
        auto raw=std::make_unique<PollRaw>();ResidentScope scope;
        PioneerQwen35ResidentHostPci<PollRaw,ResidentScope> host(*raw,scope);
        assert(host.Submit(command));
        raw->panel[(DataflowResidentCandidateAddress-DataflowPciL1ViewBase)/4+7]=command.trigger;raw->reads=0;
        assert(host.PollCandidate()==decltype(host)::CandidateRetained && raw->reads==17
            && host.IsRetained() && !host.AcknowledgeCandidate());
    }
    puts("PASS waiting candidate uses one read and rejects future or corrupt commits; SIMULATED ONLY");
}
