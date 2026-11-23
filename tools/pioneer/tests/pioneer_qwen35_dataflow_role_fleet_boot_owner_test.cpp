/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowRoleFleetBootOwner.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace PioneerQwen35;
struct FleetBootIO {
    std::vector<uint32_t> boots,shims;
    bool held=true,defaultRoute=true,corruptEnabled=false;
    int failCore=-1,corruptFrom=-1,corruptTarget=-1;
    unsigned writes=0;
    mutable bool failHeldOnStagedCore=false;
    FleetBootIO():boots(DataflowBarrierWorkers*2),shims(DataflowBarrierWorkers*DataflowRoleShimWords) {
        for (unsigned i=0;i<boots.size();i++) boots[i]=i^0x23456789U;
        for (unsigned i=0;i<shims.size();i++) shims[i]=i^0x8a9bcdefU;
    }
    bool AllRolesHeld(unsigned core) const {
        if (failHeldOnStagedCore && core==3 && writes) {
            failHeldOnStagedCore=false; return false;
        }
        return held && core<DataflowBarrierWorkers;
    }
    bool DefaultBootRouting(unsigned core) const { return defaultRoute && core<DataflowBarrierWorkers; }
    bool ReadBoot(unsigned core,unsigned address,uint32_t& value) {
        assert(AllRolesHeld(core) && DefaultBootRouting(core));
        const unsigned role=address==DataflowRoleBriscBootAddress?0:address==DataflowRoleNcriscBootAddress?1:2;
        assert(role<2); value=boots[core*2+role]; return true;
    }
    bool WriteBoot(unsigned core,unsigned address,uint32_t value) {
        assert(AllRolesHeld(core) && DefaultBootRouting(core));
        const unsigned role=address==DataflowRoleBriscBootAddress?0:address==DataflowRoleNcriscBootAddress?1:2;
        assert(role<2); ++writes; boots[core*2+role]=value;
        if (int(core)==failCore) { failCore=-1; return false; }
        MaybeCorrupt(core); return true;
    }
    bool ReadShim(unsigned core,unsigned address,uint32_t* output,unsigned bytes) {
        assert(AllRolesHeld(core) && DefaultBootRouting(core) && output && !(bytes&3));
        if (address<DataflowRoleReaderShimAddress || address>DataflowRoleWriterShimLimit
            || bytes>DataflowRoleWriterShimLimit-address) return false;
        const unsigned offset=core*DataflowRoleShimWords+(address-DataflowRoleReaderShimAddress)/4;
        for (unsigned i=0;i<bytes/4;i++) output[i]=shims[offset+i];
        return true;
    }
    bool WriteShim(unsigned core,unsigned address,const uint32_t* input,unsigned bytes) {
        assert(AllRolesHeld(core) && DefaultBootRouting(core) && input && !(bytes&3));
        if (address<DataflowRoleReaderShimAddress || address>DataflowRoleWriterShimLimit
            || bytes>DataflowRoleWriterShimLimit-address) return false;
        const unsigned offset=core*DataflowRoleShimWords+(address-DataflowRoleReaderShimAddress)/4;
        ++writes;
        if (int(core)==failCore) { shims[offset]=input[0]; failCore=-1; return false; }
        for (unsigned i=0;i<bytes/4;i++) shims[offset+i]=input[i];
        MaybeCorrupt(core); return true;
    }
    void MaybeCorrupt(unsigned core) {
        if (corruptEnabled && int(core)==corruptFrom) {
            shims[unsigned(corruptTarget)*DataflowRoleShimWords]^=1; corruptEnabled=false;
        }
    }
};
int main()
{
    const uint32_t reader[]={1,2,3,4},writer[]={5,6,7};
    {
        FleetBootIO io; const auto boots=io.boots,shims=io.shims;
        DataflowRoleFleetBootOwner fleet;
        assert(fleet.CaptureAll(io)); io.failHeldOnStagedCore=true;
        assert(!fleet.StageAll(reader,4,writer,3,io) && fleet.RestorationRequired());
        assert(!io.failHeldOnStagedCore && io.writes>0);
        assert(fleet.RestoreAll(io) && fleet.RestoredOriginal());
        assert(io.boots==boots && io.shims==shims);
    }
    {
        FleetBootIO io; const auto boots=io.boots,shims=io.shims;
        DataflowRoleFleetBootOwner fleet;
        assert(fleet.CaptureAll(io)); io.defaultRoute=false;
        assert(!fleet.StageAll(reader,4,writer,3,io));
        assert(fleet.RestorationRequired() && !fleet.RestoreAll(io) && io.writes==0);
        io.defaultRoute=true; io.shims[119*DataflowRoleShimWords]^=1;
        assert(!fleet.RestoreAll(io) && fleet.RestorationRequired());
        assert(fleet.RestoreAll(io) && fleet.RestoredOriginal());
        assert(io.boots==boots && io.shims==shims);
    }
    {
        FleetBootIO io; const auto boots=io.boots,shims=io.shims;
        DataflowRoleFleetBootOwner fleet;
        assert(fleet.CaptureAll(io)); assert(fleet.StageAll(reader,4,writer,3,io));
        assert(fleet.StagedAndVerified()); io.defaultRoute=false;
        const unsigned writes=io.writes;
        assert(!fleet.RestoreAll(io) && fleet.RestorationRequired() && io.writes==writes);
        io.defaultRoute=true; io.corruptEnabled=true; io.corruptFrom=3; io.corruptTarget=0;
        assert(!fleet.RestoreAll(io) && fleet.RestorationRequired());
        assert(fleet.RestoreAll(io) && fleet.RestoredOriginal());
        assert(io.boots==boots && io.shims==shims);
    }
    {
        FleetBootIO io; const auto boots=io.boots,shims=io.shims;
        DataflowRoleFleetBootOwner fleet;
        assert(fleet.CaptureAll(io)); io.failCore=3;
        assert(!fleet.StageAll(reader,4,writer,3,io) && fleet.RestorationRequired());
        io.shims[119*DataflowRoleShimWords]^=1;
        assert(!fleet.RestoreAll(io) && fleet.RestorationRequired());
        assert(fleet.RestoreAll(io) && fleet.RestoredOriginal());
        assert(io.boots==boots && io.shims==shims);
    }
    {
        FleetBootIO io; const auto boots=io.boots,shims=io.shims;
        DataflowRoleFleetBootOwner fleet;
        assert(fleet.CaptureAll(io)); io.corruptEnabled=true; io.corruptFrom=3; io.corruptTarget=0;
        assert(!fleet.StageAll(reader,4,writer,3,io) && fleet.RestorationRequired());
        assert(fleet.RestoreAll(io) && fleet.RestoredOriginal());
        assert(io.boots==boots && io.shims==shims);
    }
    std::puts("PASS 120-core boot/shim snapshots, misroute detection and restoration; SIMULATED ONLY");
}
