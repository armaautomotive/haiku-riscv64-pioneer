/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowBarrierSetup.h"
#include <cassert>
#include <cstdio>

using namespace PioneerQwen35;
struct HeldMemory {
    uint32_t words[(DataflowBarrierArrivalEnd-DataflowBarrierOutbox)/4];
    unsigned writes=0,reads=0,failWrite=0,corruptRead=0,holdChecks=0,failHold=0;
    HeldMemory() { for (auto& word:words) word=0x12345678; }
    bool CoreHeld(unsigned core) { return core<DataflowBarrierWorkers && ++holdChecks!=failHold; }
    bool WriteCoreMemory(unsigned,unsigned address,const void* data,unsigned bytes) {
        assert(address>=DataflowBarrierOutbox && address+bytes<=DataflowBarrierArrivalEnd && bytes==64);
        if (++writes==failWrite) return false;
        const auto* source=static_cast<const uint32_t*>(data);
        for (unsigned i=0;i<16;i++) words[(address-DataflowBarrierOutbox)/4+i]=source[i];
        return true;
    }
    bool ReadCoreMemory(unsigned,unsigned address,void* data,unsigned bytes) {
        assert(address>=DataflowBarrierOutbox && address+bytes<=DataflowBarrierArrivalEnd && bytes==64);
        ++reads;auto* destination=static_cast<uint32_t*>(data);
        for (unsigned i=0;i<16;i++) destination[i]=words[(address-DataflowBarrierOutbox)/4+i];
        if (reads==corruptRead) destination[0]=1;
        return true;
    }
};

int main()
{
    for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
        HeldMemory io;
        assert(DataflowBarrierPrepareHeldCore(io,core));
        assert(io.writes==io.reads);
        for (unsigned i=0;i<32;i++)
            assert(io.words[(DataflowActivationShareReadyAddress-DataflowBarrierOutbox)/4+i]==0);
        assert(io.words[(DataflowActivationShareReadyAddress-DataflowBarrierOutbox)/4-1]==0x12345678);
        assert(io.words[(DataflowActivationShareScratchAddress-DataflowBarrierOutbox)/4+16]==0x12345678);
    }
    for (unsigned failure=1;failure<=5;failure++) {
        HeldMemory failedWrite;failedWrite.failWrite=failure;
        assert(!DataflowBarrierPrepareHeldCore(failedWrite,119));
        assert(failedWrite.writes==failure && failedWrite.reads==failure-1);
        HeldMemory failedRead;failedRead.corruptRead=failure;
        assert(!DataflowBarrierPrepareHeldCore(failedRead,119));
        assert(failedRead.writes==failure && failedRead.reads==failure);
    }
    HeldMemory lostHold;lostHold.failHold=11;
    assert(!DataflowBarrierPrepareHeldCore(lostHold,119));
    std::puts("PASS held fleet clears sharing records and retains startup failures; SIMULATED ONLY");
}
