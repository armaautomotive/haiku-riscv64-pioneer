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
    int failCore=-1,corruptFrom=-1,corruptTarget=-1;
    unsigned writes=0;
    FleetRamIO():memory(DataflowRoleFleetLocalRamOwner::RequiredWords()) {
        for (unsigned i=0;i<memory.size();i++) memory[i]=i^0x7733cc44U;
    }
    bool AllRolesHeld(unsigned core) const { return held && core<DataflowBarrierWorkers; }
    bool LocalRamStable(unsigned core) const { return stable && core<DataflowBarrierWorkers; }
    bool ReadRam(unsigned core,unsigned role,uint32_t address,uint32_t* output,unsigned bytes) {
        assert(AllRolesHeld(core) && LocalRamStable(core) && output && DataflowRoleLocalRamRange(role,address,bytes));
        const uint32_t base=role==DataflowRoleReader?DataflowRoleBriscLocalRam:DataflowRoleNcriscLocalRam;
        const unsigned offset=core*PerCore+role*DataflowRoleLocalRamWords+(address-base)/4;
        for (unsigned i=0;i<bytes/4;i++) output[i]=memory[offset+i];
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
    std::puts("PASS 120-core private RAM snapshot, reset-clear and misroute recovery; SIMULATED ONLY");
}
