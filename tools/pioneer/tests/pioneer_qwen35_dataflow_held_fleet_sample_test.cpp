/* SPDX-License-Identifier: MIT */
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35DataflowHeldFleetSample.h"
#include <cassert>
#include <cstdio>
using namespace PioneerQwen35;

struct FakeIO {
    unsigned runtimeCore=7,route=7,selections=0,samples=0;
    unsigned busyCore=DataflowBarrierWorkers,busyNoc=0,busyOffset=0;
    unsigned busyAfterSelections=0;
    unsigned wrongIdCore=DataflowBarrierWorkers,resetBadCore=DataflowBarrierWorkers;
    unsigned revokeCore=DataflowBarrierWorkers;
    unsigned fifoCore=DataflowBarrierWorkers,fifoNoc=0,clockCore=DataflowBarrierWorkers,clockNoc=0;
    uint32_t fifoAvailable=0x10101010U,fifoOverflow=0;
    unsigned configReads=0,flipConfigAt=0,flipClockAt=0,resetReads=0;
    int failSelection=-1;
    bool mapped=true,revoked=false,failRestore=false,revokeMappingOnPause=false;
    bool SelectRuntimeCore(unsigned core) {
        const unsigned attempt=selections++;
        if (int(attempt)==failSelection || (failRestore && attempt==240)) {
            route=UINT32_MAX; return false; // partially programmed route
        }
        runtimeCore=route=core;
        if (core==revokeCore) revoked=true;
        return true;
    }
    bool DataflowPanelMappingStable() const { return mapped; }
    uint32_t ResetRead() { ++resetReads; return runtimeCore==resetBadCore?0x6000:0x47800; }
    uint32_t ResidentNocRead(unsigned offset) {
        const unsigned noc=offset>=0x10000?1:0;
        const unsigned local=offset-(noc?0x10000:0);
        if (local==0x100) {
            ++configReads;
            uint32_t value=runtimeCore==fifoCore && noc==fifoNoc?1U<<16:0;
            if (runtimeCore==clockCore && noc==clockNoc) value|=1U<<12;
            if (configReads==flipConfigAt) value^=1U<<16;
            if (configReads==flipClockAt) value^=1U<<12;
            return value;
        }
        if (local==0x64) return fifoAvailable;
        if (local==0x68) return fifoOverflow;
        if (local==DataflowNocLogicalIdOffset) {
            samples++;
            return DataflowBarrierWorkerRoute(runtimeCore==wrongIdCore
                ?(runtimeCore+1)%DataflowBarrierWorkers:runtimeCore);
        }
        if (runtimeCore==busyCore && noc==busyNoc && local==busyOffset
            && selections>=busyAfterSelections) return 1;
        return 0;
    }
    void Pause() { if (revokeMappingOnPause) mapped=false; }
};
struct Scope {
    bool allowed=true;
    bool ProveRoleStatusInspectionAllowed(FakeIO& io) const { return allowed && !io.revoked; }
};
struct Guard {
    FakeIO& io;
    bool ProveSelectedRoute(unsigned core) const {
        return io.mapped && core==io.runtimeCore && core==io.route;
    }
};

int main()
{
    {
        FakeIO io; unsigned inspected=0;
        for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
            io.runtimeCore=core;
            assert(PioneerQwen35WithSelectedClock(io,core,[&]() {
                ++inspected; return io.ResetRead()==0x47800;
            }));
        }
        assert(inspected==DataflowBarrierWorkers && io.resetReads==DataflowBarrierWorkers);
    }
    for (unsigned noc=0;noc<2;noc++) {
        FakeIO io; unsigned touched=0;
        io.clockCore=7; io.clockNoc=noc;
        assert(!PioneerQwen35WithSelectedClock(io,7,[&]() { ++touched; return io.ResetRead()!=0; }));
        assert(!touched && !io.resetReads);
    }
    for (unsigned noc=0;noc<2;noc++) {
        FakeIO io; unsigned touched=0;
        io.flipClockAt=2+2*noc;
        assert(!PioneerQwen35WithSelectedClock(io,7,[&]() { ++touched; return true; }));
        assert(!touched && !io.resetReads);
    }
    {
        FakeIO io; unsigned touched=0;
        io.wrongIdCore=7;
        assert(!PioneerQwen35WithSelectedClock(io,7,[&]() { ++touched; return true; }));
        assert(!touched);
        io.wrongIdCore=DataflowBarrierWorkers; io.mapped=false;
        assert(!PioneerQwen35WithSelectedClock(io,7,[&]() { ++touched; return true; }));
        assert(!touched);
        io.mapped=true;
        assert(!PioneerQwen35WithSelectedClock(io,8,[&]() { ++touched; return true; }));
        assert(!touched);
        io.revokeMappingOnPause=true;
        assert(!PioneerQwen35WithSelectedClock(io,7,[&]() { ++touched; return true; }));
        assert(!touched);
    }
    {
        FakeIO io; unsigned touched=0;
        io.fifoCore=7; io.fifoNoc=0; io.fifoAvailable=0x10100f10U;
        // A clock check does not claim that the running NIU is idle.
        assert(PioneerQwen35WithSelectedClock(io,7,[&]() { ++touched; return true; }));
        assert(touched==1);
    }
    for (unsigned noc=0;noc<2;noc++) {
        FakeIO io; Scope scope; Guard guard{io};
        io.clockCore=7; io.clockNoc=noc;
        assert(!PioneerQwen35SampleSelectedHeldTile(io,scope,guard,7));
        assert(io.resetReads==0); // no RISC register access on a disabled tile
    }
    for (unsigned noc=0;noc<2;noc++) {
        FakeIO io; Scope scope; Guard guard{io};
        io.fifoCore=7; io.fifoNoc=noc;
        assert(PioneerQwen35SampleSelectedHeldTile(io,scope,guard,7));
        io.fifoAvailable=0x10100f10U;
        assert(!PioneerQwen35SampleSelectedHeldTile(io,scope,guard,7));
        io.fifoAvailable=0x10101010U; io.fifoOverflow=1;
        assert(!PioneerQwen35SampleSelectedHeldTile(io,scope,guard,7));
    }
    {
        FakeIO io; Scope scope; Guard guard{io};
        io.flipConfigAt=2;
        assert(!PioneerQwen35SampleSelectedHeldTile(io,scope,guard,7));
        assert(io.resetReads==0);
    }
    {
        FakeIO io; Scope scope; Guard guard{io}; unsigned failed=0; bool restore=true;
        assert(PioneerQwen35SampleHeldFleet(io,scope,guard,failed,restore));
        assert(!restore && failed==DataflowBarrierWorkers && io.runtimeCore==7 && io.route==7);
        assert(io.selections==241 && io.samples==2*DataflowBarrierWorkers*4);
    }
    for (unsigned noc=0;noc<2;noc++) for (unsigned i=0;i<36;i++) {
        FakeIO io; Scope scope; Guard guard{io}; unsigned failed=0; bool restore=false;
        io.busyCore=17; io.busyNoc=noc;
        io.busyOffset=i<4?0x40+i*0x800:0x240+(i-4)*4;
        assert(!PioneerQwen35SampleHeldFleet(io,scope,guard,failed,restore));
        assert(failed==17 && !restore && io.runtimeCore==7 && io.route==7);
    }
    for (unsigned kind=0;kind<3;kind++) {
        FakeIO io; Scope scope; Guard guard{io}; unsigned failed=0; bool restore=false;
        if (kind==0) io.wrongIdCore=23;
        if (kind==1) io.resetBadCore=23;
        if (kind==2) io.failSelection=23;
        assert(!PioneerQwen35SampleHeldFleet(io,scope,guard,failed,restore));
        if (kind==2)
            assert(failed==23 && restore && io.route==UINT32_MAX && io.selections==24);
        else
            assert(failed==23 && !restore && io.runtimeCore==7 && io.route==7);
    }
    {
        FakeIO io; Scope scope; Guard guard{io}; unsigned failed=0; bool restore=false;
        io.busyCore=17; io.busyNoc=1; io.busyOffset=0x240; io.busyAfterSelections=121;
        assert(!PioneerQwen35SampleHeldFleet(io,scope,guard,failed,restore));
        assert(failed==17 && !restore && io.runtimeCore==7 && io.route==7 && io.selections==139);
    }
    {
        FakeIO io; Scope scope; Guard guard{io}; unsigned failed=0; bool restore=false;
        io.failRestore=true;
        assert(!PioneerQwen35SampleHeldFleet(io,scope,guard,failed,restore));
        assert(restore && io.route==UINT32_MAX);
    }
    {
        FakeIO io; Scope scope; Guard guard{io}; unsigned failed=0; bool restore=false;
        io.revokeCore=4;
        assert(!PioneerQwen35SampleHeldFleet(io,scope,guard,failed,restore));
        assert(failed==4 && restore && io.runtimeCore==4 && io.selections==5);
    }
    {
        FakeIO io; Scope scope; Guard guard{io}; unsigned failed=0; bool restore=true;
        scope.allowed=false;
        assert(!PioneerQwen35SampleHeldFleet(io,scope,guard,failed,restore));
        assert(!restore && !io.selections && io.route==7);
    }
    std::puts("PASS held-fleet samples, routing faults and restoration; SIMULATED ONLY");
}
