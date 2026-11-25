/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_FABRIC_CORE_RECOVERY_SEQUENCE_H
#define PIONEER_QWEN35_FABRIC_CORE_RECOVERY_SEQUENCE_H
#ifndef PIONEER_QWEN35_FABRIC_GUARD_WITNESS
#error Fabric core recovery sequence requires the non-loadable fabric witness
#endif
#include "PioneerQwen35DataflowRoleFleetRecoveryOwner.h"
namespace PioneerQwen35 {
// Single-use orchestration of the retained role owner's core-local recovery.
// IO must bind the actual fresh session, recovery ticket, held routes, saved
// state and continuous writer exclusion. This sequence does not supply those
// physical proofs, restore the parent graph or retire the session.
class DataflowFabricCoreRecoverySequence {
public:
    enum Phase { Empty,Running,Retained,Restored };
    enum Step { None,Stage,Prepare,Release,Complete,Hold,Mark,Handoff,Fleet };
private:
    Phase fPhase=Empty;
    Step fStep=None;
    unsigned fCore=DataflowBarrierWorkers;
    bool Fail(Step step,unsigned core) { fStep=step;fCore=core;fPhase=Retained;return false; }
public:
    DataflowFabricCoreRecoverySequence()=default;
    DataflowFabricCoreRecoverySequence(const DataflowFabricCoreRecoverySequence&)=delete;
    DataflowFabricCoreRecoverySequence& operator=(const DataflowFabricCoreRecoverySequence&)=delete;
    Phase CurrentPhase() const { return fPhase; }
    Step FailureStep() const { return fStep; }
    unsigned FailureCore() const { return fCore; }
    template<class IO>
    bool Run(DataflowRoleFleetRecoveryOwner& owner,IO& io,const uint32_t* reader,unsigned readerWords,
        const uint32_t* writer,unsigned writerWords,bool* releaseAttempted,bool* holdAttempted,unsigned polls) {
        if (fPhase!=Empty || !owner.FabricCoreRecoveryActive() || !reader || !readerWords
            || !writer || !writerWords || !releaseAttempted || !holdAttempted
            || !polls || polls>65536 || !io.FabricRecoveryConversionAllowed()) return false;
        fPhase=Running;
        for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
            if (releaseAttempted[core] || holdAttempted[core]) return Fail(Stage,core);
            if (!owner.StacklessCopiesCaptured(core) || owner.FabricCoreAlreadyRestored(core)) continue;
            if (!io.FabricRecoveryConversionAllowed()
                || !owner.StageCoreLocalRestore(core,reader,readerWords,writer,writerWords,io)
                || !io.FabricRecoveryConversionAllowed()) return Fail(Stage,core);
            if (!owner.PrepareCoreLocalRestoreHandoff(core,io)
                || !io.FabricRecoveryConversionAllowed()) return Fail(Prepare,core);
            if (!owner.BeginCoreLocalRestoreRelease(core,io)
                || !io.FabricRecoveryConversionAllowed()
                || !io.ReleaseCoreLocalRestore(core,releaseAttempted[core])
                || !io.FabricRecoveryConversionAllowed()) return Fail(Release,core);
            bool completed=false;
            for (unsigned n=0;n<polls;n++) {
                if (!io.FabricRecoveryConversionAllowed()) return Fail(Complete,core);
                if (owner.ConfirmCoreLocalRestoreHandoff(core,io)) { completed=true;break; }
                io.Pause();
            }
            if (!completed || !io.FabricRecoveryConversionAllowed()) return Fail(Complete,core);
            if (!io.HoldCoreLocalRestore(core,holdAttempted[core])
                || !io.FabricRecoveryConversionAllowed()) return Fail(Hold,core);
            if (!io.RestoreCoreHoldObserved(core) || !owner.MarkCoreRestoredByCore(core,io)
                || !io.FabricRecoveryConversionAllowed()) return Fail(Mark,core);
            if (!owner.RestoreCoreLocalHandoff(core,io)
                || !io.FabricRecoveryConversionAllowed()) return Fail(Handoff,core);
        }
        if (!io.FabricRecoveryConversionAllowed() || !owner.RestoreAfterCoreLocalFleetProof(io)
            || !io.FabricRecoveryConversionAllowed()) return Fail(Fleet,DataflowBarrierWorkers);
        fStep=None;fCore=DataflowBarrierWorkers;fPhase=Restored;return true;
    }
};
}
#endif
