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
    DataflowRoleProgramBounds bounds={{32,32},{64,64},
        {DataflowRoleReaderShimAddress+0x44,DataflowRoleWriterShimAddress+0x44}};
    assert(bounds.Valid());
    for (unsigned role=0;role<DataflowRoleCount;role++) {
        const unsigned shim=role==DataflowRoleReader?DataflowRoleReaderShimAddress:DataflowRoleWriterShimAddress;
        const unsigned code=role==DataflowRoleReader?DataflowRoleReaderCodeAddress:DataflowRoleWriterCodeAddress;
        assert(bounds.ContainsRunning(role,shim));
        assert(bounds.ContainsRunning(role,shim+4));
        assert(bounds.ContainsRunning(role,code+63*4));
        assert(!bounds.ContainsRunning(role,code+64*4));
        assert(!bounds.ContainsRunning(role,bounds.park[role]));
        assert(!bounds.ContainsRunning(role,shim+1));
        assert(!bounds.ContainsRunning(role,UINT32_MAX));
    }
    assert(!bounds.ContainsRunning(DataflowRoleReader,DataflowRoleWriterCodeAddress));
    assert(!bounds.ContainsRunning(DataflowRoleCount,DataflowRoleReaderCodeAddress));
    auto invalid=bounds; invalid.codeWords[0]=0;
    assert(!invalid.Valid() && !invalid.ContainsRunning(0,DataflowRoleReaderCodeAddress));
    invalid=bounds; invalid.park[1]=DataflowRoleWriterShimLimit;
    assert(!invalid.Valid());
    std::puts("PASS offline role boot jumps stay in their own low shim slots; NO BOOT IMAGE");
}
