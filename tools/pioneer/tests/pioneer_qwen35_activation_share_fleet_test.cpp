/* SPDX-License-Identifier: MIT */
// Concurrent synthetic fleet: actual sharing receive and graph barrier code,
// simulated L1 and NoC. This is not a hardware or throughput qualification.
#include "../PioneerQwen35DataflowProjectionQualification.h"
#include "../PioneerQwen35DataflowDeviceBarrier.h"
#include "../PioneerQwen35DataflowActivationShareProtocol.h"
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

using namespace PioneerQwen35;
using Fixture=DataflowProjectionQualification;
struct BarrierSlot { DataflowBarrierPacket payload{},commit{}; };
struct Fleet {
    struct Core {
        DataflowActivationSharePacket ready{};
        std::vector<uint8_t> cache=std::vector<uint8_t>(DataflowActivationCacheBytes);
    };
    std::array<Core,DataflowCoreCount> cores;
    std::array<BarrierSlot,DataflowCoreCount> arrival{},release{};
    std::mutex mutex;
    std::atomic<unsigned> completed{0},reads{0},arrived[3]{};
};
struct BarrierIO {
    Fleet& fleet;unsigned core;
    bool SendArrival(unsigned worker,const DataflowBarrierPacket& payload,const DataflowBarrierPacket& commit) {
        assert(worker==core && core);
        std::lock_guard<std::mutex> lock(fleet.mutex);fleet.arrival[core]={payload,commit};return true;
    }
    bool SendRelease(unsigned worker,const DataflowBarrierPacket& payload,const DataflowBarrierPacket& commit) {
        assert(DataflowBarrierReleaseEdge(core,worker));
        std::lock_guard<std::mutex> lock(fleet.mutex);fleet.release[worker]={payload,commit};return true;
    }
    DataflowBarrierReadResult ReadArrival(unsigned worker,unsigned epoch,unsigned step,DataflowBarrierPacket& out) {
        assert(DataflowBarrierReleaseEdge(core,worker));std::lock_guard<std::mutex> lock(fleet.mutex);
        auto& slot=fleet.arrival[worker];return DataflowBarrierReadCommitted(slot.payload.word,slot.commit.word,epoch,step,out);
    }
    DataflowBarrierReadResult ReadRelease(unsigned worker,unsigned epoch,unsigned step,DataflowBarrierPacket& out) {
        assert(worker==core && core);std::lock_guard<std::mutex> lock(fleet.mutex);
        auto& slot=fleet.release[worker];return DataflowBarrierReadCommitted(slot.payload.word,slot.commit.word,epoch,step,out);
    }
};
struct ShareIO {
    Fleet& fleet;const DataflowCoreInvocation& program;std::vector<uint8_t>& local;
    unsigned epoch,step;DataflowActivationSharePacket scratch{};
    bool ContextMatches(unsigned e,unsigned s) const { return e==epoch && s==step; }
    bool Read(unsigned parent,unsigned remote,unsigned destination,unsigned bytes) {
        if (!DataflowActivationSharePeerReadAllowed(program,parent,DataflowBarrierWorkerRoute(parent),
            remote,destination,bytes)) return false;
        {
            std::lock_guard<std::mutex> lock(fleet.mutex);
            const auto& source=fleet.cores[parent];
            if (remote==DataflowActivationShareReadyAddress) scratch=source.ready;
            else {
                const unsigned offset=remote-DataflowActivationCacheAddress;
                assert(offset+bytes<=local.size());
                std::memcpy(local.data()+offset,source.cache.data()+offset,bytes);
            }
            ++fleet.reads;
        }
        if (remote==DataflowActivationShareReadyAddress)
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        return true;
    }
    uint32_t ScratchWord(unsigned i) const { return scratch.word[i]; }
};
static uint8_t Expected(unsigned offset,unsigned epoch,unsigned step,unsigned leader) {
    return uint8_t(offset*17U+epoch*53U+step*31U+leader*97U);
}
static void Verify(const std::vector<uint8_t>& cache,unsigned start,unsigned bytes,unsigned epoch,unsigned step,unsigned leader) {
    for (unsigned i=0;i<bytes;i++) assert(cache[start+i]==Expected(start+i,epoch,step,leader));
}

static void Run(bool mixed)
{
    (void)&PioneerQ38ProjectionQuantize;
    unsigned routes[2][Banks]{};
    for (unsigned n=0;n<2;n++) for (unsigned b=0;b<Banks;b++) routes[n][b]=b+1;
    auto program=std::make_unique<DataflowProjectionProgram>();
    auto schedule=std::make_unique<PioneerQ35ProjectionProgram>();
    assert(Fixture::Build(0,BankBase,Fixture::ScratchBytes,routes,*program,*schedule));
    auto fleet=std::make_unique<Fleet>();
    auto commands=std::make_unique<DataflowCoreInvocation[]>(DataflowCoreCount);
    for (unsigned core=0;core<DataflowCoreCount;core++) {
        auto& command=commands[core];assert(DataflowProgramBuilder::BindCore(*program,core,command));
        if (mixed && (core&1)) command.input.parts[0].address+=0x100000;
        for (unsigned i=0;i<DataflowCoreCount;i++)
            if (!mixed || ((i^core)&1U)==0) command.sharing.members[i/32]|=uint32_t(1)<<(i%32);
        command.sharing.leader=mixed?(core&1U):0;command.sharing.packing=12;
        command.sharing.inputKey=DataflowActivationShareInputKey(command);
        assert(DataflowCoreHeaderValid(command));
    }
    const unsigned packed=(Fixture::Inputs/256)*DataflowInputPageBytes;
    const unsigned sums=(Fixture::Inputs/256)*DataflowActivationSumsPerChunk*sizeof(int32_t);
    std::vector<std::thread> threads;
    for (unsigned core=0;core<DataflowCoreCount;core++) threads.emplace_back([&,core] {
        auto& command=commands[core];std::vector<uint8_t> local(DataflowActivationCacheBytes);
        BarrierIO barrierIO{*fleet,core};DataflowDeviceBarrier<BarrierIO> barrier(barrierIO);
        const unsigned epochs[]={1,1,2},steps[]={1,2,1};
        for (unsigned run=0;run<3;run++) {
            if (run) assert(fleet->arrived[run-1].load()==DataflowCoreCount);
            const unsigned epoch=epochs[run],step=steps[run];
            const unsigned leader=command.sharing.leader;
            if (core==leader) {
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
                for (unsigned i=0;i<packed;i++) local[i]=Expected(i,epoch,step,leader);
                for (unsigned i=0;i<sums;i++) local[DataflowActivationSumsOffset+i]=Expected(DataflowActivationSumsOffset+i,epoch,step,leader);
            } else {
                ShareIO io{*fleet,command,local,epoch,step};
                assert(DataflowActivationShareReceive(command,epoch,step,io,100000));
            }
            Verify(local,0,packed,epoch,step,leader);Verify(local,DataflowActivationSumsOffset,sums,epoch,step,leader);
            DataflowActivationSharePacket packet{};
            assert(DataflowActivationSharePacketBuild(command,epoch,step,core,packet));
            {
                std::lock_guard<std::mutex> lock(fleet->mutex);auto& target=fleet->cores[core];
                std::memcpy(target.cache.data(),local.data(),packed);
            }
            std::this_thread::yield();
            {
                std::lock_guard<std::mutex> lock(fleet->mutex);auto& target=fleet->cores[core];
                std::memcpy(target.cache.data()+DataflowActivationSumsOffset,local.data()+DataflowActivationSumsOffset,sums);
            }
            if ((core%11)==0) std::this_thread::sleep_for(std::chrono::microseconds(50));
            {
                std::lock_guard<std::mutex> lock(fleet->mutex);auto& target=fleet->cores[core];
                DataflowActivationSharePublish(target.ready.word,packet);
            }
            ++fleet->arrived[run];
            // Delayed branches force earlier branches to wait for fleet arrival.
            if (core==119 || core==37) std::this_thread::sleep_for(std::chrono::milliseconds(2));
            assert(barrier.Start(core,epoch,step,0,0));
            for (;;) {
                const auto result=barrier.Poll();
                if (result==DataflowBarrierPassed) break;
                assert(result==DataflowBarrierPending);
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
            assert(barrier.NextStep());
        }
        ++fleet->completed;
    });
    for (auto& thread:threads) thread.join();
    assert(fleet->completed==DataflowCoreCount && fleet->reads>=3*(DataflowCoreCount-(mixed?2:1))*3);
}
int main()
{
    Run(false);Run(true);
    std::puts("PASS 120-core sharing, sparse groups, tree barriers and new epochs; SIMULATED ONLY");
}
