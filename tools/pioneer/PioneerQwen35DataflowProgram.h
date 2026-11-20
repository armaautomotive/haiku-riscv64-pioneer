/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_PROGRAM_H
#define PIONEER_QWEN35_DATAFLOW_PROGRAM_H

#include "PioneerQwen35DeviceLayout.h"
#include "PioneerQwen35ProgramSchedule.h"

namespace PioneerQwen35 {

enum : unsigned {
	DataflowProgramMagic=0x31504635,
	DataflowProgramVersion=1,
	DataflowCoreCount=120,
	DataflowMaxSegments=13,
	DataflowCircularBufferCount=3,
	DataflowCircularBufferPages=2,
	DataflowReaderRole=1U<<0,
	DataflowComputeRole=1U<<1,
	DataflowWriterRole=1U<<2,
	DataflowAllRoles=DataflowReaderRole|DataflowComputeRole|DataflowWriterRole,
	DataflowWeightBufferAddress=0x90000,
	DataflowInputBufferAddress=0x92000,
	DataflowOutputBufferAddress=0x92800,
	DataflowActivationCacheAddress=0x93000,
	DataflowActivationCacheBytes=9216*sizeof(float),
	DataflowQueueStateAddress=0x9c000,
	DataflowQueueStateBytes=3*16,
	DataflowActivationChunkInputs=256,
	DataflowInputPageBytes=576,
	DataflowOutputPageBytes=64,
	DataflowLocalBufferEnd=0xa0000
};
static_assert((PioneerQwen35Projection::MaxInputs/DataflowActivationChunkInputs)
	*DataflowInputPageBytes<=DataflowActivationCacheBytes,
	"packed projection activations fit in the local activation cache");

struct DataflowCircularBuffer {
	unsigned id,address,pageBytes,pages,bytes;
};

struct DataflowWeightSegment {
	unsigned bank,address,firstGroup,groups,bytes;
};

struct DataflowOutputSegment {
	unsigned bank,address,firstRow,rows,bytes;
};

struct DataflowCoreProgram {
	unsigned active,segmentCount,groups;
	DataflowWeightSegment weight[DataflowMaxSegments];
	DataflowOutputSegment output[DataflowMaxSegments];
};

// Host-side program metadata only. Numeric work belongs to the reader,
// compute, and writer device roles; no callback here performs inference.
struct DataflowProjectionProgram {
	unsigned magic,version,type,inputs,outputRows,groups,blocks,activeCores,kernelRoles;
	unsigned inputRows,inputFragmentCount,outputFragmentCount;
	DataflowCircularBuffer circularBuffer[DataflowCircularBufferCount];
	RowSpace weights,input,output;
	DataflowCoreProgram core[DataflowCoreCount];

	bool Valid() const
	{
		if (magic!=DataflowProgramMagic || version!=DataflowProgramVersion
			|| kernelRoles!=DataflowAllRoles || !PioneerQwen35Projection::Supports(type,inputs)
			|| !outputRows || (outputRows&15) || groups!=outputRows/16
			|| activeCores!=DataflowCoreCount || inputRows!=inputs
			|| !inputFragmentCount || inputFragmentCount>Banks
			|| !outputFragmentCount || outputFragmentCount>Banks
			|| !input.Valid() || !output.Valid() || input.rows<inputRows
			|| input.rowBytes!=sizeof(float) || input.unit!=16
			|| output.rows<outputRows || output.rowBytes!=sizeof(float)
			|| output.unit!=16) return false;
		const unsigned tileBytes=PioneerQwen35Projection::TileBytes(type);
		if (!tileBytes || !blocks || !circularBuffer[0].bytes
			|| !circularBuffer[1].bytes || !circularBuffer[2].bytes) return false;
		if (!weights.Valid() || weights.rows!=outputRows || weights.unit!=16
			|| weights.rowBytes*16!=tileBytes*blocks) return false;
		if (DataflowActivationCacheAddress<DataflowWeightBufferAddress
			|| DataflowActivationCacheAddress>=DataflowLocalBufferEnd
			|| DataflowActivationCacheBytes>DataflowLocalBufferEnd-DataflowActivationCacheAddress
			|| DataflowQueueStateAddress<DataflowWeightBufferAddress
			|| DataflowQueueStateAddress>=DataflowLocalBufferEnd
			|| DataflowQueueStateBytes>DataflowLocalBufferEnd-DataflowQueueStateAddress)
			return false;
		for (unsigned c=0;c<DataflowCircularBufferCount;c++) {
			const auto& buffer=circularBuffer[c];
			if (buffer.id!=(c==2?16U:c) || !buffer.address || (buffer.address&63)
				|| !buffer.pageBytes || buffer.pages!=DataflowCircularBufferPages
				|| buffer.bytes!=buffer.pageBytes*buffer.pages
				|| buffer.address<DataflowWeightBufferAddress
				|| buffer.address>=DataflowLocalBufferEnd
				|| buffer.bytes>DataflowLocalBufferEnd-buffer.address) return false;
			for (unsigned other=0;other<c;other++) {
				const auto& prior=circularBuffer[other];
				if (buffer.address<uint64_t(prior.address)+prior.bytes
					&& prior.address<uint64_t(buffer.address)+buffer.bytes) return false;
			}
			if (buffer.address<DataflowActivationCacheAddress+DataflowActivationCacheBytes
				&& DataflowActivationCacheAddress<uint64_t(buffer.address)+buffer.bytes)
				return false;
			if (buffer.address<DataflowQueueStateAddress+DataflowQueueStateBytes
				&& DataflowQueueStateAddress<uint64_t(buffer.address)+buffer.bytes)
				return false;
		}
		if (circularBuffer[0].pageBytes!=tileBytes
			|| circularBuffer[1].pageBytes!=DataflowInputPageBytes
			|| circularBuffer[2].pageBytes!=DataflowOutputPageBytes) return false;
		unsigned active=0,totalGroups=0,nextGroup=0;
		for (unsigned c=0;c<DataflowCoreCount;c++) {
			const auto& assignment=core[c];
			if (!assignment.active || !assignment.groups || !assignment.segmentCount
				|| assignment.segmentCount>DataflowMaxSegments) return false;
			active++;
			unsigned coreGroups=0;
			for (unsigned s=0;s<assignment.segmentCount;s++) {
				const auto& weight=assignment.weight[s];
				const auto& outputPart=assignment.output[s];
				if (weight.bank>=Banks || outputPart.bank>=Banks || !weight.address
					|| !weight.groups || weight.firstGroup!=nextGroup || weight.firstGroup>groups
					|| weight.groups>groups-weight.firstGroup
					|| weight.bytes!=weight.groups*16*weights.rowBytes
					|| outputPart.firstRow!=weight.firstGroup*16
					|| outputPart.rows!=weight.groups*16
					|| outputPart.bytes!=outputPart.rows*sizeof(float)) return false;
				Range expectedWeight{},expectedOutput{};
				if (!weights.Resolve(outputPart.firstRow,outputPart.rows,0,weight.bytes,expectedWeight)
					|| expectedWeight.bank!=weight.bank || expectedWeight.address!=weight.address
					|| expectedWeight.bytes!=weight.bytes
					|| !output.Resolve(outputPart.firstRow,outputPart.rows,0,outputPart.bytes,expectedOutput)
					|| expectedOutput.bank!=outputPart.bank || expectedOutput.address!=outputPart.address
					|| expectedOutput.bytes!=outputPart.bytes) return false;
				coreGroups+=weight.groups;
				nextGroup+=weight.groups;
			}
			if (coreGroups!=assignment.groups || coreGroups>groups-totalGroups) return false;
			totalGroups+=coreGroups;
		}
		return active==DataflowCoreCount && totalGroups==groups;
	}
};

class DataflowProgramBuilder {
	static bool Add(unsigned& total,unsigned value)
	{
		if (value>0xffffffffU-total) return false;
		total+=value;
		return true;
	}

public:
	static bool Build(const TensorSpace& weight,const RowSpace& input,
		unsigned inputRows,const RowSpace& output,DataflowProjectionProgram& result)
	{
		result={};
		if (!PioneerQwen35Projection::Supports(weight.type,weight.inputs)
			|| inputRows!=weight.inputs || !input.Valid() || !output.Valid()
			|| input.rows<inputRows || input.rowBytes!=sizeof(float) || input.unit!=16
			|| output.rows<weight.place.rows || output.rowBytes!=sizeof(float)
			|| output.unit!=16 || !weight.place.Valid() || weight.place.unit!=16
			|| !weight.place.rows || (weight.place.rows&15)) return false;
		const unsigned tileBytes=PioneerQwen35Projection::TileBytes(weight.type);
		const unsigned blocks=weight.inputs>>(weight.type==8?5:8);
		if (!tileBytes || !blocks || weight.place.rowBytes*16!=tileBytes*blocks) return false;

		PioneerQ35WorkFragment fragments[Banks]={};
		for (unsigned f=0;f<weight.place.count;f++) {
			const auto& part=weight.place.parts[f];
			if ((part.firstRow|part.rows)&15) return false;
			fragments[f]={part.firstRow/16,part.rows/16};
		}
		PioneerQ35ProjectionProgram schedule{};
		if (!PioneerQ35BuildProjectionProgram(fragments,weight.place.count,schedule)
			|| schedule.activeCores!=DataflowCoreCount) return false;

		DataflowProjectionProgram candidate{};
		candidate.magic=DataflowProgramMagic;
		candidate.version=DataflowProgramVersion;
		candidate.type=weight.type;
		candidate.inputs=weight.inputs;
		candidate.outputRows=weight.place.rows;
		candidate.groups=schedule.groups;
		candidate.blocks=blocks;
		candidate.activeCores=schedule.activeCores;
		candidate.kernelRoles=DataflowAllRoles;
		candidate.inputRows=inputRows;
		candidate.inputFragmentCount=input.count;
		candidate.outputFragmentCount=output.count;
		candidate.weights=weight.place;
		candidate.input=input;
		candidate.output=output;
		candidate.circularBuffer[0]={0,DataflowWeightBufferAddress,tileBytes,
			DataflowCircularBufferPages,tileBytes*DataflowCircularBufferPages};
		candidate.circularBuffer[1]={1,DataflowInputBufferAddress,DataflowInputPageBytes,
			DataflowCircularBufferPages,DataflowInputPageBytes*DataflowCircularBufferPages};
		candidate.circularBuffer[2]={16,DataflowOutputBufferAddress,DataflowOutputPageBytes,
			DataflowCircularBufferPages,DataflowOutputPageBytes*DataflowCircularBufferPages};
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			const auto& scheduled=schedule.core[core];
			auto& assignment=candidate.core[core];
			assignment.active=scheduled.groups!=0;
			assignment.groups=scheduled.groups;
			if (!assignment.active || !scheduled.segmentCount
				|| scheduled.segmentCount>PioneerQ35ProgramMaxFragments) return false;
			for (unsigned s=0;s<scheduled.segmentCount;s++) {
				const auto& segment=scheduled.segment[s];
				if (segment.fragment>=weight.place.count) return false;
				const auto& part=weight.place.parts[segment.fragment];
				if (segment.firstGroup<part.firstRow/16
					|| segment.groups>part.rows/16-(segment.firstGroup-part.firstRow/16))
					return false;
				unsigned firstGroup=segment.firstGroup,remaining=segment.groups;
				while (remaining) {
					unsigned outputPartIndex=output.count;
					for (unsigned p=0;p<output.count;p++) {
						const auto& target=output.parts[p];
						if (firstGroup*16>=target.firstRow
							&& firstGroup*16<target.firstRow+target.rows) {
							outputPartIndex=p; break;
						}
					}
					if (outputPartIndex==output.count
						|| assignment.segmentCount>=DataflowMaxSegments) return false;
					const auto& target=output.parts[outputPartIndex];
					const unsigned available=(target.firstRow+target.rows-firstGroup*16)/16;
					const unsigned segmentGroups=remaining<available?remaining:available;
					const unsigned firstRow=firstGroup*16,rows=segmentGroups*16;
					const uint64_t source=uint64_t(part.address)
						+uint64_t(firstRow-part.firstRow)*weight.place.rowBytes;
					const unsigned bytes=rows*weight.place.rowBytes;
					Range destination{};
					if (!segmentGroups || source>UINT32_MAX || bytes>BankLimit-source
						|| !output.Resolve(firstRow,rows,0,rows*sizeof(float),destination)
						|| destination.bank!=target.bank || destination.bank>=Banks
						|| !destination.bytes) return false;
					const unsigned outputSegment=assignment.segmentCount++;
					assignment.weight[outputSegment]={part.bank,unsigned(source),firstGroup,
						segmentGroups,bytes};
					assignment.output[outputSegment]={destination.bank,destination.address,
						firstRow,rows,destination.bytes};
					firstGroup+=segmentGroups;
					remaining-=segmentGroups;
				}
			}
		}
		if (!candidate.Valid()) return false;
		result=candidate;
		return true;
	}
};

} // namespace PioneerQwen35

#endif
