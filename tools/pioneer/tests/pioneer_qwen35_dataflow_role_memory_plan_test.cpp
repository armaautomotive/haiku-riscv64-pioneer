/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowRoleMemoryPlan.h"
#include <cassert>
#include <cstdio>
using namespace PioneerQwen35;
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
