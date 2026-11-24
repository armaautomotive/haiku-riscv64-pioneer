/* SPDX-License-Identifier: MIT */
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35DataflowL2CpuResetProbe.h"
#include <cassert>
#include <cstdio>
#include <cstring>

struct FakeIO {
    unsigned runtimeCore=7,writes=0,resetReads=0,bootReads=0;
    uint32_t initial[3]={11,22,33},tlb[3]={11,22,33};
    uint32_t reset=0x0f,boot=5;
    int dropWrite=-1;
    bool mapped=true,driftReset=false,driftBoot=false;
    uint32_t DramTlbRead(unsigned slot,unsigned word) { assert(slot==1 && word<3); return tlb[word]; }
    void DramTlbWrite(unsigned slot,unsigned word,uint32_t value) {
        assert(slot==1 && word<3);
        if (int(writes++)!=dropWrite) tlb[word]=value;
    }
    bool SharedArcResetMapped() const { return mapped; }
    uint32_t ScratchRead(unsigned word) {
        assert(word==2 && ArcSelected());
        return boot+(driftBoot && bootReads++?1U:0U);
    }
    uint32_t SharedArcL2CpuResetRead() {
        assert(ArcSelected());
        return reset+(driftReset && resetReads++?1U:0U);
    }
    void Pause() {}
    bool ArcSelected() const {
        for (unsigned i=0;i<3;i++) if (tlb[i]!=BlackholeArcScratchTlbWord(i)) return false;
        return true;
    }
    bool Restored() const { return !std::memcmp(initial,tlb,sizeof(tlb)); }
};
struct Scope {
    bool allowed=true;
    unsigned calls=0;
    int failAt=-1;
    bool ProveRoleStatusInspectionAllowed(FakeIO&) { return allowed && int(calls++)!=failAt; }
};
struct Guard {
    FakeIO& io;
    bool allowed=true;
    unsigned calls=0;
    int failAt=-1;
    bool ProveSelectedRoute(unsigned core) {
        return allowed && int(calls++)!=failAt && core==io.runtimeCore && io.Restored();
    }
};

int main()
{
    {
        FakeIO io; Scope scope; Guard guard{io}; uint32_t reset=99; bool restoreFailed=true;
        assert(PioneerQwen35SampleL2CpuReset(io,scope,guard,reset,restoreFailed));
        assert(reset==0x0f && PioneerQwen35L2CpuRiscsHeld(reset) && !restoreFailed && io.Restored() && io.writes==6);
    }
    {
        FakeIO io; Scope scope; Guard guard{io}; uint32_t reset=99; bool restoreFailed=false;
        io.reset=0x1f; // observation succeeds, but caller must reject running L2CPU0
        assert(PioneerQwen35SampleL2CpuReset(io,scope,guard,reset,restoreFailed));
        assert(!PioneerQwen35L2CpuRiscsHeld(reset) && !restoreFailed && io.Restored());
    }
    for (unsigned kind=0;kind<4;kind++) {
        FakeIO io; Scope scope; Guard guard{io}; uint32_t reset=99; bool restoreFailed=true;
        if (kind==0) io.driftReset=true;
        if (kind==1) io.driftBoot=true;
        if (kind==2) io.reset=UINT32_MAX;
        if (kind==3) io.dropWrite=1; // incomplete ARC TLB configuration
        assert(!PioneerQwen35SampleL2CpuReset(io,scope,guard,reset,restoreFailed));
        assert(reset==99 && !restoreFailed && io.Restored());
    }
    {
        FakeIO io; Scope scope; Guard guard{io}; uint32_t reset=99; bool restoreFailed=false;
        io.dropWrite=3; // first restore write does not reach the TLB
        assert(!PioneerQwen35SampleL2CpuReset(io,scope,guard,reset,restoreFailed));
        assert(restoreFailed && reset==99 && !io.Restored());
    }
    {
        FakeIO io; Scope scope; Guard guard{io}; uint32_t reset=99; bool restoreFailed=false;
        scope.allowed=false;
        assert(!PioneerQwen35SampleL2CpuReset(io,scope,guard,reset,restoreFailed));
        assert(!restoreFailed && !io.writes && io.Restored());
    }
    {
        FakeIO io; Scope scope; Guard guard{io}; uint32_t reset=99; bool restoreFailed=false;
        guard.failAt=2; // route no longer matches after restoring the TLB
        assert(!PioneerQwen35SampleL2CpuReset(io,scope,guard,reset,restoreFailed));
        assert(restoreFailed && io.Restored());
    }
    {
        FakeIO io; Scope scope; Guard guard{io}; uint32_t reset=99; bool restoreFailed=false;
        scope.failAt=3; // ownership is lost immediately after the first TLB write
        assert(!PioneerQwen35SampleL2CpuReset(io,scope,guard,reset,restoreFailed));
        assert(restoreFailed && io.writes==1 && !io.Restored());
    }
    assert(!PioneerQwen35L2CpuRiscsHeld(UINT32_MAX));
    std::puts("PASS bounded L2CPU reset sampling and TLB restoration; SIMULATED ONLY");
    return 0;
}
