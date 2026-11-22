/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_ROLE_MEMORY_PLAN_H
#define PIONEER_QWEN35_DATAFLOW_ROLE_MEMORY_PLAN_H

#include "PioneerQwen35DataflowBarrierLayout.h"
#include <stdint.h>

namespace PioneerQwen35 {
// Offline placement plan only. Current PCI ownership ends at 0xb0000 and
// does not admit these addresses. A future owner must first snapshot and
// validate the entire extra region, boot words, reset state and local RAM,
// then prove all roles have stopped before restoring anything.
enum : unsigned {
    DataflowRoleReaderCodeAddress=0xb0000,
    DataflowRoleReaderCodeLimit=0xb4000,
    DataflowRoleWriterCodeAddress=0xb4000,
    DataflowRoleWriterCodeLimit=0xb6000,
    DataflowRoleExtraL1Address=DataflowRoleReaderCodeAddress,
    DataflowRoleExtraL1Limit=DataflowRoleWriterCodeLimit,
    DataflowRoleBriscBootAddress=0x00000,
    DataflowRoleNcriscBootAddress=0x12000,
    DataflowRolePrivateStackBase=0xffb00000,
    DataflowRolePrivateStackBytes=8192
};
static_assert(unsigned(DataflowRoleExtraL1Address)==unsigned(DataflowBarrierL1Limit)
    && DataflowRoleReaderCodeLimit==DataflowRoleWriterCodeAddress
    && DataflowRoleExtraL1Limit<=0x180000,
    "proposed additional role code follows, but is not in, the saved L1 view");
inline bool DataflowRoleExtraL1Range(unsigned address,unsigned bytes)
{
    return bytes && !(address&3) && !(bytes&3)
        && address>=DataflowRoleExtraL1Address && address<DataflowRoleExtraL1Limit
        && bytes<=DataflowRoleExtraL1Limit-address;
}
}
#endif
