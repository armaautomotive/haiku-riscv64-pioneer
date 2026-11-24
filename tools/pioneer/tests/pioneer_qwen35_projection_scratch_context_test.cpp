/* SPDX-License-Identifier: MIT */
// Execute the exact context scratch wrapper and internal scope against the
// real bank/transfer classes over a simulated device.
#define PIONEER_QWEN35_PROJECTION_SCRATCH_MOCK_ONLY 1
#include "pioneer_qwen35_projection_scratch_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35ProjectionQualificationOwner.h"

struct Context {
    PioneerQ35ProjectionQualificationOwner& qwen35QualificationOwner;
    bool qwen35QualificationInternalActive=false;
    BanksMock& io;PioneerResidentBanks& banks;tt_host_page_result& host;PioneerResidentLease& lease;
    uint8_t* initialized;
    struct InnerC { tt_dram_samples_result& dram; };
    struct InnerB { InnerC baseline;tt_compute_noc_result& noc; };
    struct InnerA { InnerB baseline; };
    struct Baseline { InnerA baseline; } baseline;
    Context(PioneerQ35ProjectionQualificationOwner& owner,ScratchDevice<>& device)
        :qwen35QualificationOwner(owner),io(device.io),banks(device.banks),host(device.host),lease(device.lease),
         initialized(device.initialized.data()),baseline{{{{device.dram},device.noc}}} {}
#include "pioneer_q35_projection_internal_scope.inc"
#include "pioneer_q35_projection_scratch_operation.inc"
};

struct Scope : ScratchScope {
    Context* context=nullptr;
    bool operationAllowed=true;
    bool QualificationScratchOperationAllowed(const PioneerQ35ProjectionQualificationOwner& holder,
        const PioneerQ35ProjectionScratch& scratch,unsigned operation,const Context& requested) const {
        return operationAllowed && context==&requested && requested.qwen35QualificationInternalActive
            && holder.Owns(scratch) && operation<=PioneerQ35ProjectionQualificationOwner::Restore;
    }
};
int main() {
    (void)&Input; (void)&Prepare; (void)&Upload; (void)&Close;
    auto owner=std::make_unique<PioneerQ35ProjectionQualificationOwner>();
    assert(owner->Allocate());
    auto device=std::make_unique<ScratchDevice<>>();
    Context context(*owner,*device);Scope scope;scope.context=&context;
    scope.operationAllowed=false;
    assert(!context.DataflowQualificationScratchOperation(PioneerQ35ProjectionQualificationOwner::Capture,scope)
        && !context.qwen35QualificationInternalActive && owner->Scratch()->IsEmpty()
        && !device->banks.Q35StreamClaimed());
    scope.operationAllowed=true;
    assert(context.DataflowQualificationScratchOperation(PioneerQ35ProjectionQualificationOwner::Capture,scope)
        && !context.qwen35QualificationInternalActive);
    assert(context.DataflowQualificationScratchOperation(PioneerQ35ProjectionQualificationOwner::Stage,scope)
        && !context.qwen35QualificationInternalActive);
    assert(owner->Scratch()->MarkRunning(scope));
    assert(!context.DataflowQualificationScratchOperation(PioneerQ35ProjectionQualificationOwner::Verify,scope)
        && !context.qwen35QualificationInternalActive);
    device->SetOutput();scope.stopped=true;
    assert(owner->Scratch()->MarkStopped(scope));
    assert(context.DataflowQualificationScratchOperation(PioneerQ35ProjectionQualificationOwner::Verify,scope)
        && context.DataflowQualificationScratchOperation(PioneerQ35ProjectionQualificationOwner::Restore,scope)
        && owner->Scratch()->IsRestored() && !context.qwen35QualificationInternalActive);
    std::puts("PASS actual context scratch wrapper, controlled admission and full restoration; SIMULATED ONLY");
    return 0;
}
