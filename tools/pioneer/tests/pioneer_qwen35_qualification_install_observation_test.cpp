/* SPDX-License-Identifier: MIT */
// Compose the actual qualification admission and service installation with
// simulated PCI/worker state. This does not grant physical hardware authority.
#define main projection_scope_existing_test_main
#include "pioneer_qwen35_projection_haiku_scope_test.cpp"
#undef main
#define main dataflow_service_existing_test_main
#include "pioneer_qwen35_dataflow_service_owner_test.cpp"
#undef main

struct InstallationIO : MockDataflowIO {
    PioneerStoreContext& context;
    bool observed=false;
    explicit InstallationIO(PioneerStoreContext& c):context(c) {}

    bool SharedCaptureAndStageHeld() {
        auto& service=*context.qwen35DataflowService;
        assert(service.SharedRoleCapturePending() && context.Unsafe());
        assert(!context.UnsafeForQ35HeldObservation());
        assert(PioneerQ35QualificationHeldOwnerAllowed(context,context.io));
        context.otherUnsafe=true;
        assert(context.UnsafeForQ35HeldObservation()
            && !PioneerQ35QualificationHeldOwnerAllowed(context,context.io));
        context.otherUnsafe=false;
        auto* binding=context.qwen35SharedRoleBinding;
        context.qwen35SharedRoleBinding=nullptr;
        assert(!PioneerQ35QualificationHeldOwnerAllowed(context,context.io));
        context.qwen35SharedRoleBinding=binding;
        auto* memory=context.qwen35DataflowMemory;
        context.qwen35DataflowMemory=nullptr;
        assert(!PioneerQ35QualificationHeldOwnerAllowed(context,context.io));
        context.qwen35DataflowMemory=memory;
        context.qwen35QualificationInternalActive=false;
        assert(context.UnsafeForQ35HeldObservation()
            && !PioneerQ35QualificationHeldOwnerAllowed(context,context.io));
        context.qwen35QualificationInternalActive=true;
        observed=true;
        return false; // Stop before staging or releasing any simulated role.
    }
};

int main() {
    PioneerStoreContext context;
    PioneerQwen35RoleBorrowedScopeWitness borrowed;borrowed.held=true;
    context.io.qwen35BorrowedRoleScope=&borrowed;
    PioneerQ35ProjectionQualificationScope scope(context);
    assert(context.qwen35QualificationOwner.Allocate());
    context.qwen35QualificationInternalActive=true;
    FakeRoleBinding binding;
    DataflowServiceOwner service;
    context.qwen35SharedRoleBinding=&binding;
    context.qwen35DataflowMemory=&context.memory;
    context.qwen35DataflowService=&service;
    assert(!PioneerQ35QualificationHeldOwnerAllowed(context,context.io));
    uint32_t code[96]{},dispatchCode[80]{};
    DataflowServiceImageView image={DataflowServiceBootWord,code,96,dispatchCode,80};
    auto io=std::make_unique<InstallationIO>(context);
    assert(service.Install(image,*io)==DataflowServiceSnapshotFailed && io->observed);
    assert(service.Faulted() && context.Unsafe() && context.UnsafeForQ35HeldObservation()
        && !PioneerQ35QualificationHeldOwnerAllowed(context,context.io));
    assert(PioneerQ35QualificationHeldRollbackOwnerAllowed(context,context.io));
    context.memory.confirmed=false;
    assert(!PioneerQ35QualificationHeldRollbackOwnerAllowed(context,context.io));
    context.memory.confirmed=true;context.otherUnsafe=true;
    assert(!PioneerQ35QualificationHeldRollbackOwnerAllowed(context,context.io));
    context.otherUnsafe=false;
    assert(io->startCount==0 && io->roleStages==0);
    assert(service.Close(*io)==DataflowServiceClosed && service.Closed()
        && io->restoreCount==DataflowCoreCount && io->resumeCount==1);
    context.qwen35SharedRoleBinding=nullptr;context.qwen35DataflowMemory=nullptr;
    assert(context.io.SelectRuntimeCore(119));
    auto* scratch=context.qwen35QualificationOwner.Scratch();assert(scratch);
    assert(scope.QualificationRestoreUnstagedRoute(*scratch,context) && context.io.runtimeCore==0);
    assert(context.io.SelectRuntimeCore(119));
    context.io.failSelection=0;context.io.failSelectionAfter=context.io.selects;
    assert(!scope.QualificationRestoreUnstagedRoute(*scratch,context)
        && context.qwen35QualificationOwner.Retained() && sColdRecoveryRequired);
    std::puts("PASS install denial, audited service close and unstaged route restoration; SIMULATED ONLY");
}
