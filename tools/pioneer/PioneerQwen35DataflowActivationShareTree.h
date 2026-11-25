/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_ACTIVATION_SHARE_TREE_H
#define PIONEER_QWEN35_DATAFLOW_ACTIVATION_SHARE_TREE_H

#include "PioneerQwen35DataflowProgram.h"

namespace PioneerQwen35 {

// Candidate unicast topology for a sealed activation-sharing membership.
// Ranks are in logical-core order, not physical rectangle order. This makes
// the tree deterministic even when a group contains noncontiguous cores.
// This describes edges only: it neither transfers data nor authorizes access.
enum : unsigned { DataflowActivationShareTreeFanout=4 };
struct DataflowActivationShareTreePosition {
    unsigned rank, count, parent;
};

inline bool DataflowActivationShareTreeLocate(
    const DataflowActivationShareMembership& group,unsigned core,
    DataflowActivationShareTreePosition& result)
{
    if (core>=DataflowCoreCount || group.leader>=DataflowCoreCount
        || (group.members[3]&0xff000000U)
        || (group.packing!=8 && group.packing!=12)) return false;

    unsigned count=0,rank=DataflowCoreCount,first=DataflowCoreCount;
    for (unsigned i=0;i<DataflowCoreCount;i++) {
        if (!(group.members[i/32]&(uint32_t(1)<<(i%32)))) continue;
        if (first==DataflowCoreCount) first=i;
        if (i==core) rank=count;
        ++count;
    }
    if (first!=group.leader || rank==DataflowCoreCount) return false;
    unsigned parent=DataflowCoreCount;
    if (rank) {
        const unsigned parentRank=(rank-1)/DataflowActivationShareTreeFanout;
        unsigned index=0;
        for (unsigned i=0;i<DataflowCoreCount;i++) {
            if (!(group.members[i/32]&(uint32_t(1)<<(i%32)))) continue;
            if (index++==parentRank) { parent=i;break; }
        }
        if (parent>=core) return false;
    }
    result={rank,count,parent};
    return true;
}

}
#endif
