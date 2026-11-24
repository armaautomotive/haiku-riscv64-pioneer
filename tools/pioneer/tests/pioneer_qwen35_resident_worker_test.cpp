/* SPDX-License-Identifier: MIT */
// Actual resident mailbox and queue transitions over local simulated memory.
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS 1
#define Q35_PUBLISHER_FIXTURE_ONLY
#include "pioneer_qwen35_graph_publication_test.cpp"
#include "../PioneerQwen35DataflowResidentServiceIO.h"

int main() {
    (void)&q35_publisher_fixture_main;
    auto f=std::make_unique<Fixture>();auto plan=std::make_unique<DataflowStreamPlan>();
    assert(DataflowStreamPlanBuilder::Build(f->packed,f->manifest,f->workspace,f->rope,f->routes,
        99,DataflowGraphDecodeSteps,*plan));
    DataflowDynamicTokenContext token{};assert(DataflowDynamicTokenContext::Build(f->manifest,12,1,token));
    DataflowGraphRunDescriptor descriptor{};
    assert(DataflowGraphRunBuild(plan->binding[0],plan->lease[0],f->routes,token,
        {6,DataflowGraphPromptSteps+1,DataflowGraphDecodeSteps},2,100,0,descriptor));
    DataflowResidentCommand command{};assert(DataflowResidentCommandBuild(descriptor,command));
    DataflowWorkerMailbox box{};
    box.magic=DataflowWorkerMailboxMagic;box.version=DataflowWorkerMailboxVersion;
    box.logicalCore=0;box.commandSequence=box.completedSequence=box.acknowledgedSequence=1;
    box.state=DataflowWorkerReady;
    DataflowRoleMailbox roles{};roles.magic=DataflowRoleMailboxMagic;roles.version=DataflowRoleMailboxVersion;
    roles.started[DataflowRoleReader]=DataflowRoleStartedReader;
    roles.started[DataflowRoleWriter]=DataflowRoleStartedWriter;
    DataflowQueueControl queues[DataflowCircularBufferCount]{};
    unsigned previous=99;
    auto begin=[&]() { return DataflowResidentMailboxBegin(box,roles,queues,0,command,descriptor,previous); };
    roles.started[0]=0;assert(!begin());roles.started[0]=DataflowRoleStartedReader;
    box.shutdownSequence=1;assert(!begin());box.shutdownSequence=0;
    box.acknowledgedSequence=0;assert(!begin());box.acknowledgedSequence=1;
    queues[0].produced=1;assert(!begin());queues[0].produced=0;
    auto wrong=descriptor;wrong.token.token++;wrong.checksum=DataflowGraphRunChecksum(wrong);
    assert(DataflowGraphRunValid(wrong,0)
        && !DataflowResidentMailboxBegin(box,roles,queues,0,command,wrong,previous));
    assert(begin() && previous==1 && box.commandSequence==2 && box.completedSequence==1
        && box.acknowledgedSequence==1 && box.state==DataflowWorkerRunning && box.phase==1);
    assert(!begin());
    DataflowWorkerPublish(box,2,DataflowWorkerComplete,0,2);
    const unsigned outputSequence=command.firstSequence+command.count-1;
    auto ready=[&]() { return DataflowResidentMailboxReady(box,roles,queues,0,2,previous,true,outputSequence); };
    DataflowArgmaxResult before{};
    assert(!DataflowArgmaxReadResult(box,outputSequence,before));
    DataflowResidentServiceIO serviceIO(0,box,roles,queues);
    assert(!serviceIO.PublishResidentCandidate(0,command)); // no valid result may be published
    const unsigned tokenResult=42,score=0x3f800000;
    box.reserved[0]=tokenResult;box.reserved[1]=score;box.reserved[2]=outputSequence;
    box.reserved[3]=DataflowArgmaxResultMagic;box.reserved[4]=~tokenResult;
    box.reserved[5]=DataflowArgmaxResultChecksum(tokenResult,score,outputSequence);
    queues[0].produced=3;queues[0].consumed=2;assert(!ready() && box.acknowledgedSequence==1);
    queues[0].consumed=3;queues[0].computeFailure=7;assert(!ready() && box.acknowledgedSequence==1);
    queues[0].computeFailure=0;roles.command=1;assert(!ready() && box.acknowledgedSequence==1);
    roles.command=0;box.failure=1;assert(!ready() && box.acknowledgedSequence==1);box.failure=0;
    assert(ready() && box.acknowledgedSequence==2 && box.completedSequence==2
        && box.commandSequence==2 && box.state==DataflowWorkerReady && !box.phase
        && !queues[0].produced && !queues[0].consumed);
    DataflowArgmaxResult result{};
    assert(DataflowArgmaxReadResult(box,outputSequence,result) && result.token==tokenResult);
    assert(!ready());
    DataflowWorkerClearReceipt(box);
    assert(!DataflowArgmaxReadResult(box,outputSequence,result));
    DataflowWorkerMailbox other{};other.magic=DataflowWorkerMailboxMagic;other.version=DataflowWorkerMailboxVersion;
    other.logicalCore=1;other.commandSequence=other.completedSequence=2;other.acknowledgedSequence=1;
    other.state=DataflowWorkerComplete;other.phase=2;other.reserved[0]=1;
    assert(!DataflowResidentMailboxReady(other,roles,queues,1,2,1,false,outputSequence));
    other.reserved[0]=0;
    assert(DataflowResidentMailboxReady(other,roles,queues,1,2,1,false,outputSequence)
        && other.state==DataflowWorkerReady && other.acknowledgedSequence==2);
    puts("PASS resident worker mailbox, role-idle and queue transitions; SIMULATED ONLY");
}
