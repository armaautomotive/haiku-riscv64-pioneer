/* SPDX-License-Identifier: MIT */
// Actual separate diagnostic revision, compile/inspect ONLY. Not Work.
#include "../../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp"
static_assert(PIONEER_Q38_TOKEN_POSTED_CANCEL==1 && PioneerQ38TokenFaultProgramSelection==2
 && PioneerQ38TokenFaultNativePhysicalSelection==1 && PioneerQ38TokenNativeProfile==3,
 "explicit posted-cancel revision of actual G3 fault-ROM diagnostic");
static_assert(sizeof(PioneerQ38TokenTerminalResult)==64
 && __builtin_offsetof(PioneerQ38TokenTerminalResult,faultError)==56
 && __builtin_offsetof(PioneerQ38TokenTerminalResult,faultSavedLink)==60,"unchanged failure DATA ABI");
