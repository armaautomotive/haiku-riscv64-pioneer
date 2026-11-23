/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowRoleFleetRecoveryOwner.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace PioneerQwen35;
struct RecoveryIO {
    static constexpr unsigned CodePerCore=DataflowRoleCodeOwner::RequiredWords();
    static constexpr unsigned RamPerCore=DataflowRoleLocalRamOwner::RequiredWords();
    std::vector<uint32_t> code,ram,boots,shims,resetDisable,stacklessCopies;
    bool held=true,defaultRoute=true,stable=true,corruptRamOnBoot=false,corruptBootOnRam=false;
    bool corruptPriorDisableOnWrite=false;
    bool stacklessPreparation=true,stacklessSafe=true,uncapturedProtected=true;
    bool coreRestoreProof=false,restoreReleaseReady=false,staleRestoreReceipt=false;
    bool restoreCompletion=false,restoreCompletedHeld=false;
    bool autoArmProof=false,autoHeldProof=false,autoRetirementProof=false;
    DataflowRoleSnapshotHandoff restoreHandoff[DataflowBarrierWorkers]{};
    unsigned releaseReadyCalls=0,failReleaseReadyCall=0;
    bool copyProof=true,copyCaptured[DataflowBarrierWorkers]{};
    bool wrongCopyPointer=false,corruptCopyOnShimWrite=false;
    bool retirementProof=false,retirementRetry=false;
    int failRetirementCodeCore=-1;
    const uint32_t* retainedRam=nullptr;
    unsigned privateRamReads=0;
    unsigned copyCalls=0,failCopyCall=UINT32_MAX;
    unsigned failDisableCore=UINT32_MAX;
    unsigned writes=0;
    mutable unsigned heldChecks=0,failHeldCheck=0;
    RecoveryIO():code(DataflowRoleFleetRecoveryOwner::CodeWords()),
        ram(DataflowRoleFleetRecoveryOwner::RamWords()),boots(DataflowBarrierWorkers*2),
        shims(DataflowBarrierWorkers*DataflowRoleShimWords),resetDisable(DataflowBarrierWorkers) {
        for (unsigned i=0;i<code.size();i++) code[i]=i^0x1234abcdU;
        for (unsigned i=0;i<ram.size();i++) ram[i]=i^0x6543abcdU;
        for (unsigned i=0;i<boots.size();i++) boots[i]=i^0x5678abcdU;
        for (unsigned i=0;i<shims.size();i++) shims[i]=i^0x9abc1234U;
        for (unsigned i=0;i<resetDisable.size();i++) resetDisable[i]=(i*37U)&0x3ffU;
        stacklessCopies=ram;
    }
    bool AllRolesHeld(unsigned core) const {
        if (++heldChecks==failHeldCheck) return false;
        return held && core<DataflowBarrierWorkers;
    }
    bool DefaultBootRouting(unsigned core) const { return defaultRoute && core<DataflowBarrierWorkers; }
    bool LocalRamStable(unsigned core) const { return stable && core<DataflowBarrierWorkers; }
    bool StacklessFleetPreparationSafe(unsigned core) const {
        return held && stacklessPreparation && core<DataflowBarrierWorkers;
    }
    bool StacklessUncapturedRamProtected(unsigned core) const {
        return held && uncapturedProtected && core<DataflowBarrierWorkers;
    }
    void InstallHeldCopies(unsigned core) {
        assert(core<DataflowBarrierWorkers && retainedRam);
        const unsigned words=DataflowRoleRamCopyBytes/4;
        const unsigned readerAt=core*CodePerCore+(DataflowRoleReaderRamCopy-DataflowRoleExtraL1Address)/4;
        const unsigned writerAt=core*CodePerCore+(DataflowRoleWriterRamCopy-DataflowRoleExtraL1Address)/4;
        for (unsigned i=0;i<words;i++) {
            code[readerAt+i]=retainedRam[core*RamPerCore+i];
            code[writerAt+i]=retainedRam[core*RamPerCore+words+i];
        }
        copyCaptured[core]=true;
    }
    bool StacklessHeldCopySources(unsigned core,const uint32_t*& reader,const uint32_t*& writer,bool& captured) {
        if (!copyProof || !AllRolesHeld(core) || !retainedRam) return false;
        captured=copyCaptured[core];reader=writer=nullptr;
        if (captured) {
            reader=retainedRam+core*RamPerCore+(wrongCopyPointer?1:0);
            writer=retainedRam+core*RamPerCore+DataflowRoleLocalRamWords;
        }
        return true;
    }
    bool StacklessHeldCopySourceSafe(unsigned core,const uint32_t* reader,const uint32_t* writer) {
        const uint32_t* expectedReader=nullptr;const uint32_t* expectedWriter=nullptr;bool captured=false;
        return StacklessHeldCopySources(core,expectedReader,expectedWriter,captured) && captured
            && reader==expectedReader && writer==expectedWriter;
    }
    bool RetirementRamSnapshotSources(unsigned core,const uint32_t*& reader,const uint32_t*& writer,bool& captured) {
        return StacklessHeldCopySources(core,reader,writer,captured);
    }
    bool FleetRamImageRetirementSafe() const { return held && defaultRoute && retirementProof; }
    bool FleetRamImageRetirementRetrySafe() const { return FleetRamImageRetirementSafe() && retirementRetry; }
    bool CoreRamImageRetirementSafe(unsigned core,const uint32_t* reader,const uint32_t* writer) {
        return FleetRamImageRetirementSafe() && StacklessHeldCopySourceSafe(core,reader,writer);
    }
    bool CoreRamImageRetirementRetrySafe(unsigned core,const uint32_t* reader,const uint32_t* writer) {
        return retirementRetry && CoreRamImageRetirementSafe(core,reader,writer);
    }
    bool StacklessCoreRestoreReleaseReady(unsigned core,const uint32_t* reader,const uint32_t* writer) {
        return ++releaseReadyCalls!=failReleaseReadyCall && restoreReleaseReady && !staleRestoreReceipt
            && StacklessHeldCopySourceSafe(core,reader,writer);
    }
    bool AutoRestoreArmSafe(unsigned core,const uint32_t* reader,const uint32_t* writer) const {
        return autoArmProof && !staleRestoreReceipt && !held && retainedRam && core<DataflowBarrierWorkers
            && reader==retainedRam+core*RamPerCore
            && writer==reader+DataflowRoleLocalRamWords;
    }
    bool AutoRestoreHeldSafe(unsigned core,const uint32_t* reader,const uint32_t* writer) {
        return autoHeldProof && StacklessHeldCopySourceSafe(core,reader,writer);
    }
    bool AutoRestoreRetirementSafe(unsigned core,const uint32_t* reader,const uint32_t* writer) {
        return autoRetirementProof && StacklessHeldCopySourceSafe(core,reader,writer);
    }
    bool RestoreHandoffHeldSafe(unsigned core) const { return held && core<DataflowBarrierWorkers; }
    bool RestoreCompletionSafe(unsigned core) const { return !held && restoreCompletion && core<DataflowBarrierWorkers; }
    bool RestoreCompletedAndHeld(unsigned core) const { return held && restoreCompletedHeld && core<DataflowBarrierWorkers; }
    bool RestoreHandoffRetrySafe(unsigned core) const { return held && core<DataflowBarrierWorkers; }
    bool ReadRestoreHandoff(unsigned core,DataflowRoleSnapshotHandoff& result) const {
        if (core>=DataflowBarrierWorkers) return false;
        result=restoreHandoff[core];return true;
    }
    bool WriteRestoreHandoffWord(unsigned core,unsigned word,uint32_t value) {
        if (!RestoreHandoffHeldSafe(core) || word>=3) return false;
        ++writes;
        if (word==0) restoreHandoff[core].readerReady=value;
        else if (word==1) restoreHandoff[core].writerReady=value;
        else restoreHandoff[core].release=value;
        return true;
    }
    bool StacklessCoreRestoreProven(unsigned core,const uint32_t* reader,const uint32_t* writer) const {
        if (!coreRestoreProof || !AllRolesHeld(core) || !retainedRam) return false;
        const uint32_t* expected=retainedRam+core*RamPerCore;
        if (reader!=expected || writer!=expected+DataflowRoleLocalRamWords) return false;
        for (unsigned i=0;i<RamPerCore;i++) if (ram[core*RamPerCore+i]!=expected[i]) return false;
        return true;
    }
    bool StacklessCopiesSafe(unsigned core) const {
        return !held && stacklessSafe && core<DataflowBarrierWorkers;
    }
    bool ReadStacklessRamCopy(unsigned core,unsigned role,unsigned offset,uint32_t* out,unsigned bytes) {
        if (!StacklessCopiesSafe(core) || role>=DataflowRoleCount || !out || !bytes
            || (offset&3) || (bytes&3) || offset>DataflowRoleLocalRamBytes
            || bytes>DataflowRoleLocalRamBytes-offset) return false;
        if (copyCalls++==failCopyCall) return false;
        const unsigned at=core*RamPerCore+role*DataflowRoleLocalRamWords+offset/4;
        for (unsigned i=0;i<bytes/4;i++) out[i]=stacklessCopies[at+i];
        return true;
    }
    bool ReadResetDisable(unsigned core,uint32_t& value) {
        if (!AllRolesHeld(core)) return false;
        value=resetDisable[core]; return true;
    }
    bool WriteResetDisable(unsigned core,uint32_t value) {
        if (!AllRolesHeld(core)) return false;
        ++writes; resetDisable[core]=value;
        if (corruptPriorDisableOnWrite && core==37) {
            resetDisable[0]^=1; corruptPriorDisableOnWrite=false;
        }
        if (core==failDisableCore) { failDisableCore=UINT32_MAX; return false; }
        return true;
    }
    bool Read(unsigned core,unsigned address,uint32_t* out,unsigned bytes) {
        assert(AllRolesHeld(core) && out && DataflowRoleExtraL1Range(address,bytes));
        const unsigned at=core*CodePerCore+(address-DataflowRoleExtraL1Address)/4;
        for (unsigned i=0;i<bytes/4;i++) out[i]=code[at+i];
        return true;
    }
    bool Write(unsigned core,unsigned address,const uint32_t* in,unsigned bytes) {
        assert(AllRolesHeld(core) && in && DataflowRoleExtraL1Range(address,bytes));
        const unsigned at=core*CodePerCore+(address-DataflowRoleExtraL1Address)/4;
        ++writes;
        if (int(core)==failRetirementCodeCore) { code[at]=in[0];failRetirementCodeCore=-1;return false; }
        for (unsigned i=0;i<bytes/4;i++) code[at+i]=in[i];
        return true;
    }
    bool ReadBoot(unsigned core,unsigned address,uint32_t& value) {
        assert(AllRolesHeld(core) && DefaultBootRouting(core));
        const unsigned role=address==DataflowRoleBriscBootAddress?0:address==DataflowRoleNcriscBootAddress?1:2;
        assert(role<2); value=boots[core*2+role]; return true;
    }
    bool WriteBoot(unsigned core,unsigned address,uint32_t value) {
        assert(AllRolesHeld(core) && DefaultBootRouting(core));
        const unsigned role=address==DataflowRoleBriscBootAddress?0:address==DataflowRoleNcriscBootAddress?1:2;
        assert(role<2); ++writes; boots[core*2+role]=value;
        if (corruptRamOnBoot && core==3) { ram[119*RamPerCore]^=1; corruptRamOnBoot=false; }
        return true;
    }
    bool ReadShim(unsigned core,unsigned address,uint32_t* out,unsigned bytes) {
        assert(AllRolesHeld(core) && DefaultBootRouting(core) && out && !(bytes&3));
        if (address<DataflowRoleReaderShimAddress || address>DataflowRoleWriterShimLimit
            || bytes>DataflowRoleWriterShimLimit-address) return false;
        const unsigned at=core*DataflowRoleShimWords+(address-DataflowRoleReaderShimAddress)/4;
        for (unsigned i=0;i<bytes/4;i++) out[i]=shims[at+i];
        return true;
    }
    bool WriteShim(unsigned core,unsigned address,const uint32_t* in,unsigned bytes) {
        assert(AllRolesHeld(core) && DefaultBootRouting(core) && in && !(bytes&3));
        if (address<DataflowRoleReaderShimAddress || address>DataflowRoleWriterShimLimit
            || bytes>DataflowRoleWriterShimLimit-address) return false;
        const unsigned at=core*DataflowRoleShimWords+(address-DataflowRoleReaderShimAddress)/4;
        ++writes; for (unsigned i=0;i<bytes/4;i++) shims[at+i]=in[i];
        if (corruptCopyOnShimWrite) {
            code[core*CodePerCore+(DataflowRoleReaderRamCopy-DataflowRoleExtraL1Address)/4]^=1;
            corruptCopyOnShimWrite=false;
        }
        return true;
    }
    bool ReadRam(unsigned core,unsigned role,uint32_t address,uint32_t* out,unsigned bytes) {
        assert(AllRolesHeld(core) && LocalRamStable(core) && out && DataflowRoleLocalRamRange(role,address,bytes));
        ++privateRamReads;
        const uint32_t base=role==DataflowRoleReader?DataflowRoleBriscLocalRam:DataflowRoleNcriscLocalRam;
        const unsigned at=core*RamPerCore+role*DataflowRoleLocalRamWords+(address-base)/4;
        for (unsigned i=0;i<bytes/4;i++) out[i]=ram[at+i];
        return true;
    }
    bool WriteRam(unsigned core,unsigned role,uint32_t address,const uint32_t* in,unsigned bytes) {
        assert(AllRolesHeld(core) && LocalRamStable(core) && in && DataflowRoleLocalRamRange(role,address,bytes));
        const uint32_t base=role==DataflowRoleReader?DataflowRoleBriscLocalRam:DataflowRoleNcriscLocalRam;
        const unsigned at=core*RamPerCore+role*DataflowRoleLocalRamWords+(address-base)/4;
        ++writes; for (unsigned i=0;i<bytes/4;i++) ram[at+i]=in[i];
        if (corruptBootOnRam && core==3) { boots[0]^=1; corruptBootOnRam=false; }
        return true;
    }
};
int main()
{
    std::vector<uint32_t> codeSnapshot(DataflowRoleFleetRecoveryOwner::CodeWords());
    std::vector<uint32_t> ramSnapshot(DataflowRoleFleetRecoveryOwner::RamWords());
    const uint32_t codeA[]={1,2,3},codeB[]={4,5},shimA[]={6,7},shimB[]={8,9};
    {
        RecoveryIO io; const auto originalRam=io.ram,originalCode=io.code,originalBoots=io.boots,
            originalShims=io.shims,originalDisable=io.resetDisable;
        DataflowRoleFleetRecoveryOwner owner;
        assert(owner.SelectAutoRestoreMode() && owner.AutoRestoreMode()
            && !owner.SelectAutoRestoreMode() && !owner.SelectCoreLocalRestoreMode());
        assert(owner.CaptureAllStackless(codeSnapshot.data(),codeSnapshot.size(),
            ramSnapshot.data(),ramSnapshot.size(),io));
        assert(owner.StageImages(codeA,3,codeB,2,shimA,2,shimB,2,io)
            && owner.PreparePossibleRelease(io) && owner.BeginReleaseAttempt(io));
        io.retainedRam=ramSnapshot.data();io.held=false;
        assert(!owner.ArmAutoRestoreCore(0,io)); // no captured copy
        assert(owner.CaptureStacklessCore(0,io));
        io.autoArmProof=true;
        assert(!owner.ArmAutoRestoreCore(0,io)); // RAM not yet marked possibly dirty
        assert(owner.MarkStacklessMayRun(0,io));
        io.staleRestoreReceipt=true;
        assert(!owner.ArmAutoRestoreCore(0,io));
        io.staleRestoreReceipt=false;
        assert(owner.ArmAutoRestoreCore(0,io) && !owner.ArmAutoRestoreCore(0,io));
        io.ram[0]^=1;io.held=true;io.InstallHeldCopies(0);
        assert(!owner.MarkAutoRestoredCore(0,io));
        io.autoHeldProof=true;io.coreRestoreProof=true;
        assert(!owner.MarkAutoRestoredCore(0,io)); // saved RAM does not yet match
        io.ram[0]=originalRam[0];
        assert(owner.MarkAutoRestoredCore(0,io) && !owner.MarkAutoRestoredCore(0,io));
        assert(!owner.StageCoreLocalRestore(0,shimA,2,shimB,2,io));
        const unsigned writes=io.writes,reads=io.privateRamReads;
        io.retirementProof=true;
        assert(!owner.RestoreAfterCoreLocalFleetProof(io)); // never use the separate-shim path
        assert(!owner.RestoreAfterAutoRestoreFleetProof(io) && io.writes==writes);
        io.autoRetirementProof=true;io.wrongCopyPointer=true;
        assert(!owner.RestoreAfterAutoRestoreFleetProof(io) && io.writes==writes);
        io.wrongCopyPointer=false;
        io.failRetirementCodeCore=0;
        assert(!owner.RestoreAfterAutoRestoreFleetProof(io) && owner.RestorationRequired());
        const unsigned partial=io.writes;
        assert(!owner.RestoreAfterAutoRestoreFleetProof(io) && io.writes==partial);
        io.retirementRetry=true;
        assert(owner.RestoreAfterAutoRestoreFleetProof(io) && owner.CoveredStateRestored()
            && owner.VerifyAfterAutoResidentRestore(io) && io.privateRamReads==reads);
        assert(io.ram==originalRam && io.code==originalCode && io.boots==originalBoots
            && io.shims==originalShims && io.resetDisable==originalDisable);
        io.autoRetirementProof=false;
        assert(!owner.VerifyAfterAutoResidentRestore(io) && owner.RestorationRequired());
    }
    {
        // A partial fleet cannot retire until every captured core has an
        // independently proved restore, even when another core is complete.
        RecoveryIO io; DataflowRoleFleetRecoveryOwner owner;
        assert(owner.SelectAutoRestoreMode()
            && owner.CaptureAllStackless(codeSnapshot.data(),codeSnapshot.size(),
                ramSnapshot.data(),ramSnapshot.size(),io)
            && owner.StageImages(codeA,3,codeB,2,shimA,2,shimB,2,io)
            && owner.PreparePossibleRelease(io) && owner.BeginReleaseAttempt(io));
        io.retainedRam=ramSnapshot.data();io.held=false;io.autoArmProof=true;
        for (unsigned core=0;core<2;core++)
            assert(owner.CaptureStacklessCore(core,io) && owner.MarkStacklessMayRun(core,io)
                && owner.ArmAutoRestoreCore(core,io));
        io.held=true;io.InstallHeldCopies(0);io.InstallHeldCopies(1);
        io.autoHeldProof=true;io.coreRestoreProof=true;
        assert(owner.MarkAutoRestoredCore(0,io));
        io.retirementProof=true;io.autoRetirementProof=true;
        const unsigned writes=io.writes;
        assert(!owner.RestoreAfterAutoRestoreFleetProof(io) && io.writes==writes);
        io.autoHeldProof=false;
        assert(!owner.MarkAutoRestoredCore(1,io));
        io.autoHeldProof=true;
        assert(owner.MarkAutoRestoredCore(1,io)
            && owner.RestoreAfterAutoRestoreFleetProof(io)
            && owner.VerifyAfterAutoResidentRestore(io));
    }
    for (bool autoMode:{false,true}) {
        // Before any release ledger, cleanup has no captured private RAM to
        // touch and can safely restore staged images without a host RAM read.
        RecoveryIO io; const auto code=io.code,boots=io.boots,shims=io.shims,disable=io.resetDisable;
        DataflowRoleFleetRecoveryOwner owner;
        assert((autoMode?owner.SelectAutoRestoreMode():owner.SelectCoreLocalRestoreMode())
            && owner.CaptureAllStackless(codeSnapshot.data(),codeSnapshot.size(),
                ramSnapshot.data(),ramSnapshot.size(),io)
            && owner.StageImages(codeA,3,codeB,2,shimA,2,shimB,2,io));
        assert(!owner.CopyAwareReleaseStarted());
        assert(owner.RestoreAll(io) && owner.CoveredStateRestored() && !io.privateRamReads);
        assert(owner.VerifyAfterResidentRestore(io));
        assert(io.code==code && io.boots==boots && io.shims==shims && io.resetDisable==disable);
    }
    {
        // An uncertain first release with no captured core still needs
        // uncaptured-fleet protection, but can complete without host RAM IO.
        RecoveryIO io;const auto code=io.code,boots=io.boots,shims=io.shims,disable=io.resetDisable;
        DataflowRoleFleetRecoveryOwner owner;
        assert(owner.SelectCoreLocalRestoreMode()
            && owner.CaptureAllStackless(codeSnapshot.data(),codeSnapshot.size(),
                ramSnapshot.data(),ramSnapshot.size(),io)
            && owner.StageImages(codeA,3,codeB,2,shimA,2,shimB,2,io)
            && owner.PreparePossibleRelease(io) && owner.BeginReleaseAttempt(io));
        io.retainedRam=ramSnapshot.data();io.retirementProof=true;io.uncapturedProtected=false;
        const unsigned writes=io.writes;
        assert(!owner.RestoreAfterCoreLocalFleetProof(io) && io.writes==writes);
        io.uncapturedProtected=true;
        assert(owner.RestoreAfterCoreLocalFleetProof(io) && owner.CoveredStateRestored()
            && !io.privateRamReads && io.code==code && io.boots==boots && io.shims==shims
            && io.resetDisable==disable);
    }
    {
        // Restore shim staging is bound to the fleet ledger's actual retained
        // snapshot pointers, with complete held-fleet and reset audits.
        RecoveryIO io; DataflowRoleFleetRecoveryOwner owner;
        assert(owner.SelectCoreLocalRestoreMode()
            && owner.CaptureAllStackless(codeSnapshot.data(),codeSnapshot.size(),
                ramSnapshot.data(),ramSnapshot.size(),io)
            && owner.StageImages(codeA,3,codeB,2,shimA,2,shimB,2,io)
            && owner.PreparePossibleRelease(io) && owner.BeginReleaseAttempt(io));
        io.held=false;
        assert(owner.CaptureStacklessCore(0,io) && owner.MarkStacklessMayRun(0,io)
            && owner.CaptureStacklessCore(1,io) && owner.MarkStacklessMayRun(1,io));
        io.held=true;io.retainedRam=ramSnapshot.data();
        io.InstallHeldCopies(0);io.InstallHeldCopies(1);
        const unsigned reads=io.privateRamReads,writes=io.writes;
        assert(!owner.StageCoreLocalRestore(2,shimB,2,shimA,2,io) && io.writes==writes);
        io.wrongCopyPointer=true;
        assert(!owner.StageCoreLocalRestore(0,shimB,2,shimA,2,io) && io.writes==writes);
        io.wrongCopyPointer=false;
        io.copyCaptured[1]=false;
        assert(!owner.StageCoreLocalRestore(0,shimB,2,shimA,2,io) && io.writes==writes);
        io.copyCaptured[1]=true;
        assert(owner.StageCoreLocalRestore(0,shimB,2,shimA,2,io));
        assert(owner.StageCoreLocalRestore(1,shimB,2,shimA,2,io));
        assert(!owner.StageCoreLocalRestore(0,shimB,2,shimA,2,io));
        assert(!owner.BeginCoreLocalRestoreRelease(0,io)); // handoff not armed
        assert(owner.PrepareCoreLocalRestoreHandoff(0,io) && owner.PrepareCoreLocalRestoreHandoff(1,io));
        assert(!owner.PrepareCoreLocalRestoreHandoff(0,io));
        assert(!owner.BeginCoreLocalRestoreRelease(0,io)); // scope denies release
        io.restoreReleaseReady=true;io.staleRestoreReceipt=true;
        assert(!owner.BeginCoreLocalRestoreRelease(0,io));
        io.staleRestoreReceipt=false;
        io.failReleaseReadyCall=io.releaseReadyCalls+2;
        assert(!owner.BeginCoreLocalRestoreRelease(0,io)); // proof lost during audit
        io.failReleaseReadyCall=0;
        assert(owner.BeginCoreLocalRestoreRelease(0,io)
            && !owner.BeginCoreLocalRestoreRelease(0,io)
            && !owner.StageCoreLocalRestore(0,shimB,2,shimA,2,io));
        assert(owner.BeginCoreLocalRestoreRelease(1,io));
        assert(io.privateRamReads==reads && owner.RestorationRequired()
            && !owner.CoveredStateRestored() && !owner.RestoreAll(io));
    }
    {
        RecoveryIO io; DataflowRoleFleetRecoveryOwner owner;
        assert(owner.SelectCoreLocalRestoreMode()
            && owner.CaptureAllStackless(codeSnapshot.data(),codeSnapshot.size(),
                ramSnapshot.data(),ramSnapshot.size(),io)
            && owner.StageImages(codeA,3,codeB,2,shimA,2,shimB,2,io)
            && owner.PreparePossibleRelease(io) && owner.BeginReleaseAttempt(io));
        io.held=false;
        assert(owner.CaptureStacklessCore(0,io) && owner.MarkStacklessMayRun(0,io));
        io.held=true;io.retainedRam=ramSnapshot.data();io.InstallHeldCopies(0);
        io.corruptCopyOnShimWrite=true;
        assert(!owner.StageCoreLocalRestore(0,shimB,2,shimA,2,io));
        assert(owner.RestorationRequired() && !owner.CoveredStateRestored()
            && !owner.RestoreAll(io) && !io.privateRamReads);
    }
    for (unsigned fault=0;fault<2;fault++) {
        RecoveryIO io; const auto original=io.ram,originalCode=io.code,originalBoots=io.boots,
            originalShims=io.shims,originalDisable=io.resetDisable; DataflowRoleFleetRecoveryOwner owner;
        assert(owner.SelectCoreLocalRestoreMode() && owner.CoreLocalRestoreMode()
            && !owner.SelectCoreLocalRestoreMode());
        assert(!owner.CaptureAll(codeSnapshot.data(),codeSnapshot.size(),ramSnapshot.data(),ramSnapshot.size(),io));
        assert(owner.CaptureAllStackless(codeSnapshot.data(),codeSnapshot.size(),
            ramSnapshot.data(),ramSnapshot.size(),io));
        assert(owner.StageImages(codeA,3,codeB,2,shimA,2,shimB,2,io)
            && owner.PreparePossibleRelease(io) && owner.BeginReleaseAttempt(io));
        assert(!owner.SelectCoreLocalRestoreMode());
        io.held=false;
        assert(owner.CaptureStacklessCore(0,io) && owner.MarkStacklessMayRun(0,io)
            && owner.CaptureStacklessCore(1,io) && owner.MarkStacklessMayRun(1,io));
        io.ram[0]^=1;io.ram[RecoveryIO::RamPerCore]^=1;io.held=true;
        io.retainedRam=ramSnapshot.data();io.InstallHeldCopies(0);io.InstallHeldCopies(1);
        assert(!owner.MarkCoreRestoredByCore(0,io)); // no restore release ledger
        assert(owner.StageCoreLocalRestore(0,shimB,2,shimA,2,io)
            && owner.StageCoreLocalRestore(1,shimB,2,shimA,2,io));
        assert(owner.PrepareCoreLocalRestoreHandoff(0,io) && owner.PrepareCoreLocalRestoreHandoff(1,io));
        io.restoreReleaseReady=true;
        assert(owner.BeginCoreLocalRestoreRelease(0,io) && owner.BeginCoreLocalRestoreRelease(1,io));
        assert(!owner.MarkCoreRestoredByCore(0,io)); // no fresh completion
        io.held=false;io.restoreCompletion=true;
        io.restoreHandoff[0]={PIONEER_Q35_ROLE_RESTORE_READER_DONE,PIONEER_Q35_ROLE_RESTORE_WRITER_DONE,0};
        io.restoreHandoff[1]=io.restoreHandoff[0];
        assert(owner.ConfirmCoreLocalRestoreHandoff(0,io) && owner.ConfirmCoreLocalRestoreHandoff(1,io));
        io.held=true;io.restoreCompletedHeld=true;
        const unsigned writes=io.writes,reads=io.privateRamReads;
        assert(!owner.RestoreAll(io) && owner.RestorationRequired()
            && io.writes==writes && io.privateRamReads==reads);
        assert(!owner.MarkCoreRestoredByCore(0,io));
        io.coreRestoreProof=true;
        assert(!owner.MarkCoreRestoredByCore(0,io)); // private data still differs
        io.ram[0]=original[0];
        assert(owner.MarkCoreRestoredByCore(0,io) && !owner.VerifyCoreLocalRestores(io));
        io.ram[RecoveryIO::RamPerCore]=original[RecoveryIO::RamPerCore];
        assert(owner.MarkCoreRestoredByCore(1,io) && owner.VerifyCoreLocalRestores(io));
        io.retirementProof=true;
        assert(!owner.RestoreAfterCoreLocalFleetProof(io) && io.writes==writes); // handoffs retained
        io.retirementProof=false;
        assert(owner.RestoreCoreLocalHandoff(0,io) && owner.RestoreCoreLocalHandoff(1,io));
        assert(owner.RestorationRequired() && !owner.CoveredStateRestored()
            && !owner.RestoreAll(io) && io.writes==writes+6 && io.privateRamReads==reads);
        const unsigned beforeRetirement=io.writes;
        assert(!owner.RestoreAfterCoreLocalFleetProof(io) && io.writes==beforeRetirement);
        io.retirementProof=true;io.wrongCopyPointer=true;
        assert(!owner.RestoreAfterCoreLocalFleetProof(io) && io.writes==beforeRetirement);
        io.wrongCopyPointer=false;
        if (fault==0) io.failRetirementCodeCore=0;
        else io.failDisableCore=37;
        assert(!owner.RestoreAfterCoreLocalFleetProof(io) && owner.RestorationRequired());
        const unsigned partial=io.writes;
        assert(!owner.RestoreAfterCoreLocalFleetProof(io) && io.writes==partial);
        io.retirementRetry=true;
        assert(owner.RestoreAfterCoreLocalFleetProof(io) && owner.CoveredStateRestored()
            && owner.VerifyAfterCoreLocalResidentRestore(io) && io.privateRamReads==reads);
        assert(io.ram==original && io.code==originalCode && io.boots==originalBoots
            && io.shims==originalShims && io.resetDisable==originalDisable);
        if (fault==0) {
            io.code[0]^=1; // a later resident restore touches an earlier image
            assert(!owner.VerifyAfterCoreLocalResidentRestore(io) && owner.RestorationRequired());
            const unsigned beforeRetry=io.writes;io.retirementRetry=false;
            assert(!owner.RestoreAfterCoreLocalFleetProof(io) && io.writes==beforeRetry);
            io.retirementRetry=true;
            assert(owner.RestoreAfterCoreLocalFleetProof(io)
                && owner.VerifyAfterCoreLocalResidentRestore(io) && io.code==originalCode
                && io.privateRamReads==reads);
        }
    }
    {
        RecoveryIO io; DataflowRoleFleetRecoveryOwner owner;
        io.stacklessPreparation=false;
        assert(!owner.CaptureAllStackless(codeSnapshot.data(),codeSnapshot.size(),
            ramSnapshot.data(),ramSnapshot.size(),io));
        assert(owner.Untouched() && !io.writes && !io.privateRamReads);
    }
    {
        RecoveryIO io; const auto code=io.code,ram=io.ram,boots=io.boots,shims=io.shims,disable=io.resetDisable;
        DataflowRoleFleetRecoveryOwner owner;
        io.stable=false; // deferred preparation must not read private RAM
        assert(owner.CaptureAllStackless(codeSnapshot.data(),codeSnapshot.size(),
            ramSnapshot.data(),ramSnapshot.size(),io));
        assert(!owner.StacklessCopiesCaptured(0) && !owner.StacklessCopiesCaptured(DataflowBarrierWorkers));
        assert(!io.privateRamReads && !owner.OriginalRamWords(0,DataflowRoleReader));
        assert(owner.StageImages(codeA,3,codeB,2,shimA,2,shimB,2,io));
        assert(owner.PreparePossibleRelease(io) && owner.ReleaseLedgerPrepared());
        assert(owner.BeginReleaseAttempt(io) && !io.privateRamReads);
        io.held=false;
        assert(owner.CaptureStacklessCore(0,io) && owner.MarkStacklessMayRun(0,io));
        assert(owner.StacklessCopiesCaptured(0) && !owner.StacklessCopiesCaptured(1));
        assert(owner.CaptureStacklessCore(1,io)); // snapshot retained, stack not released
        assert(owner.OriginalRamWords(0,DataflowRoleReader)[0]==ram[0]);
        io.ram[0]^=1; io.held=true; io.stable=true; io.uncapturedProtected=false;
        const unsigned writes=io.writes;
        assert(!owner.RestoreAll(io) && owner.RestorationRequired() && io.writes==writes);
        io.uncapturedProtected=true;
        assert(owner.RestoreAll(io) && owner.CoveredStateRestored());
        assert(owner.VerifyAfterResidentRestore(io));
        assert(io.code==code && io.ram==ram && io.boots==boots && io.shims==shims && io.resetDisable==disable);
    }
    {
        RecoveryIO io; const auto ram=io.ram;
        DataflowRoleFleetRecoveryOwner owner;
        assert(owner.CaptureAllStackless(codeSnapshot.data(),codeSnapshot.size(),
            ramSnapshot.data(),ramSnapshot.size(),io));
        assert(owner.StageImages(codeA,3,codeB,2,shimA,2,shimB,2,io));
        assert(owner.PreparePossibleRelease(io) && owner.BeginReleaseAttempt(io));
        io.held=false;
        for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
            assert(owner.CaptureStacklessCore(core,io));
            assert(owner.MarkStacklessMayRun(core,io));
        }
        assert(!io.privateRamReads);
        io.ram[119*RecoveryIO::RamPerCore]^=1; io.held=true;
        assert(owner.RestoreAll(io) && owner.VerifyAfterResidentRestore(io) && io.ram==ram);
    }
    {
        RecoveryIO io; const auto ram=io.ram;
        DataflowRoleFleetRecoveryOwner owner;
        assert(owner.CaptureAllStackless(codeSnapshot.data(),codeSnapshot.size(),
            ramSnapshot.data(),ramSnapshot.size(),io));
        assert(owner.StageImages(codeA,3,codeB,2,shimA,2,shimB,2,io));
        assert(owner.PreparePossibleRelease(io) && owner.BeginReleaseAttempt(io));
        io.held=false;
        assert(owner.CaptureStacklessCore(0,io) && owner.MarkStacklessMayRun(0,io));
        io.ram[0]^=1; io.failCopyCall=io.copyCalls+2;
        assert(!owner.CaptureStacklessCore(1,io) && owner.RestorationRequired());
        assert(owner.StacklessCopiesCaptured(0) && !owner.StacklessCopiesCaptured(1));
        assert(!owner.OriginalRamWords(1,DataflowRoleReader));
        assert(!owner.CaptureStacklessCore(2,io));
        io.held=true; io.failCopyCall=UINT32_MAX;
        assert(owner.RestoreAll(io) && owner.VerifyAfterResidentRestore(io) && io.ram==ram);
    }
    // Fail the composed image and inner code fleet preflights after the
    // parent safety audit, with every original snapshot already captured.
    for (unsigned check : {DataflowBarrierWorkers+1U,2U*DataflowBarrierWorkers+1U}) {
        RecoveryIO io; const auto code=io.code,ram=io.ram,boots=io.boots,shims=io.shims,disable=io.resetDisable;
        DataflowRoleFleetRecoveryOwner owner;
        assert(!owner.OriginalRamWords(0,DataflowRoleReader));
        assert(owner.CaptureAll(codeSnapshot.data(),codeSnapshot.size(),ramSnapshot.data(),ramSnapshot.size(),io));
        for (unsigned core:{0U,60U,119U}) for (unsigned role=0;role<DataflowRoleCount;role++) {
            const auto* saved=owner.OriginalRamWords(core,role);
            const unsigned offset=core*RecoveryIO::RamPerCore+role*DataflowRoleLocalRamWords;
            assert(saved && saved[0]==ram[offset] && saved[DataflowRoleLocalRamWords-1]==ram[offset+DataflowRoleLocalRamWords-1]);
        }
        assert(!owner.OriginalRamWords(DataflowBarrierWorkers,0) && !owner.OriginalRamWords(0,DataflowRoleCount));
        io.heldChecks=0; io.failHeldCheck=check;
        assert(!owner.StageImages(codeA,3,codeB,2,shimA,2,shimB,2,io));
        assert(owner.RestorationRequired() && io.writes==0 && io.heldChecks==check);
        assert(owner.RestoreAll(io) && owner.CoveredStateRestored());
        assert(owner.VerifyAfterResidentRestore(io));
        assert(io.code==code && io.ram==ram && io.boots==boots && io.shims==shims && io.resetDisable==disable);
    }
    {
        RecoveryIO io; const auto code=io.code,ram=io.ram,boots=io.boots,shims=io.shims,disable=io.resetDisable;
        DataflowRoleFleetRecoveryOwner owner;
        assert(owner.CaptureAll(codeSnapshot.data(),codeSnapshot.size(),ramSnapshot.data(),ramSnapshot.size(),io));
        assert(owner.StageImages(codeA,3,codeB,2,shimA,2,shimB,2,io));
        assert(owner.PreparePossibleRelease(io) && owner.ReleaseLedgerPrepared());
        for (unsigned core=0;core<DataflowBarrierWorkers;core++)
            assert(io.resetDisable[core]==(disable[core]|DataflowRolePreserveResetMask));
        assert(owner.BeginReleaseAttempt(io) && !owner.ReleaseLedgerPrepared());
        assert(!owner.BeginReleaseAttempt(io));
        io.ram.assign(io.ram.size(),0); // simulate B/NC programs overwriting private stack RAM
        io.stable=false; const unsigned writes=io.writes;
        assert(!owner.RestoreAll(io) && owner.RestorationRequired() && io.writes==writes);
        io.stable=true; io.corruptBootOnRam=true;
        assert(!owner.RestoreAll(io) && owner.RestorationRequired());
        assert(owner.RestoreAll(io) && owner.CoveredStateRestored());
        assert(io.code==code && io.ram==ram && io.boots==boots && io.shims==shims && io.resetDisable==disable);
        assert(owner.VerifyAfterResidentRestore(io));
        // A later TRISC L1 restore can overlap a shim or misroute into a
        // different tile's role storage. Both must be detected while held.
        io.shims[0]^=1;
        io.ram[119*RecoveryIO::RamPerCore]^=1;
        io.resetDisable[118]^=8;
        assert(!owner.VerifyAfterResidentRestore(io) && owner.RestorationRequired());
        assert(!owner.RestoreAll(io) && owner.RestorationRequired()); // RAM audit now detects the second fault
        assert(owner.RestoreAll(io) && owner.CoveredStateRestored());
        assert(owner.VerifyAfterResidentRestore(io));
        assert(io.code==code && io.ram==ram && io.boots==boots && io.shims==shims && io.resetDisable==disable);
    }
    {
        RecoveryIO io; const auto disable=io.resetDisable;
        DataflowRoleFleetRecoveryOwner owner;
        assert(owner.CaptureAll(codeSnapshot.data(),codeSnapshot.size(),ramSnapshot.data(),ramSnapshot.size(),io));
        assert(owner.StageImages(codeA,3,codeB,2,shimA,2,shimB,2,io));
        io.failDisableCore=37; // write took effect but its completion is uncertain
        assert(!owner.PreparePossibleRelease(io) && owner.RestorationRequired());
        assert(!owner.BeginReleaseAttempt(io));
        assert(owner.RestoreAll(io) && owner.VerifyAfterResidentRestore(io));
        assert(io.resetDisable==disable);
    }
    {
        RecoveryIO io; const auto disable=io.resetDisable;
        DataflowRoleFleetRecoveryOwner owner;
        assert(owner.CaptureAll(codeSnapshot.data(),codeSnapshot.size(),ramSnapshot.data(),ramSnapshot.size(),io));
        assert(owner.StageImages(codeA,3,codeB,2,shimA,2,shimB,2,io));
        assert(owner.PreparePossibleRelease(io));
        io.resetDisable[119]&=~DataflowRolePreserveResetMask;
        assert(!owner.BeginReleaseAttempt(io) && owner.RestorationRequired());
        assert(owner.RestoreAll(io) && owner.VerifyAfterResidentRestore(io));
        assert(io.resetDisable==disable);
    }
    {
        RecoveryIO io; const auto disable=io.resetDisable;
        DataflowRoleFleetRecoveryOwner owner;
        assert(owner.CaptureAll(codeSnapshot.data(),codeSnapshot.size(),ramSnapshot.data(),ramSnapshot.size(),io));
        assert(owner.StageImages(codeA,3,codeB,2,shimA,2,shimB,2,io));
        io.corruptPriorDisableOnWrite=true;
        assert(!owner.PreparePossibleRelease(io) && owner.RestorationRequired());
        assert(!owner.BeginReleaseAttempt(io));
        assert(owner.RestoreAll(io) && owner.VerifyAfterResidentRestore(io));
        assert(io.resetDisable==disable);
    }
    {
        RecoveryIO io; const auto code=io.code,ram=io.ram,boots=io.boots,shims=io.shims;
        DataflowRoleFleetRecoveryOwner owner;
        assert(owner.CaptureAll(codeSnapshot.data(),codeSnapshot.size(),ramSnapshot.data(),ramSnapshot.size(),io));
        io.corruptRamOnBoot=true;
        assert(!owner.StageImages(codeA,3,codeB,2,shimA,2,shimB,2,io));
        assert(owner.RestorationRequired() && !owner.ReleaseLedgerPrepared());
        assert(owner.RestoreAll(io) && owner.CoveredStateRestored());
        assert(io.code==code && io.ram==ram && io.boots==boots && io.shims==shims);
    }
    std::puts("PASS 120-core composed image/private-RAM recovery and cross-region faults; SIMULATED ONLY");
    return 0;
}
