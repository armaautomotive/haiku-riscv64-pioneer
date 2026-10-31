/* SPDX-License-Identifier: MIT */
// Actual conventional diagnostic ABI;BUILD/INSPECT only,never a Work route.
#include "../../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp"
static_assert(PioneerQ38TokenFaultProgramSelection==2 && PioneerQ38TokenFaultNativePhysicalSelection==1,
 "separate explicitly selected physical fault-ROM revision");
static_assert(PioneerQ38TokenNativePhysicalSelection==2 && PioneerQ38TokenNativeProfile==3,
 "same bounded tag75 ABI and full G3 native owner");
static_assert(sizeof(PioneerQ38TokenTerminalResult)==64
 && __builtin_offsetof(PioneerQ38TokenTerminalResult,faultError)==56
 && __builtin_offsetof(PioneerQ38TokenTerminalResult,faultSavedLink)==60,"explicit failure DATA offsets");
