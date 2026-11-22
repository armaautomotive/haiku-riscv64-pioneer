/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowRoleFleetCodeOwner.h"
using namespace PioneerQwen35;
// Compile-only simulation: this IO does not map or touch any PCI resource.
struct FleetCodeProbeIO {
    bool AllRolesHeld(unsigned core) { return core<DataflowBarrierWorkers; }
    bool Read(unsigned,unsigned,uint32_t* output,unsigned bytes) {
        for (unsigned i=0;i<bytes/4;i++) output[i]=0;
        return true;
    }
    bool Write(unsigned,unsigned,const uint32_t*,unsigned) { return true; }
};
bool PioneerQwen35RoleFleetCodeOwnerTargetProbe(DataflowRoleFleetCodeOwner& fleet,
    uint32_t* storage,const uint32_t* code,unsigned words)
{
    FleetCodeProbeIO io;
    return fleet.CaptureAll(storage,DataflowRoleFleetCodeOwner::RequiredWords(),io)
        && fleet.StageAll(code,words,code,words,io) && fleet.RestoreAll(io);
}
