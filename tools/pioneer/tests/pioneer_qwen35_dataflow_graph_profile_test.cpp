/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowGraphProfile.h"
#include <cassert>
#include <cstdint>
using namespace PioneerQwen35;

int main()
{
	assert(DataflowGraphProfileOperation(DataflowProgramMagic)==DataflowProfileProjection);
	assert(DataflowGraphProfileOperation(DataflowDeltaMagic)==DataflowProfileDelta);
	assert(DataflowGraphProfileOperation(0)==DataflowProfileOther);
	DataflowGraphProfile profile;
	profile.Add(DataflowProfileProjection,10,50);
	profile.Add(DataflowProfileProjection,90,115);
	profile.Add(DataflowProfileDelta,120,140);
	profile.Add(DataflowProfileFetch,150,155);
	profile.Add(DataflowProfileAuthorize,160,170);
	assert(profile.Finish(0,200));
	assert(profile.Cycles(DataflowProfileProjection)==65);
	assert(profile.Cycles(DataflowProfileDelta)==20);
	assert(profile.Cycles(DataflowProfileFetch)==5);
	assert(profile.Cycles(DataflowProfileAuthorize)==10);
	assert(profile.Cycles(DataflowProfileOther)==100);
	assert(profile.Cycles(DataflowProfileTotal)==200);
	uint32_t memory[DataflowProfileBins*4+2]{};
	memory[0]=0xdeadbeef;memory[DataflowProfileBins*4+1]=0xdeadbeef;
	profile.Publish(memory+1,1000);
	assert(memory[0]==0xdeadbeef && memory[DataflowProfileBins*4+1]==0xdeadbeef);
	for (unsigned bin=0;bin<DataflowProfileBins;bin++) {
		const uint32_t* r=memory+1+bin*4;
		assert(r[0]==1000 && r[1]==uint32_t(profile.Cycles(bin))
			&& r[2]==uint32_t(profile.Cycles(bin)>>32)
			&& r[3]==unsigned(profile.Cycles(bin)!=0));
		DataflowGraphTraceRecord record{r[0],r[1],r[2],r[3]};
		assert(DataflowGraphTraceValid(record,DataflowGraphProfileTraceFirst+bin,1000));
	}
	DataflowGraphProfile bad;
	bad.Add(DataflowProfileFetch,10,9);
	assert(!bad.Finish(0,100));
	uint32_t untouched[DataflowProfileBins*4]{};
	bad.Publish(untouched,1000);assert(!untouched[0]);
	DataflowGraphProfile inconsistent;
	inconsistent.Add(DataflowProfileFetch,10,100);
	assert(!inconsistent.Finish(0,50));
}
