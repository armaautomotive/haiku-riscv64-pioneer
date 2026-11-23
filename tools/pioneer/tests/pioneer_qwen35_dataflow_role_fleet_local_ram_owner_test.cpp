/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowRoleFleetLocalRamOwner.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace PioneerQwen35;
struct FleetRamIO {
    static constexpr unsigned PerCore=DataflowRoleLocalRamOwner::RequiredWords();
    std::vector<uint32_t> memory;
    bool held=true,stable=true,corruptEnabled=false;
    int failCore=-1,readFailCore=-1,unstableAfterReadCore=-1,corruptFrom=-1,corruptTarget=-1;
    unsigned writes=0;
    unsigned reads[DataflowBarrierWorkers]{};
    FleetRamIO():memory(DataflowRoleFleetLocalRamOwner::RequiredWords()) {
        for (unsigned i=0;i<memory.size();i++) memory[i]=i^0x7733cc44U;
    }
    bool AllRolesHeld(unsigned core) const { return held && core<DataflowBarrierWorkers; }
    bool LocalRamStable(unsigned core) const { return stable && core<DataflowBarrierWorkers; }
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
    std::puts("PASS 120-core private RAM recovery, simultaneous corruption and unreadable routes; SIMULATED ONLY");
}
