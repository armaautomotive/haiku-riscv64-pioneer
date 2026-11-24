/* SPDX-License-Identifier: MIT */
// Concrete fleet PCI adapter and real Haiku mapper over simulated MMIO.
#define PIONEER_QWEN35_ROLE_WRITE_COMPILE_ONLY 1
#include <cstring>
#define main qwen35_haiku_scope_test_main
#include "pioneer_qwen35_dataflow_role_haiku_scope_test.cpp"
#undef main
#ifdef PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS
static bool PioneerQ35SharedRoleCopiesCaptured(const PioneerQwen35SharedRoleBinding*,unsigned) { return false; }
#endif

struct FleetPciTestScope {
    bool exclusive=true,held=true,routing=true,stable=true,read=true,write=true,control=true,release=false,stop=false,park=false;
    bool computeStop=false,computePark=false,hold=false,heldAfter=false,releaseDrained=false;
    bool runtime=false;
    bool stackCancel=false,stackCancelled=false;
    bool stackReleaseDrained=false;
    bool stacklessSnapshot=false;
    bool heldCopyProof=false,heldCopyCaptured=false,heldCopyDrift=false;
    bool coreRestoreProof=false;
#ifdef PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS
    bool autoRestoreCompletion=false,autoCancelHeld=false,autoRetirement=false;
    bool expectAutoCancelAttempt=false,expectAutoRetirementCancelled=false;
    bool ProveRoleAutoRestoreCompletionSafe(PioneerSessionIO&,unsigned) const { return autoRestoreCompletion; }
    bool ProveRoleAutoCancelHeldSafe(PioneerSessionIO&,unsigned,const uint32_t* reader,
        const uint32_t* writer,bool attempted) const {
        return autoCancelHeld && reader==heldReader && writer==heldWriter && attempted==expectAutoCancelAttempt;
    }
    bool ProveRoleAutoRestoreRetirementSafe(PioneerSessionIO&,unsigned,const uint32_t* reader,
        const uint32_t* writer,bool cancelled) const {
        return autoRetirement && reader==heldReader && writer==heldWriter && cancelled==expectAutoRetirementCancelled;
    }
#endif
    bool restoreHeld=false,restoreCompletion=false,restoreCompletedHeld=false,restoreRetry=false,restoreRelease=false;
    bool restorePhysicalRelease=false,restorePhysicalHold=false,restoreHeldAfter=false;
    bool retirementProof=false,retirementRetry=false;
    const uint32_t* heldReader=nullptr;const uint32_t* heldWriter=nullptr;
    mutable unsigned heldCopyChecks=0;
    mutable unsigned stackDrainChecks=0;
    unsigned stackDrainFailAt=UINT32_MAX;
    bool imageInspection=false;
    bool faultAudit=false,faultAck=false;
#ifdef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
    bool dedicatedRelease=false;
    bool dedicatedObservedStage=false;
    bool dedicatedHeldStatus=true;
    bool CanInspectDedicatedRuntime(PioneerSessionIO&,unsigned core) const {
        return runtime && core<DataflowBarrierWorkers;
    }
    bool ProveRoleDedicatedObservedStage(PioneerSessionIO&) const { return dedicatedObservedStage; }
    bool ProveRoleDedicatedHeldStatusAllowed(PioneerSessionIO&,unsigned core) const {
        return dedicatedHeldStatus && core<DataflowBarrierWorkers;
    }
    mutable unsigned dedicatedChecks=0;
    unsigned dedicatedFailAt=UINT32_MAX;
    unsigned dedicatedDisableClockAt=UINT32_MAX;
    bool CanAttemptDedicatedObservedRelease(PioneerSessionIO& io,unsigned) const {
        const unsigned check=dedicatedChecks++;
        if (check==dedicatedDisableClockAt) io.nocConfig[0]|=1U<<12;
        return dedicatedRelease && check!=dedicatedFailAt;
    }
#endif
    bool ProveExclusiveRewrite(PioneerSessionIO&) const { return gLocked && exclusive; }
    bool ProveRoleMemoryReadSafe(PioneerSessionIO&,unsigned,unsigned) const { return held && stable && read; }
    bool ProveRoleMemoryWriteSafe(PioneerSessionIO&,unsigned,unsigned,unsigned,unsigned) const
        { return held && stable && write; }
    bool ProveRoleControlWriteSafe(PioneerSessionIO&,unsigned,uint32_t,uint32_t) const
        { return held && stable && control; }
    bool ProveRoleFleetHeldAndQuiet(PioneerSessionIO&,unsigned) const { return held; }
    bool ProveRoleBootRoutingInspectionAllowed(PioneerSessionIO&,unsigned) const { return held && routing; }
    bool ProveRoleLocalRamStable(PioneerSessionIO&,unsigned) const { return held && stable; }
    bool ProveRoleStartupHandoffSafe(PioneerSessionIO&,unsigned) const { return runtime; }
    bool ProveRoleStacklessSnapshotSafe(PioneerSessionIO&,unsigned) const { return stacklessSnapshot; }
    bool ProveRoleStacklessFleetPreparationSafe(PioneerSessionIO&,unsigned) const { return false; }
    bool ProveRoleUncapturedRamProtected(PioneerSessionIO&,unsigned) const { return false; }
    bool ProveRoleHeldCopySources(PioneerSessionIO&,unsigned,const uint32_t*& reader,
        const uint32_t*& writer,bool& captured) const {
        if (!heldCopyProof) return false;
        captured=heldCopyCaptured;reader=heldReader;writer=heldWriter;
        if (heldCopyDrift && heldCopyChecks++==1) reader=nullptr;
        return true;
    }
    bool ProveRoleCoreRestore(PioneerSessionIO&,unsigned,const uint32_t* reader,const uint32_t* writer) const {
        return coreRestoreProof && reader==heldReader && writer==heldWriter;
    }
    bool ProveRoleRestoreHandoffHeldSafe(PioneerSessionIO&,unsigned) const { return restoreHeld; }
    bool ProveRoleRestoreCompletionSafe(PioneerSessionIO&,unsigned) const { return restoreCompletion; }
    bool ProveRoleRestoreCompletedAndHeld(PioneerSessionIO&,unsigned) const { return restoreCompletedHeld; }
    bool ProveRoleRestoreHandoffRetrySafe(PioneerSessionIO&,unsigned) const { return restoreRetry; }
    bool ProveRoleRestoreReleaseReady(PioneerSessionIO&,unsigned,const uint32_t* reader,const uint32_t* writer) const {
        return restoreRelease && reader==heldReader && writer==heldWriter;
    }
    bool ProveRoleRestorePhysicalReleaseSafe(PioneerSessionIO&,unsigned) const { return restorePhysicalRelease; }
    bool ProveRoleRestorePhysicalHoldSafe(PioneerSessionIO&,unsigned) const { return restorePhysicalHold; }
    bool ProveRoleRestoreHeldAfterAttempt(PioneerSessionIO&,unsigned) const { return restoreHeldAfter; }
    bool ProveRoleRetirementRamSources(PioneerSessionIO&,unsigned,const uint32_t*& reader,
        const uint32_t*& writer,bool& captured) const {
        if (!retirementProof) return false;
        reader=heldReader;writer=heldWriter;captured=heldCopyCaptured;return true;
    }
    bool ProveRoleFleetImageRetirementSafe(PioneerSessionIO&) const { return retirementProof; }
    bool ProveRoleFleetImageRetirementRetrySafe(PioneerSessionIO&) const { return retirementRetry; }
    bool ProveRoleCoreImageRetirementSafe(PioneerSessionIO&,unsigned,const uint32_t* reader,const uint32_t* writer) const {
        return retirementProof && reader==heldReader && writer==heldWriter;
    }
    bool ProveRoleCoreImageRetirementRetrySafe(PioneerSessionIO&,unsigned,const uint32_t*,const uint32_t*) const {
        return retirementRetry;
    }
    bool ProveRoleStackReleaseWriteDrained(PioneerSessionIO&,unsigned) const {
        return stackReleaseDrained && stackDrainChecks++!=stackDrainFailAt;
    }
    bool ProveRoleStackCancelSafe(PioneerSessionIO&,unsigned) const { return stackCancel; }
    bool ProveRoleStackCancelledSafe(PioneerSessionIO&,unsigned) const { return stackCancelled; }
    bool ProveRoleReleaseSafe(PioneerSessionIO&,unsigned) const { return release; }
    bool ProveRoleReleaseWriteDrained(PioneerSessionIO&,unsigned) const { return releaseDrained; }
    bool ProveRoleStopSafe(PioneerSessionIO&,unsigned,bool) const { return stop; }
    bool ProveRolePostStopSafe(PioneerSessionIO&,unsigned) const { return park; }
    bool ProveComputeStopSafe(PioneerSessionIO&,unsigned) const { return computeStop; }
    bool ProveComputePostStopSafe(PioneerSessionIO&,unsigned) const { return computePark; }
    bool ProveRoleHoldSafe(PioneerSessionIO&,unsigned) const { return hold; }
    bool ProveRoleHeldAfterAttempt(PioneerSessionIO&,unsigned) const { return heldAfter; }
    bool ProveRoleRuntimeSafe(PioneerSessionIO&,unsigned) const { return runtime; }
    bool ProveRoleImageInspectionAllowed(PioneerSessionIO& io,unsigned core) const
        { return imageInspection && io.runtimeCore==core; }
    bool ProveRoleFaultAuditSafe(PioneerSessionIO&,unsigned) const { return faultAudit; }
    bool ProveRoleFaultAckSafe(PioneerSessionIO&,unsigned) const { return faultAck; }
};
struct DriftViews {
    DataflowRoleWritableViews<PioneerQwen35RoleHaikuMap>& views;
    unsigned reads=0;
    template<class Guard>
    bool Read(unsigned core,unsigned which,unsigned offset,uint32_t& value,Guard& guard) {
        if (!views.Read(core,which,offset,value,guard)) return false;
        if (reads++==0) gMemory[DataflowRolePhysicalControl][0x234/4]^=1U;
        return true;
    }
};
struct CountingCopyViews {
    DataflowRoleWritableViews<PioneerQwen35RoleHaikuMap>& views;
    unsigned copies=0,reads=0;
    template<class Guard>
    bool CopyWords(unsigned core,unsigned which,unsigned offset,uint32_t* output,unsigned words,Guard& guard) {
        ++copies;return views.CopyWords(core,which,offset,output,words,guard);
    }
    template<class Guard>
    bool Read(unsigned core,unsigned which,unsigned offset,uint32_t& value,Guard& guard) {
        ++reads;return views.Read(core,which,offset,value,guard);
    }
};
struct ChangingImageViews {
    DataflowRoleWritableViews<PioneerQwen35RoleHaikuMap>& views;
    unsigned matches=0;
    bool Opened() const { return views.Opened(); }
    template<class Guard>
    bool Matches(unsigned core,unsigned which,unsigned offset,const uint32_t* expected,
        unsigned words,Guard& guard) {
        if (!views.Matches(core,which,offset,expected,words,guard)) return false;
        if (++matches==6) gMemory[DataflowRolePhysicalExtraCode][0]^=1;
        return true;
    }
};
struct ChangingRamCopyViews {
    DataflowRoleWritableViews<PioneerQwen35RoleHaikuMap>& views;
    unsigned matches=0;
    bool Opened() const { return views.Opened(); }
    template<class Guard>
    bool Matches(unsigned core,unsigned which,unsigned offset,const uint32_t* expected,
        unsigned words,Guard& guard) {
        if (!views.Matches(core,which,offset,expected,words,guard)) return false;
        if (++matches==2)
            gMemory[DataflowRolePhysicalExtraCode][(DataflowRoleReaderRamCopy-DataflowRoleExtraL1Address)/4]^=1;
        return true;
    }
};
int main()
{
#ifdef PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS
    (void)&Qwen35DataflowGenerate;
#endif
    PioneerStoreContext context; FakeService service; FakeGraph graph;
    context.qwen35DataflowService=&service; context.qwen35GraphOwner=&graph;
    PioneerProbeCookie cookie; cookie.context=&context; sRuntimeOwner=&cookie;
    ResetMaps(); gAllowWrite=true; assert(AcquireProbeLock(&cookie,true)==B_OK);
    FleetPciTestScope scope;
    PioneerQwen35RoleHaikuMap platform;
    DataflowRoleWritableViews<PioneerQwen35RoleHaikuMap> views(platform);
    PioneerQwen35RoleRouteGuard<PioneerSessionIO,FleetPciTestScope> guard(context.io,scope);
    assert(views.Open(context.io.profile.bar0_host_start,context.io.profile.bar0_host_end,guard));
    PioneerQwen35RoleFleetPciIO<PioneerSessionIO,FleetPciTestScope,decltype(views)> io(context.io,scope,views);
    CountingCopyViews counted{views};
    PioneerQwen35RoleFleetPciIO<PioneerSessionIO,FleetPciTestScope,CountingCopyViews> countedIo(context.io,scope,counted);
    uint32_t copied[4]{};
    assert(countedIo.Read(7,DataflowRoleExtraL1Address,copied,sizeof(copied))
        && counted.copies==1 && counted.reads==0);
    assert(countedIo.ReadRam(7,DataflowRoleReader,DataflowRoleBriscLocalRam,copied,sizeof(copied))
        && counted.copies==1 && counted.reads==4);
    for (unsigned core=0;core<DataflowBarrierWorkers;core++)
        assert(io.AllRolesHeld(core) && io.DefaultBootRouting(core) && io.LocalRamStable(core));
    {
        const uint32_t readerWords[]={1},writerWords[]={2};
        const uint32_t* reader=readerWords;const uint32_t* writer=writerWords;bool captured=true;
        assert(!io.StacklessHeldCopySources(7,reader,writer,captured)
            && !reader && !writer && !captured);
        scope.heldCopyProof=true;
        assert(io.StacklessHeldCopySources(7,reader,writer,captured)
            && !reader && !writer && !captured);
        scope.heldCopyCaptured=true;scope.heldReader=readerWords;scope.heldWriter=writerWords;
        assert(io.StacklessHeldCopySources(7,reader,writer,captured)
            && captured && reader==readerWords && writer==writerWords);
        assert(io.StacklessHeldCopySourceSafe(7,readerWords,writerWords));
        assert(!io.StacklessHeldCopySourceSafe(7,writerWords,readerWords));
        const uint32_t* retiredReader=readerWords;const uint32_t* retiredWriter=writerWords;bool retiredCaptured=true;
        assert(!io.RetirementRamSnapshotSources(7,retiredReader,retiredWriter,retiredCaptured)
            && !retiredReader && !retiredWriter && !retiredCaptured && !io.FleetRamImageRetirementSafe());
        scope.retirementProof=true;
        assert(io.RetirementRamSnapshotSources(7,retiredReader,retiredWriter,retiredCaptured)
            && retiredCaptured && retiredReader==readerWords && retiredWriter==writerWords
            && io.CoreRamImageRetirementSafe(7,readerWords,writerWords)
            && !io.CoreRamImageRetirementSafe(7,writerWords,readerWords)
            && !io.CoreRamImageRetirementRetrySafe(7,readerWords,writerWords));
        scope.retirementRetry=true;
        assert(io.CoreRamImageRetirementRetrySafe(7,readerWords,writerWords));
#ifdef PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS
        assert(!io.AutoRestoreCancelHeldSafe(7,readerWords,writerWords,false));
        scope.autoCancelHeld=true;
        assert(io.AutoRestoreCancelHeldSafe(7,readerWords,writerWords,false)
            && !io.AutoRestoreCancelHeldSafe(7,readerWords,writerWords,true)
            && !io.AutoRestoreCancelHeldSafe(7,writerWords,readerWords,false));
        scope.expectAutoCancelAttempt=true;
        assert(io.AutoRestoreCancelHeldSafe(7,readerWords,writerWords,true));
        assert(!io.AutoRestoreRetirementSafe(7,readerWords,writerWords,true));
        scope.autoRetirement=true;scope.expectAutoRetirementCancelled=true;
        assert(io.AutoRestoreRetirementSafe(7,readerWords,writerWords,true)
            && !io.AutoRestoreRetirementSafe(7,readerWords,writerWords,false)
            && !io.AutoRestoreRetirementSafe(7,writerWords,readerWords,true));
        scope.autoCancelHeld=scope.autoRetirement=false;
#endif
        scope.retirementProof=scope.retirementRetry=false;
        assert(!io.StacklessCoreRestoreReleaseReady(7,readerWords,writerWords));
        DataflowRoleSnapshotHandoff restoreHandoff{9,9,9};
        const unsigned initialWrites=context.io.restoreReceiptWrites;
        const unsigned initialStackCancelWrites=context.io.stackCancelWrites;
        assert(!io.ReadRestoreHandoff(7,restoreHandoff)
            && !io.WriteRestoreHandoffWord(7,0,0)
            && context.io.restoreReceiptWrites==initialWrites);
        scope.restoreHeld=true;
        context.io.roleHandoffs[7]={11,22,33};
        assert(io.ReadRestoreHandoff(7,restoreHandoff)
            && restoreHandoff.readerReady==11 && restoreHandoff.writerReady==22 && restoreHandoff.release==33);
        context.io.dropRestoreReceiptWrite=true;
        assert(!io.WriteRestoreHandoffWord(7,0,0));
        context.io.dropRestoreReceiptWrite=false;
        assert(io.WriteRestoreHandoffWord(7,0,0) && io.WriteRestoreHandoffWord(7,1,0)
            && io.WriteRestoreHandoffWord(7,2,0) && !io.WriteRestoreHandoffWord(7,3,0));
        assert(!io.RestoreHandoffRetrySafe(7) && !io.RestoreCompletedAndHeld(7));
        scope.restoreRetry=scope.restoreCompletedHeld=scope.restoreRelease=true;
#ifdef PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS
        scope.restorePhysicalRelease=true;
#endif
        assert(io.RestoreHandoffRetrySafe(7) && io.RestoreCompletedAndHeld(7)
            && io.StacklessCoreRestoreReleaseReady(7,readerWords,writerWords));
        context.io.roleHandoffs[7].readerReady=1;
        assert(!io.StacklessCoreRestoreReleaseReady(7,readerWords,writerWords));
        context.io.handoffChangeAt=context.io.handoffReads+1;
        assert(!io.ReadRestoreHandoff(7,restoreHandoff));
        context.io.handoffChangeAt=UINT32_MAX;
        context.io.reset=0x7000;scope.restoreCompletion=true;
        context.io.roleHandoffs[7]={PIONEER_Q35_ROLE_RESTORE_READER_DONE,PIONEER_Q35_ROLE_RESTORE_WRITER_DONE,0};
        assert(!io.RestoreHandoffHeldSafe(7) && io.RestoreCompletionSafe(7)
            && io.ReadRestoreHandoff(7,restoreHandoff)
            && restoreHandoff.readerReady==PIONEER_Q35_ROLE_RESTORE_READER_DONE);
        context.io.reset=0x47800;scope.restoreHeld=scope.restoreCompletion=scope.restoreCompletedHeld=false;
        scope.restoreRetry=scope.restoreRelease=false;
        scope.restorePhysicalRelease=false;
        context.io.stackCancelWrites=initialStackCancelWrites;
        assert(!io.StacklessCoreRestoreProven(7,readerWords,writerWords));
        scope.coreRestoreProof=true;
        assert(io.StacklessCoreRestoreProven(7,readerWords,writerWords)
            && !io.StacklessCoreRestoreProven(7,writerWords,readerWords));
        scope.coreRestoreProof=false;
        scope.heldCopyDrift=true;scope.heldCopyChecks=0;
        assert(!io.StacklessHeldCopySources(7,reader,writer,captured)
            && !reader && !writer && !captured);
        scope.heldCopyProof=false;scope.heldCopyCaptured=false;scope.heldCopyDrift=false;
        scope.heldReader=scope.heldWriter=nullptr;
    }
#ifdef PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS
    {
        // Physical release/hold writes are one-shot; an uncertain hold can
        // only be observed later, never reissued.
        const unsigned priorRelease=context.io.sharedResetWrites,priorHold=context.io.holdWrites;
        const uint32_t priorDisable=gMemory[DataflowRolePhysicalControl][0x224/4];
        gMemory[DataflowRolePhysicalControl][0x224/4]|=DataflowRolePreserveResetMask;
        context.io.roleHandoffs[7]={0,0,0};scope.restoreHeld=true;
        bool releaseAttempted=false;
        assert(!io.ReleaseCoreLocalRestore(7,releaseAttempted) && !releaseAttempted);
        scope.restorePhysicalRelease=true;
        gMemory[DataflowRolePhysicalControl][0x224/4]&=~uint32_t(DataflowRolePreserveResetMask);
        assert(!io.ReleaseCoreLocalRestore(7,releaseAttempted) && !releaseAttempted);
        gMemory[DataflowRolePhysicalControl][0x224/4]|=DataflowRolePreserveResetMask;
        context.io.roleHandoffs[7].readerReady=PIONEER_Q35_ROLE_RESTORE_READER_DONE;
        assert(!io.ReleaseCoreLocalRestore(7,releaseAttempted) && !releaseAttempted);
        context.io.roleHandoffs[7].readerReady=0;
        assert(io.ReleaseCoreLocalRestore(7,releaseAttempted) && releaseAttempted
            && context.io.reset==0x7000 && context.io.sharedResetWrites==priorRelease+1);
        assert(!io.ReleaseCoreLocalRestore(7,releaseAttempted) && context.io.sharedResetWrites==priorRelease+1);
        scope.restoreCompletion=true;scope.restorePhysicalHold=true;
        context.io.roleHandoffs[7]={PIONEER_Q35_ROLE_RESTORE_READER_DONE,PIONEER_Q35_ROLE_RESTORE_WRITER_DONE,0};
        bool holdAttempted=false;context.io.dropHoldWrite=true;
        assert(!io.HoldCoreLocalRestore(7,holdAttempted) && holdAttempted && context.io.holdWrites==priorHold+1);
        assert(!io.HoldCoreLocalRestore(7,holdAttempted) && context.io.holdWrites==priorHold+1);
        scope.restoreHeldAfter=true;
        assert(!io.RestoreCoreHoldObserved(7));
        context.io.reset=0x47800;context.io.dropHoldWrite=false;
        assert(io.RestoreCoreHoldObserved(7));
        context.io.roleHandoffs[7]={0,0,0};context.io.dropPrestackRelease=true;releaseAttempted=false;
        assert(!io.ReleaseCoreLocalRestore(7,releaseAttempted) && releaseAttempted
            && !io.ReleaseCoreLocalRestore(7,releaseAttempted));
        context.io.dropPrestackRelease=false;
        context.io.sharedResetWrites=priorRelease;context.io.holdWrites=priorHold;
        gMemory[DataflowRolePhysicalControl][0x224/4]=priorDisable;
        scope.restoreHeld=scope.restorePhysicalRelease=scope.restoreCompletion=false;
        scope.restorePhysicalHold=scope.restoreHeldAfter=false;
    }
#endif
    {
        // Exercise held shim replacement through the real PCI adapter and
        // mapped views, then verify the original boot words and bytes.
        const auto originalShim=gMemory[DataflowRolePhysicalShim];
        const uint32_t originalB=gMemory[DataflowRolePhysicalBriscBoot][0];
        const uint32_t originalNC=gMemory[DataflowRolePhysicalNcriscBoot][0];
        const uint32_t reader[]={1,2,3,4},writer[]={5,6,7},restore[]={8,9};
        DataflowRoleBootOwner owner;
        assert(owner.Capture(7,io) && owner.Stage(reader,4,writer,3,io));
        assert(owner.Restage(restore,2,restore,2,io) && owner.VerifyStaged(io));
        assert(owner.Restore(io) && gMemory[DataflowRolePhysicalShim]==originalShim
            && gMemory[DataflowRolePhysicalBriscBoot][0]==originalB
            && gMemory[DataflowRolePhysicalNcriscBoot][0]==originalNC);
    }
#ifdef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
    // The final stop audit uses status admission even after stage access has
    // been revoked; it must still reject unsafe or inaccessible samples.
    scope.held=false;scope.dedicatedObservedStage=false;
    assert(!io.AllRolesHeld(7) && io.AllRolesHeldForStop(7));
    scope.dedicatedHeldStatus=false;assert(!io.AllRolesHeldForStop(7));
    scope.dedicatedHeldStatus=true;context.io.reset=0x6000;
    assert(!io.AllRolesHeldForStop(7));
    context.io.reset=0x47800;context.io.nocConfig[0]=1U<<12;
    context.io.resetReadsClockDisabled=0;
    assert(!io.AllRolesHeldForStop(7) && !context.io.resetReadsClockDisabled);
    context.io.nocConfig[0]=0;scope.held=true;
#endif
    for (unsigned noc=0;noc<2;noc++) {
        context.io.nocConfig[noc]=1U<<12;
        const unsigned previousReads=context.io.resetReads;
        assert(!io.AllRolesHeld(7) && context.io.resetReads==previousReads);
        context.io.nocConfig[noc]=1U<<16;
        context.io.nocAvailable[noc]=0x10101010U;
        assert(io.AllRolesHeld(7));
        context.io.nocAvailable[noc]=0x10100f10U;
        assert(!io.AllRolesHeld(7));
        context.io.nocAvailable[noc]=0x10101010U;
        context.io.nocOverflow[noc]=1;
        assert(!io.AllRolesHeld(7));
        context.io.nocConfig[noc]=context.io.nocOverflow[noc]=context.io.nocAvailable[noc]=0;
    }
    context.io.reset=0x46800;
    assert(!io.AllRolesHeld(7) && !io.DefaultBootRouting(7));
    context.io.reset=0x47800;
    context.io.nocDoorbell[1][3]=1; assert(!io.AllRolesHeld(7)); context.io.nocDoorbell[1][3]=0;
    context.io.nocOutstanding[0][31]=1; assert(!io.AllRolesHeld(7)); context.io.nocOutstanding[0][31]=0;
    gMemory[DataflowRolePhysicalControl][0x23c/4]|=1U;
    assert(!io.DefaultBootRouting(7));
    gMemory[DataflowRolePhysicalControl][0x23c/4]&=~1U;
    gMemory[DataflowRolePhysicalControl][0x234/4]|=1U;
    assert(!io.DefaultBootRouting(7));
    gMemory[DataflowRolePhysicalControl][0x234/4]&=~1U;
    // T1/T2 remain held; their override selectors do not alter T0's boot PC.
    gMemory[DataflowRolePhysicalControl][0x234/4]|=6U;
    assert(io.DefaultBootRouting(7));
    gMemory[DataflowRolePhysicalControl][0x234/4]&=~6U;
    context.io.resetReads=0; context.io.resetChangeAt=2;
    assert(!io.DefaultBootRouting(7));
    context.io.resetChangeAt=-1;
    DriftViews drift{views};
    PioneerQwen35RoleFleetPciIO<PioneerSessionIO,FleetPciTestScope,DriftViews> unstable(context.io,scope,drift);
    assert(!unstable.DefaultBootRouting(7) && drift.reads==4);
    gMemory[DataflowRolePhysicalControl][0x234/4]^=1U;
    uint32_t words[]={0x11223344,0x55667788},out[2]={};
    assert(io.WriteBoot(7,DataflowRoleBriscBootAddress,words[0]));
    assert(io.ReadBoot(7,DataflowRoleBriscBootAddress,out[0]) && out[0]==words[0]);
    assert(io.WriteBoot(7,DataflowRoleNcriscBootAddress,words[1]));
    assert(io.ReadBoot(7,DataflowRoleNcriscBootAddress,out[0]) && out[0]==words[1]);
    assert(!io.WriteBoot(7,0x6000,words[0]));
    assert(io.Write(7,DataflowRoleExtraL1Address+8,words,sizeof(words)));
    assert(io.Read(7,DataflowRoleExtraL1Address+8,out,sizeof(out)) && !memcmp(out,words,sizeof(words)));
    assert(!io.Write(7,DataflowRoleExtraL1Limit-4,words,sizeof(words)));
    assert(io.WriteShim(7,DataflowRoleWriterShimAddress,words,sizeof(words)));
    assert(io.ReadShim(7,DataflowRoleWriterShimAddress,out,sizeof(out)) && !memcmp(out,words,sizeof(words)));
    assert(!io.WriteShim(7,DataflowRoleWriterShimLimit-4,words,sizeof(words)));
    for (unsigned role=0;role<DataflowRoleCount;role++) {
        const uint32_t base=role==DataflowRoleReader?DataflowRoleBriscLocalRam:DataflowRoleNcriscLocalRam;
        assert(io.WriteRam(7,role,base+8,words,sizeof(words)));
        assert(io.ReadRam(7,role,base+8,out,sizeof(out)) && !memcmp(out,words,sizeof(words)));
        assert(!io.WriteRam(7,role,base+DataflowRoleLocalRamBytes-4,words,sizeof(words)));
    }
    assert(!io.WriteRam(7,DataflowRoleCount,DataflowRoleBriscLocalRam,words,4));
    const uint32_t original=gMemory[DataflowRolePhysicalControl][0x224/4];
    assert(io.WriteResetDisable(7,original|DataflowRolePreserveResetMask));
    assert(!io.WriteResetDisable(7,(original|DataflowRolePreserveResetMask)^1));
    assert(io.WriteResetDisable(7,original));
    scope.write=false;
    assert(!io.Write(7,DataflowRoleExtraL1Address,words,4)); scope.write=true;
    scope.held=false;
    const unsigned selected=context.io.runtimeCore;
    assert(!io.AllRolesHeld(8) && context.io.runtimeCore==selected);
    assert(!io.Write(7,DataflowRoleExtraL1Address,words,4)); scope.held=true;
    context.io.mapping=false;
    assert(!io.Read(7,DataflowRoleExtraL1Address,out,4)); context.io.mapping=true;
    scope.routing=false; assert(!io.DefaultBootRouting(7)); scope.routing=true;
    scope.stable=false; assert(!io.LocalRamStable(7)); scope.stable=true;
    bool attempted=false;
    context.io.runtimeCore=7;
    assert(!io.ReleaseCore(8,attempted) && !attempted && context.io.runtimeCore==7
        && !context.io.sharedResetWrites);
    scope.release=true;
    scope.releaseDrained=true; scope.heldAfter=true;
    for (unsigned noc=0;noc<2;noc++) {
        context.io.nocConfig[noc]=1U<<12;
        const unsigned previousReads=context.io.resetReads;
        const unsigned previousWrites=context.io.sharedResetWrites;
        uint32_t boot=0;
        assert(!io.ReadBoot(8,DataflowRoleBriscBootAddress,boot));
        assert(!io.DefaultBootRouting(8));
        assert(!io.ReleaseCore(8,attempted) && !attempted);
        assert(!io.UnconfirmedReleaseHeld(8));
        assert(!io.HoldObserved(8));
        assert(context.io.resetReads==previousReads && context.io.sharedResetWrites==previousWrites);
        context.io.nocConfig[noc]=0;
    }
    scope.releaseDrained=false; scope.heldAfter=false;
    gMemory[DataflowRolePhysicalControl][0x224/4]&=~uint32_t(DataflowRolePreserveResetMask);
    assert(!io.ReleaseCore(8,attempted) && !attempted && !context.io.sharedResetWrites);
    gMemory[DataflowRolePhysicalControl][0x224/4]|=DataflowRolePreserveResetMask;
    assert(io.ReleaseCore(8,attempted) && attempted && context.io.sharedResetWrites==1
        && context.io.reset==0x6000);
    assert(!io.ReleaseCore(8,attempted) && context.io.sharedResetWrites==1);
    context.io.reset=0x47800; context.io.dropSharedResetWrite=true; attempted=false;
    assert(!io.ReleaseCore(9,attempted) && attempted && context.io.sharedResetWrites==2);
    assert(!io.ReleaseCore(9,attempted) && context.io.sharedResetWrites==2);
    assert(!io.UnconfirmedReleaseHeld(9)); // a held read is not a write fence
    scope.releaseDrained=true;
    assert(io.UnconfirmedReleaseHeld(9));
    context.io.reset=0x6000;
    assert(!io.UnconfirmedReleaseHeld(9)); // release really occurred
    context.io.reset=0x47800;
    scope.releaseDrained=false;
    context.io.dropSharedResetWrite=false;
    bool allAttempts[DataflowBarrierWorkers]{};
    for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
        context.io.reset=0x47800;
        assert(io.ReleaseCore(core,allAttempts[core]) && allAttempts[core]);
    }
    assert(context.io.sharedResetWrites==2+DataflowBarrierWorkers);
    {
        const unsigned core=16;
        auto& handoff=context.io.roleHandoffs[core];
        handoff.readerReady=DataflowRoleSnapshotReaderReady;
        handoff.writerReady=DataflowRoleSnapshotWriterReady;
        decltype(io)::StackReleaseObservation observed=decltype(io)::StackReleaseObserved;
        assert(!io.ObserveDrainedStackRelease(core,observed)
            && observed==decltype(io)::StackReleaseObserved);
        scope.stackReleaseDrained=true;
        assert(io.ObserveDrainedStackRelease(core,observed)
            && observed==decltype(io)::StackReleaseNotObserved);
        scope.stackDrainChecks=0; scope.stackDrainFailAt=1;
        assert(!io.ObserveDrainedStackRelease(core,observed));
        scope.stackDrainFailAt=UINT32_MAX;
        handoff.release=DataflowRoleSnapshotRelease;
        assert(io.ObserveDrainedStackRelease(core,observed)
            && observed==decltype(io)::StackReleaseObserved);
        handoff.release=DataflowRoleSnapshotCancel;
        assert(!io.ObserveDrainedStackRelease(core,observed)
            && observed==decltype(io)::StackReleaseObserved);
        handoff.release=0;
        context.io.handoffReads=0; context.io.handoffChangeAt=1;
        assert(!io.ObserveDrainedStackRelease(core,observed));
        context.io.handoffChangeAt=UINT32_MAX;
        scope.stackReleaseDrained=false;
        bool cancelAttempted=false;
        assert(!io.RequestStackCancel(core,cancelAttempted) && !cancelAttempted && !context.io.stackCancelWrites);
        scope.stackCancel=true;
        handoff.writerReady=0;
        assert(!io.RequestStackCancel(core,cancelAttempted) && !cancelAttempted);
        handoff.writerReady=DataflowRoleSnapshotWriterReady;
        handoff.release=DataflowRoleSnapshotRelease;
        assert(!io.RequestStackCancel(core,cancelAttempted) && !cancelAttempted);
        handoff.release=0;
        assert(!io.RequestStackCancel(core,cancelAttempted,true) && !cancelAttempted);
        scope.stackReleaseDrained=true;
        assert(io.RequestStackCancel(core,cancelAttempted,true) && cancelAttempted && context.io.stackCancelWrites==1);
        assert(!io.RequestStackCancel(core,cancelAttempted) && context.io.stackCancelWrites==1);
        const unsigned readerPark=DataflowRoleReaderShimAddress+0x30;
        const unsigned writerPark=DataflowRoleWriterShimAddress+0x30;
        context.io.rolePc[0]=readerPark; context.io.rolePc[1]=writerPark;
        scope.stackCancelled=true;
        assert(!io.StackCancelledAndParked(core,readerPark,writerPark));
        handoff.readerReady=DataflowRoleSnapshotReaderCancelled;
        assert(!io.StackCancelledAndParked(core,readerPark,writerPark));
        handoff.writerReady=DataflowRoleSnapshotWriterCancelled;
        assert(io.StackCancelledAndParked(core,readerPark,writerPark));
        context.io.handoffReads=0; context.io.handoffChangeAt=1;
        assert(!io.StackCancelledAndParked(core,readerPark,writerPark));
        context.io.handoffChangeAt=UINT32_MAX;
        context.io.rolePcChangeAt[0]=context.io.rolePcReads[0]+1;
        context.io.rolePcChanged[0]=readerPark+4;
        assert(!io.StackCancelledAndParked(core,readerPark,writerPark));
        context.io.rolePcChangeAt[0]=UINT32_MAX;
        assert(!io.StackCancelledAndParked(core,readerPark+4,writerPark));
        scope.stackCancelled=false;
        assert(!io.StackCancelledAndParked(core,readerPark,writerPark));
        auto& uncertain=context.io.roleHandoffs[17];
        uncertain.readerReady=DataflowRoleSnapshotReaderReady;
        uncertain.writerReady=DataflowRoleSnapshotWriterReady;
        context.io.dropStackCancelWrite=true; cancelAttempted=false;
        assert(!io.RequestStackCancel(17,cancelAttempted) && cancelAttempted && context.io.stackCancelWrites==2);
        context.io.dropStackCancelWrite=false;
        uncertain.release=DataflowRoleSnapshotCancel; // delayed write, observed only
        assert(!io.RequestStackCancel(17,cancelAttempted) && context.io.stackCancelWrites==2);

        // A partially started tile can finish the shim copy during cleanup.
        // Wait for its missing receipt without sending an early cancel.
        auto& delayed=context.io.roleHandoffs[18];
        delayed={DataflowRoleSnapshotReaderReady,0,0};
        context.io.handoffReads=0;cancelAttempted=false;
        assert(!io.WaitAndRequestStackCancel(18,cancelAttempted,false,0)
            && !io.WaitAndRequestStackCancel(18,cancelAttempted,false,65537));
        scope.stackCancel=false;
        assert(!io.WaitAndRequestStackCancel(18,cancelAttempted,false,3)
            && !cancelAttempted && context.io.stackCancelWrites==2);
        scope.stackCancel=true;
        assert(!io.WaitAndRequestStackCancel(18,cancelAttempted,false,2)
            && !cancelAttempted && context.io.stackCancelWrites==2);
        context.io.handoffReads=0;
        context.io.pauseHook=[](PioneerSessionIO* p) {
            if (p->handoffReads>=2) p->roleHandoffs[18].writerReady=DataflowRoleSnapshotWriterReady;
        };
        assert(io.WaitAndRequestStackCancel(18,cancelAttempted,false,3)
            && cancelAttempted && context.io.stackCancelWrites==3);
        context.io.pauseHook=nullptr;
        assert(!io.WaitAndRequestStackCancel(18,cancelAttempted,false,3)
            && context.io.stackCancelWrites==3);

        auto& bad=context.io.roleHandoffs[19];
        bad={DataflowRoleSnapshotReaderReady,0xdead,0};cancelAttempted=false;
        assert(!io.WaitAndRequestStackCancel(19,cancelAttempted,false,3)
            && !cancelAttempted && context.io.stackCancelWrites==3);
        bad.writerReady=DataflowRoleSnapshotWriterReady;
        scope.stackReleaseDrained=false;
        assert(!io.WaitAndRequestStackCancel(19,cancelAttempted,true,3)
            && !cancelAttempted && context.io.stackCancelWrites==3);
        scope.stackReleaseDrained=true;context.io.dropStackCancelWrite=true;
        assert(!io.WaitAndRequestStackCancel(19,cancelAttempted,true,3)
            && cancelAttempted && context.io.stackCancelWrites==4);
        context.io.dropStackCancelWrite=false;
        assert(!io.WaitAndRequestStackCancel(19,cancelAttempted,true,3)
            && context.io.stackCancelWrites==4);
    }
    auto initRoleBox=[&](unsigned core) {
        auto& box=context.io.roleBoxes[core];
        box.magic=DataflowRoleMailboxMagic; box.version=DataflowRoleMailboxVersion;
        for (unsigned role=0;role<DataflowRoleCount;role++)
            assert(DataflowRoleMailboxPublishStarted(box,role));
    };
    initRoleBox(8);
#ifdef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
    for (unsigned core=8;core<=9;core++) {
        auto& box=context.io.workerBoxes[core];
        box.magic=DataflowWorkerMailboxMagic;box.version=DataflowWorkerMailboxVersion;
        box.logicalCore=core;box.state=DataflowWorkerReady;
    }
#endif
    bool stopAttempted=false; uint32_t stopToken=0;
    context.io.runtimeCore=7;
    assert(!io.RequestStop(8,false,stopAttempted,stopToken)
        && !stopAttempted && !stopToken && context.io.runtimeCore==7 && !context.io.roleStopWrites);
    scope.stop=true;
#ifdef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
    context.io.queueBoxes[8][0].produced=1;
    assert(!io.RequestStop(8,false,stopAttempted,stopToken) && !stopAttempted && !context.io.roleStopWrites);
    context.io.queueBoxes[8][0].produced=0;context.io.workerBoxes[8].state=DataflowWorkerComplete;
    assert(!io.RequestStop(8,false,stopAttempted,stopToken) && !stopAttempted && !context.io.roleStopWrites);
    context.io.workerBoxes[8].state=DataflowWorkerReady;
#endif
    assert(io.RequestStop(8,false,stopAttempted,stopToken) && stopAttempted && stopToken==1);
    assert(io.StopRequestObserved(8,stopToken) && context.io.roleStopWrites==1);
    assert(!io.RequestStop(8,false,stopAttempted,stopToken) && context.io.roleStopWrites==1);
    context.io.rolePc[DataflowRoleReader]=DataflowRoleReaderShimAddress+0x30;
    context.io.rolePc[DataflowRoleWriter]=DataflowRoleWriterShimAddress+0x30;
    auto& cleanBox=context.io.roleBoxes[8];
    scope.park=true;
    assert(!io.RolesStoppedAndParked(8,stopToken,context.io.rolePc[0],context.io.rolePc[1]));
    assert(DataflowRoleMailboxObserveStop(cleanBox,DataflowRoleReader));
    assert(!io.RolesStoppedAndParked(8,stopToken,context.io.rolePc[0],context.io.rolePc[1]));
    assert(DataflowRoleMailboxObserveStop(cleanBox,DataflowRoleWriter));
    scope.park=false;
    assert(!io.RolesStoppedAndParked(8,stopToken,context.io.rolePc[0],context.io.rolePc[1]));
    scope.park=true;
    assert(io.RolesStoppedAndParked(8,stopToken,context.io.rolePc[0],context.io.rolePc[1]));
    assert(!io.RolesStoppedAndParked(8,stopToken,context.io.rolePc[0]+4,context.io.rolePc[1]));
    assert(!io.RolesStoppedAndParked(8,stopToken,context.io.rolePc[0],DataflowRoleWriterShimLimit));
#ifdef PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS
    constexpr uint32_t readerRestorePark=DataflowRoleReaderCodeAddress+0x4e80;
    constexpr uint32_t writerRestorePark=DataflowRoleWriterCodeAddress+0x2e80;
    context.io.rolePc[DataflowRoleReader]=readerRestorePark;
    context.io.rolePc[DataflowRoleWriter]=writerRestorePark;
    auto& autoHandoff=context.io.roleHandoffs[8];
    autoHandoff={PIONEER_Q35_ROLE_RESTORE_READER_DONE,PIONEER_Q35_ROLE_RESTORE_WRITER_DONE,
        DataflowRoleSnapshotRelease};
    assert(!io.RolesAutoRestoredAndParked(8,stopToken,readerRestorePark,writerRestorePark));
    scope.autoRestoreCompletion=true;
    assert(io.RolesAutoRestoredAndParked(8,stopToken,readerRestorePark,writerRestorePark));
    autoHandoff.readerReady=PIONEER_Q35_ROLE_RESTORE_READER_FAILED;
    assert(!io.RolesAutoRestoredAndParked(8,stopToken,readerRestorePark,writerRestorePark));
    autoHandoff.readerReady=PIONEER_Q35_ROLE_RESTORE_READER_DONE;
    autoHandoff.release=0;
    assert(!io.RolesAutoRestoredAndParked(8,stopToken,readerRestorePark,writerRestorePark));
    autoHandoff.release=DataflowRoleSnapshotRelease;
    assert(!io.RolesAutoRestoredAndParked(8,stopToken,readerRestorePark+4,writerRestorePark));
    scope.autoRestoreCompletion=false;
    context.io.rolePc[DataflowRoleReader]=DataflowRoleReaderShimAddress+0x30;
    context.io.rolePc[DataflowRoleWriter]=DataflowRoleWriterShimAddress+0x30;
#endif
    initRoleBox(9); stopAttempted=false; stopToken=0; context.io.dropRoleStopWrite=true;
    assert(!io.RequestStop(9,false,stopAttempted,stopToken) && stopAttempted && stopToken==1);
    assert(!io.StopRequestObserved(9,stopToken) && context.io.roleStopWrites==2);
    context.io.roleBoxes[9].stopRequest=stopToken; // a delayed MMIO completion
    assert(io.StopRequestObserved(9,stopToken));
    assert(!io.RequestStop(9,false,stopAttempted,stopToken) && context.io.roleStopWrites==2);
    context.io.dropRoleStopWrite=false;
    initRoleBox(10); auto& faultBox=context.io.roleBoxes[10];
    assert(DataflowRoleMailboxIssue(faultBox,1));
    for (unsigned role=0;role<DataflowRoleCount;role++) {
        assert(DataflowRoleMailboxBegin(faultBox,role,1));
        assert(DataflowRoleMailboxComplete(faultBox,role,1,role!=DataflowRoleReader));
    }
    stopAttempted=false; stopToken=0;
    assert(!io.RequestStop(10,false,stopAttempted,stopToken) && !stopAttempted);
    assert(io.RequestStop(10,true,stopAttempted,stopToken) && stopAttempted && stopToken==1);
    assert(io.StopRequestObserved(10,stopToken) && faultBox.acknowledged==0);
    auto& worker=context.io.workerBoxes[8];
    worker.magic=DataflowWorkerMailboxMagic; worker.version=DataflowWorkerMailboxVersion;
    worker.logicalCore=8; worker.state=DataflowWorkerReady;
    bool computeAttempted=false;
    assert(!io.RequestComputeStop(8,computeAttempted) && !computeAttempted && !context.io.computeStopWrites);
    scope.computeStop=true;
    assert(io.RequestComputeStop(8,computeAttempted) && computeAttempted && context.io.computeStopWrites==1);
    assert(io.ComputeStopObserved(8));
    context.io.computePc=DataflowServiceEntryAddress+0x100;
    scope.computePark=true;
    assert(!io.ComputeStoppedAndParked(8,context.io.computePc));
    worker.state=DataflowWorkerStopped;
    context.io.queueBoxes[8][0].produced=1;
    assert(!io.ComputeStoppedAndParked(8,context.io.computePc));
    context.io.queueBoxes[8][0].produced=0;
    assert(io.ComputeStoppedAndParked(8,context.io.computePc));
    assert(!io.ComputeStoppedAndParked(8,context.io.computePc+4));
    bool holdAttempted=false;
    assert(!io.HoldCore(8,holdAttempted) && !holdAttempted && !context.io.holdWrites);
    scope.hold=true; scope.heldAfter=true;
    assert(io.HoldCore(8,holdAttempted) && holdAttempted && context.io.reset==0x47800);
    assert(io.HoldObserved(8));
    assert(!io.HoldCore(8,holdAttempted) && context.io.holdWrites==1);
    context.io.reset=0x6000; context.io.dropHoldWrite=true; holdAttempted=false;
    assert(!io.HoldCore(8,holdAttempted) && holdAttempted && context.io.holdWrites==2);
    assert(!io.HoldObserved(8));
    context.io.reset=0x47800; // delayed reset write became visible
    assert(io.HoldObserved(8));
    context.io.reset=0x6000;
    auto& delayedWorker=context.io.workerBoxes[9];
    delayedWorker.magic=DataflowWorkerMailboxMagic; delayedWorker.version=DataflowWorkerMailboxVersion;
    delayedWorker.logicalCore=9; delayedWorker.state=DataflowWorkerReady;
    bool delayedComputeAttempt=false; context.io.dropComputeStopWrite=true;
    assert(!io.RequestComputeStop(9,delayedComputeAttempt) && delayedComputeAttempt);
    assert(!io.ComputeStopObserved(9));
    delayedWorker.shutdownSequence=DataflowWorkerShutdownRequest;
    assert(io.ComputeStopObserved(9));
    assert(!io.RequestComputeStop(9,delayedComputeAttempt) && context.io.computeStopWrites==2);
    initRoleBox(11); auto& readyWorker=context.io.workerBoxes[11];
    readyWorker.magic=DataflowWorkerMailboxMagic; readyWorker.version=DataflowWorkerMailboxVersion;
    readyWorker.logicalCore=11; readyWorker.state=DataflowWorkerReady;
    DataflowWorkerSnapshot readySnapshot{};
    assert(context.io.ReadWorkerSnapshot(11,readySnapshot));
    context.io.computePc=DataflowServiceEntryAddress+0x100;
    assert(!io.RunningReady(11,readySnapshot,0x1000,0x100));
    scope.runtime=true;
    assert(io.RunningReady(11,readySnapshot,0x1000,0x100));
    auto& handoff=context.io.roleHandoffs[11];
    assert(!io.StartupHandoffWriteAllowed(11));
    handoff.readerReady=DataflowRoleSnapshotReaderReady;
    assert(!io.StartupHandoffWriteAllowed(11));
    handoff.writerReady=DataflowRoleSnapshotWriterReady;
    assert(io.StartupHandoffWriteAllowed(11));
    context.io.handoffReads=0; context.io.handoffChangeAt=1;
    assert(!io.StartupHandoffWriteAllowed(11));
    context.io.handoffChangeAt=UINT32_MAX;
    handoff.release=DataflowRoleSnapshotRelease;
    assert(!io.StartupHandoffWriteAllowed(11));
    handoff.release=0; scope.runtime=false;
    assert(!io.StartupHandoffWriteAllowed(11));
    scope.runtime=true;
    {
        const uint32_t readerShim[]={1,2,3,4},writerShim[]={5,6,7,8};
        const uint32_t readerCode[]={9,10,11},writerCode[]={12,13,14};
        DataflowRoleProgramImage image={{{4,4},{3,3},
            {DataflowRoleReaderShimAddress+8,DataflowRoleWriterShimAddress+8}},
            {readerShim,writerShim},{readerCode,writerCode}};
        uint32_t readerBoot=0,writerBoot=0;
        assert(DataflowRoleBootWord(DataflowRoleReader,DataflowRoleReaderShimAddress,readerBoot));
        assert(DataflowRoleBootWord(DataflowRoleWriter,DataflowRoleWriterShimAddress,writerBoot));
        gMemory[DataflowRolePhysicalBriscBoot][0]=readerBoot;
        gMemory[DataflowRolePhysicalNcriscBoot][0]=writerBoot;
        for (unsigned i=0;i<4;i++) {
            gMemory[DataflowRolePhysicalShim][i]=readerShim[i];
            gMemory[DataflowRolePhysicalShim][(DataflowRoleWriterShimAddress-DataflowRoleReaderShimAddress)/4+i]=writerShim[i];
        }
        const unsigned writerOffset=(DataflowRoleWriterCodeAddress-DataflowRoleExtraL1Address)/4;
        for (unsigned i=0;i<3;i++) {
            gMemory[DataflowRolePhysicalExtraCode][i]=readerCode[i];
            gMemory[DataflowRolePhysicalExtraCode][writerOffset+i]=writerCode[i];
        }
        assert(!io.InspectRunningRoleImages(11,image));
        scope.imageInspection=true;
        for (unsigned core=0;core<DataflowBarrierWorkers;core++)
            assert(io.InspectRunningRoleImages(core,image));
        for (unsigned window : {DataflowRolePhysicalBriscBoot,DataflowRolePhysicalNcriscBoot,
                DataflowRolePhysicalShim,DataflowRolePhysicalExtraCode}) {
            gMemory[window][0]^=1;
            assert(!io.InspectRunningRoleImages(11,image));
            gMemory[window][0]^=1;
        }
        gMemory[DataflowRolePhysicalExtraCode][writerOffset+2]^=1;
        assert(!io.InspectRunningRoleImages(11,image));
        gMemory[DataflowRolePhysicalExtraCode][writerOffset+2]^=1;
        ChangingImageViews changing{views};
        PioneerQwen35RoleFleetPciIO<PioneerSessionIO,FleetPciTestScope,ChangingImageViews>
            changingIO(context.io,scope,changing);
        assert(!changingIO.InspectRunningRoleImages(11,image) && changing.matches==8);
        gMemory[DataflowRolePhysicalExtraCode][0]^=1;
        auto bad=image; bad.code[0]=nullptr;
        assert(!io.InspectRunningRoleImages(11,bad));
        std::vector<uint32_t> readerRam(DataflowRoleLocalRamWords),writerRam(DataflowRoleLocalRamWords);
        const unsigned readerCopy=(DataflowRoleReaderRamCopy-DataflowRoleExtraL1Address)/4;
        const unsigned writerCopy=(DataflowRoleWriterRamCopy-DataflowRoleExtraL1Address)/4;
        for (unsigned i=0;i<DataflowRoleLocalRamWords;i++) {
            readerRam[i]=0x12340000U^i; writerRam[i]=0x98760000U^i;
            gMemory[DataflowRolePhysicalExtraCode][readerCopy+i]=readerRam[i];
            gMemory[DataflowRolePhysicalExtraCode][writerCopy+i]=writerRam[i];
        }
        for (unsigned core:{0U,60U,119U})
            assert(io.InspectRunningRoleRamCopies(core,readerRam.data(),writerRam.data()));
        {
            DataflowRoleLocalRamOwner captured;
            std::vector<uint32_t> snapshot(DataflowRoleLocalRamOwner::RequiredWords());
            assert(!captured.CaptureStackless(11,snapshot.data(),snapshot.size(),io));
            scope.stacklessSnapshot=true;
            assert(captured.CaptureStackless(11,snapshot.data(),snapshot.size(),io));
            for (unsigned i=0;i<DataflowRoleLocalRamWords;i++) {
                assert(snapshot[i]==readerRam[i]);
                assert(snapshot[DataflowRoleLocalRamWords+i]==writerRam[i]);
            }
            assert(captured.MarkMayRunStackless(io) && captured.RestorationRequired());
            uint32_t invalid=0;
            assert(!io.ReadStacklessRamCopy(11,DataflowRoleCount,0,&invalid,4));
            assert(!io.ReadStacklessRamCopy(11,DataflowRoleReader,DataflowRoleLocalRamBytes-4,&invalid,8));
            handoff.release=DataflowRoleSnapshotRelease;
            assert(!io.StacklessCopiesSafe(11));
            handoff.release=0;
            scope.stacklessSnapshot=false;
        }
        gMemory[DataflowRolePhysicalExtraCode][readerCopy+123]^=1;
        assert(!io.InspectRunningRoleRamCopies(11,readerRam.data(),writerRam.data()));
        gMemory[DataflowRolePhysicalExtraCode][readerCopy+123]^=1;
        gMemory[DataflowRolePhysicalExtraCode][writerCopy+DataflowRoleLocalRamWords-1]^=1;
        assert(!io.InspectRunningRoleRamCopies(11,readerRam.data(),writerRam.data()));
        gMemory[DataflowRolePhysicalExtraCode][writerCopy+DataflowRoleLocalRamWords-1]^=1;
        ChangingRamCopyViews changingCopy{views};
        PioneerQwen35RoleFleetPciIO<PioneerSessionIO,FleetPciTestScope,ChangingRamCopyViews>
            changingCopyIO(context.io,scope,changingCopy);
        assert(!changingCopyIO.InspectRunningRoleRamCopies(11,readerRam.data(),writerRam.data())
            && changingCopy.matches==2);
        gMemory[DataflowRolePhysicalExtraCode][readerCopy]^=1;
        assert(!io.InspectRunningRoleRamCopies(11,nullptr,writerRam.data()));
        context.io.reset=0x47800;
        assert(!io.InspectRunningRoleImages(11,image));
        assert(!io.InspectRunningRoleRamCopies(11,readerRam.data(),writerRam.data()));
        context.io.reset=0x6000; scope.imageInspection=false;
    }
    DataflowRoleProgramBounds bounds={{32,32},{64,64},
        {DataflowRoleReaderShimAddress+0x44,DataflowRoleWriterShimAddress+0x44}};
    bounds.loopStart[0]=DataflowRoleReaderShimAddress+0x20;bounds.loopEnd[0]=DataflowRoleReaderShimAddress+0x40;
    bounds.loopStart[1]=DataflowRoleWriterShimAddress+0x20;bounds.loopEnd[1]=DataflowRoleWriterShimAddress+0x40;
    assert(io.RunningRolePcs(11,bounds,false));
    for (unsigned noc=0;noc<2;noc++) {
        context.io.nocConfig[noc]=1U<<12;
        const unsigned previousResetReads=context.io.resetReads;
        assert(!io.RunningRolePcs(11,bounds,false) && context.io.resetReads==previousResetReads);
        context.io.nocConfig[noc]=0;
    }
    for (unsigned role=0;role<DataflowRoleCount;role++) {
        const uint32_t originalPc=context.io.rolePc[role];
        context.io.rolePc[role]=bounds.park[role];
        assert(!io.RunningRolePcs(11,bounds,false));
        context.io.rolePc[role]=role==DataflowRoleReader?DataflowRoleReaderShimAddress:DataflowRoleWriterShimAddress;
        assert(!io.RunningRolePcs(11,bounds,false));
        context.io.rolePc[role]=role==DataflowRoleReader?DataflowRoleWriterCodeAddress:DataflowRoleReaderCodeAddress;
        assert(!io.RunningRolePcs(11,bounds,false));
        context.io.rolePc[role]=originalPc;
        context.io.rolePcChangeAt[role]=context.io.rolePcReads[role]+1;
        context.io.rolePcChanged[role]=UINT32_MAX;
        assert(!io.RunningRolePcs(11,bounds,false));
        context.io.rolePcChangeAt[role]=UINT32_MAX;
    }
    context.io.rolePc[0]=DataflowRoleReaderCodeAddress;
    context.io.rolePc[1]=DataflowRoleWriterCodeAddress;
    assert(io.RunningRolePcs(11,bounds,false));
    scope.runtime=false;
    assert(!io.RunningRolePcs(11,bounds,false));
    scope.runtime=true;
    assert(!io.RunningRolePcs(11,bounds,true));
    scope.faultAudit=true;
    assert(io.RunningRolePcs(11,bounds,true));
    scope.faultAudit=false;
    auto invalidBounds=bounds; invalidBounds.shimWords[0]=0;
    assert(!io.RunningRolePcs(11,invalidBounds,false));
    context.io.roleBoxes[11].command=1;
    assert(!io.RunningReady(11,readySnapshot,0x1000,0x100));
    context.io.roleBoxes[11].command=0;
    context.io.queueBoxes[11][0].produced=1;
    assert(!io.RunningReady(11,readySnapshot,0x1000,0x100));
    context.io.queueBoxes[11][0].produced=0;
    context.io.computePc=DataflowServiceTextLimit;
    assert(!io.RunningReady(11,readySnapshot,0x1000,0x100));
    context.io.computePc=DataflowServiceEntryAddress+0x100;
    auto& idleRoles=context.io.roleBoxes[11];
    idleRoles.command=idleRoles.acknowledged=1;
    for (unsigned role=0;role<DataflowRoleCount;role++) {
        idleRoles.completed[role]=1; idleRoles.status[role]=DataflowRoleComplete;
    }
    assert(io.RunningReady(11,readySnapshot,0x1000,0x100));
    readyWorker.commandSequence=readyWorker.completedSequence=2;
    readyWorker.acknowledgedSequence=1; readyWorker.state=DataflowWorkerComplete; readyWorker.phase=2;
    context.io.queueBoxes[11][0].produced=context.io.queueBoxes[11][0].consumed=3;
    assert(io.RuntimeStable(11,0x1000,0x100));
    assert(io.CompletedForAcknowledgement(11,0x1000,0x100));
    idleRoles.command=2;
    assert(!io.CompletedForAcknowledgement(11,0x1000,0x100));
    idleRoles.command=1;
    context.io.queueBoxes[11][0].consumed=2;
    assert(!io.CompletedForAcknowledgement(11,0x1000,0x100));
    context.io.queueBoxes[11][0].consumed=3;
    readyWorker.state=DataflowWorkerFailed; readyWorker.failure=DataflowFailureQueue; readyWorker.phase=5;
    assert(!io.CompletedForAcknowledgement(11,0x1000,0x100));
    DataflowWorkerSnapshot faultSnapshot{};
    assert(context.io.ReadWorkerSnapshot(11,faultSnapshot));
    assert(!io.FaultSettled(11,faultSnapshot,0x1000,0x100));
    scope.faultAudit=true;
    assert(io.FaultSettled(11,faultSnapshot,0x1000,0x100));
    assert(!io.FaultCompletedForAcknowledgement(11,0x1000,0x100));
    scope.faultAck=true;
    assert(io.FaultCompletedForAcknowledgement(11,0x1000,0x100));
    idleRoles.command=2;idleRoles.completed[DataflowRoleReader]=2;
    idleRoles.status[DataflowRoleReader]=DataflowRoleFailed;
    assert(!io.FaultSettled(11,faultSnapshot,0x1000,0x100));
    idleRoles.completed[DataflowRoleWriter]=2;idleRoles.status[DataflowRoleWriter]=DataflowRoleComplete;
    assert(io.FaultSettled(11,faultSnapshot,0x1000,0x100));
    context.io.queueBoxes[11][0].readerFailure=DataflowFailureReader;
    assert(io.FaultSettled(11,faultSnapshot,0x1000,0x100));
    context.io.queueBoxes[11][0].consumed=2;
    assert(io.FaultSettled(11,faultSnapshot,0x1000,0x100)); // one valid retained page
    assert(io.FaultCompletedForAcknowledgement(11,0x1000,0x100));
    context.io.queueBoxes[11][0].produced=6; // invalid occupancy cannot be discarded
    assert(!io.FaultSettled(11,faultSnapshot,0x1000,0x100));
    context.io.queueBoxes[11][0].produced=3;
    idleRoles.completed[DataflowRoleWriter]=1; // writer has not returned
    assert(!io.FaultSettled(11,faultSnapshot,0x1000,0x100));
    idleRoles.completed[DataflowRoleWriter]=2;
    readyWorker.state=DataflowWorkerComplete; readyWorker.failure=0; readyWorker.phase=2;
    assert(context.io.ReadWorkerSnapshot(11,faultSnapshot));
    assert(!io.FaultSettled(11,faultSnapshot,0x1000,0x100)); // successful work must drain
    readyWorker.state=DataflowWorkerFailed; readyWorker.failure=DataflowFailureQueue; readyWorker.phase=5;
    assert(context.io.ReadWorkerSnapshot(11,faultSnapshot));
    context.io.queueBoxes[11][0].consumed=3;
    faultSnapshot.commandSequence++;
    assert(!io.FaultSettled(11,faultSnapshot,0x1000,0x100));
#ifdef PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS
    {
        context.io.reset=0x47800;scope.release=true;scope.exclusive=true;
        scope.releaseDrained=false;
        assert(!io.UnconfirmedReleaseRunning(4));
        scope.releaseDrained=true;
        assert(!io.UnconfirmedReleaseRunning(4)); // a held worker is not running
        context.io.reset=0x6000;
        assert(io.UnconfirmedReleaseRunning(4));
        scope.releaseDrained=false;
        assert(!io.UnconfirmedReleaseRunning(4));
        context.io.reset=0x47800;
        gMemory[DataflowRolePhysicalControl][0x224/4]&=~uint32_t(DataflowRolePreserveResetMask);
        const unsigned prior=context.io.sharedResetWrites;
        bool dedicatedAttempted=false;
        assert(!io.ReleaseDedicatedCore(4,dedicatedAttempted) && !dedicatedAttempted
            && context.io.sharedResetWrites==prior);
        // The dedicated takeover has its own complete release/recovery
        // contract; it must not require the legacy snapshot release hook.
        scope.release=false;
        scope.dedicatedRelease=true;
        scope.dedicatedChecks=0;scope.dedicatedFailAt=1;
        assert(!io.ReleaseDedicatedCore(4,dedicatedAttempted) && !dedicatedAttempted
            && context.io.sharedResetWrites==prior); // proof lost just before release
        scope.dedicatedFailAt=UINT32_MAX;
        // If the clock is gated while revalidating authorization, do not
        // touch reset or consume the release attempt.
        scope.dedicatedChecks=0;scope.dedicatedDisableClockAt=1;
        context.io.resetReadsClockDisabled=0;
        assert(!io.ReleaseDedicatedCore(4,dedicatedAttempted) && !dedicatedAttempted
            && context.io.sharedResetWrites==prior && !context.io.resetReadsClockDisabled);
        scope.dedicatedDisableClockAt=UINT32_MAX;context.io.nocConfig[0]=0;
        // If the clock changes after the write, retain the uncertain attempt
        // without reading an inaccessible reset register or retrying it.
        context.io.disableClockAfterRelease=true;
        assert(!io.ReleaseDedicatedCore(4,dedicatedAttempted) && dedicatedAttempted
            && context.io.sharedResetWrites==prior+1 && !context.io.resetReadsClockDisabled);
        assert(!io.ReleaseDedicatedCore(4,dedicatedAttempted));
        context.io.disableClockAfterRelease=false;context.io.nocConfig[0]=0;
        context.io.reset=0x47800;dedicatedAttempted=false;
        assert(io.ReleaseDedicatedCore(4,dedicatedAttempted) && dedicatedAttempted
            && context.io.sharedResetWrites==prior+2 && context.io.reset==0x6000);
        assert(!io.ReleaseDedicatedCore(4,dedicatedAttempted));
        context.io.reset=0x47800;context.io.dropSharedResetWrite=true;dedicatedAttempted=false;
        assert(!io.ReleaseDedicatedCore(5,dedicatedAttempted) && dedicatedAttempted);
        assert(!io.ReleaseDedicatedCore(5,dedicatedAttempted));
        context.io.dropSharedResetWrite=false;
    }
#endif
    assert(views.Close()); gAllowWrite=false;
    assert(release_sem(sLock)==B_OK && !gLocked);
    std::puts("PASS role fleet PCI range translation and guarded MMIO on 120 simulated routes; NO HARDWARE");
}
