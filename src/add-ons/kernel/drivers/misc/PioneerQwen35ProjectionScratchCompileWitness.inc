/* SPDX-License-Identifier: MIT */
// Type-check the private scratch transaction with the real Haiku session IO.
// This witness grants no card authority and is present only in a driver whose
// initialization deliberately rejects loading.
#if !defined(PIONEER_QWEN35_RECOVERY_COMPILE_ONLY) || !defined(PIONEER_QWEN35_DEVICE_DISPATCH)
#error Projection scratch compile witness requires the non-loadable recovery driver
#endif
#include "PioneerQwen35ProjectionScratch.h"
#include "../../../../../tools/pioneer/PioneerQwen35DataflowProjectionQualificationService.h"

class PioneerQ35ProjectionScratchCompileScope {
public:
    bool permitted=false;
    PioneerStoreContext* context=nullptr;
    bool ScratchReserveAllowed(const void*,unsigned,unsigned,unsigned) const { return permitted; }
    bool ScratchTransferAllowed(const void*,unsigned,unsigned,bool) const { return permitted; }
    bool ScratchStageAllowed(const void*) const { return permitted; }
    bool ScratchRunAllowed(const void*) const { return permitted; }
    bool ScratchStopped(const void*) const { return permitted; }
    bool ScratchOutputAllowed(const void*) const { return permitted; }
    bool ScratchRestoreAllowed(const void*) const { return permitted; }
    void RetainScratch(const void*) {
        if (context) context->qwen35QualificationOwner.Retain();
        permitted=false;
    }
    bool QualificationOwnerAllocationAllowed(const PioneerStoreContext& requested) const {
        return permitted && context==&requested;
    }
    bool QualificationScratchOperationAllowed(const PioneerQ35ProjectionQualificationOwner& holder,
        const PioneerQ35ProjectionScratch& scratch,unsigned operation,const PioneerStoreContext& requested) const {
        return permitted && context==&requested && holder.Owns(scratch)
            && operation<=PioneerQ35ProjectionQualificationOwner::Restore;
    }
    bool QualificationPrepareRoutesAllowed(const PioneerQ35ProjectionScratch& scratch,
        const PioneerStoreContext& requested) const {
        using Fixture=PioneerQwen35::DataflowProjectionQualification;
        return permitted && context==&requested
            && scratch.MatchesStagedLease(0,Fixture::ScratchAddress,Fixture::ScratchBytes)
            && scratch.MatchesBankClaim(context->banks,context->baseline.baseline.baseline.baseline.dram);
    }
    bool QualificationInstallationAllowed(const PioneerQ35ProjectionScratch& scratch,
        const PioneerStoreContext& requested) const {
        return QualificationPrepareRoutesAllowed(scratch,requested) && context->qwen35DataflowRoutesAudited;
    }
    bool QualificationRunAllowed(const PioneerQ35ProjectionQualificationOwner& holder,
        const PioneerQ35ProjectionScratch& scratch,const PioneerQwen35::DataflowProjectionQualificationService&,
        const PioneerStoreContext& requested) const {
        return permitted && context==&requested && holder.Owns(scratch);
    }
    bool QualificationCloseAllowed(const PioneerQ35ProjectionQualificationOwner& holder,
        const PioneerQ35ProjectionScratch& scratch,const PioneerQwen35::DataflowProjectionQualificationService&,
        const PioneerStoreContext& requested) const {
        return permitted && context==&requested && holder.Owns(scratch);
    }
    bool QualificationOwnerRetirementAllowed(const PioneerQ35ProjectionQualificationOwner& holder,
        const PioneerQ35ProjectionScratch& scratch,
        const PioneerQwen35::DataflowProjectionQualificationService&) const {
        return permitted && context && holder.Owns(scratch) && context->qwen35DataflowService
            && context->qwen35DataflowService->Closed();
    }
    bool QualificationOwnerAbandonAllowed(const PioneerQ35ProjectionQualificationOwner&) const {
        return permitted && context && !context->qwen35DataflowService;
    }
    bool QualificationOwnerAbandonRestoredAllowed(const PioneerQ35ProjectionQualificationOwner& holder,
        const PioneerQ35ProjectionScratch& scratch,
        const PioneerQwen35::DataflowProjectionQualificationService& qualification) const {
        return permitted && context && holder.Owns(scratch) && scratch.IsRestored()
            && qualification.IsUnstarted() && !context->qwen35DataflowService;
    }
    bool QualificationLeaseMatches(const PioneerQ35ProjectionScratch& scratch,unsigned bank,
        unsigned address,unsigned bytes,const unsigned (&routes)[2][PioneerQwen35::Banks]) const {
        return permitted && context && scratch.MatchesStagedLease(bank,address,bytes)
            && scratch.MatchesBankClaim(context->banks,context->baseline.baseline.baseline.baseline.dram)
            && PioneerQwen35::DataflowBankRoutesValid(routes);
    }
    bool QualificationExecutionSafe(const PioneerQ35ProjectionScratch& scratch,
        const PioneerQwen35::DataflowProjectionProgram& program) const {
        return permitted && context && (scratch.IsStaged() || scratch.IsRunning()) && program.Valid()
            && scratch.MatchesBankClaimOwner(context->banks,context->baseline.baseline.baseline.baseline.dram);
    }
    bool AuditQualification(PioneerSessionIO& io,const PioneerQwen35::DataflowProjectionProgram& program,
        unsigned sequence,const PioneerQwen35::DataflowWorkerSnapshot (&receipts)[PioneerQwen35::DataflowCoreCount]) {
        return permitted && PioneerQwen35DataflowAuditProjectionReceipts(io,program,sequence,receipts);
    }
    void RetainQualification(const void*,const void*) {
        if (context) context->qwen35QualificationOwner.Retain();
        permitted=false;
    }
};

template<class Scope> static bool
PioneerQ35ProjectionScratchCompileWitnessImpl(PioneerStoreContext& context,Scope& scope,unsigned action)
{
    if (action==10) return context.AllocateDataflowQualification(scope);
    auto* scratch=context.qwen35QualificationOwner.Scratch();
    auto* qualification=context.qwen35QualificationOwner.Qualification();
    auto* saved=context.qwen35QualificationOwner.Snapshot();
    if (!scratch || !qualification || !saved) return false;
    if (action==0) return context.DataflowQualificationScratchOperation(PioneerQ35ProjectionQualificationOwner::Capture,scope);
    if (action==1) return context.DataflowQualificationScratchOperation(PioneerQ35ProjectionQualificationOwner::Stage,scope);
    if (action==2) return scratch->MarkRunning(scope);
    if (action==3) return context.MarkDataflowQualificationStopped(scope);
    if (action==4) return context.DataflowQualificationScratchOperation(PioneerQ35ProjectionQualificationOwner::Verify,scope);
    if (action==5) return context.DataflowQualificationScratchOperation(PioneerQ35ProjectionQualificationOwner::Restore,scope);
    if (action==6) return context.BindDataflowQualification(scope);
    if (action==7) return context.RunDataflowQualification(scope)==PioneerQwen35::DataflowServiceComplete;
    if (action==8) {
        uint32_t stage=0,core=0;
        return context.PrepareDataflowQualificationRoutes(*scratch,scope,stage,core);
    }
    if (action==9) return context.InstallDataflowQualification(*scratch,scope)
        ==PioneerQwen35::DataflowServiceComplete;
    if (action==11) return context.CloseDataflowQualification(scope)==PioneerQwen35::DataflowServiceClosed;
    if (action==12) return context.RetireDataflowQualification(scope);
    if (action==13) return context.AbandonDataflowQualification(scope);
    return false;
}

static __attribute__((used,noinline)) bool
PioneerQ35ProjectionScratchCompileWitness(PioneerStoreContext& context,
    PioneerQ35ProjectionScratchCompileScope& scope,unsigned action)
{
    scope.context=&context;
    return PioneerQ35ProjectionScratchCompileWitnessImpl(context,scope,action);
}
#if defined(PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS) && defined(PIONEER_QWEN35_ROLE_WRITE_COMPILE_ONLY)
static __attribute__((used,noinline)) bool
PioneerQ35ProjectionScratchHaikuScopeCompileWitness(PioneerStoreContext& context,
    PioneerQ35ProjectionQualificationScope& scope,unsigned action)
{
    return PioneerQ35ProjectionScratchCompileWitnessImpl(context,scope,action);
}
#endif
