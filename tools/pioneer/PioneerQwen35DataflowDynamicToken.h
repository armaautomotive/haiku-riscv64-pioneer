/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_DYNAMIC_TOKEN_H
#define PIONEER_QWEN35_DATAFLOW_DYNAMIC_TOKEN_H

#include "PioneerQwen35DataflowDeviceStream.h"

namespace PioneerQwen35 {

// The kernel must bind this context to the validated live model and keep it
// immutable during a run. Valid() establishes bounds, not model ownership.
struct DataflowDynamicTokenContext {
	uint32_t token=0,position=0,context=0;
	RowSpace tied{};
	bool Valid() const {
		return token<Vocabulary && context && context<=MaxPositions && position<context
			&& tied.Valid() && tied.rows==Vocabulary && tied.rowBytes==2120 && tied.unit==16;
	}
	static bool Build(const DeviceLayout& manifest,unsigned token,unsigned position,
		DataflowDynamicTokenContext& out) {
		if (manifest.magic!=LayoutMagic || manifest.version!=LayoutVersion || manifest.reserved
			|| manifest.tied.type!=14 || manifest.tied.inputs!=Hidden) return false;
		DataflowDynamicTokenContext candidate{};
		candidate.token=token;candidate.position=position;candidate.context=manifest.context;
		candidate.tied=manifest.tied.place;
		if (!candidate.Valid()) return false;
		out=candidate;return true;
	}
};

inline bool DataflowDynamicTokenMatches(const DataflowDynamicTokenContext& a,
	const DataflowDynamicTokenContext& b) {
	return a.Valid() && b.Valid() && a.token==b.token && a.position==b.position
		&& a.context==b.context && DataflowRowSpaceMatches(a.tied,b.tied);
}

template<class Invocation>
inline void DataflowDynamicRead(const DataflowStreamRecord& record,Invocation& invocation) {
	const auto* input=reinterpret_cast<const uint8_t*>(record.payload);
	auto* output=reinterpret_cast<uint8_t*>(&invocation);
	for (unsigned i=0;i<sizeof(invocation);i++) output[i]=input[i];
}
template<class Invocation>
inline void DataflowDynamicWrite(DataflowStreamRecord& record,const Invocation& invocation) {
	const auto* input=reinterpret_cast<const uint8_t*>(&invocation);
	auto* output=reinterpret_cast<uint8_t*>(record.payload);
	for (unsigned i=0;i<sizeof(invocation);i++) output[i]=input[i];
}

// The unchecked inner patch is private: only Checked() and Fetch() can call it.
// Checked() validates the template itself; Fetch() uses the validation already
// performed by DataflowStreamFetchTransform before it invokes the callback.
// The latter still validates the patched result before publishing a command.
class DataflowDynamicPatcher {
	__attribute__((noinline)) static bool PatchValidated(DataflowStreamRecord& record,
		const DataflowDynamicTokenContext& run)
	{
		if (!run.Valid()) return false;
		switch (record.header[5]) {
		case DataflowEmbeddingMagic: {
			DataflowEmbeddingInvocation invocation{};DataflowDynamicRead(record,invocation);
			Range embedding{};
			if (!run.tied.Resolve(run.token&~15U,16,0,run.tied.rowBytes*16,embedding)
				|| embedding.bytes!=DataflowEmbeddingTileCount*DataflowQ6KEmbeddingTileBytes) return false;
			invocation.token=run.token;invocation.lane=run.token&15U;
			invocation.weightBank=embedding.bank;invocation.weightBase=embedding.address;
			if (!invocation.Valid()) return false;
			DataflowDynamicWrite(record,invocation);break;
		}
		case DataflowConvMagic: {
			DataflowConvInvocation invocation{};DataflowDynamicRead(record,invocation);
			if ((invocation.position^run.position)&1U) {
				const RowSpace old=invocation.historyRead;
				invocation.historyRead=invocation.historyWrite;invocation.historyWrite=old;
			}
			invocation.position=run.position;
			if (!invocation.Valid()) return false;
			DataflowDynamicWrite(record,invocation);break;
		}
		case DataflowDeltaMagic: {
			DataflowDeltaInvocation invocation{};DataflowDynamicRead(record,invocation);
			if ((invocation.program.position^run.position)&1U) {
				const RowSpace old=invocation.program.stateRead;
				invocation.program.stateRead=invocation.program.stateWrite;invocation.program.stateWrite=old;
			}
			invocation.program.position=run.position;
			if (!invocation.Valid()) return false;
			DataflowDynamicWrite(record,invocation);break;
		}
		case DataflowDenseAttentionMagic: {
			DataflowDenseAttentionInvocation invocation{};DataflowDynamicRead(record,invocation);
			if (invocation.program.context!=run.context) return false;
			invocation.program.position=run.position;
			if (!invocation.Valid()) return false;
			DataflowDynamicWrite(record,invocation);break;
		}
		default:break;
		}
		record.header[7]=DataflowStreamChecksum(record.payload,record.header[6]/4);
		return true;
	}
public:
	static bool Checked(DataflowStreamRecord& record,const DataflowDynamicTokenContext& run,
		unsigned templateEpoch,unsigned step,unsigned core) {
		return run.Valid() && DataflowStreamRecordStructureValid(record,templateEpoch,step,core)
			&& PatchValidated(record,run);
	}
	template<class IO> static bool Fetch(IO& io,const DataflowStreamBinding& binding,const Range& lease,
		const unsigned (&routes)[2][Banks],unsigned core,unsigned index,
		DataflowStreamRecord& staging,uint32_t* program,unsigned programBytes,
		const DataflowDynamicTokenContext& run,unsigned* failedStage,unsigned readBytes) {
		auto patch=[&](DataflowStreamRecord& r) { return PatchValidated(r,run); };
		return DataflowStreamFetchTransform(io,binding,lease,routes,core,index,
			staging,program,programBytes,patch,failedStage,readBytes);
	}
};

// The token, position and tied rows must come from the trusted run binding.
// A direct caller still validates the entire record before patching it.
inline bool DataflowDynamicPatch(DataflowStreamRecord& record,
	const DataflowDynamicTokenContext& run,unsigned templateEpoch,unsigned step,unsigned core)
{
	return DataflowDynamicPatcher::Checked(record,run,templateEpoch,step,core);
}

template<class IO>
bool DataflowStreamFetchDynamic(IO& io,const DataflowStreamBinding& binding,const Range& lease,
	const unsigned (&routes)[2][Banks],unsigned core,unsigned index,
	DataflowStreamRecord& staging,uint32_t* program,unsigned programBytes,
	const DataflowDynamicTokenContext& run,unsigned* failedStage=nullptr,
	unsigned readBytes=DataflowStreamRecordBytes)
{
	return DataflowDynamicPatcher::Fetch(io,binding,lease,routes,core,index,
		staging,program,programBytes,run,failedStage,readBytes);
}

inline bool DataflowBlackholeStreamFetchDynamic(const DataflowStreamBinding& binding,
	const Range& lease,const unsigned (&routes)[2][Banks],unsigned core,unsigned index,
	const DataflowDynamicTokenContext& run,unsigned* failedStage=nullptr,
	DataflowNocDiagnostic* diagnostic=nullptr,unsigned readBytes=DataflowStreamRecordBytes)
{
	DataflowBlackholeStreamIO io(diagnostic);
	auto* staging=reinterpret_cast<DataflowStreamRecord*>(static_cast<uintptr_t>(DataflowStreamStagingAddress));
	auto* program=reinterpret_cast<uint32_t*>(static_cast<uintptr_t>(DataflowProgramAddress));
	return DataflowStreamFetchDynamic(io,binding,lease,routes,core,index,*staging,program,DataflowProgramRegionBytes,run,failedStage,readBytes);
}
}
#endif
