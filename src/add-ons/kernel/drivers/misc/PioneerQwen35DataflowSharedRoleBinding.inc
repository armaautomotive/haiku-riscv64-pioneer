/* SPDX-License-Identifier: MIT */
// Actual Haiku session/owner/mapping composition for the non-loadable shared
// role witness. The borrowed scope currently denies all physical proofs and
// SharedStartCore never releases reset: this is not an executable service.
#if !defined(PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS) || !defined(PIONEER_QWEN35_ROLE_WRITE_COMPILE_ONLY) || !defined(PIONEER_QWEN35_RECOVERY_COMPILE_ONLY) || !defined(PIONEER_QWEN35_DEVICE_DISPATCH)
#error Shared role binding is a non-loadable recovery witness only
#endif
class PioneerQwen35SharedRoleBinding {
    using Views=PioneerQwen35::DataflowRoleWritableViews<PioneerQwen35RoleHaikuMap>;
    using Adapter=PioneerQwen35RoleFleetPciIO<PioneerSessionIO,PioneerQwen35RoleBorrowedScopeWitness,Views>;
    PioneerStoreContext& fContext;
    PioneerQwen35RoleHaikuMap fPlatform;
    Views fViews;
    PioneerQwen35::DataflowRoleFleetRecoveryOwner fOwner;
    uint32_t* fCodeSnapshot=NULL;
    uint32_t* fRamSnapshot=NULL;
    bool fCaptureAttempted=false;
    PioneerQwen35RoleBorrowedScopeWitness* Scope() {
        auto* scope=fContext.io.qwen35BorrowedRoleScope;
        return scope && scope->ProveExclusiveRewrite(fContext.io)?scope:NULL;
    }
    bool OpenForRecovery(PioneerQwen35RoleBorrowedScopeWitness& scope) {
        if (fViews.Opened()) return true;
        PioneerQwen35RoleRouteGuard<PioneerSessionIO,PioneerQwen35RoleBorrowedScopeWitness> guard(fContext.io,scope);
        if (fViews.HasMappings()) return fViews.ReopenForRecovery(guard);
        return fViews.Open(fContext.io.profile.bar0_host_start,fContext.io.profile.bar0_host_end,guard);
    }
public:
    explicit PioneerQwen35SharedRoleBinding(PioneerStoreContext& context)
        :fContext(context),fViews(fPlatform) {
        fCodeSnapshot=(uint32_t*)malloc(sizeof(uint32_t)*PioneerQwen35::DataflowRoleFleetRecoveryOwner::CodeWords());
        fRamSnapshot=(uint32_t*)malloc(sizeof(uint32_t)*PioneerQwen35::DataflowRoleFleetRecoveryOwner::RamWords());
    }
    PioneerQwen35SharedRoleBinding(const PioneerQwen35SharedRoleBinding&)=delete;
    PioneerQwen35SharedRoleBinding& operator=(const PioneerQwen35SharedRoleBinding&)=delete;
    ~PioneerQwen35SharedRoleBinding() { free(fCodeSnapshot); free(fRamSnapshot); }
    bool Valid() const { return fCodeSnapshot && fRamSnapshot; }
    bool CanDestroy() const { return !fViews.HasMappings() && !fOwner.RestorationRequired(); }
    __attribute__((noinline)) bool CaptureAndStageHeld() {
        using namespace PioneerQwen35;
        if (fCaptureAttempted || !Valid()) return false;
        fCaptureAttempted=true;
        auto* scope=Scope();
        // No mappings, TLB changes or card writes while the real fleet proof
        // is unavailable. The proof is intentionally false in this build.
        if (!scope || !scope->ProveRoleFleetHeldAndQuiet(fContext.io,0) || !OpenForRecovery(*scope)) return false;
        Adapter io(fContext.io,*scope,fViews);
        if (!fOwner.CaptureAll(fCodeSnapshot,DataflowRoleFleetRecoveryOwner::CodeWords(),
                fRamSnapshot,DataflowRoleFleetRecoveryOwner::RamWords(),io)) return false;
        return fOwner.StageImages(DataflowSharedReaderCodeWords,
            sizeof(DataflowSharedReaderCodeWords)/sizeof(uint32_t),DataflowSharedWriterCodeWords,
            sizeof(DataflowSharedWriterCodeWords)/sizeof(uint32_t),DataflowSharedReaderShimWords,
            sizeof(DataflowSharedReaderShimWords)/sizeof(uint32_t),DataflowSharedWriterShimWords,
            sizeof(DataflowSharedWriterShimWords)/sizeof(uint32_t),io);
    }
    __attribute__((noinline)) bool ArmAndBeginRelease() {
        auto* scope=Scope();
        if (!scope || !fViews.Opened()) return false;
        Adapter io(fContext.io,*scope,fViews);
        return fOwner.PreparePossibleRelease(io) && fOwner.BeginReleaseAttempt(io);
    }
    __attribute__((noinline)) bool FleetHeldAndQuiescent() {
        auto* scope=Scope();
        if (!scope || !fViews.Opened()) return false;
        Adapter io(fContext.io,*scope,fViews);
        for (unsigned core=0;core<PioneerQwen35::DataflowBarrierWorkers;core++)
            if (!io.AllRolesHeld(core)) return false;
        return true;
    }
    // No reset-release path exists yet. This must fail rather than use the
    // old single-controller StartCore or claim that reader/writer are live.
    bool StartCore(unsigned) { return false; }
    bool CoreReady(unsigned,const PioneerQwen35::DataflowWorkerSnapshot&) { return false; }
    bool StopAndHoldAll(unsigned& stage,unsigned& core) {
        // StartCore above never attempts reset. If this changes, replace this
        // branch with role-aware stop, PC/NoC audits and one-shot hold logic.
        return fContext.io.StopAndAuditAll(stage,core);
    }
    __attribute__((noinline)) bool RestoreRolesHeld() {
        if (fOwner.Untouched()) return true;
        auto* scope=Scope();
        if (!scope || !OpenForRecovery(*scope)) return false;
        Adapter io(fContext.io,*scope,fViews);
        if (fOwner.CoveredStateRestored()) return fOwner.VerifyAfterResidentRestore(io);
        return fOwner.RestoreAll(io);
    }
    __attribute__((noinline)) bool VerifyAfterResidentRestore() {
        if (fOwner.Untouched()) return true;
        auto* scope=Scope();
        if (!scope || !OpenForRecovery(*scope)) return false;
        Adapter io(fContext.io,*scope,fViews);
        return fOwner.CoveredStateRestored() && fOwner.VerifyAfterResidentRestore(io);
    }
    bool FinalizeBeforeResume() { return (fOwner.Untouched() || fOwner.CoveredStateRestored())
        ?fViews.Close():false; }
};
static_assert(alignof(PioneerQwen35SharedRoleBinding)<=alignof(void*),"shared role binding requires ordinary kernel allocation");

static PioneerQwen35SharedRoleBinding*
PioneerQ35CreateSharedRoleBinding(PioneerStoreContext& context)
{
    auto* binding=new(std::nothrow) PioneerQwen35SharedRoleBinding(context);
    if (binding && !binding->Valid()) { delete binding; binding=NULL; }
    return binding;
}
static bool
PioneerQ35DestroySharedRoleBinding(PioneerQwen35SharedRoleBinding* binding)
{
    if (!binding) return true;
    if (!binding->CanDestroy()) return false;
    delete binding; return true;
}
bool PioneerSessionIO::SharedCaptureAndStageHeld() {
    return qwen35DataflowContext && qwen35DataflowContext->qwen35SharedRoleBinding
        && qwen35DataflowContext->qwen35SharedRoleBinding->CaptureAndStageHeld();
}
bool PioneerSessionIO::SharedArmAndBeginRelease() {
    return qwen35DataflowContext && qwen35DataflowContext->qwen35SharedRoleBinding
        && qwen35DataflowContext->qwen35SharedRoleBinding->ArmAndBeginRelease();
}
bool PioneerSessionIO::SharedFleetHeldAndQuiescent() {
    return qwen35DataflowContext && qwen35DataflowContext->qwen35SharedRoleBinding
        && qwen35DataflowContext->qwen35SharedRoleBinding->FleetHeldAndQuiescent();
}
bool PioneerSessionIO::SharedStartCore(unsigned core) {
    return qwen35DataflowContext && qwen35DataflowContext->qwen35SharedRoleBinding
        && qwen35DataflowContext->qwen35SharedRoleBinding->StartCore(core);
}
bool PioneerSessionIO::SharedCoreReady(unsigned core,const PioneerQwen35::DataflowWorkerSnapshot& snapshot) {
    return qwen35DataflowContext && qwen35DataflowContext->qwen35SharedRoleBinding
        && qwen35DataflowContext->qwen35SharedRoleBinding->CoreReady(core,snapshot);
}
bool PioneerSessionIO::SharedStopAndHoldAll(unsigned& stage,unsigned& core) {
    return qwen35DataflowContext && qwen35DataflowContext->qwen35SharedRoleBinding
        && qwen35DataflowContext->qwen35SharedRoleBinding->StopAndHoldAll(stage,core);
}
bool PioneerSessionIO::SharedRestoreRolesHeld() {
    return qwen35DataflowContext && qwen35DataflowContext->qwen35SharedRoleBinding
        && qwen35DataflowContext->qwen35SharedRoleBinding->RestoreRolesHeld();
}
bool PioneerSessionIO::SharedVerifyRolesAfterResidentRestore() {
    return qwen35DataflowContext && qwen35DataflowContext->qwen35SharedRoleBinding
        && qwen35DataflowContext->qwen35SharedRoleBinding->VerifyAfterResidentRestore();
}
bool PioneerSessionIO::SharedFinalizeBeforeResume() {
    return qwen35DataflowContext && qwen35DataflowContext->qwen35SharedRoleBinding
        && qwen35DataflowContext->qwen35SharedRoleBinding->FinalizeBeforeResume();
}
