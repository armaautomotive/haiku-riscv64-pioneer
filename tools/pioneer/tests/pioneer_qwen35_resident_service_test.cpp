/* SPDX-License-Identifier: MIT */
// Exercise command receipt through graph callback and candidate gather using
// 120 resident worker actors and simulated NoC. No real graph or device runs.
#define Q35_PUBLISHER_FIXTURE_ONLY
#include "pioneer_qwen35_graph_publication_test.cpp"
#include "../PioneerQwen35DataflowResidentService.h"
#include "../PioneerQwen35DataflowResidentReceipt.h"
#include "../PioneerQwen35DataflowResidentSenderReceipt.h"
#include <array>
struct ServiceFabric {
    DataflowBarrierPacket payload[DataflowCoreCount]{},marker[DataflowCoreCount]{};
    DataflowBarrierPacket arrival[DataflowCoreCount]{},arrivalMarker[DataflowCoreCount]{};
    bool begun[DataflowCoreCount]{},ready[DataflowCoreCount]{};
    DataflowResidentSenderReceipt sender[DataflowCoreCount]{};
    unsigned graphCalls=0,candidates=0,started=0;
    int failGraph=-1,failGatherAfterDelivery=-1,corruptSender=-1,failReceiptRead=-1,delaySender=-1;
    unsigned settlementReads=0,delayRounds=0;DataflowResidentCommand delayed{};
};
struct ServiceIO {
    ServiceFabric& f;unsigned core;
    DataflowBarrierReadResult ReadResidentRequest(unsigned completed,unsigned epoch,DataflowResidentCommand& out) {
#ifdef PIONEER_QWEN35_TREE_SETTLEMENT
        if (!core) return DataflowResidentRootCommandRead(f.payload[core].word,f.marker[core].word,completed,epoch,out);
#endif
        return DataflowResidentCommandRead(f.payload[core].word,f.marker[core].word,completed,epoch,out);
    }
    bool ResidentBegin(unsigned worker,const DataflowResidentCommand&,const DataflowGraphRunDescriptor& d) {
        assert(worker==core && d.core==core && !f.begun[core]);f.begun[core]=true;++f.started;return true;
    }
    bool ResidentResetAndReady(unsigned worker,unsigned) {
        assert(worker==core && f.begun[core] && !f.ready[core]);f.ready[core]=true;return true;
    }
    bool SendResidentDispatch(unsigned source,unsigned target,const DataflowBarrierPacket& p,const DataflowBarrierPacket& c) {
        assert(source==core && DataflowBarrierReleaseEdge(source,target));f.payload[target]=p;f.marker[target]=c;return true;
    }
    bool SendArrival(unsigned worker,const DataflowBarrierPacket& p,const DataflowBarrierPacket& c) {
        assert(worker==core && worker);
#ifdef PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS
        const bool delivered=DataflowBarrierSendShared(p,c,[&](bool marker,const DataflowBarrierPacket& packet) {
            if (marker) f.arrivalMarker[worker]=packet;else f.arrival[worker]=packet;return true;
        });
#else
        f.arrival[worker]=p;f.arrivalMarker[worker]=c;const bool delivered=true;
#endif
        return delivered && !(int(worker)==f.failGatherAfterDelivery
            && c.word[2]==(DataflowResidentGatherTag|2U));
    }
    bool SendRelease(unsigned child,const DataflowBarrierPacket& p,const DataflowBarrierPacket& c) {
        assert(DataflowBarrierReleaseEdge(core,child));
#ifdef PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS
        return DataflowBarrierSendShared(p,c,[&](bool marker,const DataflowBarrierPacket& packet) {
            if (marker) f.marker[child]=packet;else f.payload[child]=packet;return true;
        });
#else
        f.payload[child]=p;f.marker[child]=c;return true;
#endif
    }
    DataflowBarrierReadResult ReadArrival(unsigned child,unsigned epoch,unsigned step,DataflowBarrierPacket& out) {
        if (!DataflowBarrierReleaseEdge(core,child)) return DataflowBarrierInvalid;
#ifdef PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS
        return DataflowBarrierReadShared(f.arrival[child].word,f.arrivalMarker[child].word,epoch,step,child,out);
#else
        return DataflowBarrierReadCommitted(f.arrival[child].word,f.arrivalMarker[child].word,epoch,step,out);
#endif
    }
    DataflowBarrierReadResult ReadRelease(unsigned worker,unsigned epoch,unsigned step,DataflowBarrierPacket& out) {
        if (worker!=core || !core) return DataflowBarrierInvalid;
#ifdef PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS
        return DataflowBarrierReadShared(f.payload[core].word,f.marker[core].word,epoch,step,worker,out);
#else
        return DataflowBarrierReadCommitted(f.payload[core].word,f.marker[core].word,epoch,step,out);
#endif
    }
#ifdef PIONEER_QWEN35_TREE_SETTLEMENT
    DataflowBarrierReadResult ReadChildSettlement(unsigned child,unsigned epoch,unsigned trigger) {
        assert(DataflowBarrierReleaseEdge(core,child));++f.settlementReads;
        if (int(child)==f.failReceiptRead) return DataflowBarrierInvalid;
        const auto receipt=f.sender[child];
        const uint32_t words[4]={receipt.trigger,receipt.epoch,receipt.core,receipt.checksum};
        return DataflowResidentSenderReceiptReadResult(words,words,child,trigger,epoch);
    }
#endif
    bool ExecuteResidentGraph(unsigned worker,const DataflowGraphRunDescriptor& d) {
        assert(worker==core && DataflowGraphRunValid(d,core));++f.graphCalls;return int(core)!=f.failGraph;
    }
    bool RecordResidentGraphResult(unsigned worker,unsigned,bool) { return worker==core; }
    bool PublishResidentSenderReceipt(unsigned worker,const DataflowResidentCommand& command) {
#ifdef PIONEER_QWEN35_TREE_SETTLEMENT
        if (worker==core && int(core)==f.delaySender && f.delayRounds) {
            f.delayed=command;return true; // simulate delayed L1 visibility
        }
#endif
        const bool okay=worker==core && DataflowResidentSenderReceiptPublish(
            reinterpret_cast<volatile uint32_t*>(&f.sender[core]),core,command.trigger,command.epoch);
        if (okay && int(core)==f.corruptSender) f.sender[core].checksum^=1;
        return okay;
    }
    bool PublishResidentCandidate(unsigned worker,const DataflowResidentCommand&) {
        assert(worker==core && f.ready[core]);if (!core) ++f.candidates;return true;
    }
};
using Service=DataflowResidentServiceRun<ServiceIO>;
struct ServiceFixture {
    std::unique_ptr<Fixture> f=std::make_unique<Fixture>();
    std::unique_ptr<DataflowStreamPlan> plan=std::make_unique<DataflowStreamPlan>();
    std::unique_ptr<ServiceFabric> fabric=std::make_unique<ServiceFabric>();
    std::array<DataflowGraphRunDescriptor,DataflowCoreCount> installed{};
    std::array<std::unique_ptr<ServiceIO>,DataflowCoreCount> io;
    std::array<std::unique_ptr<Service>,DataflowCoreCount> service;
    std::array<DataflowResidentServiceResult,DataflowCoreCount> state{};
    DataflowResidentCommand command{};
    explicit ServiceFixture(unsigned polls=10000) {
        assert(DataflowStreamPlanBuilder::Build(f->packed,f->manifest,f->workspace,f->rope,f->routes,
            99,DataflowGraphDecodeSteps,*plan));
        DataflowDynamicTokenContext old{},token{};
        assert(DataflowDynamicTokenContext::Build(f->manifest,11,0,old)
            && DataflowDynamicTokenContext::Build(f->manifest,12,1,token));
        for (unsigned core=0;core<DataflowCoreCount;core++) {
            assert(DataflowGraphRunBuild(plan->binding[core],plan->lease[core],f->routes,old,
                {5,1,DataflowGraphPromptSteps},1,polls,0,installed[core]));
            io[core]=std::make_unique<ServiceIO>(ServiceIO{*fabric,core});
            service[core]=std::make_unique<Service>(*io[core],core,installed[core],1,5,DataflowGraphPromptSteps);
        }
        DataflowGraphRunDescriptor root{};
        assert(DataflowGraphRunBuild(plan->binding[0],plan->lease[0],f->routes,token,
            {6,DataflowGraphPromptSteps+1,DataflowGraphDecodeSteps},2,polls,0,root)
            && DataflowResidentCommandBuild(root,command));
    }
    void Submit() {
        fabric->payload[0]=DataflowResidentCommandPayload(command);
        fabric->marker[0]=DataflowResidentCommandCommit(command);
#ifdef PIONEER_QWEN35_TREE_SETTLEMENT
        for (unsigned i=4;i<16;i++) fabric->marker[0].word[i]=0xa5a5a5a5U;
#endif
    }
    void Next() {
        const unsigned last=DataflowGraphPromptSteps+DataflowGraphDecodeSteps;
        DataflowDynamicTokenContext token{};
        assert(DataflowDynamicTokenContext::Build(f->manifest,13,2,token));
        DataflowGraphRunDescriptor root{};
        assert(DataflowGraphRunBuild(plan->binding[0],plan->lease[0],f->routes,token,
            {7,last+1,DataflowGraphDecodeSteps},3,command.pollLimit,0,root)
            && DataflowResidentCommandBuild(root,command));
        for (unsigned core=0;core<DataflowCoreCount;core++) {
            assert(state[core]==DataflowResidentServiceCandidate && fabric->ready[core]);
            fabric->begun[core]=fabric->ready[core]=false;
            service[core]=std::make_unique<Service>(*io[core],core,installed[core],2,6,last);
            state[core]=DataflowResidentServiceIdle;
        }
        Submit();
    }
    void Tick() {
#ifdef PIONEER_QWEN35_TREE_SETTLEMENT
        if (fabric->delayed.trigger && fabric->delayRounds && !--fabric->delayRounds) {
            const unsigned core=unsigned(fabric->delaySender);
            assert(DataflowResidentSenderReceiptPublish(reinterpret_cast<volatile uint32_t*>(&fabric->sender[core]),
                core,fabric->delayed.trigger,fabric->delayed.epoch));
        }
#endif
        for (unsigned core=0;core<DataflowCoreCount;core++)
            if (state[core]!=DataflowResidentServiceCandidate && state[core]!=DataflowResidentServiceRetained)
                state[core]=service[core]->Tick();
    }
};
int main() {
    (void)&q35_publisher_fixture_main;
    {
        auto run=std::make_unique<ServiceFixture>();run->Tick();
        assert(!run->fabric->started && run->state[0]==DataflowResidentServiceIdle);
        volatile uint32_t words[8]{};
        assert(DataflowResidentCandidatePublish(words,run->command));
        DataflowResidentCandidate receipt{};auto* raw=reinterpret_cast<uint8_t*>(&receipt);
        for (unsigned i=0;i<8;i++) for (unsigned b=0;b<4;b++) raw[4*i+b]=uint8_t(words[i]>>(8*b));
        assert(DataflowResidentCandidateMatches(receipt,run->command));
        receipt.epoch++;assert(!DataflowResidentCandidateMatches(receipt,run->command));
        receipt.epoch--;receipt.commit=0;assert(!DataflowResidentCandidateMatches(receipt,run->command));
        volatile uint32_t ack[4]={run->command.trigger,~run->command.trigger,run->command.epoch,~run->command.epoch};
        assert(DataflowResidentCandidateAckMatches(ack,run->command.trigger,run->command.epoch));
        assert(!DataflowResidentCandidateAckMatches(ack,run->command.trigger+1,run->command.epoch));
        ack[3]^=1;assert(!DataflowResidentCandidateAckMatches(ack,run->command.trigger,run->command.epoch));
        run->Submit();
        for (unsigned round=0;round<300 && run->state[0]!=DataflowResidentServiceCandidate;round++) run->Tick();
        assert(run->state[0]==DataflowResidentServiceCandidate && run->fabric->candidates==1
            && run->fabric->started==DataflowCoreCount && run->fabric->graphCalls==DataflowCoreCount);
        for (unsigned core=0;core<DataflowCoreCount;core++)
            assert(run->state[core]==DataflowResidentServiceCandidate && run->fabric->ready[core]
                && DataflowResidentSenderReceiptMatches(run->fabric->sender[core],core,run->command)
                && run->service[core]->CompletedSequence()==2 && run->service[core]->CompletedEpoch()==6
                && run->service[core]->CompletedLastStep()==DataflowGraphPromptSteps+DataflowGraphDecodeSteps);
        run->Next();
        for (unsigned round=0;round<300 && run->state[0]!=DataflowResidentServiceCandidate;round++) run->Tick();
        assert(run->state[0]==DataflowResidentServiceCandidate && run->fabric->candidates==2
            && run->fabric->started==2*DataflowCoreCount && run->fabric->graphCalls==2*DataflowCoreCount);
        for (unsigned core=0;core<DataflowCoreCount;core++)
            assert(run->state[core]==DataflowResidentServiceCandidate && run->fabric->ready[core]
                && DataflowResidentSenderReceiptMatches(run->fabric->sender[core],core,run->command)
                && run->service[core]->CompletedSequence()==3 && run->service[core]->CompletedEpoch()==7);
    }
    {
        // A gather can be delivered even while its sender reports failure.
        // The failed sender must not publish its local receipt.
#ifdef PIONEER_QWEN35_TREE_SETTLEMENT
        for (unsigned failed: {1U,4U,5U,29U,30U,119U}) {
            auto run=std::make_unique<ServiceFixture>(64);run->fabric->failGatherAfterDelivery=int(failed);run->Submit();
            for (unsigned round=0;round<300;round++) run->Tick();
            assert(run->state[0]==DataflowResidentServiceRetained && run->state[failed]==DataflowResidentServiceRetained
                && !run->fabric->candidates && !DataflowResidentSenderReceiptMatches(run->fabric->sender[failed],failed,run->command));
        }
#else
        auto run=std::make_unique<ServiceFixture>();run->fabric->failGatherAfterDelivery=119;run->Submit();
        for (unsigned round=0;round<300 && run->state[0]!=DataflowResidentServiceCandidate;round++) run->Tick();
        assert(run->state[0]==DataflowResidentServiceCandidate && run->state[119]==DataflowResidentServiceRetained
            && !DataflowResidentSenderReceiptMatches(run->fabric->sender[119],119,run->command));
#endif
    }
#ifdef PIONEER_QWEN35_TREE_SETTLEMENT
    {
        auto run=std::make_unique<ServiceFixture>(64);run->fabric->failReceiptRead=30;run->Submit();
        for (unsigned round=0;round<300;round++) run->Tick();
        assert(run->state[0]==DataflowResidentServiceRetained && !run->fabric->candidates
            && run->fabric->settlementReads>0);
    }
    {
        auto run=std::make_unique<ServiceFixture>(128);run->fabric->delaySender=119;
        run->fabric->delayRounds=20;run->Submit();
        for (unsigned round=0;round<300 && run->state[0]!=DataflowResidentServiceCandidate;round++) run->Tick();
        assert(run->state[0]==DataflowResidentServiceCandidate && run->fabric->candidates==1
            && !run->fabric->delayRounds && run->fabric->settlementReads>0);
    }
    {
        auto run=std::make_unique<ServiceFixture>(64);run->fabric->corruptSender=117;run->Submit();
        for (unsigned round=0;round<300;round++) run->Tick();
        assert(run->state[0]==DataflowResidentServiceRetained && !run->fabric->candidates
            && run->fabric->settlementReads>0);
    }
#endif
    {
        auto run=std::make_unique<ServiceFixture>(12);run->fabric->failGraph=37;run->Submit();
        for (unsigned round=0;round<80;round++) run->Tick();
        assert(run->state[37]==DataflowResidentServiceRetained
            && run->state[0]==DataflowResidentServiceRetained && !run->fabric->candidates);
    }
    {
        auto run=std::make_unique<ServiceFixture>();run->Submit();run->fabric->payload[0].word[8]^=1;
        run->Tick();assert(run->state[0]==DataflowResidentServiceRetained
            && !run->fabric->started && !run->fabric->graphCalls);
    }
    puts("PASS resident service command through graph callback and candidate gather; SIMULATED ONLY");
}
