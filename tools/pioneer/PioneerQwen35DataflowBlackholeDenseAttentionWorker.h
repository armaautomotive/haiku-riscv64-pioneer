/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_BLACKHOLE_DENSE_ATTENTION_WORKER_H
#define PIONEER_QWEN35_DATAFLOW_BLACKHOLE_DENSE_ATTENTION_WORKER_H

#include "PioneerQwen35DataflowBlackholeIO.h"
#include "PioneerQwen35DataflowDenseAttentionMath.h"
#include "PioneerQwen35DataflowDenseAttentionProgram.h"

namespace PioneerQwen35 {

enum : unsigned {
	DataflowDenseKeyTileAddress=DataflowActivationCacheAddress,
	DataflowDenseValueTileAddress=DataflowDenseKeyTileAddress+16384,
	DataflowDenseQueryAddress=DataflowDenseValueTileAddress+16384,
	DataflowDenseGateAddress=DataflowDenseQueryAddress+DenseHeadWidth*sizeof(float),
	DataflowDenseNormAddress=DataflowDenseGateAddress+DenseHeadWidth*sizeof(float),
	DataflowDenseRopeAddress=DataflowDenseNormAddress+DenseHeadWidth*sizeof(float),
	DataflowDenseAttentionTilePositions=32,
	DataflowDenseAttentionTileBytes=DataflowDenseAttentionTilePositions
		*DataflowDenseAttentionHeadBytes
};

static_assert(DataflowDenseRopeAddress+Rotary*sizeof(float)
	<=DataflowActivationCacheAddress+DataflowActivationCacheBytes,
	"dense attention local tiles fit the reserved activation cache");
static_assert(unsigned(DataflowDenseAttentionTileBytes)
	==unsigned(DataflowNocMaxTransfer),
	"dense KV reads use one bounded NoC transfer per tile");

template<class IO>
bool PioneerQwen35DataflowDenseAttentionWorker(
	const DataflowDenseAttentionInvocation& invocation,
	float* keyValueTile,float* valueTile,float* query,float* gate,
	float* norm,float* rope,float* output,IO& io)
{
	if (!invocation.Valid() || !keyValueTile || !valueTile || !query || !gate
		|| !norm || !rope || !output || !io.Bind(invocation)) return false;
	const DataflowDenseAttentionProgram& program=invocation.program;
	if (!invocation.headCount) return true;
	if (program.mode==DataflowDenseAttentionPackKey) {
		for (unsigned local=0;local<invocation.headCount;local++) {
			const unsigned head=invocation.firstHead+local;
			if (!io.ReadProjectedRows(head*DenseHeadWidth,query)
				|| !io.ReadNorm(false,norm)
				|| !io.ReadRope(rope)
				|| !DataflowDenseNormalize(query,norm,query)
				|| !DataflowDenseRotate(query,rope)
				|| !DataflowDensePackHead(query,
					reinterpret_cast<uint32_t*>(keyValueTile))
				|| !io.WriteKv(head,false,
					reinterpret_cast<const uint32_t*>(keyValueTile))) return false;
		}
		return true;
	}
	if (program.mode==DataflowDenseAttentionPackValue) {
		for (unsigned local=0;local<invocation.headCount;local++) {
			const unsigned head=invocation.firstHead+local;
			if (!io.ReadProjectedRows(head*DenseHeadWidth,query)
				|| !DataflowDensePackHead(query,
					reinterpret_cast<uint32_t*>(keyValueTile))
				|| !io.WriteKv(head,true,
					reinterpret_cast<const uint32_t*>(keyValueTile))) return false;
		}
		return true;
	}
#ifdef PIONEER_QWEN35_DENSE_SHARD_COMPILE_ONLY
	// A shard image executes run records only through the dedicated worker.
	return false;
#else
	for (unsigned local=0;local<invocation.headCount;local++) {
		const unsigned head=invocation.firstHead+local;
		const unsigned queryBase=2*head*DenseHeadWidth;
		if (!io.ReadProjectedRows(queryBase,query)
			|| !io.ReadProjectedRows(queryBase+DenseHeadWidth,gate)
			|| !io.ReadNorm(true,norm)
			|| !io.ReadRope(rope)
			|| !DataflowDenseNormalize(query,norm,query)
			|| !DataflowDenseRotate(query,rope)) return false;
		float* sums=output;
		for (unsigned lane=0;lane<DenseHeadWidth;lane++) sums[lane]=0;
		DataflowDenseAttentionRow row{sums};
		const unsigned kvHead=head/(DenseHeads/DenseKeyHeads);
		for (unsigned first=0;first<=program.position;) {
			unsigned positions=program.position-first+1;
			if (positions>DataflowDenseAttentionTilePositions)
				positions=DataflowDenseAttentionTilePositions;
			if (!io.ReadKvTile(kvHead,false,first,positions,keyValueTile)
				|| !io.ReadKvTile(kvHead,true,first,positions,valueTile)) return false;
			const auto* keys=reinterpret_cast<const uint32_t*>(keyValueTile);
			const auto* values=reinterpret_cast<const uint32_t*>(valueTile);
			for (unsigned index=0;index<positions;index++)
				if (!row.Add(query,keys+index*DenseHeadWidth/2,
					values+index*DenseHeadWidth/2)) return false;
			first+=positions;
		}
		if (!row.Finish(gate)
			|| !io.WriteContext(head*DenseHeadWidth,sums)) return false;
	}
	return true;
#endif
}

class DataflowBlackholeDenseAttentionOps {
	const DataflowDenseAttentionInvocation* fInvocation=nullptr;
#if defined(PIONEER_QWEN35_DENSE_SHARD_COMPILE_ONLY) || defined(PIONEER_QWEN35_FLASH_DECODE_COMPILE_ONLY)
	DataflowDenseAttentionInvocation fShardBinding{};
#endif
	DataflowBlackholeNoc<0> fReader;
	DataflowBlackholeNoc<1> fWriter;

	bool ReadRows(const RowSpace& space,unsigned firstRow,unsigned rows,
		float* output) const
	{
		if (!fInvocation || !output || !space.Valid() || !rows
			|| firstRow>=space.rows || rows>space.rows-firstRow
			|| space.rowBytes!=sizeof(float) || ((firstRow|rows)&15)) return false;
		unsigned completed=0;
		while (completed<rows) {
			const unsigned row=firstRow+completed;
			const Fragment* owner=nullptr;
			for (unsigned index=0;index<space.count;index++) {
				const Fragment& part=space.parts[index];
				if (row>=part.firstRow && row-part.firstRow<part.rows) {
					owner=&part;
					break;
				}
			}
			if (!owner) return false;
			unsigned count=owner->firstRow+owner->rows-row;
			if (count>rows-completed) count=rows-completed;
			count&=~15U;
			if (!count || count*sizeof(float)>DataflowNocMaxTransfer) return false;
			Range range{};
			if (!space.Resolve(row,count,0,count*sizeof(float),range)
				|| !fReader.Read(range.address,
					fInvocation->program.bankRoute[0][range.bank],
					reinterpret_cast<uint8_t*>(output+completed),range.bytes))
				return false;
			completed+=count;
		}
		return completed==rows;
	}

	bool ReadKvRange(unsigned head,bool value,unsigned first,
		unsigned positions,uint8_t* output) const
	{
		if (!fInvocation || !output || head>=DenseKeyHeads || !positions
			|| first>fInvocation->program.position
			|| positions>fInvocation->program.position-first+1
			|| positions>DataflowDenseAttentionTilePositions) return false;
		Range range{};
		const unsigned row=head+(value?DenseKeyHeads:0U);
		const unsigned bytes=positions*DataflowDenseAttentionHeadBytes;
		if (!fInvocation->program.kv.Resolve(row,1,
			first*DataflowDenseAttentionHeadBytes,bytes,range)
			|| range.bytes!=bytes || (range.address&63) || (bytes&63)) return false;
		return fReader.Read(range.address,
			fInvocation->program.bankRoute[0][range.bank],output,bytes);
	}

	bool WriteKvRange(unsigned head,bool value,const uint8_t* input) const
	{
		if (!fInvocation || !input || head>=DenseKeyHeads) return false;
		Range range{};
		const unsigned row=head+(value?DenseKeyHeads:0U);
		if (!fInvocation->program.kv.Resolve(row,1,
			fInvocation->program.position*DataflowDenseAttentionHeadBytes,
			DataflowDenseAttentionHeadBytes,range)
			|| !DataflowDenseAttentionProgram::AlignedRange(range,
				DataflowDenseAttentionHeadBytes)) return false;
		return fWriter.Write(range.address,
			fInvocation->program.bankRoute[1][range.bank],input,range.bytes);
	}

public:
	bool Bind(const DataflowDenseAttentionInvocation& invocation)
	{
		fInvocation=nullptr;
		if (!invocation.Valid()) return false;
		fInvocation=&invocation;
		return true;
	}

#if defined(PIONEER_QWEN35_DENSE_SHARD_COMPILE_ONLY) || defined(PIONEER_QWEN35_FLASH_DECODE_COMPILE_ONLY)
	bool BindShardProgram(const DataflowDenseAttentionProgram& program)
	{
		fInvocation=nullptr;
		if (program.mode!=DataflowDenseAttentionRun
			|| !DataflowDenseAttentionProgramBuilder::BindCore(program,0,fShardBinding)) return false;
		fInvocation=&fShardBinding;
		return true;
	}
#endif

	bool ReadProjectedRows(unsigned firstRow,float* output) const
	{
		if (!fInvocation) return false;
		const RowSpace& source=fInvocation->program.mode==DataflowDenseAttentionRun
			?fInvocation->program.qkv:fInvocation->program.gate;
		return ReadRows(source,firstRow,DenseHeadWidth,output);
	}

	bool ReadNorm(bool query,float* output) const
	{
		if (!fInvocation || !output) return false;
		const TensorSpace& tensor=query?fInvocation->program.queryNorm
			:fInvocation->program.keyNorm;
		Range range{};
		if (!tensor.place.Resolve(0,1,0,DenseHeadWidth*sizeof(float),range)
			|| !DataflowDenseAttentionProgram::AlignedRange(range,
				DenseHeadWidth*sizeof(float))) return false;
		return fReader.Read(range.address,
			fInvocation->program.bankRoute[0][range.bank],
			reinterpret_cast<uint8_t*>(output),range.bytes);
	}

	bool ReadRope(float* output) const
	{
		if (!fInvocation || !output) return false;
		const auto& program=fInvocation->program;
		const uint32_t address=program.rope.address
			+program.position*Rotary*sizeof(float);
		return fReader.Read(address,program.bankRoute[0][program.rope.bank],
			reinterpret_cast<uint8_t*>(output),Rotary*sizeof(float));
	}

	bool WriteKv(unsigned head,bool value,const uint32_t* packed) const
	{
		return packed && WriteKvRange(head,value,
			reinterpret_cast<const uint8_t*>(packed));
	}

	bool ReadKvTile(unsigned head,bool value,unsigned first,
		unsigned positions,float* output) const
	{
		return ReadKvRange(head,value,first,positions,
			reinterpret_cast<uint8_t*>(output));
	}

	bool WriteContextSlice(unsigned firstRow,unsigned rows,const float* values) const
	{
		if (!fInvocation || !values) return false;
		const RowSpace& output=fInvocation->program.contextOutput;
		if (!rows || ((firstRow|rows)&15) || firstRow>=DenseHeads*DenseHeadWidth
			|| rows>DenseHeads*DenseHeadWidth-firstRow) return false;
		unsigned completed=0;
		while (completed<rows) {
			const unsigned row=firstRow+completed;
			const Fragment* owner=nullptr;
			for (unsigned index=0;index<output.count;index++) {
				const Fragment& part=output.parts[index];
				if (row>=part.firstRow && row-part.firstRow<part.rows) {
					owner=&part;
					break;
				}
			}
			if (!owner) return false;
			unsigned count=owner->firstRow+owner->rows-row;
			if (count>rows-completed) count=rows-completed;
			count&=~15U;
			if (!count || count*sizeof(float)>DataflowNocMaxTransfer) return false;
			Range range{};
			if (!output.Resolve(row,count,0,count*sizeof(float),range)
				|| !fWriter.Write(range.address,
					fInvocation->program.bankRoute[1][range.bank],
					reinterpret_cast<const uint8_t*>(values+completed),range.bytes))
				return false;
			completed+=count;
		}
		return completed==rows;
	}

	bool WriteContext(unsigned firstRow,const float* values) const
	{
		return WriteContextSlice(firstRow,DenseHeadWidth,values);
	}
};

} // namespace PioneerQwen35

#endif
