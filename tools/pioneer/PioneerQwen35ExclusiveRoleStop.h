/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_EXCLUSIVE_ROLE_STOP_H
#define PIONEER_QWEN35_EXCLUSIVE_ROLE_STOP_H
#include "PioneerQwen35DataflowRoleBoot.h"

namespace PioneerQwen35 {
// Stop a dedicated fleet whose old private RAM is deliberately discarded.
// IO owns the physical route, release-write drainage, role/compute settlement
// and exclusive device scope. An uncertain command is observed on retry,
// never reissued. This does not itself reset/reinitialize the ASIC or allow
// the enclosing owner to discard its retained mappings.
class DataflowExclusiveRoleStop {
    bool fHeldBeforeStop[DataflowBarrierWorkers]{};
    bool fRoleStopAttempted[DataflowBarrierWorkers]{};
    uint32_t fRoleStopToken[DataflowBarrierWorkers]{};
    bool fComputeStopAttempted[DataflowBarrierWorkers]{};
    bool fHoldAttempted[DataflowBarrierWorkers]{};
public:
    DataflowExclusiveRoleStop()=default;
    DataflowExclusiveRoleStop(const DataflowExclusiveRoleStop&)=delete;
    DataflowExclusiveRoleStop& operator=(const DataflowExclusiveRoleStop&)=delete;
    template<class IO> bool StopAndHold(const bool* released,const bool* confirmed,
        uint32_t readerPark,uint32_t writerPark,uint32_t computePark,unsigned polls,
        IO& io,unsigned& stage,unsigned& failedCore) {
        auto fail=[&](unsigned s,unsigned c) { stage=s;failedCore=c;return false; };
        if (!released || !confirmed || !polls || polls>65536) return fail(17,DataflowBarrierWorkers);
        for (unsigned core=0;core<DataflowBarrierWorkers;core++)
            if (confirmed[core] && !released[core]) return fail(17,core);
        // An unconfirmed posted release cannot be classified from a reset
        // sample alone. The adapter must prove this particular write drained.
        for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
            if (!released[core] || confirmed[core] || fRoleStopAttempted[core]
                || fComputeStopAttempted[core] || fHoldAttempted[core]) continue;
            if (fHeldBeforeStop[core]) {
                if (!io.UnconfirmedReleaseHeld(core)) return fail(15,core);
            } else if (io.UnconfirmedReleaseHeld(core)) fHeldBeforeStop[core]=true;
            else if (!io.UnconfirmedReleaseRunning(core)) return fail(15,core);
        }
        for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
            if (!released[core] || fHeldBeforeStop[core]) continue;
            if (fHoldAttempted[core]) {
                if (!io.HoldObserved(core)) return fail(11,core);
                continue;
            }
            bool okay=fRoleStopAttempted[core]?io.StopRequestObserved(core,fRoleStopToken[core])
                :io.RequestStop(core,false,fRoleStopAttempted[core],fRoleStopToken[core]);
            if (!okay && !fRoleStopAttempted[core])
                okay=io.RequestStop(core,true,fRoleStopAttempted[core],fRoleStopToken[core]);
            if (!okay) return fail(8,core);
        }
        for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
            if (!released[core] || fHeldBeforeStop[core] || fHoldAttempted[core]) continue;
            bool parked=false;
            for (unsigned n=0;n<polls;n++) {
                if (io.RolesStoppedAndParked(core,fRoleStopToken[core],readerPark,writerPark))
                    { parked=true;break; }
                io.Pause();
            }
            if (!parked) return fail(9,core);
        }
        for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
            if (!released[core] || fHeldBeforeStop[core] || fHoldAttempted[core]) continue;
            const bool okay=fComputeStopAttempted[core]?io.ComputeStopObserved(core)
                :io.RequestComputeStop(core,fComputeStopAttempted[core]);
            if (!okay) return fail(10,core);
        }
        for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
            if (!released[core] || fHeldBeforeStop[core] || fHoldAttempted[core]) continue;
            bool parked=false;
            for (unsigned n=0;n<polls;n++) {
                if (io.ComputeStoppedAndParked(core,computePark)) { parked=true;break; }
                io.Pause();
            }
            if (!parked) return fail(12,core);
        }
        for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
            if (!released[core] || fHeldBeforeStop[core]) continue;
            if (fHoldAttempted[core]? !io.HoldObserved(core):!io.HoldCore(core,fHoldAttempted[core]))
                return fail(13,core);
        }
        for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
            if (fHeldBeforeStop[core] && !io.UnconfirmedReleaseHeld(core)) return fail(15,core);
            if (!io.AllRolesHeld(core)) return fail(14,core);
        }
        stage=0;failedCore=DataflowBarrierWorkers;return true;
    }
};
}
#endif
