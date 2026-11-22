/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowRoleMemoryPlan.h"
#include <cassert>
#include <cstdio>
using namespace PioneerQwen35;
static_assert(DataflowRoleReaderShimAddress==0x70000 && DataflowRoleReaderShimLimit==0x70080
    && DataflowRoleWriterShimAddress==0x70080 && DataflowRoleWriterShimLimit==0x70100,
    "proposed shims fit the saved low-L1 gap");
static_assert(DataflowRoleReaderCodeAddress==0xb0000 && DataflowRoleReaderCodeLimit==0xb5000
    && DataflowRoleWriterCodeAddress==0xb5000 && DataflowRoleWriterCodeLimit==0xb8000,
    "witness linker slots match the proposed memory plan");
int main()
{
    assert(DataflowRoleExtraL1Range(DataflowRoleReaderCodeAddress,
        DataflowRoleExtraL1Limit-DataflowRoleReaderCodeAddress));
    assert(DataflowRoleExtraL1Range(DataflowRoleWriterCodeAddress,4));
    assert(!DataflowRoleExtraL1Range(DataflowRoleReaderCodeAddress-4,4));
    assert(!DataflowRoleExtraL1Range(DataflowRoleExtraL1Limit,4));
    assert(!DataflowRoleExtraL1Range(DataflowRoleExtraL1Limit-4,8));
    assert(!DataflowRoleExtraL1Range(DataflowRoleReaderCodeAddress,0));
    assert(!DataflowRoleExtraL1Range(DataflowRoleReaderCodeAddress+1,4));
    assert(!DataflowRoleExtraL1Range(DataflowRoleReaderCodeAddress,0xffffffffU));
    std::puts("PASS offline additional role L1 bounds; existing owner does not admit them");
}
