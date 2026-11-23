/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_ACTIVE_ROOT_LEASE_H
#define PIONEER_QWEN35_ACTIVE_ROOT_LEASE_H
#include "BlackholeResidentReadProbe.h"

// Preparation owns core 0's NIU and sets bit 16 of its saved configuration.
// A fresh DMA audit must compare with that active value, not the pre-lease
// baseline. The lease identity and all other source words still must match.
template<class IO,class Lease>
bool PioneerQwen35ActiveRootLeaseMatches(IO& io,const tt_compute_noc_result& noc,const Lease& lease)
{
    return lease.controls_saved && lease.configured && lease.ready && lease.saved
        && !lease.pending && !lease.recovery_required && !lease.niu_restored
        && lease.config==noc.source[0][0]
        && BlackholeResidentSourceStable(io,noc,lease.config|0x10000U);
}
#endif
