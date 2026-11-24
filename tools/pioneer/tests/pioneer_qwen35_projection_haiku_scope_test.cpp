/* SPDX-License-Identifier: MIT */
// Exercise the actual Haiku diagnostic scope over simulated session, bank and
// fleet proofs. Enabling a fake proof here does not enable it on the Pioneer.
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS 1
#define PIONEER_QWEN35_ROLE_WRITE_COMPILE_ONLY 1
#define PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS 1
#define PIONEER_QWEN35_PROJECTION_SCRATCH_MOCK_ONLY 1
#include "pioneer_qwen35_projection_scratch_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35ProjectionQualificationOwner.h"

static unsigned sOpenCookies=1;
static bool sColdRecoveryRequired=false,sReloadQuiesced=false,sBankStorageOnly=true;
struct PioneerStoreContext;
class PioneerQwen35RoleBorrowedScopeWitness;
struct PioneerSessionIO : BanksMock {
    PioneerStoreContext* qwen35DataflowContext=nullptr;
    PioneerQwen35RoleBorrowedScopeWitness* qwen35BorrowedRoleScope=nullptr;
    bool firmware=true,selectOkay=true,idle=true;unsigned selected=0,selects=0;
    unsigned failSelection=UINT32_MAX;
    unsigned failSelectionAfter=0;
    unsigned runtimeCore=0;
    bool SharedRoleFirmwareMatches() const { return firmware; }
    bool DataflowPanelMappingStable() const { return true; }
    bool SelectRuntimeCore(unsigned c) {
        selects++;
        if (!selectOkay || c>=120 || (c==failSelection && selects>failSelectionAfter)) return false;
        selected=runtimeCore=c;return true;
    }
    bool CoreIdle(unsigned c) const { return idle && c==0 && selected==0; }
};
class PioneerQwen35RoleBorrowedScopeWitness {
public:
    mutable bool exclusive=true;
    bool held=false,preparation=false,runtime=false,restored=false;
    bool inspectionOkay=true,inspectionRestoreFailed=false;
    unsigned inspections=0;
    unsigned deniedHeldCore=UINT32_MAX,loseIdentityCore=UINT32_MAX,deniedPreparationCore=UINT32_MAX;
    bool ProveExclusiveRewrite(PioneerSessionIO&) const { return exclusive; }
    bool InspectQualificationHeldFleet(PioneerSessionIO&,bool& restoreFailed) {
        ++inspections;restoreFailed=inspectionRestoreFailed;return inspectionOkay;
    }
    bool ProveRetainedOwnerIdentity(PioneerSessionIO&) const { return exclusive && sColdRecoveryRequired; }
    bool ProveRoleFleetHeldAndQuiet(PioneerSessionIO& io,unsigned c) const {
        if (c==loseIdentityCore) { exclusive=false;return false; }
        return held && io.selected==c && c!=deniedHeldCore;
    }
    bool ProveRoleStacklessFleetPreparationSafe(PioneerSessionIO& io,unsigned c) const {
        return preparation && io.selected==c && c!=deniedPreparationCore;
    }
    bool ProveRoleRuntimeSafe(PioneerSessionIO& io,unsigned c) const { return runtime && io.selected==c; }
    bool ProveRoleProjectionInFlightSafe(PioneerSessionIO&,const void*,const void*) const { return false; }
    bool ProveRoleAutoRestoreCompletionSafe(PioneerSessionIO& io,unsigned c) const { return restored && io.selected==c; }
};
template<class Work> static bool PioneerQwen35WithSelectedClock(PioneerSessionIO&,unsigned,Work work) { return work(); }
struct FakeDispatch { unsigned prepared=120;bool unsafe=false,closed=false,rootRunning=false; };
struct FakeRoleBinding { bool restored=false; };
static bool PioneerQ35SharedRoleProjectionRestored(const FakeRoleBinding* binding) {
    return binding && binding->restored;
}
struct PioneerStoreContext {
    PioneerQ35ProjectionQualificationOwner qwen35QualificationOwner;
    PioneerSessionIO io;
    tt_dram_samples_result dram;
    tt_compute_noc_result noc;
    PioneerResidentBanks banks;
    struct InnerC { tt_dram_samples_result& dram; };
    struct InnerB { InnerC baseline;tt_compute_noc_result& noc; };
    struct InnerA { InnerB baseline; };
    struct Baseline { InnerA baseline; } baseline;
    std::vector<uint8_t> initializedStorage=std::vector<uint8_t>(PioneerResidentBanks::Pages,0);
    uint8_t* initialized=nullptr;
    PioneerResidentLease lease;tt_host_page_result host{};
    FakeDispatch dispatch;FakeDispatch* q38Dispatch=&dispatch;
    bool q38DispatchRoutes=true,bankStorageOnly=true,qwen35QualificationInternalActive=false;
    uint64_t bytes=PioneerResidentBanks::Total;
    void* qwen35DataflowAuthority=nullptr;void* qwen35DataflowProgramScratch=nullptr;void* qwen35GraphOwner=nullptr;
    PioneerQwen35::DataflowServiceOwner* qwen35DataflowService=nullptr;
    FakeRoleBinding* qwen35SharedRoleBinding=nullptr;
    bool qwen35DataflowRoutesAudited=false,otherUnsafe=false;
    unsigned qwen35DataflowRoutes[2][PioneerQwen35::Banks]{};
    PioneerStoreContext():dram(BankProof(io)),noc(io.Compute()),baseline{{{{dram},noc}}} {
        initialized=initializedStorage.data();io.qwen35DataflowContext=this;
        assert(banks.Bind(dram) && Prepared(io,host,lease));
    }
    bool Unsafe(bool controlledGraph=false) const {
        return otherUnsafe || (qwen35QualificationOwner.Active() && !qwen35QualificationInternalActive)
            || (qwen35DataflowService && qwen35DataflowService->Started()
                && !qwen35DataflowService->Closed() && !controlledGraph);
    }
#include "pioneer_q35_projection_internal_scope.inc"
#include "pioneer_q35_projection_scratch_operation.inc"
#include "pioneer_q35_projection_abandon.inc"
};
static bool PioneerQwen35DataflowAuditProjectionReceipts(PioneerSessionIO&,
    const PioneerQwen35::DataflowProjectionProgram&,unsigned,
    const PioneerQwen35::DataflowWorkerSnapshot (&)[PioneerQwen35::DataflowCoreCount]) { return true; }
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35ProjectionQualificationScope.inc"

int main() {
    (void)&Input; (void)&Prepare; (void)&Upload; (void)&Close;
    {
        PioneerStoreContext context;PioneerQwen35RoleBorrowedScopeWitness borrowed;
        PioneerQ35ProjectionQualificationScope scope(context);
        assert(!scope.QualificationOwnerAllocationAllowed(context));
        context.io.qwen35BorrowedRoleScope=&borrowed;
        borrowed.exclusive=false;assert(!scope.QualificationOwnerAllocationAllowed(context));
        borrowed.exclusive=true;sOpenCookies=2;assert(!scope.QualificationOwnerAllocationAllowed(context));
        sOpenCookies=1;context.io.firmware=false;assert(!scope.QualificationOwnerAllocationAllowed(context));
        context.io.firmware=true;assert(scope.QualificationOwnerAllocationAllowed(context));
        assert(context.qwen35QualificationOwner.Allocate());
        auto* scratch=context.qwen35QualificationOwner.Scratch();assert(scratch);
        auto* qualification=context.qwen35QualificationOwner.Qualification();assert(qualification);
        assert(!scope.QualificationOwnerPrepareRetirementAllowed(context.qwen35QualificationOwner,*scratch,*qualification));
        PioneerQwen35::DataflowServiceOwner unstartedService;
        assert(!scope.QualificationHoldAllowed(scratch,unstartedService)
            && !scope.QualificationHeldAllowed(scratch,unstartedService)
            && !scope.QualificationResumeAllowed(scratch,unstartedService));
        assert(!context.DataflowQualificationScratchOperation(PioneerQ35ProjectionQualificationOwner::Capture,scope)
            && !context.qwen35QualificationInternalActive && scratch->IsEmpty()
            && !context.banks.Q35StreamClaimed());
        context.qwen35QualificationInternalActive=true;
        // The real borrowed scope currently denies held/quiet proof. The
        // diagnostic must refuse before it reserves or modifies a page.
        assert(!scope.QualificationScratchOperationAllowed(context.qwen35QualificationOwner,*scratch,
            PioneerQ35ProjectionQualificationOwner::Capture,context));
        assert(!scope.ScratchReserveAllowed(scratch,0,Fixture::ScratchOffset,Fixture::ScratchBytes)
            && !context.banks.Q35StreamClaimed());
        borrowed.held=true; // simulated proof only
        assert(scope.QualificationScratchOperationAllowed(context.qwen35QualificationOwner,*scratch,
            PioneerQ35ProjectionQualificationOwner::Capture,context));
        assert(scope.ScratchReserveAllowed(scratch,0,Fixture::ScratchOffset,Fixture::ScratchBytes));
        assert(!scope.ScratchReserveAllowed(scratch,0,0,Fixture::ScratchBytes));
        PioneerQ35StreamBankRange ranges[PioneerQwen35::DataflowCoreCount]{};
        for (unsigned n=0;n<PioneerQwen35::DataflowCoreCount;n++) {
            const unsigned pages=n+1==PioneerQwen35::DataflowCoreCount
                ?Fixture::ScratchPages-(PioneerQwen35::DataflowCoreCount-1):1;
            ranges[n]={0,Fixture::ScratchOffset+n*4096,pages*4096};
        }
        assert(context.banks.ReserveQ35Stream(context.dram,scratch,ranges,
            PioneerQwen35::DataflowCoreCount,context.initialized,PioneerResidentBanks::Pages));
        assert(scope.ScratchTransferAllowed(scratch,0,Fixture::ScratchOffset,false)
            && !scope.ScratchTransferAllowed(scratch,0,0,false));
        assert(!scope.ScratchRunAllowed(scratch) && !scope.ScratchStopped(scratch));
        borrowed.exclusive=false;
        assert(!scope.ScratchTransferAllowed(scratch,0,Fixture::ScratchOffset,true));
        borrowed.exclusive=true;
        // Retention must quarantine even after the holder was already marked
        // retained by another part of the same failure path.
        context.qwen35QualificationOwner.Retain();scope.RetainScratch(scratch);
        assert(context.qwen35QualificationOwner.Retained() && context.banks.Quarantined()
            && context.lease.recovery_required && context.io.HostReadMock::quarantined && sColdRecoveryRequired);
        assert(!scope.ScratchRetainedObservationAllowed(scratch,context.io));
        context.lease.pending=true;
        assert(scope.ScratchRetainedObservationAllowed(scratch,context.io));
        borrowed.exclusive=false;assert(!scope.ScratchRetainedObservationAllowed(scratch,context.io));borrowed.exclusive=true;
        context.dispatch.unsafe=true;assert(!scope.ScratchRetainedObservationAllowed(scratch,context.io));
        context.dispatch.unsafe=false;
        context.dispatch.rootRunning=true;assert(!scope.ScratchRetainedObservationAllowed(scratch,context.io));
        context.dispatch.rootRunning=false;
        context.qwen35QualificationInternalActive=false;
        assert(!scope.ScratchRetainedObservationAllowed(scratch,context.io));
        context.qwen35QualificationInternalActive=true;
        context.io.active=true;assert(!scope.ScratchRetainedObservationAllowed(scratch,context.io));context.io.active=false;
        assert(scope.ScratchRetainedObservationAllowed(scratch,context.io));
    }
    sColdRecoveryRequired=false;
    {
        PioneerStoreContext context;PioneerQwen35RoleBorrowedScopeWitness borrowed;
        borrowed.held=true;borrowed.deniedHeldCore=7;context.io.qwen35BorrowedRoleScope=&borrowed;
        PioneerQ35ProjectionQualificationScope scope(context);
        assert(context.qwen35QualificationOwner.Allocate());
        auto* scratch=context.qwen35QualificationOwner.Scratch();
        context.qwen35QualificationInternalActive=true;
        assert(!scope.QualificationScratchOperationAllowed(context.qwen35QualificationOwner,*scratch,
            PioneerQ35ProjectionQualificationOwner::Capture,context));
        assert(context.io.runtimeCore==0 && context.io.selected==0 && !sColdRecoveryRequired
            && !context.qwen35QualificationOwner.Retained() && !context.banks.Quarantined());
    }
    for (unsigned failureMode=0;failureMode<2;failureMode++) {
        PioneerStoreContext context;PioneerQwen35RoleBorrowedScopeWitness borrowed;
        borrowed.held=true;context.io.qwen35BorrowedRoleScope=&borrowed;
        PioneerQ35ProjectionQualificationScope scope(context);
        assert(context.qwen35QualificationOwner.Allocate());
        auto* scratch=context.qwen35QualificationOwner.Scratch();
        context.qwen35QualificationInternalActive=true;
        // Fail during the sweep, or only when restoring worker zero.
        context.io.failSelection=failureMode?0:7;
        context.io.failSelectionAfter=failureMode?120:0;
        assert(!scope.QualificationScratchOperationAllowed(context.qwen35QualificationOwner,*scratch,
            PioneerQ35ProjectionQualificationOwner::Capture,context));
        assert(sColdRecoveryRequired && context.qwen35QualificationOwner.Retained()
            && context.banks.Quarantined() && context.lease.recovery_required
            && context.io.HostReadMock::quarantined);
        assert(context.io.selects==(failureMode?121U:8U)
            && context.io.runtimeCore==(failureMode?119U:6U));
        sColdRecoveryRequired=false;
    }
    {
        PioneerStoreContext context;PioneerQwen35RoleBorrowedScopeWitness borrowed;
        borrowed.held=true;borrowed.loseIdentityCore=7;context.io.qwen35BorrowedRoleScope=&borrowed;
        PioneerQ35ProjectionQualificationScope scope(context);
        assert(context.qwen35QualificationOwner.Allocate());
        auto* scratch=context.qwen35QualificationOwner.Scratch();
        context.qwen35QualificationInternalActive=true;
        assert(!scope.QualificationScratchOperationAllowed(context.qwen35QualificationOwner,*scratch,
            PioneerQ35ProjectionQualificationOwner::Capture,context));
        assert(context.io.runtimeCore==7 && context.io.selects==8 && sColdRecoveryRequired
            && context.qwen35QualificationOwner.Retained() && context.banks.Quarantined());
        sColdRecoveryRequired=false;
    }
    {
        PioneerStoreContext context;PioneerQwen35RoleBorrowedScopeWitness borrowed;
        context.io.qwen35BorrowedRoleScope=&borrowed;
        PioneerQ35ProjectionQualificationScope scope(context);
        assert(scope.QualificationOwnerAllocationAllowed(context) && context.qwen35QualificationOwner.Allocate());
        context.qwen35QualificationInternalActive=true;
        assert(context.qwen35QualificationOwner.AbandonUnclaimed(scope,context.banks));
    }
    {
        PioneerStoreContext context;PioneerQwen35RoleBorrowedScopeWitness borrowed;
        borrowed.held=true;context.io.qwen35BorrowedRoleScope=&borrowed; // simulated proof only
        PioneerQ35ProjectionQualificationScope scope(context);
        assert(scope.QualificationOwnerAllocationAllowed(context) && context.qwen35QualificationOwner.Allocate());
        for (unsigned n=0;n<Fixture::ScratchPages;n++) {
            auto& page=context.io.memory[context.io.Key(18|(12<<6),Fixture::ScratchAddress+n*4096)];
            for (unsigned i=0;i<1024;i++) page[i]=0x75000000U^(n*65537U+i*13U);
        }
        assert(context.DataflowQualificationScratchOperation(PioneerQ35ProjectionQualificationOwner::Capture,scope));
        assert(context.qwen35QualificationOwner.Scratch()->IsCaptured());
        assert(context.AbandonDataflowQualification(scope) && context.qwen35QualificationOwner.Retired()
            && !context.qwen35QualificationInternalActive && context.banks.Q35StreamClaimed());
        for (unsigned n=0;n<Fixture::ScratchPages;n++) {
            const auto& page=context.io.memory.at(context.io.Key(18|(12<<6),Fixture::ScratchAddress+n*4096));
            for (unsigned i=0;i<1024;i++) assert(page[i]==(0x75000000U^(n*65537U+i*13U)));
        }
    }
    {
        PioneerStoreContext context;PioneerQwen35RoleBorrowedScopeWitness borrowed;
        borrowed.held=true; // simulated fleet proof; production still denies it
        context.io.qwen35BorrowedRoleScope=&borrowed;
        PioneerQ35ProjectionQualificationScope scope(context);
        assert(scope.QualificationOwnerAllocationAllowed(context) && context.qwen35QualificationOwner.Allocate());
        for (unsigned n=0;n<Fixture::ScratchPages;n++) {
            auto& page=context.io.memory[context.io.Key(18|(12<<6),Fixture::ScratchAddress+n*4096)];
            for (unsigned i=0;i<1024;i++) page[i]=0x63000000U^(n*104729U+i*17U);
        }
        assert(context.DataflowQualificationScratchOperation(PioneerQ35ProjectionQualificationOwner::Capture,scope)
            && context.DataflowQualificationScratchOperation(PioneerQ35ProjectionQualificationOwner::Stage,scope));
        auto* scratch=context.qwen35QualificationOwner.Scratch();assert(scratch && scratch->IsStaged());
        for (unsigned n=0;n<2;n++) for (unsigned b=0;b<PioneerQwen35::Banks;b++)
            context.qwen35DataflowRoutes[n][b]=context.dram.translated[b][0]|context.dram.translated[b][1]<<6;
        context.qwen35DataflowRoutesAudited=true;context.qwen35QualificationInternalActive=true;
        borrowed.inspectionOkay=false;
        assert(!scope.QualificationInstallationAllowed(*scratch,context)
            && borrowed.inspections==1 && !context.qwen35QualificationOwner.Retained());
        borrowed.inspectionOkay=true;
        borrowed.preparation=true;borrowed.deniedPreparationCore=7;
        assert(!scope.QualificationInstallationAllowed(*scratch,context)
            && context.io.runtimeCore==0 && context.io.selected==0
            && borrowed.inspections==2 && !context.qwen35QualificationOwner.Retained() && !sColdRecoveryRequired);
        borrowed.deniedPreparationCore=UINT32_MAX;
        assert(scope.QualificationInstallationAllowed(*scratch,context) && borrowed.inspections==3);
        auto* qualification=context.qwen35QualificationOwner.Qualification();
        assert(qualification && qualification->Bind(*scratch,scope,0,Fixture::ScratchAddress,
            context.qwen35DataflowRoutes) && qualification->IsUnstarted());
        context.qwen35QualificationInternalActive=false;
        assert(!context.qwen35QualificationOwner.AbandonRestoredUnrun(scope,context.banks,context.dram));
        assert(context.DataflowQualificationScratchOperation(PioneerQ35ProjectionQualificationOwner::Restore,scope)
            && scratch->IsRestored());
        assert(context.AbandonDataflowQualification(scope) && !context.qwen35QualificationInternalActive);
        assert(context.qwen35QualificationOwner.Retired() && !context.Unsafe() && context.banks.Q35StreamClaimed());
        for (unsigned n=0;n<Fixture::ScratchPages;n++) {
            const auto& page=context.io.memory.at(context.io.Key(18|(12<<6),Fixture::ScratchAddress+n*4096));
            for (unsigned i=0;i<1024;i++) assert(page[i]==(0x63000000U^(n*104729U+i*17U)));
        }
    }
    {
        PioneerStoreContext context;PioneerQwen35RoleBorrowedScopeWitness borrowed;
        borrowed.held=true;context.io.qwen35BorrowedRoleScope=&borrowed;
        PioneerQ35ProjectionQualificationScope scope(context);
        assert(scope.QualificationOwnerAllocationAllowed(context) && context.qwen35QualificationOwner.Allocate());
        for (unsigned n=0;n<Fixture::ScratchPages;n++)
            context.io.memory[context.io.Key(18|(12<<6),Fixture::ScratchAddress+n*4096)];
        assert(context.DataflowQualificationScratchOperation(PioneerQ35ProjectionQualificationOwner::Capture,scope)
            && context.DataflowQualificationScratchOperation(PioneerQ35ProjectionQualificationOwner::Stage,scope));
        auto* scratch=context.qwen35QualificationOwner.Scratch();assert(scratch);
        context.qwen35DataflowRoutesAudited=true;context.qwen35QualificationInternalActive=true;
        borrowed.inspectionOkay=false;borrowed.inspectionRestoreFailed=true;
        assert(!scope.QualificationInstallationAllowed(*scratch,context) && borrowed.inspections==1
            && context.qwen35QualificationOwner.Retained() && context.banks.Quarantined()
            && context.lease.recovery_required && sColdRecoveryRequired);
        context.qwen35QualificationInternalActive=false;
    }
    sColdRecoveryRequired=false;
    for (unsigned closed=0;closed<2;closed++) {
        PioneerStoreContext context;PioneerQwen35RoleBorrowedScopeWitness borrowed;
        context.io.qwen35BorrowedRoleScope=&borrowed;
        PioneerQ35ProjectionQualificationScope scope(context);
        assert(context.qwen35QualificationOwner.Allocate());
        auto* scratch=context.qwen35QualificationOwner.Scratch();
        PioneerQwen35::DataflowServiceOwner unstarted;
        context.qwen35DataflowService=&unstarted;context.qwen35QualificationInternalActive=true;
        context.io.runtimeCore=context.io.selected=7;
        const bool result=closed?scope.QualificationRestoreClosedRoute(*scratch,unstarted,context.io)
            :scope.QualificationRestoreHeldRoute(scratch,unstarted,context.io);
        assert(!result && !context.io.selects && context.io.runtimeCore==7
            && context.qwen35QualificationOwner.Retained() && sColdRecoveryRequired);
        sColdRecoveryRequired=false;
    }
    std::puts("PASS actual Haiku diagnostic scope identity, fleet-proof denial and retained failure; SIMULATED ONLY");
    return 0;
}
