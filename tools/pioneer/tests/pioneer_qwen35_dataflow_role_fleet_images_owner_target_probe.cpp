/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowRoleFleetImagesOwner.h"
using namespace PioneerQwen35;
// Compile-only simulation: no physical memory, reset or processor access.
struct FleetImagesProbeIO {
    bool AllRolesHeld(unsigned core) { return core<DataflowBarrierWorkers; }
    bool DefaultBootRouting(unsigned) { return true; }
    bool Read(unsigned,unsigned,uint32_t* out,unsigned bytes) {
        for (unsigned i=0;i<bytes/4;i++) out[i]=0;
        return true;
    }
    bool Write(unsigned,unsigned,const uint32_t*,unsigned) { return true; }
    bool ReadBoot(unsigned,unsigned,uint32_t& value) { value=0; return true; }
    bool WriteBoot(unsigned,unsigned,uint32_t) { return true; }
    bool ReadShim(unsigned,unsigned,uint32_t* out,unsigned bytes) {
        for (unsigned i=0;i<bytes/4;i++) out[i]=0;
        return true;
    }
    bool WriteShim(unsigned,unsigned,const uint32_t*,unsigned) { return true; }
};
bool PioneerQwen35RoleFleetImagesOwnerTargetProbe(DataflowRoleFleetImagesOwner& fleet,
    uint32_t* storage,const uint32_t* image,unsigned words)
{
    FleetImagesProbeIO io;
    return fleet.CaptureAll(storage,DataflowRoleFleetImagesOwner::RequiredWords(),io)
        && fleet.StageAll(image,words,image,words,image,words,image,words,io)
        && fleet.RestoreAll(io);
}
