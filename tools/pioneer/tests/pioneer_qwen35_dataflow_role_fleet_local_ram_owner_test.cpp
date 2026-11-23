/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowRoleFleetLocalRamOwner.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace PioneerQwen35;
struct FleetRamIO {
    static constexpr unsigned PerCore=DataflowRoleLocalRamOwner::RequiredWords();
    std::vector<uint32_t> memory,copies;
    bool held=true,stable=true,corruptEnabled=false;
    bool stacklessSafe=true,uncapturedProtected=true;
    bool coreRestoreProof=false;
    int driftWhenProvingCore=-1;
    const uint32_t* ownedSnapshots=nullptr;
    int failCore=-1,readFailCore=-1,unstableAfterReadCore=-1,corruptFrom=-1,corruptTarget=-1;
    int failCopyCore=-1;
    unsigned revokeProtectionAtWrite=UINT32_MAX;
    unsigned writes=0;
    unsigned reads[DataflowBarrierWorkers]{};
    FleetRamIO():memory(DataflowRoleFleetLocalRamOwner::RequiredWords()) {
        for (unsigned i=0;i<memory.size();i++) memory[i]=i^0x7733cc44U;
        copies=memory;
    }
    bool AllRolesHeld(unsigned core) const { return held && core<DataflowBarrierWorkers; }
    bool LocalRamStable(unsigned core) const { return stable && core<DataflowBarrierWorkers; }
    bool StacklessCopiesSafe(unsigned core) const { return !held && stacklessSafe && core<DataflowBarrierWorkers; }
    bool StacklessUncapturedRamProtected(unsigned core) const {
        return held && uncapturedProtected && core<DataflowBarrierWorkers;
    }
    bool StacklessCoreRestoreProven(unsigned core,const uint32_t* reader,const uint32_t* writer) {
        if (!coreRestoreProof || !AllRolesHeld(core) || !ownedSnapshots) return false;
        if (int(core)==driftWhenProvingCore) { memory[0]^=1;driftWhenProvingCore=-1; }
        const uint32_t* expected=ownedSnapshots+core*PerCore;
        if (reader!=expected || writer!=expected+DataflowRoleLocalRamWords) return false;
        for (unsigned i=0;i<PerCore;i++) if (memory[core*PerCore+i]!=expected[i]) return false;
        return true;
    }
    bool ReadStacklessRamCopy(unsigned core,unsigned role,unsigned offset,uint32_t* output,unsigned bytes) {
        if (!StacklessCopiesSafe(core) || int(core)==failCopyCore || role>=DataflowRoleCount || !output
            || !bytes || (offset&3) || (bytes&3) || offset>DataflowRoleLocalRamBytes
            || bytes>DataflowRoleLocalRamBytes-offset) return false;
        const unsigned first=core*PerCore+role*DataflowRoleLocalRamWords+offset/4;
        for (unsigned i=0;i<bytes/4;i++) output[i]=copies[first+i];
        return true;
    }
    bool ReadRam(unsigned core,unsigned role,uint32_t address,uint32_t* output,unsigned bytes) {
        assert(AllRolesHeld(core) && LocalRamStable(core) && output && DataflowRoleLocalRamRange(role,address,bytes));
        ++reads[core];
        if (int(core)==readFailCore) return false;
        const uint32_t base=role==DataflowRoleReader?DataflowRoleBriscLocalRam:DataflowRoleNcriscLocalRam;
        const unsigned offset=core*PerCore+role*DataflowRoleLocalRamWords+(address-base)/4;
        for (unsigned i=0;i<bytes/4;i++) output[i]=memory[offset+i];
        if (int(core)==unstableAfterReadCore) stable=false;
        return true;
    }
    bool WriteRam(unsigned core,unsigned role,uint32_t address,const uint32_t* input,unsigned bytes) {
        assert(AllRolesHeld(core) && LocalRamStable(core) && input && DataflowRoleLocalRamRange(role,address,bytes));
        const uint32_t base=role==DataflowRoleReader?DataflowRoleBriscLocalRam:DataflowRoleNcriscLocalRam;
        const unsigned offset=core*PerCore+role*DataflowRoleLocalRamWords+(address-base)/4;
        ++writes;
        if (writes==revokeProtectionAtWrite) uncapturedProtected=false;
        if (int(core)==failCore) { memory[offset]=input[0]; failCore=-1; return false; }
        for (unsigned i=0;i<bytes/4;i++) memory[offset+i]=input[i];
        if (corruptEnabled && int(core)==corruptFrom) {
            memory[unsigned(corruptTarget)*PerCore]^=1; corruptEnabled=false;
        }
        return true;
    }
};
int main()
{
    std::vector<uint32_t> snapshot(DataflowRoleFleetLocalRamOwner::RequiredWords());
    FleetRamIO io; const auto original=io.memory; DataflowRoleFleetLocalRamOwner fleet;
    assert(fleet.CaptureAll(snapshot.data(),snapshot.size(),io));
    assert(fleet.MarkMayRunAll(io) && fleet.MarkedForPossibleRelease());
    io.memory.assign(io.memory.size(),0); // reset clears private RAMs
    io.stable=false; const unsigned writes=io.writes;
    assert(!fleet.RestoreAll(io) && io.writes==writes && !fleet.MarkedForPossibleRelease());
    io.stable=true; io.failCore=3;
    assert(!fleet.RestoreAll(io) && fleet.RestorationRequired());
    io.corruptEnabled=true; io.corruptFrom=3; io.corruptTarget=0;
    assert(!fleet.RestoreAll(io) && fleet.RestorationRequired());
    assert(fleet.RestoreAll(io) && fleet.RestoredOriginal() && io.memory==original);
    io.memory[119*FleetRamIO::PerCore]^=1;
    assert(!fleet.VerifyOriginalAll(io) && fleet.RestorationRequired());
    assert(fleet.RestoreAll(io) && io.memory==original);
    // Multiple corruptions are all identified before the single restore.
    for (unsigned core=0;core<DataflowBarrierWorkers;core++)
        io.memory[core*FleetRamIO::PerCore]^=1;
    assert(!fleet.VerifyOriginalAll(io) && fleet.RestorationRequired());
    assert(fleet.RestoreAll(io) && fleet.RestoredOriginal() && io.memory==original);
    // A failed read is not classified as a normal mismatch. Stop the scan
    // before subsequent cores and do not write during the failed audit.
    for (unsigned& count:io.reads) count=0;
    io.memory[2*FleetRamIO::PerCore]^=1; io.readFailCore=5;
    const unsigned before=io.writes;
    assert(!fleet.VerifyOriginalAll(io) && io.writes==before && io.reads[5]==1);
    for (unsigned core=6;core<DataflowBarrierWorkers;core++) assert(io.reads[core]==0);
    io.readFailCore=-1;
    assert(fleet.RestoreAll(io) && io.memory==original);
    // A differing read is not accepted as a safe mismatch if the selected
    // worker loses its stable-RAM proof during that same read.
    for (unsigned& count:io.reads) count=0;
    io.memory[4*FleetRamIO::PerCore]^=1; io.unstableAfterReadCore=4;
    assert(!fleet.VerifyOriginalAll(io) && io.reads[4]==1);
    for (unsigned core=5;core<DataflowBarrierWorkers;core++) assert(io.reads[core]==0);
    io.unstableAfterReadCore=-1; io.stable=true;
    assert(fleet.RestoreAll(io) && io.memory==original);
    {
        FleetRamIO staged; const auto before=staged.memory;
        std::vector<uint32_t> saved(DataflowRoleFleetLocalRamOwner::RequiredWords());
        DataflowRoleFleetLocalRamOwner partial;
        assert(!partial.PrepareStackless(saved.data(),saved.size()-1));
        assert(partial.PrepareStackless(saved.data(),saved.size()));
        assert(!partial.OriginalWords(0,DataflowRoleReader));
        staged.held=false;
        for (unsigned core=0;core<3;core++) assert(partial.CaptureStacklessCore(core,staged));
        assert(!partial.CaptureStacklessCore(2,staged));
        assert(!partial.OriginalWords(3,DataflowRoleReader));
        assert(partial.MarkStacklessMayRun(0,staged) && partial.MarkStacklessMayRun(1,staged));
        staged.memory[0]^=1; staged.memory[FleetRamIO::PerCore]^=2;
        assert(!partial.RestoreStackless(staged) && !staged.writes);
        staged.held=true; staged.uncapturedProtected=false;
        assert(!partial.RestoreStackless(staged) && !staged.writes);
        staged.uncapturedProtected=true;
        staged.revokeProtectionAtWrite=staged.writes+1;
        assert(!partial.RestoreStackless(staged) && staged.writes==1);
        staged.revokeProtectionAtWrite=UINT32_MAX; staged.uncapturedProtected=true; staged.failCore=1;
        assert(!partial.RestoreStackless(staged) && partial.RestorationRequired());
        staged.corruptEnabled=true; staged.corruptFrom=1; staged.corruptTarget=0;
        assert(!partial.RestoreStackless(staged) && partial.RestorationRequired());
        assert(partial.RestoreStackless(staged) && partial.RestoredOriginal() && staged.memory==before);
        assert(partial.VerifyStacklessRestored(staged));
        staged.uncapturedProtected=false;
        assert(!partial.VerifyStacklessRestored(staged) && partial.RestorationRequired());
        staged.uncapturedProtected=true;
        assert(partial.RestoreStackless(staged) && staged.memory==before);
        staged.memory[0]^=1;
        assert(!partial.VerifyStacklessRestored(staged) && partial.RestorationRequired());
        assert(partial.RestoreStackless(staged) && staged.memory==before);
    }
    {
        FleetRamIO staged; const auto before=staged.memory;
        std::vector<uint32_t> saved(DataflowRoleFleetLocalRamOwner::RequiredWords());
        DataflowRoleFleetLocalRamOwner partial;
        assert(partial.PrepareStackless(saved.data(),saved.size()));staged.held=false;
        assert(partial.CaptureStacklessCore(0,staged) && partial.CaptureStacklessCore(1,staged));
        assert(partial.MarkStacklessMayRun(0,staged) && partial.MarkStacklessMayRun(1,staged));
        staged.memory[0]^=1;staged.memory[FleetRamIO::PerCore]^=2;
        staged.held=true;staged.ownedSnapshots=saved.data();
        const unsigned writes=staged.writes;
        assert(!partial.MarkCoreRestoredByCore(0,staged) && partial.RestorationRequired());
        staged.coreRestoreProof=true;
        assert(!partial.MarkCoreRestoredByCore(0,staged)); // exact data still differs
        staged.memory[0]=before[0];
        assert(partial.MarkCoreRestoredByCore(0,staged));
        assert(!partial.VerifyStacklessRestoredByCore(staged)); // core 1 not yet proved
        staged.memory[FleetRamIO::PerCore]=before[FleetRamIO::PerCore];
        assert(partial.MarkCoreRestoredByCore(1,staged) && partial.VerifyStacklessRestoredByCore(staged)
            && partial.StacklessRestoredByCore());
        staged.driftWhenProvingCore=1;
        assert(!partial.VerifyStacklessRestoredByCore(staged) && partial.RestorationRequired());
        staged.memory[0]=before[0];
        assert(partial.MarkCoreRestoredByCore(0,staged) && partial.VerifyStacklessRestoredByCore(staged));
        for (unsigned reads:staged.reads) assert(!reads);
        assert(staged.writes==writes);
        staged.uncapturedProtected=false;
        assert(!partial.VerifyStacklessRestoredByCore(staged) && partial.RestorationRequired());
        staged.uncapturedProtected=true;
        assert(partial.VerifyStacklessRestoredByCore(staged));
        staged.memory[0]^=1;
        assert(!partial.VerifyStacklessRestoredByCore(staged) && partial.RestorationRequired());
        staged.memory[0]=before[0];
        assert(partial.MarkCoreRestoredByCore(0,staged) && partial.VerifyStacklessRestoredByCore(staged));
        for (unsigned reads:staged.reads) assert(!reads);
        assert(staged.writes==writes);
    }
    {
        FleetRamIO staged; const auto before=staged.memory;
        std::vector<uint32_t> saved(DataflowRoleFleetLocalRamOwner::RequiredWords());
        DataflowRoleFleetLocalRamOwner partial;
        assert(partial.PrepareStackless(saved.data(),saved.size())); staged.held=false;
        assert(partial.CaptureStacklessCore(0,staged));
        staged.stacklessSafe=false;
        assert(!partial.MarkStacklessMayRun(0,staged) && partial.RestorationRequired());
        assert(!partial.CaptureStacklessCore(1,staged));
        staged.held=true;
        assert(partial.RestoreStackless(staged) && staged.memory==before);
    }
    {
        FleetRamIO staged; const auto before=staged.memory;
        std::vector<uint32_t> saved(DataflowRoleFleetLocalRamOwner::RequiredWords());
        DataflowRoleFleetLocalRamOwner partial;
        assert(partial.PrepareStackless(saved.data(),saved.size()));
        staged.held=false;
        assert(partial.CaptureStacklessCore(0,staged) && partial.MarkStacklessMayRun(0,staged));
        staged.memory[0]^=1; staged.failCopyCore=1;
        assert(!partial.CaptureStacklessCore(1,staged));
        assert(!partial.OriginalWords(1,DataflowRoleReader));
        assert(!partial.CaptureStacklessCore(2,staged) && !partial.MarkStacklessMayRun(0,staged));
        staged.held=true;
        assert(partial.RestoreStackless(staged) && staged.memory==before);
    }
    {
        FleetRamIO staged;
        std::vector<uint32_t> saved(DataflowRoleFleetLocalRamOwner::RequiredWords());
        DataflowRoleFleetLocalRamOwner all;
        assert(all.PrepareStackless(saved.data(),saved.size())); staged.held=false;
        for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
            assert(all.CaptureStacklessCore(core,staged));
            assert(all.MarkStacklessMayRun(core,staged));
            assert(all.OriginalWords(core,DataflowRoleWriter)
                ==saved.data()+core*FleetRamIO::PerCore+DataflowRoleLocalRamWords);
        }
        for (unsigned count:staged.reads) assert(!count); // no private-RAM NoC snapshots
        staged.held=true;
        assert(all.RestoreStackless(staged) && all.VerifyStacklessRestored(staged));
    }
    std::puts("PASS 120-core private RAM recovery, simultaneous corruption and unreadable routes; SIMULATED ONLY");
}
