/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowRoleFleetRecoveryOwner.h"
using namespace PioneerQwen35;
// Compile-only simulation: no physical PCI, reset or processor access.
struct FleetRecoveryProbeIO {
    bool AllRolesHeld(unsigned core) { return core<DataflowBarrierWorkers; }
    bool DefaultBootRouting(unsigned) { return true; }
    bool LocalRamStable(unsigned) { return true; }
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
    bool ReadRam(unsigned,unsigned,uint32_t,uint32_t* out,unsigned bytes) {
        for (unsigned i=0;i<bytes/4;i++) out[i]=0;
        return true;
    }
    bool WriteRam(unsigned,unsigned,uint32_t,const uint32_t*,unsigned) { return true; }
};
bool PioneerQwen35RoleFleetRecoveryOwnerTargetProbe(DataflowRoleFleetRecoveryOwner& fleet,
    uint32_t* codeStorage,uint32_t* ramStorage,const uint32_t* image,unsigned words)
{
    FleetRecoveryProbeIO io;
    return fleet.CaptureAll(codeStorage,DataflowRoleFleetRecoveryOwner::CodeWords(),
            ramStorage,DataflowRoleFleetRecoveryOwner::RamWords(),io)
        && fleet.StageImages(image,words,image,words,image,words,image,words,io)
        && fleet.PreparePossibleRelease(io) && fleet.BeginReleaseAttempt(io)
        && fleet.RestoreAll(io);
}
