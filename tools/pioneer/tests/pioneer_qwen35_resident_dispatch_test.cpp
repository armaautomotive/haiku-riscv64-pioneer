/* SPDX-License-Identifier: MIT */
// Real graph descriptors and fixed-tree transport over simulated mailboxes.
// This does not execute the card, start a graph or measure inference speed.
#define Q35_PUBLISHER_FIXTURE_ONLY
#include "pioneer_qwen35_graph_publication_test.cpp"
#include "../PioneerQwen35DataflowResidentDispatch.h"

struct Fabric {
    DataflowBarrierPacket payload[DataflowCoreCount]{},commit[DataflowCoreCount]{};
    unsigned sends=0;int failAt=-1;
    bool SendResidentDispatch(unsigned parent,unsigned child,const DataflowBarrierPacket& p,
        const DataflowBarrierPacket& c) {
        assert(DataflowBarrierReleaseEdge(parent,child));
        payload[child]=p;commit[child]=c; // even a failed send can be delivered
        return int(sends++)!=failAt;
    }
};
int main() {
    (void)&q35_publisher_fixture_main;
    auto f=std::make_unique<Fixture>();auto plan=std::make_unique<DataflowStreamPlan>();
    assert(DataflowStreamPlanBuilder::Build(f->packed,f->manifest,f->workspace,f->rope,f->routes,
        99,DataflowGraphDecodeSteps,*plan));
    DataflowDynamicTokenContext oldToken{},newToken{};
    assert(DataflowDynamicTokenContext::Build(f->manifest,11,0,oldToken)
        && DataflowDynamicTokenContext::Build(f->manifest,12,1,newToken));
    DataflowGraphRunDescriptor root{};
    assert(DataflowGraphRunBuild(plan->binding[0],plan->lease[0],f->routes,newToken,
        {6,DataflowGraphPromptSteps+1,DataflowGraphDecodeSteps},2,100,0,root));
    DataflowResidentCommand command{};assert(DataflowResidentCommandBuild(root,command));
    auto fabric=std::make_unique<Fabric>();
    // The actual fanout performs 119 parent/child deliveries, not 120 host
    // descriptor updates. Every worker expands the same compact command.
    for (unsigned core=0;core<DataflowCoreCount;core++) {
        DataflowResidentCommand received=command;
        if (core) assert(DataflowResidentCommandRead(fabric->payload[core].word,fabric->commit[core].word,
            1,5,received)==DataflowBarrierReady);
        DataflowGraphRunDescriptor installed{},expanded{},expected{};
        assert(DataflowGraphRunBuild(plan->binding[core],plan->lease[core],f->routes,oldToken,
            {5,1,DataflowGraphPromptSteps},1,100,0,installed));
        assert(DataflowResidentCommandExpand(received,installed,core,1,5,DataflowGraphPromptSteps,expanded));
        assert(DataflowGraphRunBuild(plan->binding[core],plan->lease[core],f->routes,newToken,
            {6,DataflowGraphPromptSteps+1,DataflowGraphDecodeSteps},2,100,0,expected));
        assert(!std::memcmp(&expanded,&expected,sizeof(expected)));
        DataflowResidentFanout fanout;assert(fanout.Send(core,received,*fabric) && fanout.Complete());
        assert(!fanout.Send(core,received,*fabric));
    }
    assert(fabric->sends==119);
    DataflowResidentCommand received{};
    assert(DataflowResidentCommandRead(fabric->payload[1].word,fabric->commit[1].word,2,6,received)==DataflowBarrierWaiting);
    auto badPayload=fabric->payload[1];badPayload.word[8]^=1;
    assert(DataflowResidentCommandRead(badPayload.word,fabric->commit[1].word,1,5,received)==DataflowBarrierInvalid);
    auto torn=fabric->commit[1];torn.word[7]^=1;
    assert(DataflowResidentCommandRead(fabric->payload[1].word,torn.word,1,5,received)==DataflowBarrierWaiting);
    DataflowGraphRunDescriptor expanded{};
    assert(!DataflowResidentCommandExpand(command,root,0,2,5,DataflowGraphPromptSteps,expanded)
        && !DataflowResidentCommandExpand(command,root,0,1,6,DataflowGraphPromptSteps,expanded)
        && !DataflowResidentCommandExpand(command,root,0,1,5,DataflowGraphPromptSteps+1,expanded)
        && !DataflowResidentCommandExpand(command,root,1,1,5,DataflowGraphPromptSteps,expanded));
    auto invalid=command;invalid.position=f->manifest.context;invalid.checksum=DataflowResidentCommandChecksum(invalid);
    assert(DataflowResidentCommandValid(invalid)
        && !DataflowResidentCommandExpand(invalid,root,0,1,5,DataflowGraphPromptSteps,expanded));
    invalid=command;invalid.reserved[0]=1;assert(!DataflowResidentCommandValid(invalid));
    auto failed=std::make_unique<Fabric>();failed->failAt=1;
    DataflowResidentFanout once;
    assert(!once.Send(0,command,*failed) && once.Attempted() && !once.Complete() && failed->sends==2);
    assert(!once.Send(0,command,*failed) && failed->sends==2);
    puts("PASS resident graph command expansion and fixed-tree fanout; SIMULATED ONLY");
}
