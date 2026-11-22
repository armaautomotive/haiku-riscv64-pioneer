/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowRoleFleetCodeOwner.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace PioneerQwen35;
struct FleetIO {
    static constexpr unsigned PerCore=DataflowRoleCodeOwner::RequiredWords();
    std::vector<uint32_t> memory;
    bool held=true,corruptEnabled=false;
    int failCore=-1,corruptFrom=-1,corruptTarget=-1;
    unsigned writes=0;
    mutable bool failHeldOnStagedCore=false;
    FleetIO():memory(DataflowRoleFleetCodeOwner::RequiredWords()) {
        for (unsigned i=0;i<memory.size();i++) memory[i]=i^0x5a33c5a3U;
    }
    bool AllRolesHeld(unsigned core) const {
        if (failHeldOnStagedCore && core==3 && writes) {
            failHeldOnStagedCore=false; return false;
        }
        return held && core<DataflowBarrierWorkers;
    }
    bool Read(unsigned core,unsigned address,uint32_t* out,unsigned bytes) {
        assert(AllRolesHeld(core) && out && DataflowRoleExtraL1Range(address,bytes));
        const unsigned offset=core*PerCore+(address-DataflowRoleExtraL1Address)/4;
        for (unsigned i=0;i<bytes/4;i++) out[i]=memory[offset+i];
        return true;
    }
    bool Write(unsigned core,unsigned address,const uint32_t* in,unsigned bytes) {
        assert(AllRolesHeld(core) && in && DataflowRoleExtraL1Range(address,bytes));
        const unsigned offset=core*PerCore+(address-DataflowRoleExtraL1Address)/4;
        ++writes;
        if (int(core)==failCore) {
            memory[offset]=in[0]; failCore=-1; return false;
        }
        for (unsigned i=0;i<bytes/4;i++) memory[offset+i]=in[i];
        if (corruptEnabled && int(core)==corruptFrom) {
            memory[unsigned(corruptTarget)*PerCore]^=1; corruptEnabled=false;
        }
        return true;
    }
};
int main()
{
    std::vector<uint32_t> snapshots(DataflowRoleFleetCodeOwner::RequiredWords());
    std::vector<uint32_t> reader(100,0x12345678U),writer(70,0xabcdef01U);
    {
        FleetIO io; const auto original=io.memory; DataflowRoleFleetCodeOwner fleet;
        assert(fleet.CaptureAll(snapshots.data(),snapshots.size(),io));
        io.failHeldOnStagedCore=true;
        assert(!fleet.StageAll(reader.data(),reader.size(),writer.data(),writer.size(),io));
        assert(fleet.RestorationRequired() && !io.failHeldOnStagedCore && io.writes>0);
        assert(fleet.RestoreAll(io) && fleet.RegionRestored() && io.memory==original);
    }
    {
        FleetIO io; const auto original=io.memory; DataflowRoleFleetCodeOwner fleet;
        assert(fleet.CaptureAll(snapshots.data(),snapshots.size(),io));
        io.held=false;
        assert(!fleet.StageAll(reader.data(),reader.size(),writer.data(),writer.size(),io));
        assert(fleet.RestorationRequired() && !fleet.RestoreAll(io) && io.writes==0);
        io.held=true; io.memory[119*FleetIO::PerCore]^=1;
        assert(!fleet.RestoreAll(io) && fleet.RestorationRequired());
        assert(fleet.RestoreAll(io) && fleet.RegionRestored() && io.memory==original);
    }
    {
        FleetIO io; const auto original=io.memory; DataflowRoleFleetCodeOwner fleet;
        assert(!fleet.CaptureAll(snapshots.data(),snapshots.size()-1,io));
        assert(fleet.CaptureAll(snapshots.data(),snapshots.size(),io));
        assert(fleet.StageAll(reader.data(),reader.size(),writer.data(),writer.size(),io));
        assert(fleet.StagedAndVerified() && fleet.RestorationRequired());
        io.held=false; const unsigned writes=io.writes;
        assert(!fleet.RestoreAll(io) && io.writes==writes && fleet.RestorationRequired());
        io.held=true;
        // A later restore transfer corrupts an earlier restored core. The
        // final fleet readback detects it and preserves a retryable snapshot.
        io.corruptEnabled=true; io.corruptFrom=3; io.corruptTarget=0;
        assert(!fleet.RestoreAll(io) && fleet.RestorationRequired());
        assert(fleet.RestoreAll(io) && fleet.RegionRestored() && io.memory==original);
    }
    {
        FleetIO io; const auto original=io.memory; DataflowRoleFleetCodeOwner fleet;
        assert(fleet.CaptureAll(snapshots.data(),snapshots.size(),io));
        io.failCore=3;
        assert(!fleet.StageAll(reader.data(),reader.size(),writer.data(),writer.size(),io));
        assert(fleet.RestorationRequired() && !fleet.StagedAndVerified());
        // Simulate a partial write also hitting an otherwise untouched core.
        io.memory[119*FleetIO::PerCore]^=1;
        assert(!fleet.RestoreAll(io) && fleet.RestorationRequired());
        assert(fleet.RestoreAll(io) && fleet.RegionRestored() && io.memory==original);
    }
    {
        FleetIO io; const auto original=io.memory; DataflowRoleFleetCodeOwner fleet;
        assert(fleet.CaptureAll(snapshots.data(),snapshots.size(),io));
        io.corruptEnabled=true; io.corruptFrom=3; io.corruptTarget=0;
        assert(!fleet.StageAll(reader.data(),reader.size(),writer.data(),writer.size(),io));
        assert(fleet.RestorationRequired());
        assert(fleet.RestoreAll(io) && fleet.RegionRestored() && io.memory==original);
    }
    std::puts("PASS 120-core code snapshots, misroute detection and retryable restoration; SIMULATED ONLY");
}
