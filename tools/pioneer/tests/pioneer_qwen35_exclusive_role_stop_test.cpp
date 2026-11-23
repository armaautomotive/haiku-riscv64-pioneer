/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35ExclusiveRoleStop.h"
#include <cassert>
#include <cstdio>
using namespace PioneerQwen35;

struct FakeStopIO {
    bool held[DataflowBarrierWorkers]{},drained=true;
    bool roleIssued[DataflowBarrierWorkers]{},roleVisible[DataflowBarrierWorkers]{};
    bool computeIssued[DataflowBarrierWorkers]{},computeVisible[DataflowBarrierWorkers]{};
    unsigned roleWrites=0,computeWrites=0,holdWrites=0;
    unsigned loseRole=UINT32_MAX,loseCompute=UINT32_MAX,loseHold=UINT32_MAX;
    FakeStopIO() { for (bool& value:held) value=true; }
    bool UnconfirmedReleaseHeld(unsigned core) { return drained && held[core]; }
    bool UnconfirmedReleaseRunning(unsigned core) { return drained && !held[core]; }
    bool RequestStop(unsigned core,bool,bool& attempted,uint32_t& token) {
        assert(!attempted && !roleIssued[core]);attempted=roleIssued[core]=true;
        token=1;++roleWrites;
        if (core==loseRole) return false;
        roleVisible[core]=true;return true;
    }
    bool StopRequestObserved(unsigned core,uint32_t token) { return token==1 && roleVisible[core]; }
    bool RolesStoppedAndParked(unsigned core,uint32_t token,uint32_t,uint32_t) {
        return token==1 && roleVisible[core];
    }
    bool RequestComputeStop(unsigned core,bool& attempted) {
        assert(!attempted && !computeIssued[core]);attempted=computeIssued[core]=true;++computeWrites;
        if (core==loseCompute) return false;
        computeVisible[core]=true;return true;
    }
    bool ComputeStopObserved(unsigned core) { return computeVisible[core]; }
    bool ComputeStoppedAndParked(unsigned core,uint32_t) { return computeVisible[core]; }
    bool HoldCore(unsigned core,bool& attempted) {
        assert(!attempted);attempted=true;++holdWrites;
        if (core==loseHold) return false;
        held[core]=true;return true;
    }
    bool HoldObserved(unsigned core) { return held[core]; }
    bool AllRolesHeld(unsigned core) { return held[core]; }
    void Pause() {}
};
static bool Run(DataflowExclusiveRoleStop& owner,const bool* released,const bool* confirmed,
    FakeStopIO& io,unsigned& stage,unsigned& core) {
    return owner.StopAndHold(released,confirmed,0x70040,0x70140,0x72040,3,io,stage,core);
}
int main() {
    unsigned stage=0,core=0;bool released[DataflowBarrierWorkers]{},confirmed[DataflowBarrierWorkers]{};
    {
        FakeStopIO io;DataflowExclusiveRoleStop owner;
        for (unsigned i=0;i<DataflowBarrierWorkers;i++) released[i]=confirmed[i]=true,io.held[i]=false;
        assert(Run(owner,released,confirmed,io,stage,core) && !stage && core==DataflowBarrierWorkers);
        assert(io.roleWrites==DataflowBarrierWorkers && io.computeWrites==DataflowBarrierWorkers
            && io.holdWrites==DataflowBarrierWorkers);
        assert(Run(owner,released,confirmed,io,stage,core));
        assert(io.roleWrites==DataflowBarrierWorkers && io.computeWrites==DataflowBarrierWorkers
            && io.holdWrites==DataflowBarrierWorkers);
    }
    for (unsigned i=0;i<DataflowBarrierWorkers;i++) released[i]=confirmed[i]=false;
    {
        FakeStopIO io;DataflowExclusiveRoleStop owner;
        released[0]=confirmed[0]=true;io.held[0]=false;
        released[1]=true;io.held[1]=false; // unconfirmed, but actually running
        released[2]=true; // unconfirmed and still held
        io.drained=false;
        assert(!Run(owner,released,confirmed,io,stage,core) && stage==15 && core==1
            && !io.roleWrites && !io.computeWrites && !io.holdWrites);
        io.drained=true;
        assert(Run(owner,released,confirmed,io,stage,core));
        assert(io.roleWrites==2 && io.computeWrites==2 && io.holdWrites==2 && io.held[2]);
    }
    for (unsigned i=0;i<DataflowBarrierWorkers;i++) released[i]=confirmed[i]=false;
    for (unsigned failed=0;failed<3;failed++) {
        FakeStopIO io;DataflowExclusiveRoleStop owner;
        released[0]=confirmed[0]=true;io.held[0]=false;
        if (failed==0) io.loseRole=0;
        if (failed==1) io.loseCompute=0;
        if (failed==2) io.loseHold=0;
        assert(!Run(owner,released,confirmed,io,stage,core));
        const unsigned firstRole=io.roleWrites,firstCompute=io.computeWrites,firstHold=io.holdWrites;
        assert(!Run(owner,released,confirmed,io,stage,core)); // cannot retry an uncertain write
        assert(firstRole==io.roleWrites && firstCompute==io.computeWrites && firstHold==io.holdWrites);
        if (failed==0) io.roleVisible[0]=true;
        if (failed==1) io.computeVisible[0]=true;
        if (failed==2) io.held[0]=true;
        assert(Run(owner,released,confirmed,io,stage,core));
        assert(io.roleWrites==1 && io.computeWrites==1 && io.holdWrites==1);
        released[0]=confirmed[0]=false;
    }
    {
        FakeStopIO io;DataflowExclusiveRoleStop owner;confirmed[4]=true;
        assert(!Run(owner,released,confirmed,io,stage,core) && stage==17 && core==4 && !io.roleWrites);
    }
    std::puts("PASS dedicated stop: 120 workers, partial release, uncertain writes and observed retries; SIMULATED ONLY");
}
