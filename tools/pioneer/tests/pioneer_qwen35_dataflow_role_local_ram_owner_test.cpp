/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowRoleLocalRamOwner.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace PioneerQwen35;
struct RamIO {
    std::vector<uint32_t> ram;
    bool held=true,stable=true,corruptEnabled=false;
    int failWrite=-1,corruptAt=-1;
    unsigned writes=0;
    RamIO():ram(DataflowRoleLocalRamOwner::RequiredWords()) {
        for (unsigned i=0;i<ram.size();i++) ram[i]=i^0xabc13579U;
    }
    bool AllRolesHeld(unsigned core) const { return held && core==9; }
    bool LocalRamStable(unsigned core) const { return core==9 && stable; }
    bool ReadRam(unsigned core,unsigned role,uint32_t address,uint32_t* output,unsigned bytes) {
        assert(AllRolesHeld(core) && LocalRamStable(core) && output && DataflowRoleLocalRamRange(role,address,bytes));
        const uint32_t base=role==DataflowRoleReader?DataflowRoleBriscLocalRam:DataflowRoleNcriscLocalRam;
        const unsigned offset=role*DataflowRoleLocalRamWords+(address-base)/4;
        for (unsigned i=0;i<bytes/4;i++) output[i]=ram[offset+i];
        return true;
    }
    bool WriteRam(unsigned core,unsigned role,uint32_t address,const uint32_t* input,unsigned bytes) {
        assert(AllRolesHeld(core) && LocalRamStable(core) && input && DataflowRoleLocalRamRange(role,address,bytes));
        const uint32_t base=role==DataflowRoleReader?DataflowRoleBriscLocalRam:DataflowRoleNcriscLocalRam;
        const unsigned offset=role*DataflowRoleLocalRamWords+(address-base)/4;
        ++writes;
        if (int(writes)==failWrite) { ram[offset]=input[0]; return false; }
        for (unsigned i=0;i<bytes/4;i++) ram[offset+i]=input[i];
        if (corruptEnabled && int(writes)==corruptAt) { ram[0]^=1; corruptEnabled=false; }
        return true;
    }
};
int main()
{
    assert(DataflowRoleLocalRamRange(DataflowRoleReader,DataflowRoleBriscLocalRam,DataflowRoleLocalRamBytes));
    assert(DataflowRoleLocalRamRange(DataflowRoleWriter,DataflowRoleNcriscLocalRam,DataflowRoleLocalRamBytes));
    assert(!DataflowRoleLocalRamRange(DataflowRoleReader,DataflowRoleNcriscLocalRam,4));
    assert(!DataflowRoleLocalRamRange(DataflowRoleWriter,DataflowRoleNcriscLocalRam+DataflowRoleLocalRamBytes-4,8));
    assert(!DataflowRoleLocalRamRange(DataflowRoleWriter,DataflowRoleNcriscLocalRam,0xffffffffU));
    std::vector<uint32_t> snapshot(DataflowRoleLocalRamOwner::RequiredWords());
    {
        RamIO io; const auto original=io.ram; DataflowRoleLocalRamOwner owner;
        io.stable=false; assert(!owner.Capture(9,snapshot.data(),snapshot.size(),io));
        io.stable=true; assert(owner.Capture(9,snapshot.data(),snapshot.size(),io));
        assert(owner.MarkMayRun(io));
        io.ram.assign(io.ram.size(),0); // simulated reset clearing both RAMs
        io.stable=false; assert(!owner.Restore(io) && !io.writes && owner.RestorationRequired());
        io.stable=true; io.failWrite=2;
        assert(!owner.Restore(io) && owner.RestorationRequired());
        io.failWrite=-1; io.corruptEnabled=true; io.corruptAt=int(io.writes)+2;
        assert(!owner.Restore(io) && owner.RestorationRequired());
        assert(owner.Restore(io) && owner.RestoredOriginal() && io.ram==original);
        io.ram[0]^=1;
        assert(!owner.VerifyOriginal(io) && owner.RestorationRequired());
        assert(owner.Restore(io) && io.ram==original);
    }
    {
        RamIO io; const auto original=io.ram; DataflowRoleLocalRamOwner owner;
        assert(owner.Capture(9,snapshot.data(),snapshot.size(),io));
        io.held=false; assert(!owner.MarkMayRun(io) && owner.RestorationRequired());
        io.held=true; assert(!owner.MarkMayRun(io)); // retain, do not permit release
        assert(owner.Restore(io) && io.ram==original);
    }
    std::puts("PASS private role RAM snapshots, reset-clear retention and verified retry; SIMULATED ONLY");
}
