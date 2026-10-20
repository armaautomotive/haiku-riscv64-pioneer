/* SPDX-License-Identifier: MIT */
// Compile-only adapter contract:deliberately NO root/reset/NoC/PCI/data-write
// APIs. This object has no transport implementations and is NOT a driver.
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38NonRootHeld.h"
struct NonRootCompileIO {
	bool NonRootHeldOwner(); bool NonRootHeldMappingStable();
	uint32_t SourceRead(unsigned); uint32_t ResetRead();
	void DramTlbWrite(unsigned,unsigned,uint32_t); uint32_t DramTlbRead(unsigned,unsigned);
};
extern "C" bool pioneer_q38_non_root_held_compile(NonRootCompileIO& io,const tt_dram_samples_result& proof) {
	return PioneerQ38NonRootHeld(io,proof);
}
