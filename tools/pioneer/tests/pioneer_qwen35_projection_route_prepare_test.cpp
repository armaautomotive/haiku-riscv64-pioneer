/* SPDX-License-Identifier: MIT */
// Execute the actual route wrapper with simulated route selection and faults.
#include <cassert>
#include <cstdint>
#include <cstdio>
namespace PioneerQwen35 { enum { DataflowCoreCount=120 }; }
struct Scratch { bool staged=true; bool IsStaged() const { return staged; } };
struct Owner { Scratch* owned=nullptr; bool Owns(const Scratch& s) const { return owned==&s; } };
struct Banks {
    bool match=true;
    bool Q35StreamOwnerMatches(int,const Scratch*,unsigned count) const { return match && count==120; }
};
struct IO {
    bool idle=true,audit=true,select=true,mapping=true;
    unsigned current=0,audits=0,selections=0;
    bool SessionIdle() const { return idle; }
    bool ProveFleetRoutes(uint32_t& stage,uint32_t& core) {
        ++audits;current=119;stage=42;core=119;return audit;
    }
    bool SelectRuntimeCore(unsigned core) { ++selections;if (!select) return false;current=core;return true; }
    bool DataflowPanelMappingStable() const { return mapping; }
};
struct Context {
    Owner qwen35QualificationOwner;
    bool qwen35QualificationInternalActive=false;
    void *qwen35DataflowAuthority=nullptr,*qwen35DataflowProgramScratch=nullptr,*qwen35GraphOwner=nullptr;
    void *qwen35DataflowMemory=nullptr,*qwen35DataflowService=nullptr;
    Banks banks;IO io;
    struct A { int dram=0; };struct B { A baseline; };struct C { B baseline; };struct D { C baseline; } baseline;
    struct QualificationInternalScope {
        bool& active;explicit QualificationInternalScope(bool& a):active(a) { assert(!active);active=true; }
        ~QualificationInternalScope() { active=false; }
    };
#include "pioneer_q35_projection_route_prepare.inc"
};
struct Scope {
    bool allowed=true,retained=false;
    bool QualificationPrepareRoutesAllowed(const Scratch&,const Context& c) const {
        return allowed && c.qwen35QualificationInternalActive;
    }
    void RetainScratch(const Scratch*) { retained=true; }
};
int main() {
    for (unsigned scenario=0;scenario<6;scenario++) {
        Context c;Scratch scratch;c.qwen35QualificationOwner.owned=&scratch;Scope scope;
        if (scenario==1) scope.allowed=false;
        if (scenario==2) c.io.idle=false;
        if (scenario==3) c.io.audit=false;
        if (scenario==4) c.io.select=false;
        if (scenario==5) c.io.mapping=false;
        uint32_t stage=99,core=99;
        const bool okay=c.PrepareDataflowQualificationRoutes(scratch,scope,stage,core);
        assert(!c.qwen35QualificationInternalActive);
        if (!scenario) assert(okay && c.io.current==0 && c.io.audits==1 && c.io.selections==1 && !scope.retained);
        else if (scenario<=2) assert(!okay && !c.io.audits && !scope.retained && stage==0 && core==0xff);
        else assert(!okay && scope.retained && c.io.audits==1 && stage==42 && core==119);
    }
    std::puts("PASS projection route audit restores worker zero and retains uncertain failures; SIMULATED ONLY");
}
