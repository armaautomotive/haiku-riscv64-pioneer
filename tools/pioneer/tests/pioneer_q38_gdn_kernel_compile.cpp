/* SPDX-License-Identifier: MIT */
// RV64 kernel-compatible compile/stack smoke ONLY. Not linked into a driver.
#define PIONEER_Q38_GDN_MUTABLE_STATE
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnBanks.h"
extern "C" bool pioneer_gdn_bind_compile(PioneerQ38GdnBanks& state,PioneerResidentBanks& banks,
 const tt_dram_samples_result& proof,const PioneerQ38GdnIdentity& id,const uint8_t* initialized,unsigned pages,
 const PioneerQ38GdnRegion* regions,unsigned count) {
 return state.Bind(banks,proof,id,initialized,pages,regions,count);
}
extern "C" bool pioneer_gdn_extent_compile(PioneerQ38GdnBanks& state,const PioneerQ38GdnIdentity& id,
 unsigned layer,unsigned slot,unsigned worker,unsigned region,unsigned first,unsigned pages,PioneerQ38GdnBankExtent& output) {
 return state.InitializeHalf(id,layer,slot,worker,output) && state.InitializeSpan(id,region,first,pages,output);
}
extern "C" void pioneer_gdn_fault_compile(PioneerQ38GdnBanks& state) { state.Fault(); }
