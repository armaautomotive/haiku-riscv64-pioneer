/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_ROLE_FLEET_BOOT_OWNER_H
#define PIONEER_QWEN35_DATAFLOW_ROLE_FLEET_BOOT_OWNER_H
#include "PioneerQwen35DataflowRoleBootOwner.h"
namespace PioneerQwen35 {
// Offline fleet transaction for boot words and low shims only. The caller
// owns the additional code region, local RAM, reset, NoC, queues, cache, and
// physical stop proof separately. IO must serialize exclusive reset/mapping
// access and verify all roles held, relevant NoCs quiet, and default boot PCs.
class DataflowRoleFleetBootOwner {
    enum Phase { Empty, Captured, Faulted, Staged, Restored, Unavailable };
    Phase fPhase=Empty;
    unsigned fStagedCore=DataflowBarrierWorkers;
    DataflowRoleBootOwner fCore[DataflowBarrierWorkers];
    template<class IO> static bool AllHeld(IO& io) {
        for (unsigned core=0;core<DataflowBarrierWorkers;core++)
            if (!io.AllRolesHeld(core) || !io.DefaultBootRouting(core)) return false;
        return true;
    }
public:
    DataflowRoleFleetBootOwner()=default;
    DataflowRoleFleetBootOwner(const DataflowRoleFleetBootOwner&)=delete;
    DataflowRoleFleetBootOwner& operator=(const DataflowRoleFleetBootOwner&)=delete;
    bool StagedAndVerified() const { return fPhase==Staged; }
    bool CanRestageCore(unsigned core) const { return fPhase==Staged && core<DataflowBarrierWorkers
        && (fStagedCore==DataflowBarrierWorkers || core==fStagedCore); }
    bool RestorationRequired() const { return fPhase==Faulted || fPhase==Staged; }
    bool RestoredOriginal() const { return fPhase==Restored; }
    template<class IO> bool CaptureAll(IO& io) {
        if (fPhase!=Empty) return false;
        if (!AllHeld(io)) { fPhase=Unavailable; return false; }
        for (unsigned core=0;core<DataflowBarrierWorkers;core++)
            if (!fCore[core].Capture(core,io)) { fPhase=Unavailable; return false; }
        if (!AllHeld(io)) { fPhase=Unavailable; return false; }
        fPhase=Captured;
        return true;
    }
    template<class IO> bool StageAll(const uint32_t* reader,unsigned readerWords,
        const uint32_t* writer,unsigned writerWords,IO& io) {
        if (fPhase!=Captured || !reader || !readerWords || !writer || !writerWords) return false;
        if (!AllHeld(io)) { fPhase=Faulted; return false; }
        fPhase=Faulted;
        for (unsigned core=0;core<DataflowBarrierWorkers;core++)
            if (!fCore[core].Stage(reader,readerWords,writer,writerWords,io)) return false;
        for (unsigned core=0;core<DataflowBarrierWorkers;core++)
            if (!fCore[core].VerifyStaged(io)) return false;
        if (!AllHeld(io)) return false;
        fPhase=Staged;
        return true;
    }
    template<class IO> bool VerifyStagedAll(IO& io) {
        if (fPhase!=Staged) return false;
        if (!AllHeld(io)) { fPhase=Faulted; return false; }
        for (unsigned core=0;core<DataflowBarrierWorkers;core++)
            if (!(fStagedCore==DataflowBarrierWorkers || core==fStagedCore
                    ?fCore[core].VerifyStaged(io):fCore[core].VerifyOriginal(io)))
                { fPhase=Faulted; return false; }
        if (!AllHeld(io)) { fPhase=Faulted; return false; }
        return true;
    }
    template<class IO> bool StageOne(unsigned core,const uint32_t* reader,unsigned readerWords,
        const uint32_t* writer,unsigned writerWords,IO& io) {
        if (fPhase!=Captured || core>=DataflowBarrierWorkers || !reader || !readerWords
            || !writer || !writerWords) return false;
        fPhase=Faulted;fStagedCore=core;
        if (!AllHeld(io) || !fCore[core].Stage(reader,readerWords,writer,writerWords,io)) return false;
        for (unsigned i=0;i<DataflowBarrierWorkers;i++)
            if (!(i==core?fCore[i].VerifyStaged(io):fCore[i].VerifyOriginal(io))) return false;
        if (!AllHeld(io)) return false;
        fPhase=Staged;return true;
    }
    // Replace one held core's staged shims while retaining and auditing the
    // entire fleet. A cross-core misroute remains a fault requiring restore.
    template<class IO> bool RestageOne(unsigned core,const uint32_t* reader,unsigned readerWords,
        const uint32_t* writer,unsigned writerWords,IO& io) {
        if (!CanRestageCore(core)
            || !reader || !readerWords || !writer || !writerWords
            || readerWords>(DataflowRoleReaderShimLimit-DataflowRoleReaderShimAddress)/4
            || writerWords>(DataflowRoleWriterShimLimit-DataflowRoleWriterShimAddress)/4) return false;
        if (!VerifyStagedAll(io)) return false;
        fPhase=Faulted;
        if (!fCore[core].Restage(reader,readerWords,writer,writerWords,io)) return false;
        for (unsigned i=0;i<DataflowBarrierWorkers;i++)
            if (!(fStagedCore==DataflowBarrierWorkers || i==fStagedCore
                    ?fCore[i].VerifyStaged(io):fCore[i].VerifyOriginal(io))) return false;
        if (!AllHeld(io)) return false;
        fPhase=Staged;return true;
    }
    template<class IO> bool VerifyOriginalAll(IO& io) {
        if (fPhase!=Captured && fPhase!=Restored) return false;
        if (!AllHeld(io)) { fPhase=Faulted; return false; }
        for (unsigned core=0;core<DataflowBarrierWorkers;core++)
            if (!fCore[core].VerifyOriginal(io)) { fPhase=Faulted; return false; }
        if (!AllHeld(io)) { fPhase=Faulted; return false; }
        return true;
    }
    template<class IO> bool RestoreAll(IO& io) {
        if (fPhase!=Faulted && fPhase!=Staged) return false;
        fPhase=Faulted;
        if (!AllHeld(io)) return false;
        for (unsigned core=0;core<DataflowBarrierWorkers;core++)
            if (fCore[core].RestorationRequired() && !fCore[core].Restore(io)) return false;
        // Check untouched captures too, in case a transfer hit another core.
        for (unsigned core=0;core<DataflowBarrierWorkers;core++)
            if (!fCore[core].VerifyOriginal(io)) return false;
        if (!AllHeld(io)) return false;
        fPhase=Restored;
        return true;
    }
};
}
#endif
