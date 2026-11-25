/* SPDX-License-Identifier: MIT */
// Actual root-only resident host PCI transaction over simulated panel MMIO.
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS 1
#define PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS 1
#define main q35_pci_fixture_main
#include "pioneer_qwen35_dataflow_pci_memory_test.cpp"
#undef main
#define Q35_PUBLISHER_FIXTURE_ONLY
#include "pioneer_qwen35_graph_publication_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35ResidentHostPci.h"
#include "../PioneerQwen35DataflowResidentWorker.h"

struct ResidentRaw : MockPciRaw {
    int drop=-1;bool selectOkay=true;
    unsigned addresses[80]{},values[80]{};
    ResidentRaw() { reset[0]=0x6000; }
    bool SelectRuntimeCore(unsigned core) { selected=core;return selectOkay && core==0; }
    void Q38PanelWrite(unsigned address,uint32_t value) {
        assert(writes<80);addresses[writes]=address;values[writes]=value;
        if (int(writes)==drop) { ++writes;return; }
        MockPciRaw::Q38PanelWrite(address,value);
    }
};
struct ResidentScope {
    bool allowed=true,root=true,audit=true,retained=false,corruptOutputDuringAudit=false;
    unsigned checks=0,denyAfter=UINT32_MAX,audits=0;
    template<class Raw> bool ResidentSessionAllowed(Raw&,const DataflowResidentCommand&,unsigned) {
        return allowed && ++checks<=denyAfter;
    }
    template<class Raw> bool ResidentRootAllowed(Raw& io,const DataflowResidentCommand&,unsigned) {
        return root && io.selected==0;
    }
    template<class Raw> bool AuditResidentFleet(Raw& io,const DataflowResidentCommand&) {
        ++audits;if (corruptOutputDuringAudit) io.mailbox[0].reserved[0]^=1;return audit;
    }
    void RetainResident() { retained=true; }
};
using Host=PioneerQwen35ResidentHostPci<ResidentRaw,ResidentScope>;
static void PutCandidate(ResidentRaw& io,const DataflowResidentCommand& command) {
    const auto candidate=DataflowResidentCandidateBuild(command);
    uint32_t words[8];std::memcpy(words,&candidate,sizeof(words));
    for (unsigned i=0;i<8;i++) io.panel[(DataflowResidentCandidateAddress-DataflowPciL1ViewBase)/4+i]=words[i];
    auto& box=io.mailbox[0];box.magic=DataflowWorkerMailboxMagic;box.version=DataflowWorkerMailboxVersion;
    box.logicalCore=0;box.commandSequence=box.completedSequence=command.trigger;
    box.acknowledgedSequence=command.trigger-1;box.state=DataflowWorkerComplete;box.phase=2;
    if (command.count==DataflowGraphDecodeSteps) {
        const unsigned token=42,score=0x3f800000,sequence=command.firstSequence+command.count-1;
        box.reserved[0]=token;box.reserved[1]=score;box.reserved[2]=sequence;
        box.reserved[3]=DataflowArgmaxResultMagic;box.reserved[4]=~token;
        box.reserved[5]=DataflowArgmaxResultChecksum(token,score,sequence);
    }
    DataflowRoleMailbox roles{};roles.magic=DataflowRoleMailboxMagic;roles.version=DataflowRoleMailboxVersion;
    roles.started[0]=DataflowRoleStartedReader;roles.started[1]=DataflowRoleStartedWriter;
    assert(DataflowResidentMailboxReady(box,roles,io.queue[0],0,command.trigger,command.trigger-1,
        command.count==DataflowGraphDecodeSteps,command.firstSequence+command.count-1));
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
        auto io=std::make_unique<ResidentRaw>();ResidentScope scope;Host host(*io,scope);
        auto tooLate=command;tooLate.trigger=DataflowResidentRunLimit+1;
        tooLate.checksum=DataflowResidentCommandChecksum(tooLate);
        assert(DataflowResidentCommandValid(tooLate) && !host.Submit(tooLate) && !io->writes && !scope.retained);
    }
    {
        auto io=std::make_unique<ResidentRaw>();ResidentScope scope;Host host(*io,scope);
        assert(!host.AcknowledgeCandidate() && host.Submit(command) && host.Issued() && io->writes==33);
        assert(io->addresses[0]==DataflowBarrierReleaseCommit && io->values[0]==0
            && io->addresses[32]==DataflowBarrierReleaseCommit && io->values[32]==command.epoch);
        DataflowResidentCommand received{};
        assert(DataflowResidentCommandRead(io->panel+(DataflowBarrierReleasePayload-DataflowPciL1ViewBase)/4,
            io->panel+(DataflowBarrierReleaseCommit-DataflowPciL1ViewBase)/4,1,5,received)==DataflowBarrierReady
            && received.trigger==command.trigger);
        assert(host.PollCandidate()==Host::CandidateWaiting && !host.AcknowledgeCandidate());
        PutCandidate(*io,command);
        DataflowArgmaxResult result{};assert(!host.Output(result));
        assert(host.PollCandidate()==Host::CandidateReady && !host.AcknowledgeCandidate()
            && host.AuditCandidate() && scope.audits==1 && host.AcknowledgeCandidate() && host.IsAcknowledged());
        assert(host.Output(result) && result.token==42
            && result.sequence==command.firstSequence+command.count-1);
        DataflowWorkerClearReceipt(io->mailbox[0]);
        assert(!DataflowArgmaxReadResult(io->mailbox[0],result.sequence,result)
            && host.Output(result) && result.token==42);
        assert(io->writes==38 && io->addresses[33]==DataflowResidentCandidateAckAddress
            && io->values[33]==0 && io->addresses[37]==DataflowResidentCandidateAckAddress
            && io->values[37]==command.trigger);
        assert(DataflowResidentCandidateAckMatches(io->panel+(DataflowResidentCandidateAckAddress-DataflowPciL1ViewBase)/4,
            command.trigger,command.epoch));
        assert(!host.Submit(command) && !host.AcknowledgeCandidate());
    }
    for (int drop:{0,1,32}) {
        auto io=std::make_unique<ResidentRaw>();ResidentScope scope;Host host(*io,scope);io->drop=drop;
        io->panel[(DataflowBarrierReleaseCommit-DataflowPciL1ViewBase)/4]=5;
        assert(!host.Submit(command) && host.IsRetained() && scope.retained && host.Issued());
        const unsigned writes=io->writes;assert(!host.Submit(command) && io->writes==writes);
    }
    {
        auto io=std::make_unique<ResidentRaw>();ResidentScope scope;Host host(*io,scope);
        scope.allowed=false;assert(!host.Submit(command) && !scope.retained && !io->writes);
    }
    {
        auto io=std::make_unique<ResidentRaw>();ResidentScope scope;Host host(*io,scope);
        io->selectOkay=false;assert(!host.Submit(command) && host.IsRetained() && scope.retained && !io->writes);
    }
    {
        auto io=std::make_unique<ResidentRaw>();ResidentScope scope;Host host(*io,scope);
        assert(host.Submit(command));scope.allowed=false;
        assert(host.PollCandidate()==Host::CandidateRetained && scope.retained && host.IsRetained());
    }
    {
        auto io=std::make_unique<ResidentRaw>();ResidentScope scope;Host host(*io,scope);
        assert(host.Submit(command));PutCandidate(*io,command);assert(host.PollCandidate()==Host::CandidateReady);
        scope.audit=false;assert(!host.AuditCandidate() && scope.retained && !host.AcknowledgeCandidate());
    }
    for (int drop:{33,34,35,36,37}) {
        auto io=std::make_unique<ResidentRaw>();ResidentScope scope;Host host(*io,scope);
        assert(host.Submit(command));PutCandidate(*io,command);
        assert(host.PollCandidate()==Host::CandidateReady && host.AuditCandidate());
        io->panel[(DataflowResidentCandidateAckAddress-DataflowPciL1ViewBase)/4]=1;
        io->drop=drop;
        assert(!host.AcknowledgeCandidate() && host.IsRetained() && scope.retained);
        const unsigned writes=io->writes;assert(!host.AcknowledgeCandidate() && io->writes==writes);
    }
    {
        auto io=std::make_unique<ResidentRaw>();ResidentScope scope;Host host(*io,scope);
        assert(host.Submit(command));PutCandidate(*io,command);
        io->panel[(DataflowResidentCandidateAddress-DataflowPciL1ViewBase)/4+3]^=1;
        assert(host.PollCandidate()==Host::CandidateRetained && host.IsRetained() && scope.retained);
    }
    {
        auto io=std::make_unique<ResidentRaw>();ResidentScope scope;Host host(*io,scope);
        assert(host.Submit(command));PutCandidate(*io,command);assert(host.PollCandidate()==Host::CandidateReady);
        io->mailbox[0].reserved[5]^=1;
        DataflowArgmaxResult result{};
        assert(!host.AuditCandidate() && host.IsRetained() && scope.retained
            && !host.AcknowledgeCandidate() && !host.Output(result));
    }
    {
        auto io=std::make_unique<ResidentRaw>();ResidentScope scope;Host host(*io,scope);
        assert(host.Submit(command));PutCandidate(*io,command);assert(host.PollCandidate()==Host::CandidateReady);
        scope.corruptOutputDuringAudit=true;DataflowArgmaxResult result{};
        assert(!host.AuditCandidate() && host.IsRetained() && scope.retained && !host.Output(result));
    }
    {
        auto io=std::make_unique<ResidentRaw>();ResidentScope scope;Host host(*io,scope);
        assert(host.Submit(command) && !host.Expire() && host.IsRetained() && scope.retained);
    }
    puts("PASS root-only resident command, candidate audit and one-shot acknowledgement; SIMULATED ONLY");
}
