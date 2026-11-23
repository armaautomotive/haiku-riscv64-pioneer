/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowRoleBootOwner.h"
using namespace PioneerQwen35;
struct BootOwnerProbeIO {
    bool AllRolesHeld(unsigned core) { return core<DataflowBarrierWorkers; }
    bool DefaultBootRouting(unsigned) { return true; }
    bool ReadBoot(unsigned,unsigned,uint32_t& value) { value=0; return true; }
    bool WriteBoot(unsigned,unsigned,uint32_t) { return true; }
    bool ReadShim(unsigned,unsigned,uint32_t* out,unsigned bytes) {
        for (unsigned i=0;i<bytes/4;i++) static_cast<volatile uint32_t*>(out)[i]=0;
        return true;
    }
    bool WriteShim(unsigned,unsigned,const uint32_t*,unsigned) { return true; }
};
bool PioneerQwen35RoleBootOwnerTargetProbe(DataflowRoleBootOwner& owner,
    unsigned core,const uint32_t* image,unsigned words)
{
    BootOwnerProbeIO io;
    return owner.Capture(core,io) && owner.Stage(image,words,image,words,io)
        && owner.VerifyStaged(io) && owner.Restore(io);
}
