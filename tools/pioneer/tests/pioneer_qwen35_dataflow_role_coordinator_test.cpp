/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowRoleCoordinator.h"
#include <atomic>
#include <cassert>
#include <cstdio>
#include <thread>
using namespace PioneerQwen35;
struct Body { bool success=true; bool Run() { return success; } };
struct CoordinatorOps {
    DataflowQueueControl controls[DataflowCircularBufferCount]{};
    bool computeSuccess=true;
    unsigned audits=0,computes=0;
    bool RunCompute() { ++computes; return computeSuccess; }
    void Pause() { std::this_thread::yield(); }
    bool AuditAndResetCleanQueues() {
        ++audits; return DataflowRoleCleanQueuesForReuse(controls);
    }
};
static void CheckQueueReset()
{
    DataflowQueueControl c[DataflowCircularBufferCount]{};
    for (auto& q:c) q.produced=q.consumed=7;
    c[2].writerFailure=DataflowFailureWriter;
    assert(!DataflowRoleCleanQueuesForReuse(c));
    for (const auto& q:c) assert(q.produced==7 && q.consumed==7);
    c[2].writerFailure=0; c[1].consumed=6;
    assert(!DataflowRoleCleanQueuesForReuse(c));
    assert(c[0].produced==7 && c[2].produced==7);
    c[1].consumed=7; c[0].reserved[0]=1;
    assert(!DataflowRoleCleanQueuesForReuse(c));
    c[0].reserved[0]=0;
    assert(DataflowRoleCleanQueuesForReuse(c));
    for (const auto& q:c) assert(!q.produced && !q.consumed);

    DataflowRoleMailbox box{};
    box.magic=DataflowRoleMailboxMagic; box.version=DataflowRoleMailboxVersion;
    assert(DataflowRoleMailboxPublishStarted(box,DataflowRoleReader));
    assert(DataflowRoleMailboxPublishStarted(box,DataflowRoleWriter));
    assert(DataflowRoleMailboxIssue(box,1));
    assert(DataflowRoleMailboxBegin(box,DataflowRoleReader,1));
    assert(DataflowRoleMailboxBegin(box,DataflowRoleWriter,1));
    assert(DataflowRoleMailboxComplete(box,DataflowRoleReader,1,false));
    c[0].produced=1; c[0].readerFailure=DataflowFailureReader;
    assert(!DataflowRoleDiscardFailedQueues(c,box,true)); // writer still active
    assert(c[0].produced==1 && c[0].readerFailure==DataflowFailureReader);
    assert(DataflowRoleMailboxComplete(box,DataflowRoleWriter,1,true));
    assert(!DataflowRoleDiscardFailedQueues(c,box,false)); // host audit absent
    c[1].produced=DataflowCircularBufferPages+1;
    assert(!DataflowRoleDiscardFailedQueues(c,box,true));
    assert(c[0].produced==1 && c[1].produced==DataflowCircularBufferPages+1);
    c[1].produced=DataflowCircularBufferPages; c[2].reserved[1]=1;
    assert(!DataflowRoleDiscardFailedQueues(c,box,true));
    c[2].reserved[1]=0;
    assert(DataflowRoleDiscardFailedQueues(c,box,true));
    for (const auto& q:c)
        assert(!q.produced && !q.consumed && !DataflowQueueFailure(q));
    assert(box.command==1 && box.acknowledged==0); // graph remains failed
}
static void CheckCoordinator(bool computeOkay,bool readerOkay,bool queueOkay)
{
    DataflowRoleMailbox box{};
    box.magic=DataflowRoleMailboxMagic; box.version=DataflowRoleMailboxVersion;
    CoordinatorOps ops{}; ops.computeSuccess=computeOkay;
    for (auto& q:ops.controls) q.produced=q.consumed=3;
    if (!queueOkay) ops.controls[1].queueFailure=DataflowFailureQueue;
    std::atomic<bool> bad{false};
    auto roleLoop=[&](unsigned role) {
        DataflowRoleTickState state{}; Body body{role!=DataflowRoleReader || readerOkay};
        for (unsigned n=0;n<10000000 && !bad.load();n++) {
            const auto result=DataflowRoleServiceTick(box,role,state,body);
            if (result==DataflowRoleTickStopRequested) return;
            if (result==DataflowRoleTickInvalid) { bad.store(true); return; }
            if ((n&255)==0) std::this_thread::yield();
        }
        bad.store(true);
    };
    std::thread reader(roleLoop,DataflowRoleReader),writer(roleLoop,DataflowRoleWriter);
    for (unsigned n=0;n<10000000 && !DataflowRoleMailboxAllStarted(box) && !bad.load();n++)
        if ((n&255)==0) std::this_thread::yield();
    assert(DataflowRoleMailboxAllStarted(box));
    const auto result=DataflowRoleCoordinate(box,1,DataflowQueuePollLimit,ops);
    const bool success=computeOkay && readerOkay && queueOkay;
    if (success) {
        assert(result==DataflowRoleCoordinatorComplete && box.acknowledged==1);
        assert(ops.audits==1 && !ops.controls[0].produced);
        assert(DataflowRoleMailboxRequestStop(box));
    } else {
        assert(result==DataflowRoleCoordinatorRetainedFault && box.acknowledged==0);
        assert(!ops.audits || (computeOkay && readerOkay && !queueOkay));
        assert(DataflowRoleMailboxRequestFaultStop(box,true,true,true));
    }
    reader.join(); writer.join();
    assert(!bad.load() && DataflowRoleMailboxStopReceipts(box));
}
static void CheckInterleavedGraphSteps()
{
    DataflowRoleMailbox box{};
    box.magic=DataflowRoleMailboxMagic; box.version=DataflowRoleMailboxVersion;
    CoordinatorOps ops{};
    std::atomic<bool> bad{false};
    auto roleLoop=[&](unsigned role) {
        DataflowRoleTickState state{}; Body body{};
        for (unsigned n=0;n<10000000 && !bad.load();n++) {
            const auto result=DataflowRoleServiceTick(box,role,state,body);
            if (result==DataflowRoleTickStopRequested) return;
            if (result==DataflowRoleTickInvalid) { bad.store(true); return; }
            if ((n&255)==0) std::this_thread::yield();
        }
        bad.store(true);
    };
    std::thread reader(roleLoop,DataflowRoleReader),writer(roleLoop,DataflowRoleWriter);
    for (unsigned n=0;n<10000000 && !DataflowRoleMailboxAllStarted(box) && !bad.load();n++)
        if ((n&255)==0) std::this_thread::yield();
    assert(DataflowRoleMailboxAllStarted(box));
    const bool projection[]={false,true,false,false,true,false,true};
    unsigned completed=0;
    for (bool isProjection:projection) {
        if (!isProjection) { assert(box.acknowledged==completed); continue; }
        uint32_t sequence=0;
        assert(DataflowRoleNextProjectionSequence(box,sequence) && sequence==completed+1);
        assert(DataflowRoleCoordinate(box,sequence,DataflowQueuePollLimit,ops)
            ==DataflowRoleCoordinatorComplete);
        assert(box.acknowledged==++completed);
    }
    assert(DataflowRoleMailboxRequestStop(box));
    uint32_t unchanged=99;
    assert(!DataflowRoleNextProjectionSequence(box,unchanged) && unchanged==99);
    reader.join(); writer.join();
    assert(!bad.load() && DataflowRoleMailboxStopReceipts(box));
    DataflowRoleMailbox pending{};
    pending.magic=DataflowRoleMailboxMagic; pending.version=DataflowRoleMailboxVersion;
    pending.command=1;
    assert(!DataflowRoleNextProjectionSequence(pending,unchanged) && unchanged==99);
    pending.command=pending.acknowledged=DataflowWorkerSequenceLimit;
    assert(!DataflowRoleNextProjectionSequence(pending,unchanged) && unchanged==99);
}
int main()
{
    CheckQueueReset();
    CheckInterleavedGraphSteps();
    CheckCoordinator(true,true,true);
    CheckCoordinator(false,true,true);
    CheckCoordinator(true,false,true);
    CheckCoordinator(true,true,false);
    DataflowRoleMailbox box{};
    box.magic=DataflowRoleMailboxMagic; box.version=DataflowRoleMailboxVersion;
    CoordinatorOps ops{};
    assert(DataflowRoleCoordinate(box,1,1,ops)==DataflowRoleCoordinatorInvalid);
    assert(!ops.computes && !box.command);
    assert(DataflowRoleMailboxPublishStarted(box,DataflowRoleReader));
    assert(DataflowRoleCoordinate(box,1,1,ops)==DataflowRoleCoordinatorInvalid);
    assert(!ops.computes && !box.command);
    assert(DataflowRoleMailboxPublishStarted(box,DataflowRoleWriter));
    assert(DataflowRoleCoordinate(box,1,1,ops)==DataflowRoleCoordinatorRetainedFault);
    assert(box.command==1 && box.acknowledged==0 && !ops.audits);
    assert(!DataflowRoleMailboxRequestFaultStop(box,true,true,true));
    std::puts("PASS coordinator interleaved projections, retained faults and clean queue reuse; SIMULATED ONLY");
}
