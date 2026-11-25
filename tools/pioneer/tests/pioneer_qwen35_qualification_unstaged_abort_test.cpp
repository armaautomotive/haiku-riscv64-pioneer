/* SPDX-License-Identifier: MIT */
// Exercise the extracted qualification failure branch with the real service
// installation/close state machine and simulated worker IO.
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_RECOVERY_RUN_ENABLE 1
#define PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS 1
#define PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS 1
#include <memory>
#define main dataflow_service_existing_test_main
#include "pioneer_qwen35_dataflow_service_owner_test.cpp"
#undef main

struct Binding { bool abortAllowed=true; };
static bool PioneerQ35CanAbortUnstagedQualification(Binding* binding) {
    return binding && binding->abortAllowed;
}
struct QualificationOwner {
    bool retained=false;
    void Retain() { retained=true; }
};
struct Context;
struct Scope {
    bool routeOkay=true;unsigned restores=0;
    bool QualificationRestoreUnstagedRoute(int&,Context&) { ++restores;return routeOkay; }
};
struct Context {
    QualificationOwner qwen35QualificationOwner;
    Binding binding;Binding* qwen35SharedRoleBinding=&binding;
    DataflowServiceOwner service;DataflowServiceOwner* qwen35DataflowService=&service;
    struct Memory { bool confirmed=true;bool HeldWritesConfirmed() const { return confirmed; } } memory;
    Memory* qwen35DataflowMemory=&memory;
    MockDataflowIO io;unsigned closes=0;
    DataflowServiceResult CloseDataflowRewrite() {
        ++closes;const auto closed=service.Close(io);
        if (closed==DataflowServiceClosed) qwen35SharedRoleBinding=nullptr;
        return closed;
    }
    void Handle(DataflowServiceResult result,Scope& scope) {
        int scratch=0;
#include "pioneer_q35_qualification_install_failure.inc"
    }
};
static void Prepare(Context& context) {
    uint32_t code[96]{},dispatchCode[80]{};
    DataflowServiceImageView image={DataflowServiceBootWord,code,96,dispatchCode,80};
    context.io.failRoleStage=true;
    assert(context.service.Install(image,context.io)==DataflowServiceSnapshotFailed
        && context.service.Faulted() && !context.io.startCount);
}
int main() {
    {
        auto c=std::make_unique<Context>();Scope scope;Prepare(*c);
        c->Handle(DataflowServiceSnapshotFailed,scope);
        assert(!c->qwen35QualificationOwner.retained && c->service.Closed()
            && c->closes==1 && scope.restores==1 && c->io.restoreCount==DataflowCoreCount
            && c->io.resumeCount==1 && !c->qwen35SharedRoleBinding);
    }
    {
        auto c=std::make_unique<Context>();Scope scope;Prepare(*c);c->binding.abortAllowed=false;
        c->Handle(DataflowServiceSnapshotFailed,scope);
        assert(c->qwen35QualificationOwner.retained && c->service.Faulted()
            && !c->closes && !scope.restores && !c->io.restoreCount);
    }
    {
        auto c=std::make_unique<Context>();Scope scope;Prepare(*c);c->memory.confirmed=false;
        c->Handle(DataflowServiceSnapshotFailed,scope);
        assert(c->qwen35QualificationOwner.retained && c->service.Faulted()
            && !c->closes && !scope.restores);
    }
    {
        auto c=std::make_unique<Context>();Scope scope;Prepare(*c);c->io.failRoleRestore=true;
        c->Handle(DataflowServiceSnapshotFailed,scope);
        assert(c->qwen35QualificationOwner.retained && !c->service.Closed()
            && c->closes==1 && !scope.restores && !c->io.resumeCount);
    }
    {
        auto c=std::make_unique<Context>();Scope scope;Prepare(*c);scope.routeOkay=false;
        c->Handle(DataflowServiceSnapshotFailed,scope);
        assert(c->qwen35QualificationOwner.retained && c->service.Closed()
            && c->closes==1 && scope.restores==1);
    }
    {
        auto c=std::make_unique<Context>();Scope scope;Prepare(*c);c->io.failResume=true;
        c->Handle(DataflowServiceSnapshotFailed,scope);
        assert(c->qwen35QualificationOwner.retained && !c->service.Closed()
            && c->closes==1 && !scope.restores);
    }
    std::puts("PASS unstaged install denial closes service or retains failed cleanup; SIMULATED ONLY");
}
