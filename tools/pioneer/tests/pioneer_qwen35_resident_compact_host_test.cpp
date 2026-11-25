/* SPDX-License-Identifier: MIT */
// Actual host transport and root command parser; panel and scope are mocked.
#define PIONEER_QWEN35_TREE_SETTLEMENT 1
#include "resident-host-fixture.inc"

struct ReadFaultRaw : ResidentRaw {
    int failRead=-1;unsigned reads=0;
    uint32_t Q38PanelRead(unsigned address) {
        const uint32_t value=ResidentRaw::Q38PanelRead(address);
        return int(reads++)==failRead?value^1U:value;
    }
};

static void Prime(ResidentRaw& io,const DataflowResidentCommand& command) {
    const auto payload=DataflowResidentCommandPayload(command),commit=DataflowResidentCommandCommit(command);
    auto* body=io.panel+(DataflowBarrierReleasePayload-DataflowPciL1ViewBase)/4;
    auto* marker=io.panel+(DataflowBarrierReleaseCommit-DataflowPciL1ViewBase)/4;
    for (unsigned i=0;i<11;i++) body[i]=~payload.word[i];
    for (unsigned i=1;i<4;i++) marker[i]=~commit.word[i];
    const auto prior=DataflowBarrierCommit(5,DataflowResidentMarkerBit|1U);
    for (unsigned i=4;i<16;i++) marker[i]=prior.word[i];
    marker[0]=5;
}
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
        auto io=std::make_unique<ResidentRaw>();ResidentScope scope;Host host(*io,scope);Prime(*io,command);
        assert(host.Submit(command) && host.Issued() && io->writes==16);
        assert(io->addresses[0]==DataflowBarrierReleaseCommit && !io->values[0]
            && io->addresses[15]==DataflowBarrierReleaseCommit && io->values[15]==command.epoch);
        DataflowResidentCommand received{};
        const auto* body=io->panel+(DataflowBarrierReleasePayload-DataflowPciL1ViewBase)/4;
        const auto* marker=io->panel+(DataflowBarrierReleaseCommit-DataflowPciL1ViewBase)/4;
        assert(DataflowResidentRootCommandRead(body,marker,1,5,received)==DataflowBarrierReady
            && !memcmp(&received,&command,sizeof(command)));
        // A child must never accept the compact root marker as a full NoC commit.
        assert(DataflowResidentCommandRead(body,marker,1,5,received)==DataflowBarrierWaiting);
        assert(DataflowResidentRootCommandRead(body,marker,2,6,received)==DataflowBarrierWaiting);
        auto* writableBody=io->panel+(DataflowBarrierReleasePayload-DataflowPciL1ViewBase)/4;
        auto* writableMarker=io->panel+(DataflowBarrierReleaseCommit-DataflowPciL1ViewBase)/4;
        writableMarker[3]^=1;
        assert(DataflowResidentRootCommandRead(body,marker,1,5,received)==DataflowBarrierWaiting);
        writableMarker[3]^=1;writableBody[10]^=1;
        assert(DataflowResidentRootCommandRead(body,marker,1,5,received)==DataflowBarrierInvalid);
        writableBody[10]^=1;
        PutCandidate(*io,command);DataflowArgmaxResult output{};
        assert(host.PollCandidate()==Host::CandidateReady && host.AuditCandidate()
            && host.AcknowledgeCandidate() && host.Output(output) && output.token==42 && io->writes==21);
    }
    for (int drop=0;drop<16;drop++) {
        auto io=std::make_unique<ResidentRaw>();ResidentScope scope;Host host(*io,scope);Prime(*io,command);io->drop=drop;
        assert(!host.Submit(command) && host.IsRetained() && scope.retained && host.Issued());
        const unsigned writes=io->writes;assert(!host.Submit(command) && io->writes==writes);
        if (drop) assert(!io->panel[(DataflowBarrierReleaseCommit-DataflowPciL1ViewBase)/4]);
        DataflowResidentCommand received{};
        assert(DataflowResidentRootCommandRead(io->panel+(DataflowBarrierReleasePayload-DataflowPciL1ViewBase)/4,
            io->panel+(DataflowBarrierReleaseCommit-DataflowPciL1ViewBase)/4,1,5,received)!=DataflowBarrierReady);
    }
    for (int fail=0;fail<21;fail++) {
        auto io=std::make_unique<ReadFaultRaw>();ResidentScope scope;
        PioneerQwen35ResidentHostPci<ReadFaultRaw,ResidentScope> host(*io,scope);Prime(*io,command);io->failRead=fail;
        assert(!host.Submit(command) && host.IsRetained() && scope.retained && host.Issued());
        const unsigned writes=io->writes;assert(!host.Submit(command) && io->writes==writes);
        if (writes<16) assert(!io->panel[(DataflowBarrierReleaseCommit-DataflowPciL1ViewBase)/4]);
    }
    {
        unsigned failures=0,successes=0;
        for (unsigned cutoff=0;cutoff<160;cutoff++) {
            auto io=std::make_unique<ResidentRaw>();ResidentScope scope;Host host(*io,scope);Prime(*io,command);
            scope.denyAfter=cutoff;
            if (host.Submit(command)) { ++successes;assert(io->writes==16 && !scope.retained); }
            else {
                ++failures;
                if (io->writes) assert(host.IsRetained() && scope.retained);
                if (io->writes && io->writes<16)
                    assert(!io->panel[(DataflowBarrierReleaseCommit-DataflowPciL1ViewBase)/4]);
            }
        }
        assert(failures && successes);
    }
    {
        auto io=std::make_unique<ResidentRaw>();ResidentScope scope;Host host(*io,scope);Prime(*io,command);
        io->panel[(DataflowBarrierReleasePayload-DataflowPciL1ViewBase)/4+15]=1;
        assert(!host.Submit(command) && host.IsRetained() && io->writes==1
            && !io->panel[(DataflowBarrierReleaseCommit-DataflowPciL1ViewBase)/4]);
    }
    {
        auto io=std::make_unique<ResidentRaw>();ResidentScope scope;Host host(*io,scope);Prime(*io,command);
        scope.allowed=false;assert(!host.Submit(command) && !scope.retained && !io->writes);
    }
    puts("PASS compact root publication rejects every dropped write; SIMULATED ONLY");
}
