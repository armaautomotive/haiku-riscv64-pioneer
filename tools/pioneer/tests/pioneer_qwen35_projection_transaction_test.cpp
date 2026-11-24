/* SPDX-License-Identifier: MIT */
// Test the extracted single-session sequence and failure handling.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>
namespace PioneerQwen35 {
enum DataflowServiceResult { DataflowServiceComplete,DataflowServiceScratchHeld,
    DataflowServiceClosed,DataflowServiceInvalidState };
}
struct Scratch {
    bool staged=false,verified=false;
    bool IsStaged() const { return staged; }
    bool OutputVerified() const { return verified; }
};
class PioneerQ35ProjectionQualificationOwner {
public:
    enum ScratchOperation { Capture,Stage,Verify,Restore };
    struct Scratch value;bool retained=false;
    struct Scratch* Scratch() { return &value; }
    bool Retained() const { return retained; }
    void Retain() { retained=true; }
};
struct Scope {};
struct Context {
    PioneerQ35ProjectionQualificationOwner qwen35QualificationOwner;
    unsigned failure=0;bool badOutput=false,abandoned=false,failAbandon=false,failCleanupRestore=false,retainAtVerify=false;
    std::vector<unsigned> calls;
    bool Step(unsigned n) { calls.push_back(n);return failure!=n; }
    bool AllocateDataflowQualification(Scope&) { return Step(1); }
    bool DataflowQualificationScratchOperation(unsigned op,Scope&) {
        using O=PioneerQ35ProjectionQualificationOwner;
        const unsigned n=op==O::Capture?2:op==O::Stage?3:op==O::Verify?10:11;
        if (!Step(n) || (op==O::Restore && failCleanupRestore)) return false;
        if (op==O::Stage) qwen35QualificationOwner.value.staged=true;
        if (op==O::Restore) qwen35QualificationOwner.value.staged=false;
        if (op==O::Verify) {
            if (retainAtVerify) { qwen35QualificationOwner.Retain();return false; }
            qwen35QualificationOwner.value.verified=!badOutput;return !badOutput;
        }
        return true;
    }
    bool PrepareDataflowQualificationRoutes(Scratch&,Scope&,uint32_t& stage,uint32_t& core) {
        stage=4;core=119;return Step(4);
    }
    bool BindDataflowQualification(Scope&) { return Step(5); }
    PioneerQwen35::DataflowServiceResult InstallDataflowQualification(Scratch&,Scope&) {
        return Step(6)?PioneerQwen35::DataflowServiceComplete:PioneerQwen35::DataflowServiceInvalidState;
    }
    PioneerQwen35::DataflowServiceResult RunDataflowQualification(Scope&) {
        return Step(7)?PioneerQwen35::DataflowServiceComplete:PioneerQwen35::DataflowServiceInvalidState;
    }
    PioneerQwen35::DataflowServiceResult CloseDataflowQualification(Scope&) {
        return Step(8)?PioneerQwen35::DataflowServiceScratchHeld:PioneerQwen35::DataflowServiceInvalidState;
    }
    bool MarkDataflowQualificationStopped(Scope&) { return Step(9); }
    bool PrepareDataflowQualificationRetirement(Scope&) { return Step(12); }
    PioneerQwen35::DataflowServiceResult FinishDataflowQualificationClose(Scope&) {
        return Step(13)?PioneerQwen35::DataflowServiceClosed:PioneerQwen35::DataflowServiceInvalidState;
    }
    bool RetireDataflowQualification(Scope&) { return Step(14); }
    bool AbandonDataflowQualification(Scope&) { abandoned=true;return !failAbandon; }
#include "pioneer_q35_projection_transaction.inc"
};
int main() {
    Scope scope;
    for (unsigned failure=0;failure<=14;failure++) {
        Context c;c.failure=failure;
        uint32_t stage=99,routeStage=99,routeCore=99;bool output=true,cleaned=true;
        const bool result=c.RunDataflowQualificationTransaction(scope,stage,routeStage,routeCore,output,cleaned);
        if (!failure) {
            assert(result && output && cleaned && stage==14 && !c.qwen35QualificationOwner.retained);
            assert((c.calls==std::vector<unsigned>{1,2,3,4,5,6,7,8,9,10,11,12,13,14}));
        } else if (failure==10) {
            assert(!result && !output && cleaned && stage==14 && !c.qwen35QualificationOwner.retained);
        } else if (failure<=6) {
            assert(!result && !output && !cleaned && !c.qwen35QualificationOwner.retained);
            assert(c.abandoned==(failure!=1));
            if (failure>=4) assert(c.calls.back()==11);
        } else {
            assert(!result && !cleaned && c.qwen35QualificationOwner.retained);
            assert(c.calls.back()==failure);
        }
    }
    Context bad;bad.badOutput=true;
    uint32_t stage=0,routeStage=0,routeCore=0;bool output=false,cleaned=false;
    assert(!bad.RunDataflowQualificationTransaction(scope,stage,routeStage,routeCore,output,cleaned)
        && !output && cleaned && stage==14 && !bad.qwen35QualificationOwner.retained);
    Context failedAbandon;failedAbandon.failure=2;failedAbandon.failAbandon=true;
    assert(!failedAbandon.RunDataflowQualificationTransaction(scope,stage,routeStage,routeCore,output,cleaned)
        && failedAbandon.abandoned && failedAbandon.qwen35QualificationOwner.retained && !cleaned);
    Context failedRestore;failedRestore.failure=4;failedRestore.failCleanupRestore=true;
    assert(!failedRestore.RunDataflowQualificationTransaction(scope,stage,routeStage,routeCore,output,cleaned)
        && !failedRestore.abandoned && failedRestore.qwen35QualificationOwner.retained && !cleaned);
    Context transferFailure;transferFailure.retainAtVerify=true;
    assert(!transferFailure.RunDataflowQualificationTransaction(scope,stage,routeStage,routeCore,output,cleaned)
        && transferFailure.qwen35QualificationOwner.retained && transferFailure.calls.back()==10 && !cleaned);
    std::puts("PASS single-session projection order, invalid output cleanup and failure retention; SIMULATED ONLY");
}
