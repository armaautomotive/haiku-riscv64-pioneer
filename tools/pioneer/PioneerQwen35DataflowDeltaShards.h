/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_DELTA_SHARDS_H
#define PIONEER_QWEN35_DATAFLOW_DELTA_SHARDS_H

// Offline witness for a two-barrier recurrent-state schedule. This does not
// alter the resident graph or authorize any card access. The caller must
// complete a fleet-wide barrier between Update and Normalize, retain the
// graph on any failure, and keep both state slots and QKV storage exclusively
// owned for the entire transaction.
#if !defined(PIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY)
#error Delta shards are an offline compile-only witness
#endif
#include "PioneerQwen35DataflowBlackholeDeltaWorker.h"
#include "PioneerQwen35DataflowDeltaShardProgram.h"

namespace PioneerQwen35 {
// Eight panels per head: 24 heads use four workers each and eight use three.
// The final normalization must read all eight completed panels for its head.
// All scratch is worker-local, and IO is the existing device-side Delta
// transport. No host arithmetic or fallback is introduced by this witness.
struct DataflowDeltaShardScratch {
    float* query;float* key;float* value;float* stateRead;float* stateWrite;
    float* scalars;float* headOutput;float* norm;float* gate;
    bool Valid() const {
        return query && key && value && stateRead && stateWrite && scalars
            && headOutput && norm && gate;
    }
};

template<class IO>
bool PioneerQwen35DataflowDeltaShardWorker(const DataflowDeltaProgram& program,
    unsigned core,DataflowDeltaShardPhase phase,DataflowDeltaShardScratch scratch,IO& io)
{
    DataflowDeltaShardAssignment assignment{};
    if (!scratch.Valid() || !program.ShapeValid() || !DataflowDeltaShardAssign(core,phase,assignment)) return false;
    for (unsigned panel=assignment.firstPanel;panel<assignment.firstPanel+assignment.panelCount;panel++)
        if (!program.PanelRangesValid(assignment.head*DataflowDeltaPanelsPerHead+panel)) return false;
    if (!assignment.panelCount) return true;
    if (!io.BindShardProgram(program)) return false;
    const unsigned head=assignment.head;
    if (phase==DataflowDeltaShardNormalize) {
        if (!io.ReadDeltaNorm(scratch.norm) || !io.ReadGateHead(head,scratch.gate)) return false;
        for (unsigned panel=0;panel<DataflowDeltaPanelsPerHead;panel++)
            if (!io.ReadValuePanel(head*DeltaWidth+panel*16,scratch.headOutput+panel*16)) return false;
        if (!DataflowDeltaNormalizeAndGate(scratch.headOutput,scratch.norm,scratch.gate)) return false;
        for (unsigned panel=0;panel<DataflowDeltaPanelsPerHead;panel++)
            if (!io.WriteValuePanel(head*DeltaWidth+panel*16,scratch.headOutput+panel*16)) return false;
        return true;
    }
    float* const negativeA=scratch.scalars+DataflowDeltaNegativeAOffset;
    float* const bias=scratch.scalars+DataflowDeltaBiasOffset;
    float* const alpha=scratch.scalars+DataflowDeltaAlphaOffset;
    float* const betaLogit=scratch.scalars+DataflowDeltaBetaOffset;
    if (!io.ReadParameters(negativeA,bias,alpha,betaLogit)
        || !io.ReadQueryKey(head,scratch.query,scratch.key)
        || !DataflowDeltaNormalizeQk(scratch.query,scratch.key)) return false;
    float decay=0,beta=0;
    if (!DataflowDeltaHeadGates(negativeA[head],bias[head],alpha[head],betaLogit[head],decay,beta)) return false;
    for (unsigned panel=assignment.firstPanel;panel<assignment.firstPanel+assignment.panelCount;panel++) {
        const unsigned row=head*DeltaWidth+panel*16;
        if (!io.ReadValuePanel(row,scratch.value)) return false;
        if (program.position) {
            if (!io.ReadStatePanel(head,panel,scratch.stateRead)) return false;
        } else {
            volatile float* zero=scratch.stateRead;
            for (unsigned i=0;i<DeltaWidth*16;i++) zero[i]=0.0f;
        }
        if (!DataflowDeltaUpdatePanel(scratch.query,scratch.key,scratch.value,scratch.stateRead,
                scratch.stateWrite,scratch.value,decay,beta)
            || !io.WriteStatePanel(head,panel,scratch.stateWrite)
            || !io.WriteValuePanel(row,scratch.value)) return false;
    }
    return true;
}
}
#endif
