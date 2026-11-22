/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowRoleServiceTick.h"
#include <cassert>
#include <atomic>
#include <cstdio>
#include <thread>
using namespace PioneerQwen35;
struct TestBody { unsigned calls=0; bool result=true; bool Run() { ++calls; return result; } };
static void CheckConcurrentHandoffs()
{
    DataflowRoleMailbox box{};
    box.magic=DataflowRoleMailboxMagic; box.version=DataflowRoleMailboxVersion;
    std::atomic<bool> bad{false};
    auto roleLoop=[&](unsigned role) {
        DataflowRoleTickState state{}; TestBody body{};
        for (unsigned n=0;n<10000000 && !bad.load();n++) {
            const auto result=DataflowRoleServiceTick(box,role,state,body);
            if (result==DataflowRoleTickStopRequested) return;
            if (result==DataflowRoleTickInvalid || result==DataflowRoleTickFault
                || result==DataflowRoleTickWaitingRecovery) { bad.store(true); return; }
            if ((n&255)==0) std::this_thread::yield();
        }
        bad.store(true);
    };
    std::thread reader(roleLoop,DataflowRoleReader),writer(roleLoop,DataflowRoleWriter);
    for (unsigned n=0;n<10000000 && !DataflowRoleMailboxAllStarted(box) && !bad.load();n++)
        if ((n&255)==0) std::this_thread::yield();
    if (!DataflowRoleMailboxAllStarted(box)) bad.store(true);
    for (uint32_t sequence=1;sequence<=20 && !bad.load();sequence++) {
        if (!DataflowRoleMailboxIssue(box,sequence)) { bad.store(true); break; }
        bool acknowledged=false;
        for (unsigned n=0;n<10000000 && !bad.load();n++) {
            if (DataflowRoleMailboxAcknowledge(box,sequence,true,true)) {
                acknowledged=true; break;
            }
            if ((n&255)==0) std::this_thread::yield();
        }
        if (!acknowledged) { bad.store(true); break; }
    }
    if (!bad.load() && !DataflowRoleMailboxRequestStop(box)) bad.store(true);
    reader.join(); writer.join();
    assert(!bad.load() && DataflowRoleMailboxStopReceipts(box));
}
int main()
{
    DataflowRoleMailbox box{};
    assert(!DataflowRoleMailboxIssue(box,1));
    box.magic=DataflowRoleMailboxMagic; box.version=DataflowRoleMailboxVersion;
    box.reserved[2]=1;
    assert(!DataflowRoleMailboxIssue(box,1));
    box.reserved[2]=0;
    assert(!DataflowRoleMailboxIssue(box,1));
    assert(!DataflowRoleMailboxStartupReady(box));
    assert(DataflowRoleMailboxPublishStarted(box,DataflowRoleReader));
    assert(!DataflowRoleMailboxIssue(box,1));
    assert(!DataflowRoleMailboxStartupReady(box));
    assert(!DataflowRoleMailboxPublishStarted(box,DataflowRoleReader));
    assert(DataflowRoleMailboxPublishStarted(box,DataflowRoleWriter));
    assert(DataflowRoleMailboxAllStarted(box));
    assert(DataflowRoleMailboxStartupReady(box));
    box.started[DataflowRoleWriter]=DataflowRoleStartedReader;
    assert(!DataflowRoleMailboxAllStarted(box) && !DataflowRoleMailboxStartupReady(box));
    assert(!DataflowRoleMailboxIssue(box,1));
    box.started[DataflowRoleWriter]=DataflowRoleStartedWriter;
    box.stopped[0]=1;
    assert(!DataflowRoleMailboxIssue(box,1));
    box.stopped[0]=0;
    assert(!DataflowRoleMailboxIssue(box,0));
    assert(!DataflowRoleMailboxIssue(box,2));
    assert(DataflowRoleMailboxIssue(box,1));
    assert(!DataflowRoleMailboxRequestStop(box));
    assert(!DataflowRoleMailboxIssue(box,1));
    assert(!DataflowRoleMailboxBegin(box,2,1));
    assert(!DataflowRoleMailboxBegin(box,DataflowRoleReader,2));
    assert(DataflowRoleMailboxBegin(box,DataflowRoleReader,1));
    assert(!DataflowRoleMailboxBegin(box,DataflowRoleReader,1));
    assert(!DataflowRoleMailboxAcknowledge(box,1,true,true));
    assert(DataflowRoleMailboxBegin(box,DataflowRoleWriter,1));
    assert(DataflowRoleMailboxComplete(box,DataflowRoleReader,1,true));
    assert(!DataflowRoleMailboxComplete(box,DataflowRoleReader,1,true));
    assert(DataflowRoleMailboxComplete(box,DataflowRoleWriter,1,true));
    assert(!DataflowRoleMailboxAcknowledge(box,1,false,true));
    assert(!DataflowRoleMailboxAcknowledge(box,1,true,false));
    assert(DataflowRoleMailboxAcknowledge(box,1,true,true));
    assert(!DataflowRoleMailboxAcknowledge(box,1,true,true));
    assert(DataflowRoleMailboxIssue(box,2));
    assert(DataflowRoleMailboxBegin(box,DataflowRoleReader,2));
    assert(DataflowRoleMailboxBegin(box,DataflowRoleWriter,2));
    assert(DataflowRoleMailboxComplete(box,DataflowRoleReader,2,false));
    assert(!DataflowRoleMailboxRequestFaultStop(box,true,true,false));
    assert(DataflowRoleMailboxComplete(box,DataflowRoleWriter,2,true));
    assert(!DataflowRoleMailboxAcknowledge(box,2,true,true));
    assert(!DataflowRoleMailboxIssue(box,3));
    assert(!DataflowRoleMailboxRequestStop(box));
    assert(!DataflowRoleMailboxRequestFaultStop(box,false,true,false));
    assert(!DataflowRoleMailboxRequestFaultStop(box,true,false,false));
    assert(DataflowRoleMailboxRequestFaultStop(box,true,true,false));
    assert(DataflowRoleMailboxObserveStop(box,DataflowRoleReader));
    assert(DataflowRoleMailboxObserveStop(box,DataflowRoleWriter));
    assert(DataflowRoleMailboxStopReceipts(box));
    assert(box.acknowledged==1 && box.command==2);
    DataflowRoleMailbox computeFault{};
    computeFault.magic=DataflowRoleMailboxMagic; computeFault.version=DataflowRoleMailboxVersion;
    for (unsigned role=0;role<DataflowRoleCount;role++) assert(DataflowRoleMailboxPublishStarted(computeFault,role));
    assert(DataflowRoleMailboxIssue(computeFault,1));
    for (unsigned role=0;role<DataflowRoleCount;role++) {
        assert(DataflowRoleMailboxBegin(computeFault,role,1));
        assert(DataflowRoleMailboxComplete(computeFault,role,1,true));
    }
    assert(!DataflowRoleMailboxRequestFaultStop(computeFault,true,true,false));
    assert(DataflowRoleMailboxRequestFaultStop(computeFault,true,true,true));
    assert(!DataflowRoleMailboxAcknowledge(computeFault,1,true,true));
    assert(computeFault.acknowledged==0 && computeFault.command==1);
    DataflowRoleMailbox stopped{};
    stopped.magic=DataflowRoleMailboxMagic; stopped.version=DataflowRoleMailboxVersion;
    for (unsigned role=0;role<DataflowRoleCount;role++) assert(DataflowRoleMailboxPublishStarted(stopped,role));
    assert(DataflowRoleMailboxRequestStop(stopped));
    assert(!DataflowRoleMailboxRequestStop(stopped));
    assert(!DataflowRoleMailboxIssue(stopped,1));
    assert(!DataflowRoleMailboxStopReceipts(stopped));
    assert(DataflowRoleMailboxObserveStop(stopped,DataflowRoleReader));
    assert(!DataflowRoleMailboxObserveStop(stopped,DataflowRoleReader));
    assert(!DataflowRoleMailboxStopReceipts(stopped));
    assert(DataflowRoleMailboxObserveStop(stopped,DataflowRoleWriter));
    assert(DataflowRoleMailboxStopReceipts(stopped));
    DataflowRoleMailbox commandBox{};
    commandBox.magic=DataflowRoleMailboxMagic; commandBox.version=DataflowRoleMailboxVersion;
    for (unsigned role=0;role<DataflowRoleCount;role++) assert(DataflowRoleMailboxPublishStarted(commandBox,role));
    TestBody reader{},writer{}; writer.result=false;
    assert(DataflowRoleRunCommand(commandBox,DataflowRoleReader,1,reader)==DataflowRoleCommandInvalid);
    assert(reader.calls==0 && DataflowRoleMailboxIssue(commandBox,1));
    assert(DataflowRoleRunCommand(commandBox,DataflowRoleReader,1,reader)==DataflowRoleCommandComplete);
    assert(DataflowRoleRunCommand(commandBox,DataflowRoleWriter,1,writer)==DataflowRoleCommandBodyFailed);
    assert(reader.calls==1 && writer.calls==1);
    assert(!DataflowRoleMailboxAcknowledge(commandBox,1,true,true));
    DataflowRoleMailbox ticks{};
    ticks.magic=DataflowRoleMailboxMagic; ticks.version=DataflowRoleMailboxVersion;
    DataflowRoleTickState readerState{},writerState{};
    TestBody tickReader{},tickWriter{};
    assert(DataflowRoleServiceTick(ticks,DataflowRoleReader,readerState,tickReader)==DataflowRoleTickIdle);
    assert(!DataflowRoleMailboxIssue(ticks,1));
    assert(DataflowRoleServiceTick(ticks,DataflowRoleWriter,writerState,tickWriter)==DataflowRoleTickIdle);
    assert(DataflowRoleMailboxIssue(ticks,1));
    assert(DataflowRoleServiceTick(ticks,DataflowRoleReader,readerState,tickReader)==DataflowRoleTickComplete);
    assert(DataflowRoleServiceTick(ticks,DataflowRoleReader,readerState,tickReader)==DataflowRoleTickWaitingAck);
    assert(tickReader.calls==1);
    assert(DataflowRoleServiceTick(ticks,DataflowRoleWriter,writerState,tickWriter)==DataflowRoleTickComplete);
    assert(DataflowRoleMailboxAcknowledge(ticks,1,true,true));
    assert(DataflowRoleServiceTick(ticks,DataflowRoleReader,readerState,tickReader)==DataflowRoleTickIdle);
    assert(DataflowRoleMailboxRequestStop(ticks));
    assert(DataflowRoleServiceTick(ticks,DataflowRoleReader,readerState,tickReader)==DataflowRoleTickStopRequested);
    assert(DataflowRoleServiceTick(ticks,DataflowRoleReader,readerState,tickReader)==DataflowRoleTickStopRequested);
    assert(DataflowRoleServiceTick(ticks,DataflowRoleWriter,writerState,tickWriter)==DataflowRoleTickStopRequested);
    assert(DataflowRoleMailboxStopReceipts(ticks));
    DataflowRoleMailbox faultTicks{};
    faultTicks.magic=DataflowRoleMailboxMagic; faultTicks.version=DataflowRoleMailboxVersion;
    DataflowRoleTickState faultReaderState{},faultWriterState{};
    TestBody faultReader{},faultWriter{}; faultReader.result=false;
    assert(DataflowRoleServiceTick(faultTicks,DataflowRoleReader,faultReaderState,faultReader)==DataflowRoleTickIdle);
    assert(DataflowRoleServiceTick(faultTicks,DataflowRoleWriter,faultWriterState,faultWriter)==DataflowRoleTickIdle);
    assert(DataflowRoleMailboxIssue(faultTicks,1));
    assert(DataflowRoleServiceTick(faultTicks,DataflowRoleReader,faultReaderState,faultReader)==DataflowRoleTickFault);
    assert(DataflowRoleServiceTick(faultTicks,DataflowRoleReader,faultReaderState,faultReader)==DataflowRoleTickWaitingRecovery);
    assert(faultReader.calls==1);
    assert(DataflowRoleServiceTick(faultTicks,DataflowRoleWriter,faultWriterState,faultWriter)==DataflowRoleTickComplete);
    assert(DataflowRoleMailboxRequestFaultStop(faultTicks,true,true,false));
    assert(DataflowRoleServiceTick(faultTicks,DataflowRoleReader,faultReaderState,faultReader)==DataflowRoleTickStopRequested);
    assert(DataflowRoleServiceTick(faultTicks,DataflowRoleWriter,faultWriterState,faultWriter)==DataflowRoleTickStopRequested);
    assert(DataflowRoleMailboxStopReceipts(faultTicks) && faultTicks.acknowledged==0);
    DataflowRoleMailbox wrap{};
    wrap.magic=DataflowRoleMailboxMagic; wrap.version=DataflowRoleMailboxVersion;
    wrap.command=wrap.acknowledged=DataflowWorkerSequenceLimit;
    for (unsigned role=0;role<DataflowRoleCount;role++) {
        wrap.completed[role]=DataflowWorkerSequenceLimit;
        wrap.status[role]=DataflowRoleComplete;
    }
    assert(!DataflowRoleMailboxIssue(wrap,DataflowWorkerSequenceLimit+1));
    CheckConcurrentHandoffs();
    std::puts("PASS role handoff preserves command, role receipts and unacknowledged failure; SIMULATED ONLY");
}
