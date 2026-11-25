/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_GUARDED_MMIO_END_H
#define PIONEER_QWEN35_GUARDED_MMIO_END_H
#include "PioneerQwen35GuardedMmioBegin.h"
#include "PioneerQ38DispatchMmioEnd.h"

// Unmap must only dispose owned CPU mappings under the same retained-ticket
// check; it must not perform unguarded device register cleanup. Uncertain
// operations consume the close attempt and leave all remaining owners held.
template<class IO,class Check,class UnmapFn>
struct PioneerQ35GuardedMmioEndIO : PioneerQ35GuardedMmioBeginIO<IO,Check> {
    using Base=PioneerQ35GuardedMmioBeginIO<IO,Check>;
    UnmapFn& unmap;
    PioneerQ35GuardedMmioEndIO(IO& io,Check& check,UnmapFn& release):Base{io,check},unmap(release) {}
    bool Unmap() {
        if (!this->Allowed()) return false;
        const bool okay=unmap();return this->Allowed() && okay;
    }
};
template<class IO,class Check,class Unmap>
bool PioneerQ35GuardedMmioEnd(IO& io,tt_pioneer_mmio_result& result,
    tt_pioneer_mmio_transaction& transaction,Check& check,Unmap& unmap)
{
    PioneerQ35GuardedMmioEndIO<IO,Check,Unmap> guarded(io,check,unmap);
    if (!guarded.Allowed()) {
        transaction.config_attempted=false;result.state=TT_PIONEER_MMIO_RESTORE_FAILED;result.restored=0;return false;
    }
    const bool okay=PioneerQ38DispatchMmioEnd(guarded,result,transaction);
    if (guarded.lost || !guarded.Allowed()) {
        result.state=TT_PIONEER_MMIO_RESTORE_FAILED;result.restored=0;return false;
    }
    return okay;
}
#endif
