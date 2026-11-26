/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_PCIE_ROOT_RESET_READY_H
#define PIONEER_PCIE_ROOT_RESET_READY_H

#include "PioneerQ35PcieIdleAudit.h"

// Root-only observation before an endpoint configuration read. The caller
// must own the recovery transaction. A ready sample is not a guarantee that
// the link will remain up or that another writer or DMA is excluded.
template<class IO>
bool PioneerPcieRootResetReady(IO& io)
{
    PioneerQ35PcieAuditAccess<IO> access{io};
    tt_pioneer_pcie_trace_sample trace{};
    unsigned speed=0,width=0;
    return PioneerPcieRootTraceSample(access,trace) && trace.valid
        && !(trace.device_status & 0x000eU) && trace.uncorrectable==0
        && PioneerQ35PcieLinkSample(access,1,speed,width);
}

#endif
