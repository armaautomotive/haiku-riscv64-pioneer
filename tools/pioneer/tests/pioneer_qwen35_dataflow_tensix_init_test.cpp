/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowTensixInit.h"
#include <cassert>
#include <cstdint>

struct MetadataSelector {
	unsigned mode=3,writes=0;
	void Config(unsigned index,uint32_t value) {
		assert(index==73); mode=value; writes++;
	}
};
struct ReadbackSelector {
	unsigned mode=0x10000,writes=0;
	void Config(unsigned index,uint32_t value) {
		assert(index==3); mode=value; writes++;
	}
};

int main()
{
	const unsigned indices[]={0,1,2,3,5,7,11,12,13,14,15,16,17,18,19,
		28,29,30,31,32,33,34,35,37,38,39,40,41,47,48,49,50,51,52,53,54};
	for (unsigned index=0;index<80;index++) {
		bool present=false;
		for (unsigned expected:indices) present|=index==expected;
		assert(PioneerQwen35::DataflowTensixThreadConfigPresent(index)==present);
	}
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
	MetadataSelector selector;
	PioneerQwen35::DataflowTensixPrepareAffineMetadata(selector,false);
	assert(selector.mode==3 && selector.writes==0);
	PioneerQwen35::DataflowTensixPrepareAffineMetadata(selector,true);
	assert(selector.mode==0x13 && selector.writes==1);
	ReadbackSelector readback;
	PioneerQwen35::DataflowTensixSelectReadback(readback,true);
	assert(readback.mode==0 && readback.writes==1);
	PioneerQwen35::DataflowTensixSelectReadback(readback,false);
	assert(readback.mode==0x10000 && readback.writes==2);
}
