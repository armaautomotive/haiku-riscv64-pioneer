/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_ACTIVATION_SHARE_PLAN_H
#define PIONEER_QWEN35_DATAFLOW_ACTIVATION_SHARE_PLAN_H

#include "PioneerQwen35DataflowActivationReuse.h"

namespace PioneerQwen35 {
// Host-side model-recording plan for an eventual shared activation transfer.
// It makes no device write and grants no peer, NoC, or firmware authority.
// The caller must bind its result to immutable, independently authorized graph
// records; each device must still verify its own command and peer routes.
// Allocate this object outside the kernel stack. One Add per logical core,
// including inactive cores, is required before sealing. Grouping compares
// the full source layout and routes, rather than relying on the short key.
class DataflowActivationSharePlan {
    enum Phase { Building,Sealed,Faulted } phase=Building;
    struct Group {
        RowSpace input{};
        unsigned base=0,rows=0,packing=0,routes[Banks]{};
        DataflowActivationShareMembership membership{};
    };
    Group groups[DataflowCoreCount]{};
    RowSpace outputs[DataflowCoreCount]{};
    unsigned groupOf[DataflowCoreCount]{};
    bool seen[DataflowCoreCount]{};
    unsigned seenCount=0,groupCount=0,outputCount=0;
    bool Fail() { phase=Faulted;return false; }
public:
    DataflowActivationSharePlan()=default;
    DataflowActivationSharePlan(const DataflowActivationSharePlan&)=delete;
    DataflowActivationSharePlan& operator=(const DataflowActivationSharePlan&)=delete;
    bool Add(const DataflowCoreInvocation& invocation) {
        if (phase!=Building || !invocation.Valid() || seen[invocation.core]) return Fail();
        const unsigned core=invocation.core;seen[core]=true;++seenCount;
        groupOf[core]=DataflowCoreCount;
        if (!invocation.assignment.active) return true;
        const unsigned packing=DataflowActivationReusePacking(invocation.type);
        if (!packing) return Fail();
        unsigned index=0;
        for (;index<groupCount;index++) {
            const auto& group=groups[index];
            bool same=group.base==invocation.inputBase && group.rows==invocation.inputRows
                && group.packing==packing && DataflowRowSpaceMatches(group.input,invocation.input);
            for (unsigned bank=0;bank<Banks;bank++)
                same=same && group.routes[bank]==invocation.bankRoute[0][bank];
            if (same) break;
        }
        if (index==groupCount) {
            if (groupCount>=DataflowCoreCount) return Fail();
            auto& group=groups[groupCount++];
            group.input=invocation.input;group.base=invocation.inputBase;
            group.rows=invocation.inputRows;group.packing=packing;
            group.membership.packing=packing;
            group.membership.inputKey=DataflowActivationShareInputKey(invocation);
            for (unsigned bank=0;bank<Banks;bank++) group.routes[bank]=invocation.bankRoute[0][bank];
        }
        auto& membership=groups[index].membership;
        membership.members[core/32]|=uint32_t(1)<<(core%32);
        if (core<membership.leader) membership.leader=core;
        groupOf[core]=index;
        bool haveOutput=false;
        for (unsigned i=0;i<outputCount;i++) haveOutput|=DataflowRowSpaceMatches(outputs[i],invocation.output);
        if (!haveOutput) {
            if (outputCount>=DataflowCoreCount) return Fail();
            outputs[outputCount++]=invocation.output;
        }
        return true;
    }
    bool Seal() {
        if (phase!=Building || seenCount!=DataflowCoreCount || !groupCount) return Fail();
        // Sharing can move a read earlier or later. Reject a step if any
        // projection output can write any member group's input row space.
        for (unsigned i=0;i<groupCount;i++) for (unsigned j=0;j<outputCount;j++)
            if (!DataflowActivationReuseDisjoint(groups[i].input,outputs[j])) return Fail();
        phase=Sealed;return true;
    }
    unsigned Groups() const { return phase==Sealed?groupCount:0; }
    bool Get(unsigned core,DataflowActivationShareMembership& result) const {
        if (phase!=Sealed || core>=DataflowCoreCount || !seen[core]
            || groupOf[core]>=groupCount) return false;
        result=groups[groupOf[core]].membership;return true;
    }
};
}
#endif
