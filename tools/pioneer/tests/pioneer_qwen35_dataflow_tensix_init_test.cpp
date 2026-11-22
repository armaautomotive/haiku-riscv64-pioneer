/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowTensixInit.h"
#include <cassert>
#include <cstdint>

int main()
{
	uint32_t sourceA[6], sourceB[6], tile[66];
	for (auto& value:sourceA) value=0xdeadbeef;
	for (auto& value:sourceB) value=0xdeadbeef;
	for (auto& value:tile) value=0xdeadbeef;
	PioneerQwen35::DataflowTensixInitializeSourceWindows(sourceA+1,sourceB+1,tile+1);
	assert(sourceA[0]==0xdeadbeef && sourceA[5]==0xdeadbeef);
	assert(sourceB[0]==0xdeadbeef && sourceB[5]==0xdeadbeef);
	assert(tile[0]==0xdeadbeef && tile[65]==0xdeadbeef);
	for (unsigned i=1;i<5;i++) assert(!sourceA[i] && !sourceB[i]);
	for (unsigned i=1;i<65;i++) assert(!tile[i]);
}
