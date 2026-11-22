/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_ROLE_BOOT_H
#define PIONEER_QWEN35_DATAFLOW_ROLE_BOOT_H
#include "PioneerQwen35DataflowRoleMemoryPlan.h"
#include "PioneerQwen35DataflowRoleMailbox.h"
namespace PioneerQwen35 {
// Link-only boot-word planning. The target must be the audited startup shim
// inside its low-L1 shim slot; encoding a jump does not establish that a
// shim exists or that reset, boot memory, or local RAM are owned.
inline bool DataflowRoleBootWord(unsigned role,unsigned entry,uint32_t& word)
{
    if (role>=DataflowRoleCount || (entry&3)) return false;
    const unsigned first=role==DataflowRoleReader?DataflowRoleReaderShimAddress:DataflowRoleWriterShimAddress;
    const unsigned last=role==DataflowRoleReader?DataflowRoleReaderShimLimit:DataflowRoleWriterShimLimit;
    const unsigned boot=role==DataflowRoleReader?DataflowRoleBriscBootAddress:DataflowRoleNcriscBootAddress;
    if (entry<first || entry>=last) return false;
    const int64_t difference=int64_t(entry)-int64_t(boot);
    if (difference < -1048576 || difference > 1048574 || (difference&1)) return false;
    const uint32_t immediate=uint32_t(difference)&0x1fffffU;
    word=0x6fU | ((immediate>>20)&1U)<<31 | ((immediate>>1)&0x3ffU)<<21
        | ((immediate>>11)&1U)<<20 | ((immediate>>12)&0xffU)<<12;
    return true;
}
}
#endif
