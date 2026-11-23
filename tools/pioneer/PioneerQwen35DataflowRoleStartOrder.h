/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_ROLE_START_ORDER_H
#define PIONEER_QWEN35_DATAFLOW_ROLE_START_ORDER_H
#include "PioneerQwen35DataflowRoleFleetRecoveryOwner.h"
namespace PioneerQwen35 {
// Software ordering only. Physical ownership, reset, DMA and NoC proofs are
// separate. Each attempted release/handoff consumes its state before any IO;
// a failed or uncertain attempt cannot be retried or skipped by this owner.
class DataflowRoleStartOrder {
    enum Phase { Unarmed,Ready,Releasing,HandoffReady,Handoff,Complete,Closed };
    Phase fPhase=Unarmed;
    unsigned fNext=0;
public:
    DataflowRoleStartOrder()=default;
    DataflowRoleStartOrder(const DataflowRoleStartOrder&)=delete;
    DataflowRoleStartOrder& operator=(const DataflowRoleStartOrder&)=delete;
    bool Arm() {
        if (fPhase!=Unarmed) return false;
        fPhase=Ready;return true;
    }
    bool BeginRelease(unsigned core) {
        if (fPhase!=Ready || core!=fNext || core>=DataflowBarrierWorkers) return false;
        fPhase=Releasing;return true;
    }
    bool ConfirmRelease(unsigned core) {
        if (fPhase!=Releasing || core!=fNext) return false;
        fPhase=HandoffReady;return true;
    }
    bool BeginHandoff(unsigned core) {
        if (fPhase!=HandoffReady || core!=fNext) return false;
        fPhase=Handoff;return true;
    }
    bool ConfirmHandoff(unsigned core) {
        if (fPhase!=Handoff || core!=fNext) return false;
        ++fNext;fPhase=fNext==DataflowBarrierWorkers?Complete:Ready;return true;
    }
    void Close() { fPhase=Closed; }
};
}
#endif
