/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowRoleBoot.h"
#include <cassert>
#include <cstdio>
using namespace PioneerQwen35;
static int32_t DecodeJump(uint32_t word)
{
    assert((word&0xfffU)==0x6fU);
    uint32_t value=((word>>31)&1U)<<20 | ((word>>21)&0x3ffU)<<1
        | ((word>>20)&1U)<<11 | ((word>>12)&0xffU)<<12;
    if (value&(1U<<20)) value|=0xffe00000U;
    return int32_t(value);
}
int main()
{
    uint32_t word=0x12345678;
    for (unsigned role=0;role<DataflowRoleCount;role++) {
        const unsigned first=role==DataflowRoleReader?DataflowRoleReaderShimAddress:DataflowRoleWriterShimAddress;
        const unsigned last=role==DataflowRoleReader?DataflowRoleReaderShimLimit:DataflowRoleWriterShimLimit;
        const unsigned boot=role==DataflowRoleReader?DataflowRoleBriscBootAddress:DataflowRoleNcriscBootAddress;
        for (unsigned entry=first;entry<last;entry+=4) {
            assert(DataflowRoleBootWord(role,entry,word));
            assert(int64_t(boot)+DecodeJump(word)==entry);
        }
        assert(!DataflowRoleBootWord(role,first-4,word));
        assert(!DataflowRoleBootWord(role,last,word));
        assert(!DataflowRoleBootWord(role,first+1,word));
    }
    assert(!DataflowRoleBootWord(DataflowRoleCount,DataflowRoleReaderShimAddress,word));
    std::puts("PASS offline role boot jumps stay in their own low shim slots; NO BOOT IMAGE");
}
