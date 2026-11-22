/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowGraphTrace.h"
#include <cassert>
using namespace PioneerQwen35;
struct TraceIO {
	DataflowGraphTraceRecord record{};
	unsigned reads=0,expectedAddress=0;
	bool change=false,fail=false;
	bool ReadCoreMemory(unsigned core,unsigned address,void* destination,unsigned bytes) {
		assert(core==0 && address==expectedAddress && bytes==sizeof(record));
		if (fail) return false;
		auto out=record;if (change && reads) out.first^=1;
		*static_cast<DataflowGraphTraceRecord*>(destination)=out;reads++;return true;
	}
};
int main() {
	uint32_t page[16]{};page[0]=0x80000000U;page[7]=0x3f800000U;page[15]=0xbf000000U;
	const unsigned lastSequence=1000+DataflowGraphDecodeSteps-1;
	for (unsigned slot=0;slot<DataflowGraphTraceSlots;slot++) {
		const unsigned sequence=1000+DataflowGraphTraceStep(slot);
		const auto sample=DataflowGraphTraceSample(sequence,page);
		assert(sample.sequence==sequence && sample.first==page[0] && sample.last==page[15]
			&& sample.magnitudeOr==(0x3f800000U|0x3f000000U));
		assert(DataflowGraphTraceValid(sample,slot,lastSequence));
		assert(!DataflowGraphTraceValid(sample,slot,lastSequence+1));
	}
	assert(!DataflowGraphTraceValid({},DataflowGraphTraceSlots,lastSequence));
	assert(!DataflowGraphTraceSample(0,page).sequence);
	assert(!DataflowGraphTraceSample(1,nullptr).sequence);
	assert(DataflowGraphTraceStep(8)==3 && DataflowGraphTraceStep(14)==3);
	uint32_t pending[4]{};
	DataflowGraphTracePublishPending(pending,1003);assert(!pending[0]);
	DataflowGraphTracePendingSample(pending,page);
	assert(pending[0]==DataflowGraphTracePending && pending[1]==page[0] && pending[2]==page[15]);
	DataflowGraphTracePublishPending(pending,1003);assert(pending[0]==1003);
	DataflowGraphTracePublishPending(pending,1004);assert(pending[0]==1003);
	for (auto& word:page) word=0x80000000U;
	assert(!DataflowGraphTraceSample(1,page).magnitudeOr);
	DataflowGraphDiagnosticResult cache;DataflowArgmaxResult output{};
	assert(!cache.Read(output));
	DataflowArgmaxResult result{42,0x3f800000U,lastSequence,0};
	result.checksum=DataflowArgmaxResultChecksum(result.token,result.scoreBits,result.sequence);
	assert(cache.Set(result) && cache.Read(output) && output.token==42
		&& output.scoreBits==result.scoreBits && output.sequence==lastSequence);
	TraceIO io;io.expectedAddress=DataflowGraphTraceAddress+2*sizeof(DataflowGraphTraceRecord);
	io.record=DataflowGraphTraceSample(1000+DataflowGraphTraceStep(2),page);
	DataflowGraphTraceRecord traced{};
	assert(DataflowGraphTraceRead(io,2,result,traced) && io.reads==2
		&& traced.sequence==io.record.sequence);
	io.reads=0;io.change=true;assert(!DataflowGraphTraceRead(io,2,result,traced));
	io.change=false;io.reads=0;io.record.sequence++;assert(!DataflowGraphTraceRead(io,2,result,traced));
	io.reads=0;io.fail=true;assert(!DataflowGraphTraceRead(io,2,result,traced));
	io.fail=false;io.reads=0;assert(!DataflowGraphTraceRead(io,DataflowGraphTraceSlots,result,traced) && !io.reads);
	result.checksum^=1;assert(!cache.Set(result) && !cache.Read(output));
	result.checksum^=1;assert(cache.Set(result));cache.Reset();assert(!cache.Read(output));
}
