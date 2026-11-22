/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowRoleFleetBootOwner.h"
using namespace PioneerQwen35;
// Compile-only simulation: no physical boot, reset or PCI access.
struct FleetBootProbeIO {
    bool AllRolesHeld(unsigned core) { return core<DataflowBarrierWorkers; }
    bool DefaultBootRouting(unsigned) { return true; }
    bool ReadBoot(unsigned,unsigned,uint32_t& value) { value=0; return true; }
    bool WriteBoot(unsigned,unsigned,uint32_t) { return true; }
    bool ReadShim(unsigned,unsigned,uint32_t* output,unsigned bytes) {
        for (unsigned i=0;i<bytes/4;i++) output[i]=0;
        return true;
    }
    bool WriteShim(unsigned,unsigned,const uint32_t*,unsigned) { return true; }
};
bool PioneerQwen35RoleFleetBootOwnerTargetProbe(DataflowRoleFleetBootOwner& fleet,
    const uint32_t* shim,unsigned words)
{
    FleetBootProbeIO io;
    return fleet.CaptureAll(io) && fleet.StageAll(shim,words,shim,words,io)
        && fleet.RestoreAll(io);
}
