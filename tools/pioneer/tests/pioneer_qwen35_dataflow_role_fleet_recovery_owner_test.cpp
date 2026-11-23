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
        ++writes; for (unsigned i=0;i<bytes/4;i++) code[at+i]=in[i];
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
