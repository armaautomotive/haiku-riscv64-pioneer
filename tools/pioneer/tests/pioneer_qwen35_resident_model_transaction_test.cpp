/* SPDX-License-Identifier: MIT */
// The runner extracts the actual resident host-PCI test fixture. Host MMIO,
// its fleet audit and worker outputs are simulated; graph metadata is real.
#include "resident-host-fixture.inc"
#include "../PioneerQwen35DataflowResidentModelTransaction.h"

int main() {
    (void)&q35_pci_fixture_main;(void)&q35_publisher_fixture_main;
    auto f=std::make_unique<Fixture>();auto plan=std::make_unique<DataflowStreamPlan>();
    assert(DataflowStreamPlanBuilder::Build(f->packed,f->manifest,f->workspace,f->rope,f->routes,
        99,DataflowGraphDecodeSteps,*plan));
    auto liveOwner=std::make_unique<DataflowProjectionAuthority>();
    assert(liveOwner->BindValidatedModel(f->manifest,f->workspace,f->routes,f->rope));
    DataflowProjectionAuthority* live=liveOwner.get();
    auto scratch=std::make_unique<DataflowProjectionProgram>();
    DataflowGraphRunIssuer issuer;
    for (unsigned position=0;position<2;position++) {
        auto after=std::make_unique<DataflowProjectionAuthority>();
        auto io=std::make_unique<ResidentRaw>();ResidentScope scope;Host host(*io,scope);
        DataflowDynamicTokenContext token{};assert(DataflowDynamicTokenContext::Build(f->manifest,42+position,position,token));
        DataflowGraphRunTicket ticket{};
        assert(issuer.Issue(position?DataflowGraphDecodeSteps:DataflowGraphPromptSteps,ticket));
        DataflowGraphRunDescriptor root{};
        assert(DataflowGraphRunBuild(plan->binding[0],plan->lease[0],f->routes,token,ticket,position+1,100,0,root));
        DataflowResidentCommand command{};assert(DataflowResidentCommandBuild(root,command));
        DataflowResidentModelTransaction model;unsigned stage=0;
        assert(model.Prepare(live,*after,command,host,*scratch,stage));
        assert(model.Matches(command,host));
        auto altered=command;altered.position++;altered.checksum=DataflowResidentCommandChecksum(altered);
        assert(!model.Matches(altered,host));
        DataflowArgmaxResult output{};
        assert(!model.Commit(host,output) && output.token==Vocabulary && live==liveOwner.get());
        assert(host.Submit(command));PutCandidate(*io,command);
        assert(host.PollCandidate()==Host::CandidateReady && host.AuditCandidate());
        assert(!model.Commit(host,output) && host.AcknowledgeCandidate());
        auto wrongIo=std::make_unique<ResidentRaw>();ResidentScope wrongScope;Host wrong(*wrongIo,wrongScope);
        assert(wrong.Submit(command));PutCandidate(*wrongIo,command);
        assert(wrong.PollCandidate()==Host::CandidateReady && wrong.AuditCandidate() && wrong.AcknowledgeCandidate());
        assert(!model.Commit(wrong,output) && live==liveOwner.get());
        assert(model.Commit(host,output) && live==after.get() && model.Retired()==liveOwner.get());
        assert(output.token==(position?42:Vocabulary) && !model.Commit(host,output));
        liveOwner.swap(after);
    }
    DataflowDynamicTokenContext token{};assert(DataflowDynamicTokenContext::Build(f->manifest,50,2,token));
    DataflowGraphRunTicket ticket{};assert(issuer.Issue(DataflowGraphDecodeSteps,ticket));
    DataflowGraphRunDescriptor root{};
    assert(DataflowGraphRunBuild(plan->binding[0],plan->lease[0],f->routes,token,ticket,3,100,0,root));
    DataflowResidentCommand command{};assert(DataflowResidentCommandBuild(root,command));
    {
        auto deniedAfter=std::make_unique<DataflowProjectionAuthority>();
        auto deniedIo=std::make_unique<ResidentRaw>();ResidentScope deniedScope;
        Host deniedHost(*deniedIo,deniedScope);DataflowResidentModelTransaction denied;unsigned deniedStage=0;
        assert(denied.Prepare(live,*deniedAfter,command,deniedHost,*scratch,deniedStage));
        assert(deniedHost.Submit(command));PutCandidate(*deniedIo,command);
        assert(deniedHost.PollCandidate()==Host::CandidateReady);
        deniedScope.audit=false;
        assert(!deniedHost.AuditCandidate() && deniedHost.IsRetained());
        DataflowArgmaxResult deniedOutput{};
        assert(!denied.Commit(deniedHost,deniedOutput) && deniedOutput.token==Vocabulary
            && live==liveOwner.get() && !denied.Retired());
    }
    auto after=std::make_unique<DataflowProjectionAuthority>();auto io=std::make_unique<ResidentRaw>();
    ResidentScope scope;Host host(*io,scope);DataflowResidentModelTransaction model;unsigned stage=0;
    assert(model.Prepare(live,*after,command,host,*scratch,stage));
    assert(after->ActivationReady(0) && after->InvalidateActivation(0));
    assert(!model.Matches(command,host));
    assert(host.Submit(command));PutCandidate(*io,command);
    assert(host.PollCandidate()==Host::CandidateReady && host.AuditCandidate() && host.AcknowledgeCandidate());
    DataflowArgmaxResult output{};
    assert(!model.Commit(host,output) && output.token==Vocabulary && live==liveOwner.get() && !model.Retired());
    puts("PASS resident model promotion requires the exact audited host, ACK and unchanged authorities; SIMULATED ONLY");
}
