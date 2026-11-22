/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_NOC1_AUDIT_H
#define PIONEER_QWEN35_DATAFLOW_NOC1_AUDIT_H
#include "PioneerComputeFleet.h"
#include "../../../../../tools/pioneer/PioneerQwen35DataflowNocProtocol.h"

// Read-only audit of the selected worker's NoC1. The baseline is the already
// verified NoC0 geometry; the two physical NoC coordinate systems run in
// opposite directions on the 17x12 Blackhole grid. Refuse the specialized DDR
// translation modes instead of guessing their effects.
template<class IO>
bool PioneerQwen35DataflowNoC1Audit(IO& io,const tt_dram_samples_result& dram,
	unsigned core)
{
	if (core>=TT_PIONEER_WORKERS) return false;
	uint32_t first[19]={},second[19]={};
	auto sample=[&](uint32_t* out) {
		out[0]=io.Read(0x44);out[1]=io.Read(0x48);out[2]=io.Read(0x148);
		for (unsigned i=0;i<TT_DRAM_SOURCE_WORDS;i++) out[3+i]=io.Read(BlackholeDramSourceOffset(i));
	};
	sample(first);io.Pause();sample(second);
	for (unsigned i=0;i<19;i++) if (first[i]!=second[i]) return false;
	const uint32_t* source=first+3;
	if ((source[0]&0x15000)!=0x4000 || (source[15]&0xc00)
		|| ((first[0]>>12)&127)!=17 || ((first[0]>>19)&127)!=12
		|| (first[1]&0xffffff00)!=0x01010000) return false;
	uint32_t local=0;
	if (!PioneerQwen35::DataflowNocDecodeLocalRoute(first[2],local)
		|| !PioneerQwen35::DataflowNocRouteMatchesWorker(local,core)) return false;
	const unsigned x=PioneerWorkerX(core),y=PioneerWorkerY(core);
	auto matches=[&](unsigned logicalX,unsigned logicalY,unsigned physicalX,unsigned physicalY) {
		return logicalX<32 && logicalY<32 && physicalX<17 && physicalY<12
			&& !(source[14]&(1U<<logicalY)) && !(source[13]&(1U<<logicalX))
			&& BlackholeDramSourceX(source,logicalX)==16-physicalX
			&& BlackholeDramSourceY(source,logicalY)==11-physicalY;
	};
	const unsigned px=BlackholeDramSourceX(dram.source,x),py=BlackholeDramSourceY(dram.source,y);
	if (!matches(x,y,px,py) || (first[0]&0xfff)!=((16-px)|((11-py)<<6))) return false;
	for (unsigned bank=0;bank<TT_DRAM_BANKS;bank++)
		if (!matches(dram.translated[bank][0],dram.translated[bank][1],
			BlackholeDramPhysicalX(bank),BlackholeDramPhysicalY(bank))) return false;
	// Every worker can now forward a release to fixed child workers. Verify
	// all possible logical destinations at each sender, not just its own
	// coordinate and the bank routes, before any peer traffic is admitted.
	// The NIU translates X and Y independently; cover each used column and
	// row once instead of rechecking all 120 Cartesian combinations.
	for (unsigned column=0;column<12;column++) {
		const unsigned wx=PioneerWorkerX(column*10),wy=PioneerWorkerY(column*10);
		if (!matches(wx,wy,BlackholeDramSourceX(dram.source,wx),
				BlackholeDramSourceY(dram.source,wy))) return false;
	}
	for (unsigned row=1;row<10;row++) {
		const unsigned wx=PioneerWorkerX(row),wy=PioneerWorkerY(row);
		if (!matches(wx,wy,BlackholeDramSourceX(dram.source,wx),
				BlackholeDramSourceY(dram.source,wy))) return false;
	}
	for (unsigned i=0;i<4;i++) if (io.Read(0x40+i*0x800)!=0) return false;
	for (unsigned i=0;i<32;i++) if (io.Read(0x240+i*4)&255) return false;
	return true;
}
#endif
