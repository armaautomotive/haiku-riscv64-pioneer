/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_RMS_SAVE_H
#define PIONEER_QWEN35_DATAFLOW_RMS_SAVE_H

#if defined(PIONEER_QWEN35_FUSED_RMS_SAVE_COMPILE_ONLY) \
    && (!defined(PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS) || !defined(PIONEER_QWEN35_RECOVERY_COMPILE_ONLY) \
        || !defined(PIONEER_QWEN35_DEVICE_DISPATCH) || defined(PIONEER_QWEN35_DEVICE_DISPATCH_TRIAL))
#error Fused RMS/save may be selected only by the non-loadable resident graph
#endif

// Resident-graph optimization under development: one fleet operation saves
// the residual while computing RMS. This does not itself authorize a command,
// publish a graph record, release a tile, or provide recovery.
#include "PioneerQwen35DataflowBlackholeRmsWorker.h"
#include "PioneerQwen35DataflowBlackholeResidualWorker.h"

namespace PioneerQwen35 {

enum : unsigned { DataflowRmsSaveMagic=0x31534635, DataflowRmsSaveVersion=1 };
static_assert(unsigned(DataflowRmsGroups)==unsigned(DataflowResidualGroups)
    && unsigned(DataflowRmsSplitCores)==unsigned(DataflowResidualSplitCores),
    "normalization and residual save must assign the same rows to each core");

struct DataflowRmsSaveInvocation {
    unsigned magic,version,core;
    DataflowRmsInvocation rms;
    DataflowResidualInvocation residual;

    bool Valid() const {
        if (magic!=DataflowRmsSaveMagic || version!=DataflowRmsSaveVersion
            || core>=DataflowCoreCount || !rms.Valid() || !residual.Valid()
            || rms.core!=core || residual.core!=core || residual.mode!=DataflowResidualSave
            || rms.layer>=Layers || (rms.role!=AttentionNorm && rms.role!=PostAttentionNorm)
            || rms.inputSlot!=residual.activationSlot
            || !DataflowRowSpaceMatches(rms.input,residual.activation)
            || !DataflowRmsSpacesDisjoint(residual.residual,rms.input)
            || !DataflowRmsSpacesDisjoint(residual.residual,rms.output)
            || !DataflowRmsSpacesDisjoint(residual.residual,rms.weight)) return false;
        for (unsigned noc=0;noc<2;noc++) for (unsigned bank=0;bank<Banks;bank++)
            if (rms.bankRoute[noc][bank]!=residual.bankRoute[noc][bank]) return false;
        return true;
    }
};

inline bool DataflowRmsSaveBind(const DataflowRmsProgram& rms,const DataflowResidualProgram& residual,
    unsigned core,DataflowRmsSaveInvocation& output) {
    output={};
    DataflowRmsSaveInvocation candidate{};
    candidate.magic=DataflowRmsSaveMagic;candidate.version=DataflowRmsSaveVersion;candidate.core=core;
    if (!DataflowRmsProgramBuilder::BindCore(rms,core,candidate.rms)
        || !DataflowResidualProgramBuilder::BindCore(residual,core,candidate.residual)
        || !candidate.Valid()) return false;
    output=candidate;return true;
}

// The existing RMS worker reads each panel exactly once. This adapter saves
// that same panel before it is normalized; the graph's single completion
// barrier must cover both writes. On failure the caller must retain ownership.
template<class IO> class DataflowRmsSaveAdapter {
    IO& fIO;
    const DataflowResidualInvocation& fResidual;
public:
    DataflowRmsSaveAdapter(IO& io,const DataflowResidualInvocation& residual):fIO(io),fResidual(residual) {}
    bool ReadHiddenInput(float* out) { return fIO.ReadHiddenInput(out); }
    bool WriteScaleRecord(const DataflowRmsScaleRecord& value) { return fIO.WriteScaleRecord(value); }
    bool ReadScaleRecord(DataflowRmsScaleRecord& value) { return fIO.ReadScaleRecord(value); }
    bool ReadInputPanel(unsigned row,float* out) {
        if (!fIO.ReadInputPanel(row,out)) return false;
        for (unsigned i=0;i<fResidual.segmentCount;i++) {
            const auto& part=fResidual.segment[i];
            if (part.firstRow==row) return fIO.WriteResidualPanel(part,out);
        }
        return false;
    }
    bool ReadWeightPanel(unsigned row,float* out) { return fIO.ReadWeightPanel(row,out); }
    bool WriteOutput(unsigned bank,uint32_t address,const uint8_t* data,unsigned bytes) {
        return fIO.WriteOutput(bank,address,data,bytes);
    }
};

template<class IO>
__attribute__((noinline)) bool PioneerQwen35DataflowRmsSaveWorker(const DataflowRmsSaveInvocation& invocation,
    unsigned sequence,float* inputCache,float* inputPage,float* weightPage,float* outputPage,
    IO& io,unsigned* failureStage=nullptr) {
    if (!invocation.Valid()) return false;
    DataflowRmsSaveAdapter<IO> fused(io,invocation.residual);
    return PioneerQwen35DataflowRmsWorker(invocation.rms,sequence,inputCache,inputPage,
        weightPage,outputPage,fused,failureStage);
}

// Hardware adapter composes the existing bounded, synchronous NoC operations.
// Driver-side ownership and firmware exclusion remain independent requirements.
class DataflowBlackholeRmsSaveOps {
    DataflowBlackholeRmsOps fRms;
    DataflowBlackholeResidualOps fResidual;
public:
    bool Bind(const DataflowRmsSaveInvocation& invocation,unsigned sequence) {
        return invocation.Valid() && fRms.Bind(invocation.rms,sequence) && fResidual.Bind(invocation.residual);
    }
    const DataflowNocDiagnostic& Diagnostic() const { return fRms.Diagnostic(); }
    bool ReadHiddenInput(float* out) { return fRms.ReadHiddenInput(out); }
    bool WriteScaleRecord(const DataflowRmsScaleRecord& value) { return fRms.WriteScaleRecord(value); }
    bool ReadScaleRecord(DataflowRmsScaleRecord& value) { return fRms.ReadScaleRecord(value); }
    bool ReadInputPanel(unsigned row,float* out) { return fRms.ReadInputPanel(row,out); }
    bool WriteResidualPanel(const DataflowOutputSegment& segment,const float* data) {
        return fResidual.WriteResidualPanel(segment,data);
    }
    bool ReadWeightPanel(unsigned row,float* out) { return fRms.ReadWeightPanel(row,out); }
    bool WriteOutput(unsigned bank,uint32_t address,const uint8_t* data,unsigned bytes) {
        return fRms.WriteOutput(bank,address,data,bytes);
    }
};

static_assert(sizeof(DataflowRmsSaveInvocation)<=DataflowProgramRegionBytes,
    "fused RMS and save invocation must fit the resident program region");
}
#endif
