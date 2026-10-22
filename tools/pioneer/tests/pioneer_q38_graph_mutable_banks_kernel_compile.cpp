/* SPDX-License-Identifier: MIT */
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38GraphMutableBanks.h"
extern "C" bool pioneer_q38_graph_mutable_banks_compile(PioneerQ38GraphMutableBanks& heapOwner,
 PioneerResidentBanks& banks,const tt_dram_samples_result& proof,const PioneerQ38GdnIdentity& identity,
 const uint8_t* learned,unsigned pages,const PioneerQ38GdnRegion* regions,unsigned count,unsigned bank,uint64_t offset) {
 return heapOwner.Bind(banks,proof,identity,learned,pages,regions,count,bank,offset);
}
