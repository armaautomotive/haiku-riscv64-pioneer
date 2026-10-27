/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_MIXED_LIVE_FINAL_RELEASE
#include "PioneerQ38MixedLiveFinalReleaseRole.h"
#endif
#ifndef PIONEER_Q38_MIXED_LIVE_FINAL_PCI_STATE_H
#define PIONEER_Q38_MIXED_LIVE_FINAL_PCI_STATE_H
#include "BlackholePioneerMmioProbe.h"
#include "BlackholeArcClockGuard.h"
// CPU predicate ONLY,never a release/source/clock grant or status setter.
// DispatchMmioEnd pessimistically marks RESTORE_FAILED BEFORE calling
// Unmap. Admit that marker ONLY at the genuine closed-native/ended-clock
// pre-Unmap boundary. Every OTHER Unsafe condition remains independent.
template<class Context> bool PioneerQ38MixedLiveFinalPciUncertain(Context& c){
 if(c.Pci().state!=TT_PIONEER_MMIO_RESTORE_FAILED)return false;
 if(!c.bankStorageOnly || c.q38EndPhase!=50 || !c.transaction.map_attempted || c.transaction.config_attempted
  || !c.q38MixedLiveNative || !c.q38MixedLiveNative->Closed() || !c.io.q38MixedLiveMemoryMapAttempted
  || c.io.q38MixedLiveFinalUnmapAttempted || !c.io.MixedLiveCallScopeMatches()
  || c.q38Clock.Unsafe() || c.q38Clock.RequiresExplicitEnd())return true;
 const auto& clock=c.q38Clock.Snapshot();
 return clock.state!=TT_ARC_CLOCK_RESTORED || !clock.restored || clock.recovery_required;
} // no IO,flag clearing,rollback,retry or successful PCI publication
#endif
