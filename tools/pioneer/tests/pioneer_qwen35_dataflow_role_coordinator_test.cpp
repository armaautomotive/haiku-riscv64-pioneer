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
    unsigned audits=0;
    bool RunCompute() { return computeSuccess; }
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
int main()
{
    CheckQueueReset();
    CheckCoordinator(true,true,true);
    CheckCoordinator(false,true,true);
    CheckCoordinator(true,false,true);
    CheckCoordinator(true,true,false);
    DataflowRoleMailbox box{};
    box.magic=DataflowRoleMailboxMagic; box.version=DataflowRoleMailboxVersion;
    CoordinatorOps ops{};
    assert(DataflowRoleCoordinate(box,1,1,ops)==DataflowRoleCoordinatorRetainedFault);
    assert(box.command==1 && box.acknowledged==0 && !ops.audits);
    assert(!DataflowRoleMailboxRequestFaultStop(box,true,true,true));
    std::puts("PASS coordinator retains failed work and resets only clean completed queues; SIMULATED ONLY");
}
