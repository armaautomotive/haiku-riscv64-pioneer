/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowRoleFleetRecoveryOwner.h"
using namespace PioneerQwen35;
// Compile-only simulation: no physical PCI, reset or processor access.
struct FleetRecoveryProbeIO {
    bool AllRolesHeld(unsigned core) { return core<DataflowBarrierWorkers; }
    bool DefaultBootRouting(unsigned) { return true; }
    bool LocalRamStable(unsigned) { return true; }
    bool StacklessCopiesSafe(unsigned core) { return core<DataflowBarrierWorkers; }
    bool StacklessUncapturedRamProtected(unsigned core) { return core<DataflowBarrierWorkers; }
    bool ReadStacklessRamCopy(unsigned,unsigned,unsigned,uint32_t* out,unsigned bytes) {
        for (unsigned i=0;i<bytes/4;i++) static_cast<volatile uint32_t*>(out)[i]=0;
        return true;
    }
    bool ReadResetDisable(unsigned,uint32_t& value) { value=0; return true; }
    bool WriteResetDisable(unsigned,uint32_t) { return true; }
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
        && fleet.RestoreAll(io) && fleet.VerifyAfterResidentRestore(io);
}
bool PioneerQwen35StacklessFleetTargetProbe(DataflowRoleFleetLocalRamOwner& fleet,uint32_t* storage)
{
    FleetRecoveryProbeIO io;
    return fleet.PrepareStackless(storage,DataflowRoleFleetLocalRamOwner::RequiredWords())
        && fleet.CaptureStacklessCore(0,io) && fleet.MarkStacklessMayRun(0,io)
        && fleet.RestoreStackless(io) && fleet.VerifyStacklessRestored(io);
}
