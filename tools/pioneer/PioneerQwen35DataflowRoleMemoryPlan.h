/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_ROLE_MEMORY_PLAN_H
#define PIONEER_QWEN35_DATAFLOW_ROLE_MEMORY_PLAN_H

#include "PioneerQwen35DataflowBarrierLayout.h"
#include <stdint.h>
#include "PioneerQwen35DataflowRoleAsmConstants.h"

namespace PioneerQwen35 {
// Offline placement plan only. Current PCI ownership ends at 0xb0000 and
// does not admit these addresses. A future owner must first snapshot and
// validate the entire extra region, boot words, reset state and local RAM,
// then prove all roles have stopped before restoring anything.
enum : unsigned {
    // Blackhole DISABLE_RESET bits for B/NC local RAM and debug registers.
    DataflowRolePreserveResetMask=0x3c0,
    // Proposed low-L1 startup shims sit above the TRISC0 stack top and below
    // its source windows. The existing snapshot covers them, but its PCI
    // write gate does not currently admit them.
    DataflowRoleReaderShimAddress=0x70000,
    DataflowRoleReaderShimLimit=0x70080,
    DataflowRoleWriterShimAddress=0x70080,
    DataflowRoleWriterShimLimit=0x70100,
    DataflowRoleReaderCodeAddress=0xb0000,
    DataflowRoleReaderCodeLimit=0xb5000,
    DataflowRoleWriterCodeAddress=0xb5000,
    DataflowRoleWriterCodeLimit=0xb8000,
    DataflowRoleReaderRamCopy=PIONEER_Q35_ROLE_READER_RAM_COPY,
    DataflowRoleWriterRamCopy=PIONEER_Q35_ROLE_WRITER_RAM_COPY,
    DataflowRoleRamCopyBytes=8192,
    DataflowRoleExtraL1Address=DataflowRoleReaderCodeAddress,
    DataflowRoleExtraL1Limit=DataflowRoleWriterRamCopy+DataflowRoleRamCopyBytes,
    DataflowRoleBriscBootAddress=0x00000,
    DataflowRoleNcriscBootAddress=0x12000,
    DataflowRolePrivateStackBase=0xffb00000,
    DataflowRolePrivateStackBytes=8192
};
static_assert(DataflowRolePrivateStackBase+DataflowRolePrivateStackBytes
    ==PIONEER_Q35_ROLE_PRIVATE_STACK_TOP,"private B/NC stack tops match the offline shim");
static_assert(DataflowRoleReaderShimLimit==DataflowRoleWriterShimAddress
    && DataflowRoleWriterShimLimit<=0x70700,
    "proposed low shims are disjoint from existing Tensix source windows");
static_assert(unsigned(DataflowRoleExtraL1Address)==unsigned(DataflowBarrierL1Limit)
    && DataflowRoleReaderCodeLimit==DataflowRoleWriterCodeAddress
    && DataflowRoleWriterCodeLimit==DataflowRoleReaderRamCopy
    && DataflowRoleReaderRamCopy+DataflowRoleRamCopyBytes==DataflowRoleWriterRamCopy
    && DataflowRoleExtraL1Limit<=0x180000,
    "role code and pre-stack RAM copies occupy disjoint, saved L1 ranges");
inline bool DataflowRoleExtraL1Range(unsigned address,unsigned bytes)
{
    return bytes && !(address&3) && !(bytes&3)
        && address>=DataflowRoleExtraL1Address && address<DataflowRoleExtraL1Limit
        && bytes<=DataflowRoleExtraL1Limit-address;
}
}
#endif
