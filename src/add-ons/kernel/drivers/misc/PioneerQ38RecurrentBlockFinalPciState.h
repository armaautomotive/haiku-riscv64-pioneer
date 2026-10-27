/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_RECURRENT_BLOCK_FINAL_RELEASE
#include "PioneerQ38RecurrentBlockFinalReleaseRole.h"
#endif
#ifndef PIONEER_Q38_RECURRENT_BLOCK_FINAL_PCI_STATE_H
#define PIONEER_Q38_RECURRENT_BLOCK_FINAL_PCI_STATE_H
#include "BlackholePioneerMmioProbe.h"
#include "BlackholeArcClockGuard.h"
// Necessary CPU predicate ONLY. DispatchMmioEnd marks RESTORE_FAILED
// before Unmap;only its genuine closed recurrent/ended-clock boundary may
// admit that provisional marker. All other Unsafe predicates still apply.
template<class Context> bool PioneerQ38RecurrentBlockFinalPciUncertain(Context& c){
 if(c.Pci().state!=TT_PIONEER_MMIO_RESTORE_FAILED)return false;
 if(!c.bankStorageOnly || c.q38EndPhase!=50 || !c.transaction.map_attempted || c.transaction.config_attempted
  || !c.q38RecurrentBlockNative || !c.q38RecurrentBlockNative->Closed() || !c.io.q38RecurrentBlockMemoryMapAttempted
  || c.io.q38RecurrentBlockFinalUnmapAttempted || !c.io.RecurrentBlockCallScopeMatches()
  || c.q38Clock.Unsafe() || c.q38Clock.RequiresExplicitEnd())return true;
 const auto& clock=c.q38Clock.Snapshot();
 return clock.state!=TT_ARC_CLOCK_RESTORED || !clock.restored || clock.recovery_required;
} // no IO,closed setter,PCI success publication,rollback or retry
#endif
