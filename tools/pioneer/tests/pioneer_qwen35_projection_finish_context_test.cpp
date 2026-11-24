/* SPDX-License-Identifier: MIT */
// Exercise the actual extracted context finish and failure retention.
#include <cassert>
#include <cstdio>
namespace PioneerQwen35 {
enum DataflowServiceResult { DataflowServiceInvalidState, DataflowServiceClosed, DataflowServiceRestoreFailed };
}
using namespace PioneerQwen35;
struct Scratch { bool restored=true; bool IsRestored() const { return restored; } };
struct Owner {
    Scratch scratch; bool retained=false, prepared=true;
    Scratch* Scratch() { return &scratch; }
    bool Retained() const { return retained; }
    bool RetirementPrepared() const { return prepared && !retained; }
    void Retain() { retained=true; }
};
struct Scope { bool allowed=true; };
struct IO {};
struct Service {
    bool held=true, closed=false, resumeFails=false; unsigned attempts=0;
    bool ScratchHeld() const { return held; }
    DataflowServiceResult FinishScratchClose(IO&,void*,Scope& scope) {
        if (!scope.allowed) return DataflowServiceInvalidState;
        ++attempts;held=false;
        if (resumeFails) return DataflowServiceRestoreFailed;
        closed=true;return DataflowServiceClosed;
    }
};
struct Context {
    Owner qwen35QualificationOwner;
    bool qwen35QualificationInternalActive=false, cleanupFails=false;
    Service service; Service* qwen35DataflowService=&service;
    unsigned cleanups=0; IO io;
    struct QualificationInternalScope {
        bool& active; explicit QualificationInternalScope(bool& a):active(a) { assert(!active);active=true; }
        ~QualificationInternalScope() { active=false; }
    };
    DataflowServiceResult CloseDataflowRewrite() {
        assert(service.closed && qwen35QualificationInternalActive);++cleanups;
        return cleanupFails?DataflowServiceRestoreFailed:DataflowServiceClosed;
    }
#include "pioneer_q35_projection_finish.inc"
};
int main() {
    Scope scope;
    {
        Context c;c.qwen35QualificationOwner.prepared=false;
        assert(c.FinishDataflowQualificationClose(scope)==DataflowServiceInvalidState && !c.service.attempts);
        c.qwen35QualificationOwner.prepared=true;c.qwen35QualificationOwner.scratch.restored=false;
        assert(c.FinishDataflowQualificationClose(scope)==DataflowServiceInvalidState && !c.service.attempts);
        c.qwen35QualificationOwner.scratch.restored=true;scope.allowed=false;
        assert(c.FinishDataflowQualificationClose(scope)==DataflowServiceInvalidState && !c.service.attempts
            && !c.qwen35QualificationOwner.retained && !c.qwen35QualificationInternalActive);
        scope.allowed=true;
        assert(c.FinishDataflowQualificationClose(scope)==DataflowServiceClosed && c.cleanups==1
            && c.service.attempts==1 && !c.qwen35QualificationOwner.retained);
        assert(c.FinishDataflowQualificationClose(scope)==DataflowServiceInvalidState && c.service.attempts==1);
    }
    {
        Context c;c.service.resumeFails=true;
        assert(c.FinishDataflowQualificationClose(scope)==DataflowServiceRestoreFailed
            && c.qwen35QualificationOwner.retained && !c.cleanups && c.service.attempts==1);
        assert(c.FinishDataflowQualificationClose(scope)==DataflowServiceInvalidState && c.service.attempts==1);
    }
    {
        Context c;c.cleanupFails=true;
        assert(c.FinishDataflowQualificationClose(scope)==DataflowServiceRestoreFailed
            && c.qwen35QualificationOwner.retained && c.cleanups==1 && c.service.attempts==1);
        assert(c.FinishDataflowQualificationClose(scope)==DataflowServiceInvalidState && c.service.attempts==1);
    }
    std::puts("PASS context finish requires proof and retains uncertain failures; SIMULATED ONLY");
}
