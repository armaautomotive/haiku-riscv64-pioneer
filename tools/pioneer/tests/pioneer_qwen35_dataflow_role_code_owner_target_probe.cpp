/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowRoleCodeOwner.h"
using namespace PioneerQwen35;
// Compile-only IO: no physical address, reset or real hardware operation.
struct RoleCodeProbeIO {
    bool AllRolesHeld(unsigned core) { return core<DataflowBarrierWorkers; }
    bool Read(unsigned,unsigned,uint32_t* output,unsigned bytes) {
        for (unsigned i=0;i<bytes/4;i++) output[i]=0;
        return true;
    }
    bool Write(unsigned,unsigned,const uint32_t*,unsigned) { return true; }
};
bool PioneerQwen35RoleCodeOwnerTargetProbe(DataflowRoleCodeOwner& owner,
    unsigned core,uint32_t* snapshot,const uint32_t* code,unsigned words)
{
    RoleCodeProbeIO io;
    return owner.Capture(core,snapshot,DataflowRoleCodeOwner::RequiredWords(),io)
        && owner.Stage(true,code,words,io) && owner.Restore(io);
}
