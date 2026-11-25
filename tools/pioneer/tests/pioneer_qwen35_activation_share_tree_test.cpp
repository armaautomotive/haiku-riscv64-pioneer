/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowActivationShareTree.h"
#include <cassert>
#include <cstdio>

using namespace PioneerQwen35;

static void Check(unsigned stride,unsigned offset)
{
    DataflowActivationShareMembership group{};
    group.packing=12;
    unsigned members[DataflowCoreCount],count=0;
    for (unsigned core=offset;core<DataflowCoreCount;core+=stride) {
        group.members[core/32]|=uint32_t(1)<<(core%32);
        members[count++]=core;
    }
    assert(count);
    group.leader=members[0];
    unsigned children[DataflowCoreCount]{};
    for (unsigned rank=0;rank<count;rank++) {
        DataflowActivationShareTreePosition position{};
        assert(DataflowActivationShareTreeLocate(group,members[rank],position));
        assert(position.rank==rank && position.count==count);
        if (!rank) assert(position.parent==DataflowCoreCount);
        else {
            assert(position.parent==members[(rank-1)/DataflowActivationShareTreeFanout]);
            ++children[position.parent];
            unsigned cursor=members[rank],depth=0;
            while (cursor!=group.leader) {
                assert(DataflowActivationShareTreeLocate(group,cursor,position));
                assert(position.parent<cursor && ++depth<=DataflowCoreCount);
                cursor=position.parent;
            }
        }
    }
    for (unsigned core=0;core<DataflowCoreCount;core++) {
        assert(children[core]<=DataflowActivationShareTreeFanout);
        if (!(group.members[core/32]&(uint32_t(1)<<(core%32)))) {
            DataflowActivationShareTreePosition unused{};
            assert(!DataflowActivationShareTreeLocate(group,core,unused));
        }
    }
    DataflowActivationShareTreePosition unchanged{7,8,9};
    assert(!DataflowActivationShareTreeLocate(group,DataflowCoreCount,unchanged));
    assert(unchanged.rank==7 && unchanged.count==8 && unchanged.parent==9);
    auto malformed=group;malformed.members[3]|=0x80000000U;
    assert(!DataflowActivationShareTreeLocate(malformed,members[0],unchanged));
    malformed=group;malformed.packing=0;
    assert(!DataflowActivationShareTreeLocate(malformed,members[0],unchanged));
    malformed=group;malformed.packing=9;
    assert(!DataflowActivationShareTreeLocate(malformed,members[0],unchanged));
    malformed=group;malformed.leader=DataflowCoreCount;
    assert(!DataflowActivationShareTreeLocate(malformed,members[0],unchanged));
    if (count>1) {
        malformed=group;malformed.leader=members[1];
        assert(!DataflowActivationShareTreeLocate(malformed,members[0],unchanged));
    }
}

int main()
{
    for (unsigned stride=1;stride<=DataflowCoreCount;stride++)
        for (unsigned offset=0;offset<stride && offset<DataflowCoreCount;offset++)
            Check(stride,offset);
    std::puts("PASS activation-sharing tree covers dense and sparse groups; OFFLINE ONLY");
}
