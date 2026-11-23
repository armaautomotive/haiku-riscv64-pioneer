/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_PROJECTION_PAIR_H
#define PIONEER_QWEN35_DATAFLOW_PROJECTION_PAIR_H
#include "PioneerQwen35DataflowActivationReuse.h"
namespace PioneerQwen35 {
// Metadata-only placement of two independently authorized projections in one
// fleet step. Each active core executes one ordinary, unchanged invocation;
// there is no cross-core numerical reduction or host inference. The caller
// retains both immutable programs for all BindCore calls and must authorize
// both against the same model session and commit them as a single graph step.
class DataflowProjectionPair {
public:
    static bool Compatible(const DataflowProjectionProgram& a,const DataflowProjectionProgram& b) {
        if (!a.Valid() || !b.Valid() || a.activeCores+b.activeCores>DataflowCoreCount
            || a.inputRows!=b.inputRows || a.inputBase!=b.inputBase
            || !DataflowRowSpaceMatches(a.input,b.input)
            || !DataflowActivationReuseDisjoint(a.input,a.output)
            || !DataflowActivationReuseDisjoint(b.input,b.output)
            || !DataflowActivationReuseDisjoint(a.output,b.output)
            || !DataflowActivationReuseDisjoint(a.output,a.weights)
            || !DataflowActivationReuseDisjoint(a.output,b.weights)
            || !DataflowActivationReuseDisjoint(b.output,a.weights)
            || !DataflowActivationReuseDisjoint(b.output,b.weights)) return false;
        for (unsigned noc=0;noc<2;noc++) for (unsigned bank=0;bank<Banks;bank++)
            if (a.bankRoute[noc][bank]!=b.bankRoute[noc][bank]) return false;
        return true;
    }
    static bool BindCore(const DataflowProjectionProgram& a,const DataflowProjectionProgram& b,
        unsigned core,DataflowCoreInvocation& output) {
        if (core>=DataflowCoreCount || !Compatible(a,b)) return false;
        if (a.core[core].active) return DataflowProgramBuilder::BindCore(a,core,output);
        unsigned emptyRank=0;
        for (unsigned i=0;i<core;i++) if (!a.core[i].active) emptyRank++;
        for (unsigned source=0;source<DataflowCoreCount;source++) {
            if (!b.core[source].active) continue;
            if (emptyRank) { emptyRank--; continue; }
            if (!DataflowProgramBuilder::BindCore(b,source,output)) return false;
            output.core=core;
            return output.Valid();
        }
        // The first program's inactive invocation is a valid, harmless fleet
        // participant and reaches the same graph barrier as active cores.
        return DataflowProgramBuilder::BindCore(a,core,output) && !output.assignment.active;
    }
};
}
#endif
