/* SPDX-License-Identifier: MIT */
// Type/stack witness for the actual service owner with the Haiku kernel SDK.
// IO is a deliberately inert stand-in, never a PCI or card binding.
#include "../PioneerQwen35DataflowServiceOwner.h"
using namespace PioneerQwen35;
struct SharedServiceKernelProbeIO {
    volatile bool okay=false;
    bool SessionIdle() { return okay; }
    bool ProveFleetRoutes(uint32_t&,uint32_t&) { return okay; }
    bool CoreIdle(unsigned) { return okay; }
    bool CaptureCoreState(unsigned) { return okay; }
    void ReleaseCoreSnapshot(unsigned) {}
    bool HoldCore(unsigned) { return okay; }
    bool CoreHeld(unsigned) { return okay; }
    bool WriteCoreMemory(unsigned,unsigned,const void*,unsigned) { return okay; }
    bool ReadCoreMemory(unsigned,unsigned,void*,unsigned) { return okay; }
    bool SharedCaptureAndStageHeld() { return okay; }
    bool SharedArmAndBeginRelease() { return okay; }
    bool SharedFleetHeldAndQuiescent() { return okay; }
    bool SharedStartCore(unsigned) { return okay; }
    void Pause() {}
    bool WaitWorkerReady(unsigned,DataflowWorkerSnapshot&) { return okay; }
    bool SharedCoreReady(unsigned,const DataflowWorkerSnapshot&) { return okay; }
    bool ReadWorkerSnapshot(unsigned,DataflowWorkerSnapshot&) { return okay; }
    bool SharedStopAndHoldAll(unsigned&,unsigned&) { return okay; }
    bool StopAndAuditAll(unsigned&,unsigned&) { return okay; }
    bool SharedRestoreRolesHeld() { return okay; }
    bool RestoreCoreState(unsigned) { return okay; }
    bool VerifyCoreStateRestored(unsigned) { return okay; }
    bool SharedVerifyRolesAfterResidentRestore() { return okay; }
    bool SharedFinalizeBeforeResume() { return okay; }
    bool ResumeRestoredFleet(unsigned&,unsigned&,unsigned&) { return okay; }
};
DataflowServiceResult PioneerSharedServiceKernelProbe(DataflowServiceOwner& owner,
    const DataflowServiceImageView& image,SharedServiceKernelProbeIO& io,bool close)
{
    return close?owner.Close(io):owner.Install(image,io);
}
