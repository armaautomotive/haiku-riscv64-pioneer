/* SPDX-License-Identifier: MIT */
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#include "../PioneerQwen35DataflowGraphGeneration.h"
#include <cassert>
#include <cstdio>

using namespace PioneerQwen35;
struct Timer {
	uint64_t value=0;
	uint64_t NowMicros() { const auto now=value;value+=10;return now; }
};
struct Owner {
	struct Call { unsigned position,token;bool decode; } calls[16]{};
	unsigned count=0,nextToken=10,failAt=~0U,eosAt=~0U;bool corrupt=false;
	DataflowGraphSessionResult Run(unsigned position,unsigned token,bool decode,unsigned polls,
		DataflowArgmaxResult& result,unsigned& stage) {
		assert(polls==77 && count<16);calls[count]={position,token,decode};
		const unsigned current=count++;stage=0;
		if (current==failAt) return DataflowGraphSessionRetained;
		if (decode) {
			result={current==eosAt?99U:nextToken++,0x3f800000,current+1,0};
			result.checksum=DataflowArgmaxResultChecksum(result.token,result.scoreBits,result.sequence);
			if (corrupt) result.checksum^=1;
		} else result={Vocabulary,0,0,0};
		return DataflowGraphSessionComplete;
	}
};
int main() {
	const unsigned prompt[]={1,2,3};unsigned output[8]{},count=0,stage=0;
	uint64_t promptUs=0,decodeUs=0;Timer clock;Owner owner;
	assert(DataflowGraphGeneration::Run(prompt,3,3,99,32,77,owner,output,count,stage,clock,promptUs,decodeUs)
		==DataflowGenerationSequenceComplete && count==3 && stage==0 && promptUs==10 && decodeUs==10);
	assert(owner.count==5 && owner.calls[0].position==0 && owner.calls[0].token==1 && !owner.calls[0].decode
		&& owner.calls[1].position==1 && owner.calls[1].token==2 && !owner.calls[1].decode
		&& owner.calls[2].position==2 && owner.calls[2].token==3 && owner.calls[2].decode
		&& owner.calls[3].position==3 && owner.calls[3].token==10 && owner.calls[3].decode
		&& owner.calls[4].position==4 && owner.calls[4].token==11 && owner.calls[4].decode
		&& output[0]==10 && output[1]==11 && output[2]==12);
	Owner eos;Timer eosClock;eos.eosAt=2;
	assert(DataflowGraphGeneration::Run(prompt,3,3,99,32,77,eos,output,count,stage,eosClock,promptUs,decodeUs)
		==DataflowGenerationSequenceComplete && count==1 && eos.count==3 && output[0]==99 && !decodeUs);
	Owner failed;Timer failClock;failed.failAt=3;
	assert(DataflowGraphGeneration::Run(prompt,3,3,99,32,77,failed,output,count,stage,failClock,promptUs,decodeUs)
		==DataflowGenerationSequenceDecoderFailed && count==1 && stage==(3U<<16|0x8002U)
		&& promptUs==10 && decodeUs==10);
	Owner badOutput;Timer badClock;badOutput.corrupt=true;
	assert(DataflowGraphGeneration::Run(prompt,3,3,99,32,77,badOutput,output,count,stage,badClock,promptUs,decodeUs)
		==DataflowGenerationSequenceDecoderFailed && !count && stage==(2U<<16|0x8003U));
	Owner invalid;Timer invalidClock;
	assert(DataflowGraphGeneration::Run(prompt,3,3,99,3,77,invalid,output,count,stage,invalidClock,promptUs,decodeUs)
		==DataflowGenerationSequenceInvalidRequest && !invalid.count && !count && !promptUs && !decodeUs);
	puts("PASS graph generation: prompt/decode order, timings, EOS and failure propagation; SIMULATED ONLY");
	return 0;
}
