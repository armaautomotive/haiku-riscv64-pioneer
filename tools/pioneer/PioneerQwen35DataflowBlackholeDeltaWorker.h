/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_BLACKHOLE_DELTA_WORKER_H
#define PIONEER_QWEN35_DATAFLOW_BLACKHOLE_DELTA_WORKER_H

#include "PioneerGdnGateMath.h"
#include "PioneerQwen35DataflowBlackholeIO.h"
#include "PioneerQwen35DataflowDeltaProgram.h"
#include "PioneerQwen35DataflowRms.h"

namespace PioneerQwen35 {

enum : unsigned {
	DataflowDeltaStateInputAddress=0x93800,
	DataflowDeltaStateOutputAddress=0x95800,
	DataflowDeltaStateScratchBytes=DataflowDeltaStatePanelBytes,
	DataflowDeltaNegativeAOffset=0,
	DataflowDeltaBiasOffset=DeltaHeads,
	DataflowDeltaAlphaOffset=DeltaHeads*2,
	DataflowDeltaBetaOffset=DeltaHeads*3,
	DataflowDeltaScalarCount=DeltaHeads*4,
	DataflowDeltaReadPollLimit=4096
};

#if defined(__GNUC__) && !defined(__clang__)
#define PIONEER_QWEN35_DELTA_SIZE_OPT __attribute__((optimize("Os")))
#else
#define PIONEER_QWEN35_DELTA_SIZE_OPT
#endif

inline bool DataflowDeltaFinite(float value)
{
	return (PioneerAttentionBits(value)&0x7f800000U)!=0x7f800000U;
}

PIONEER_QWEN35_DELTA_SIZE_OPT inline bool
DataflowDeltaNormalizeQk(float* query,float* key)
{
	if (!query || !key) return false;
	float querySum=0,keySum=0;
	for (unsigned i=0;i<DeltaWidth;i++) {
		if (!DataflowRmsDomain(query[i]) || !DataflowRmsDomain(key[i]))
			return false;
		querySum+=PioneerQ38PrepareFlush(query[i]*query[i]);
		keySum+=PioneerQ38PrepareFlush(key[i]*key[i]);
	}
	if (!DataflowDeltaFinite(querySum) || !DataflowDeltaFinite(keySum)
		|| querySum<0 || keySum<0) return false;
	const float queryRoot=PioneerGraphSqrtPositive(querySum+1e-6f);
	const float keyRoot=PioneerGraphSqrtPositive(keySum+1e-6f);
	if (!(queryRoot>0) || !(keyRoot>0)
		|| !DataflowDeltaFinite(queryRoot) || !DataflowDeltaFinite(keyRoot)) return false;
	const float queryScale=PioneerGraphDividePositive(1.0f,queryRoot)
		*0.0883883461356163f;
	const float keyScale=PioneerGraphDividePositive(1.0f,keyRoot);
	if (!(queryScale>0) || !(keyScale>0)
		|| !DataflowDeltaFinite(queryScale) || !DataflowDeltaFinite(keyScale)) return false;
	for (unsigned i=0;i<DeltaWidth;i++) {
		query[i]*=queryScale;
		key[i]*=keyScale;
		if (!DataflowDeltaFinite(query[i]) || !DataflowDeltaFinite(key[i])) return false;
	}
	return true;
}

PIONEER_QWEN35_DELTA_SIZE_OPT inline bool
DataflowDeltaHeadGates(float negativeA,float deltaBias,
	float alpha,float betaLogit,float& decay,float& beta)
{
	if (!DataflowRmsDomain(negativeA) || !(negativeA<0)
		|| !DataflowRmsDomain(deltaBias) || !DataflowRmsDomain(alpha)
		|| !DataflowRmsDomain(betaLogit)) return false;
	const float shifted=alpha+deltaBias;
	if (!DataflowDeltaFinite(shifted)) return false;
	const float softplus=PioneerQ38PrepareSoftplus(shifted);
	const float exponent=negativeA*softplus;
	if (!DataflowDeltaFinite(softplus) || !(softplus>=0)
		|| !DataflowDeltaFinite(exponent) || exponent>0) return false;
	decay=PioneerQ38PrepareNegativeExp(exponent);
	const float e=PioneerQ38PrepareNegativeExp(
		-PioneerAttentionFloat(PioneerAttentionBits(betaLogit)&0x7fffffffU));
	const float denominator=1.0f+e;
	beta=betaLogit>=0?PioneerGraphDividePositive(1.0f,denominator)
		:PioneerGraphDividePositive(e,denominator);
	return DataflowDeltaFinite(decay) && decay>=0 && decay<=1
		&& DataflowDeltaFinite(beta) && beta>=0 && beta<=1;
}

PIONEER_QWEN35_DELTA_SIZE_OPT inline bool
DataflowDeltaUpdatePanel(const float* query,const float* key,
	const float* value,float* stateRead,float* stateWrite,float* output,
	float decay,float beta)
{
	if (!query || !key || !value || !stateRead || !stateWrite || !output
		|| !DataflowDeltaFinite(decay) || decay<0 || decay>1
		|| !DataflowDeltaFinite(beta) || beta<0 || beta>1) return false;
	for (unsigned valueLane=0;valueLane<16;valueLane++) {
		const unsigned base=valueLane*DeltaWidth;
		float prediction=0;
		for (unsigned keyLane=0;keyLane<DeltaWidth;keyLane++) {
			const float decayed=stateRead[base+keyLane]*decay;
			stateWrite[base+keyLane]=decayed;
			prediction+=decayed*key[keyLane];
		}
		const float correction=(value[valueLane]-prediction)*beta;
		float readout=0;
		for (unsigned keyLane=0;keyLane<DeltaWidth;keyLane++) {
			const unsigned at=base+keyLane;
			stateWrite[at]+=key[keyLane]*correction;
			readout+=stateWrite[at]*query[keyLane];
		}
		output[valueLane]=readout;
		if (!DataflowDeltaFinite(prediction) || !DataflowDeltaFinite(value[valueLane])
			|| !DataflowDeltaFinite(correction) || !DataflowDeltaFinite(readout))
			return false;
		for (unsigned keyLane=0;keyLane<DeltaWidth;keyLane++)
			if (!DataflowDeltaFinite(stateWrite[base+keyLane])) return false;
	}
	return true;
}

template<class IO>
PIONEER_QWEN35_DELTA_SIZE_OPT
bool PioneerQwen35DataflowDeltaWorker(const DataflowDeltaInvocation& invocation,
	float* queryPage,float* keyPage,float* valuePage,
	float* stateReadPage,float* stateWritePage,float* scalarPage,IO& io)
{
	if (!invocation.Valid() || !queryPage || !keyPage || !valuePage
		|| !stateReadPage || !stateWritePage || !scalarPage
		|| !io.Bind(invocation)) return false;
	float* const negativeA=scalarPage+DataflowDeltaNegativeAOffset;
	float* const deltaBias=scalarPage+DataflowDeltaBiasOffset;
	float* const alpha=scalarPage+DataflowDeltaAlphaOffset;
	float* const betaLogit=scalarPage+DataflowDeltaBetaOffset;
	if (!io.ReadParameters(negativeA,deltaBias,alpha,betaLogit)) return false;
	unsigned cachedHead=~0U;
	float decay=0,beta=0;
	for (unsigned local=0;local<invocation.panelCount;local++) {
		const unsigned panel=invocation.firstPanel+local;
		const unsigned head=panel/DataflowDeltaPanelsPerHead;
		const unsigned subPanel=panel%DataflowDeltaPanelsPerHead;
		const unsigned firstValue=head*DeltaWidth+subPanel*16;
		if (head!=cachedHead) {
			if (!io.ReadQueryKey(head,queryPage,keyPage)
				|| !DataflowDeltaNormalizeQk(queryPage,keyPage)
				|| !DataflowDeltaHeadGates(negativeA[head],deltaBias[head],
					alpha[head],betaLogit[head],decay,beta)) return false;
			cachedHead=head;
		}
		if (!io.ReadValuePanel(firstValue,valuePage)) return false;
		if (invocation.program.position) {
			if (!io.ReadStatePanel(head,subPanel,stateReadPage)) return false;
		} else {
			volatile float* zero=stateReadPage;
			for (unsigned i=0;i<DeltaWidth*16;i++) zero[i]=0.0f;
		}
		if (!DataflowDeltaUpdatePanel(queryPage,keyPage,valuePage,stateReadPage,
			stateWritePage,valuePage,decay,beta)
			|| !io.WriteStatePanel(head,subPanel,stateWritePage)
			|| !io.WriteValuePanel(firstValue,valuePage)) return false;
	}
	return true;
}

class DataflowBlackholeDeltaOps {
	const DataflowDeltaInvocation* fInvocation=nullptr;
	DataflowBlackholeNoc<0> fReader;
	DataflowBlackholeNoc<1> fWriter;

	bool ReadRange(const RowSpace& space,unsigned row,unsigned rows,
		unsigned offset,unsigned bytes,uint8_t* output) const
	{
		Range range;
		return fInvocation && output && space.Resolve(row,rows,offset,bytes,range)
			&& DataflowDeltaProgram::AlignedRange(range,bytes)
			&& fReader.Read(range.address,
				fInvocation->program.bankRoute[0][range.bank],output,bytes);
	}

	bool WriteRange(const RowSpace& space,unsigned row,unsigned rows,
		unsigned offset,unsigned bytes,const uint8_t* input) const
	{
		Range range;
		return fInvocation && input && space.Resolve(row,rows,offset,bytes,range)
			&& DataflowDeltaProgram::AlignedRange(range,bytes)
			&& fWriter.Write(range.address,
				fInvocation->program.bankRoute[1][range.bank],input,bytes);
	}

	bool ReadFloatRows(const RowSpace& space,unsigned firstRow,unsigned rows,
		float* output) const
	{
		if (!fInvocation || !output || !space.Valid() || !rows
			|| firstRow>=space.rows || rows>space.rows-firstRow
			|| space.rowBytes!=sizeof(float) || ((firstRow|rows)&15)) return false;
		unsigned completed=0;
		while (completed<rows) {
			const unsigned row=firstRow+completed;
			const Fragment* owner=nullptr;
			for (unsigned i=0;i<space.count;i++) {
				const Fragment& part=space.parts[i];
				if (row>=part.firstRow && row-part.firstRow<part.rows) {
					owner=&part;
					break;
				}
			}
			if (!owner) return false;
			unsigned count=owner->firstRow+owner->rows-row;
			if (count>rows-completed) count=rows-completed;
			count&=~15U;
			if (!count || !ReadRange(space,row,count,0,count*sizeof(float),
				reinterpret_cast<uint8_t*>(output+completed))) return false;
			completed+=count;
		}
		return completed==rows;
	}

	bool ReadTensor(const TensorSpace& tensor,float* output) const
	{
		if (!fInvocation || !output || tensor.type!=0
			|| tensor.inputs!=DeltaHeads || !tensor.place.Valid()
			|| tensor.place.rows!=1
			|| tensor.place.rowBytes!=DeltaHeads*sizeof(float)
			|| tensor.place.unit!=1) return false;
		Range range;
		return tensor.place.Resolve(0,1,0,DeltaHeads*sizeof(float),range)
			&& DataflowDeltaProgram::AlignedRange(range,DeltaHeads*sizeof(float))
			&& fReader.Read(range.address,
				fInvocation->program.bankRoute[0][range.bank],
				reinterpret_cast<uint8_t*>(output),DeltaHeads*sizeof(float));
	}

public:
	bool Bind(const DataflowDeltaInvocation& invocation)
	{
		fInvocation=nullptr;
		if (!invocation.Valid()) return false;
		fInvocation=&invocation;
		return true;
	}

	bool ReadParameters(float* negativeA,float* deltaBias,float* alpha,
		float* beta) const
	{
		return fInvocation && ReadTensor(fInvocation->program.negativeA,negativeA)
			&& ReadTensor(fInvocation->program.deltaBias,deltaBias)
			&& ReadFloatRows(fInvocation->program.alpha,0,DeltaHeads,alpha)
			&& ReadFloatRows(fInvocation->program.beta,0,DeltaHeads,beta);
	}

	bool ReadQueryKey(unsigned head,float* query,float* key) const
	{
		if (!fInvocation || head>=DeltaHeads || !query || !key) return false;
		const unsigned keyHead=head%DeltaKeyHeads;
		return ReadFloatRows(fInvocation->program.qkv,keyHead*DeltaWidth,
			DeltaWidth,query)
			&& ReadFloatRows(fInvocation->program.qkv,
				DataflowDeltaQChannels+keyHead*DeltaWidth,DeltaWidth,key);
	}

	bool ReadValuePanel(unsigned firstRow,float* output) const
	{
		return fInvocation && firstRow<DeltaInner && !(firstRow&15)
			&& ReadRange(fInvocation->program.qkv,
				DataflowDeltaVBase+firstRow,16,0,DataflowDeltaPanelBytes,
				reinterpret_cast<uint8_t*>(output));
	}

	bool ReadStatePanel(unsigned head,unsigned subPanel,float* output) const
	{
		return fInvocation && head<DeltaHeads
			&& subPanel<DataflowDeltaPanelsPerHead
			&& ReadRange(fInvocation->program.stateRead,head,1,
				subPanel*DataflowDeltaStatePanelBytes,
				DataflowDeltaStatePanelBytes,reinterpret_cast<uint8_t*>(output));
	}

	bool WriteStatePanel(unsigned head,unsigned subPanel,const float* input) const
	{
		return fInvocation && head<DeltaHeads
			&& subPanel<DataflowDeltaPanelsPerHead
			&& WriteRange(fInvocation->program.stateWrite,head,1,
				subPanel*DataflowDeltaStatePanelBytes,
				DataflowDeltaStatePanelBytes,reinterpret_cast<const uint8_t*>(input));
	}

	bool WriteValuePanel(unsigned firstRow,const float* input) const
	{
		return fInvocation && firstRow<DeltaInner && !(firstRow&15)
			&& WriteRange(fInvocation->program.qkv,
				DataflowDeltaVBase+firstRow,16,0,DataflowDeltaPanelBytes,
				reinterpret_cast<const uint8_t*>(input));
	}
};

static_assert(DataflowDeltaStateInputAddress+DataflowDeltaStateScratchBytes
	==DataflowDeltaStateOutputAddress
	&& DataflowDeltaStateOutputAddress+DataflowDeltaStateScratchBytes<=0x98000,
	"the two 16-by-128 delta-state panels fit the reserved worker L1 window");

#undef PIONEER_QWEN35_DELTA_SIZE_OPT

} // namespace PioneerQwen35

#endif
