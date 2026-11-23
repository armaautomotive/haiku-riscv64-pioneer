/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowRolePhysicalPlan.h"
#include <cassert>
#include <cstdio>
using namespace PioneerQwen35;
int main()
{
    const uint32_t offsets[]={0x19000000,0x19012000,0x19070000,0x190b0000,
        0x19314000,0x19316000,0x19312000,0x19313000,0x19320000,0x19330000};
    const uint32_t sizes[]={4096,4096,4096,49152,8192,8192,4096,4096,8192,8192};
    static_assert(sizeof(offsets)/sizeof(offsets[0])==DataflowRolePhysicalWindowCount,"one fixed mapping per window");
    for (unsigned i=0;i<DataflowRolePhysicalWindowCount;i++) {
        DataflowRolePhysicalWindow w{}; assert(DataflowRolePhysicalWindowPlan(i,w));
        assert(w.offset==offsets[i] && w.bytes==sizes[i]);
        for (unsigned j=0;j<i;j++) {
            DataflowRolePhysicalWindow prior{}; assert(DataflowRolePhysicalWindowPlan(j,prior));
            assert(uint64_t(w.offset)+w.bytes<=prior.offset
                || uint64_t(prior.offset)+prior.bytes<=w.offset);
        }
        uint64_t address=0; unsigned bytes=0;
        assert(DataflowRolePhysicalAddress(0x400000000ULL,0x20000000ULL,i,address,bytes));
        assert(address==0x400000000ULL+offsets[i] && bytes==sizes[i]);
        assert(!DataflowRolePhysicalAddress(0x400000000ULL,uint64_t(offsets[i])+sizes[i]-1,i,address,bytes));
        assert(!DataflowRolePhysicalAddress(UINT64_MAX-offsets[i]+1,0x20000000ULL,i,address,bytes));
    }
    assert(DataflowRolePhysicalReadAllowed(DataflowRolePhysicalBriscBoot,0));
    assert(!DataflowRolePhysicalReadAllowed(DataflowRolePhysicalBriscBoot,4));
    assert(DataflowRolePhysicalReadAllowed(DataflowRolePhysicalShim,0xfc));
    assert(!DataflowRolePhysicalReadAllowed(DataflowRolePhysicalShim,0x100));
    assert(DataflowRolePhysicalReadAllowed(DataflowRolePhysicalControl,0x1b0));
    assert(!DataflowRolePhysicalReadAllowed(DataflowRolePhysicalControl,0));
    assert(DataflowRolePhysicalReadAllowed(DataflowRolePhysicalPc,0x148));
    assert(!DataflowRolePhysicalReadAllowed(DataflowRolePhysicalPc,0x14c));
    const unsigned nocs[]={DataflowRolePhysicalNoc0,DataflowRolePhysicalNoc1};
    for (unsigned noc:nocs) {
        for (unsigned i=0;i<4;i++) assert(DataflowRolePhysicalReadAllowed(noc,0x40+i*0x800));
        for (unsigned i=0;i<32;i++) assert(DataflowRolePhysicalReadAllowed(noc,0x240+i*4));
        assert(!DataflowRolePhysicalReadAllowed(noc,0x20c));
        assert(DataflowRolePhysicalReadAllowed(noc,0x64));
        assert(DataflowRolePhysicalReadAllowed(noc,0x68));
        assert(DataflowRolePhysicalReadAllowed(noc,0x100));
        assert(!DataflowRolePhysicalReadAllowed(noc,0x104));
    }
    DataflowRolePhysicalWindow invalid{1,2};
    assert(!DataflowRolePhysicalWindowPlan(DataflowRolePhysicalWindowCount,invalid));
    assert(invalid.offset==1 && invalid.bytes==2);
    std::puts("PASS fixed role BAR window arithmetic and overflow bounds; NO HARDWARE");
}
