/* SPDX-License-Identifier: MIT */
// Interleave all 120 real lifecycle state machines over simulated NoC slots.
#define Q35_PUBLISHER_FIXTURE_ONLY
#include "pioneer_qwen35_graph_publication_test.cpp"
#include "../PioneerQwen35DataflowResidentLifecycle.h"
#include <array>

struct Fabric {
    DataflowBarrierPacket arrival[DataflowCoreCount]{},arrivalCommit[DataflowCoreCount]{};
    DataflowBarrierPacket release[DataflowCoreCount]{},releaseCommit[DataflowCoreCount]{};
    bool begun[DataflowCoreCount]{},ready[DataflowCoreCount]{};
    unsigned beginCount=0,readyCount=0,sends=0;
    int resetFailure=-1,gatherFailureAfterDelivery=-1;
    bool delayedOne=false;
};
struct NodeIO {
    Fabric& f;unsigned core;
    bool ResidentBegin(unsigned c,const DataflowResidentCommand&,const DataflowGraphRunDescriptor&) {
        assert(c==core && !f.begun[c]);f.begun[c]=true;++f.beginCount;return true;
    }
    bool ResidentResetAndReady(unsigned c,unsigned) {
        assert(c==core && f.begun[c] && !f.ready[c]);
        if (int(c)==f.resetFailure) return false;
        f.ready[c]=true;++f.readyCount;return true;
    }
    bool SendResidentDispatch(unsigned source,unsigned dest,const DataflowBarrierPacket& p,
        const DataflowBarrierPacket& c) {
        assert(source==core && DataflowBarrierReleaseEdge(source,dest));
        f.release[dest]=p;f.releaseCommit[dest]=c;++f.sends;return true;
    }
    bool SendArrival(unsigned source,const DataflowBarrierPacket& p,const DataflowBarrierPacket& c) {
        assert(source==core && source);
#ifdef PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS
        const bool okay=DataflowBarrierSendShared(p,c,[&](bool marker,const DataflowBarrierPacket& packet) {
            if (marker) f.arrivalCommit[source]=packet;else f.arrival[source]=packet;return true;
        });
#else
        f.arrival[source]=p;f.arrivalCommit[source]=c;const bool okay=true;
#endif
        return okay && !(int(source)==f.gatherFailureAfterDelivery
            && c.word[2]==(DataflowResidentGatherTag|2U));
    }
    bool SendRelease(unsigned child,const DataflowBarrierPacket& p,const DataflowBarrierPacket& c) {
        assert(DataflowBarrierReleaseEdge(core,child));
        if (core==0 && child==1 && c.word[2]==(DataflowResidentReadyTag|2U)) f.delayedOne=true;
#ifdef PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS
        return DataflowBarrierSendShared(p,c,[&](bool marker,const DataflowBarrierPacket& packet) {
            if (marker) f.releaseCommit[child]=packet;else f.release[child]=packet;return true;
        });
#else
        f.release[child]=p;f.releaseCommit[child]=c;
        return true;
#endif
    }
    DataflowBarrierReadResult ReadArrival(unsigned child,unsigned epoch,unsigned step,DataflowBarrierPacket& out) {
        if (!DataflowBarrierReleaseEdge(core,child)) return DataflowBarrierInvalid;
#ifdef PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS
        return DataflowBarrierReadShared(f.arrival[child].word,f.arrivalCommit[child].word,epoch,step,child,out);
#else
        return DataflowBarrierReadCommitted(f.arrival[child].word,f.arrivalCommit[child].word,epoch,step,out);
#endif
    }
    DataflowBarrierReadResult ReadRelease(unsigned worker,unsigned epoch,unsigned step,DataflowBarrierPacket& out) {
        if (core!=worker || !core) return DataflowBarrierInvalid;
#ifdef PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS
        return DataflowBarrierReadShared(f.release[core].word,f.releaseCommit[core].word,epoch,step,worker,out);
#else
        return DataflowBarrierReadCommitted(f.release[core].word,f.releaseCommit[core].word,epoch,step,out);
#endif
    }
};
using Lifecycle=DataflowResidentLifecycle<NodeIO>;
struct FixtureRun {
    std::unique_ptr<Fixture> fixture=std::make_unique<Fixture>();
    std::unique_ptr<DataflowStreamPlan> plan=std::make_unique<DataflowStreamPlan>();
    std::unique_ptr<Fabric> fabric=std::make_unique<Fabric>();
    std::array<std::unique_ptr<NodeIO>,DataflowCoreCount> io;
    std::array<std::unique_ptr<Lifecycle>,DataflowCoreCount> nodes;
    DataflowResidentCommand command{};
    std::array<DataflowResidentLifecycleResult,DataflowCoreCount> state{};
    explicit FixtureRun(unsigned limit=10000) {
        assert(DataflowStreamPlanBuilder::Build(fixture->packed,fixture->manifest,fixture->workspace,
            fixture->rope,fixture->routes,99,DataflowGraphDecodeSteps,*plan));
        DataflowDynamicTokenContext token{};
        assert(DataflowDynamicTokenContext::Build(fixture->manifest,12,1,token));
        DataflowGraphRunDescriptor root{};
        assert(DataflowGraphRunBuild(plan->binding[0],plan->lease[0],fixture->routes,token,
            {6,DataflowGraphPromptSteps+1,DataflowGraphDecodeSteps},2,limit,0,root)
            && DataflowResidentCommandBuild(root,command));
        for (unsigned c=0;c<DataflowCoreCount;c++) {
            io[c]=std::make_unique<NodeIO>(NodeIO{*fabric,c});nodes[c]=std::make_unique<Lifecycle>(*io[c]);
        }
    }
    void Start() {
        DataflowDynamicTokenContext old{};
        assert(DataflowDynamicTokenContext::Build(fixture->manifest,11,0,old));
        for (unsigned c=0;c<DataflowCoreCount;c++) {
            DataflowResidentCommand received=command;
            if (c) assert(DataflowResidentCommandRead(fabric->release[c].word,fabric->releaseCommit[c].word,
                1,5,received)==DataflowBarrierReady);
            DataflowGraphRunDescriptor installed{},expanded{};
            assert(DataflowGraphRunBuild(plan->binding[c],plan->lease[c],fixture->routes,old,
                {5,1,DataflowGraphPromptSteps},1,command.pollLimit,0,installed));
            state[c]=nodes[c]->Start(c,received,installed,1,5,DataflowGraphPromptSteps,expanded);
            assert(state[c]==DataflowResidentLifecycleRun && expanded.token.token==12 && expanded.token.position==1);
        }
        assert(fabric->beginCount==DataflowCoreCount && fabric->sends==DataflowCoreCount-1);
    }
    void GraphDone(unsigned skip=DataflowCoreCount) {
        for (unsigned c=0;c<DataflowCoreCount;c++) if (c!=skip)
            state[c]=nodes[c]->GraphDone(true);
    }
    void StartNext() {
        DataflowDynamicTokenContext old{},token{};
        assert(DataflowDynamicTokenContext::Build(fixture->manifest,11,0,old)
            && DataflowDynamicTokenContext::Build(fixture->manifest,13,2,token));
        const unsigned last=DataflowGraphPromptSteps+DataflowGraphDecodeSteps;
        DataflowGraphRunDescriptor root{};
        assert(DataflowGraphRunBuild(plan->binding[0],plan->lease[0],fixture->routes,token,
            {7,last+1,DataflowGraphDecodeSteps},3,command.pollLimit,0,root)
            && DataflowResidentCommandBuild(root,command));
        for (unsigned c=0;c<DataflowCoreCount;c++) {
            assert(state[c]==DataflowResidentLifecycleGathered && fabric->ready[c]);
            fabric->begun[c]=fabric->ready[c]=false;
            nodes[c]=std::make_unique<Lifecycle>(*io[c]);
            DataflowResidentCommand received=command;
            if (c) assert(DataflowResidentCommandRead(fabric->release[c].word,fabric->releaseCommit[c].word,
                2,6,received)==DataflowBarrierReady);
            DataflowGraphRunDescriptor installed{},expanded{};
            assert(DataflowGraphRunBuild(plan->binding[c],plan->lease[c],fixture->routes,old,
                {5,1,DataflowGraphPromptSteps},1,command.pollLimit,0,installed));
            state[c]=nodes[c]->Start(c,received,installed,2,6,last,expanded);
            assert(state[c]==DataflowResidentLifecycleRun && expanded.token.token==13 && expanded.token.position==2);
        }
    }
    void Tick(bool delayCoreOne=false) {
        for (unsigned c=0;c<DataflowCoreCount;c++) {
            if (delayCoreOne && c==1 && fabric->delayedOne) continue;
            if (state[c]==DataflowResidentLifecyclePending) state[c]=nodes[c]->Tick();
        }
    }
};
int main() {
    (void)&q35_publisher_fixture_main;
    {
        auto run=std::make_unique<FixtureRun>();run->Start();run->GraphDone();
        // Once root has sent the ready release, hold one child back. Root
        // must not declare completion until the descendant consumes/forwards it.
        for (unsigned round=0;round<300 && !run->fabric->delayedOne;round++) run->Tick(true);
        assert(run->fabric->delayedOne && run->state[0]==DataflowResidentLifecyclePending);
        for (unsigned round=0;round<10;round++) run->Tick(true);
        assert(run->state[0]==DataflowResidentLifecyclePending);
        for (unsigned round=0;round<300 && run->state[0]!=DataflowResidentLifecycleGathered;round++) run->Tick();
        assert(run->state[0]==DataflowResidentLifecycleGathered && run->fabric->readyCount==DataflowCoreCount);
        for (auto state:run->state) assert(state==DataflowResidentLifecycleGathered);
        run->StartNext();run->GraphDone();
        for (unsigned round=0;round<300 && run->state[0]!=DataflowResidentLifecycleGathered;round++) run->Tick();
        assert(run->state[0]==DataflowResidentLifecycleGathered
            && run->fabric->readyCount==2*DataflowCoreCount && run->fabric->sends==2*(DataflowCoreCount-1));
        for (auto state:run->state) assert(state==DataflowResidentLifecycleGathered);
    }
    {
        auto run=std::make_unique<FixtureRun>(8);run->Start();run->GraphDone(119);
        for (unsigned round=0;round<12;round++) run->Tick();
        assert(run->state[0]==DataflowResidentLifecycleRetained && !run->fabric->readyCount);
    }
    {
        auto run=std::make_unique<FixtureRun>(30);run->fabric->resetFailure=37;run->Start();run->GraphDone();
        for (unsigned round=0;round<100;round++) run->Tick();
        assert(run->state[37]==DataflowResidentLifecycleRetained
            && run->state[0]==DataflowResidentLifecycleRetained && run->fabric->readyCount<DataflowCoreCount);
    }
    {
        // A NoC write can be delivered even though its sender reports failure.
        // The root's gathered candidate alone must not authorize clean reuse.
        auto run=std::make_unique<FixtureRun>();run->fabric->gatherFailureAfterDelivery=119;
        run->Start();run->GraphDone();
        for (unsigned round=0;round<300 && run->state[0]==DataflowResidentLifecyclePending;round++) run->Tick();
        assert(run->state[119]==DataflowResidentLifecycleRetained
            && run->state[0]==DataflowResidentLifecycleGathered);
    }
    puts("PASS resident 120-worker completion, readiness gather and retained failures; SIMULATED ONLY");
}
