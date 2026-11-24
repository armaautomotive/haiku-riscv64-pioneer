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

// Round the software-binary64 sum to float after an exact power-of-two
// scaling. Q/K L2 uses power zero; gated RMS uses the mean (power -7).
PIONEER_QWEN35_DELTA_SIZE_OPT inline bool
DataflowDeltaRoundedSum(const DataflowRmsSum& sum,int power,float& result)
{
	if (!(sum.high|sum.low)) { result=0;return true; }
	uint32_t mantissa=(sum.high<<3)|(sum.low>>29);
	const uint32_t tail=sum.low&0x1fffffffU;
	int exponent=sum.exponent+power;
	if (tail>0x10000000U || (tail==0x10000000U && (mantissa&1))) mantissa++;
	if (mantissa==0x01000000U) { mantissa>>=1; exponent++; }
	if (exponent>127) return false;
	result=exponent< -126?0.0f:PioneerAttentionFloat(
		(uint32_t(exponent+127)<<23)|(mantissa&0x7fffffU));
	return true;
}

PIONEER_QWEN35_DELTA_SIZE_OPT inline bool
DataflowDeltaNormalizeQk(float* query,float* key)
{
	if (!query || !key) return false;
	DataflowRmsSum querySum,keySum;
	for (unsigned i=0;i<DeltaWidth;i++) {
		if (!DataflowRmsDomain(query[i]) || !DataflowRmsDomain(key[i])
			|| !querySum.Add(PioneerQ38PrepareFlush(query[i]*query[i]))
			|| !keySum.Add(PioneerQ38PrepareFlush(key[i]*key[i])))
			return false;
	}
	float querySquared=0,keySquared=0;
	if (!DataflowDeltaRoundedSum(querySum,0,querySquared)
		|| !DataflowDeltaRoundedSum(keySum,0,keySquared)) return false;
	float queryRoot=PioneerGraphSqrtPositive(querySquared);
	float keyRoot=PioneerGraphSqrtPositive(keySquared);
	// GGML L2 normalization clamps the norm itself; epsilon is not added
	// to the squared sum. This matters especially for low-magnitude heads.
	if (queryRoot<1e-6f) queryRoot=1e-6f;
	if (keyRoot<1e-6f) keyRoot=1e-6f;
	if (!(queryRoot>0) || !(keyRoot>0)
		|| !DataflowDeltaFinite(queryRoot) || !DataflowDeltaFinite(keyRoot)) return false;
	const float queryScale=PioneerGraphDividePositive(1.0f,queryRoot);
	const float keyScale=PioneerGraphDividePositive(1.0f,keyRoot);
	if (!(queryScale>0) || !(keyScale>0)
		|| !DataflowDeltaFinite(queryScale) || !DataflowDeltaFinite(keyScale)) return false;
	for (unsigned i=0;i<DeltaWidth;i++) {
		query[i]=(query[i]*queryScale)*0.0883883461356163f;
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

PIONEER_QWEN35_DELTA_SIZE_OPT inline bool
DataflowDeltaRoundedMean(const DataflowRmsSum& sum,float& mean)
{
	return DataflowDeltaRoundedSum(sum,-7,mean);
}

PIONEER_QWEN35_DELTA_SIZE_OPT inline bool
DataflowDeltaNormalizeAndGate(float* output,const float* norm,const float* gate)
{
	if (!output || !norm || !gate) return false;
	DataflowRmsSum sum;
	for (unsigned i=0;i<DeltaWidth;i++) {
		if (!DataflowRmsDomain(output[i]) || !DataflowRmsDomain(norm[i])
			|| !DataflowRmsDomain(gate[i])
			|| !sum.Add(PioneerQ38PrepareFlush(output[i]*output[i]))) return false;
	}
	float mean=0;
	if (!DataflowDeltaRoundedMean(sum,mean)) return false;
	const float denominator=PioneerGraphSqrtPositive(mean+1e-6f);
	if (!(denominator>0) || !DataflowDeltaFinite(denominator)) return false;
	const float scale=PioneerGraphDividePositive(1.0f,denominator);
	if (!(scale>0) || !DataflowRmsDomain(scale)) return false;
	for (unsigned i=0;i<DeltaWidth;i++) {
		const float normalized=(output[i]*scale)*norm[i];
		const float magnitude=PioneerAttentionFloat(
			PioneerAttentionBits(gate[i])&0x7fffffffU);
		const float exponential=PioneerQ38PrepareNegativeExp(-magnitude);
		const float sigmoid=PioneerGraphDividePositive(
			gate[i]>=0?1.0f:exponential,1.0f+exponential);
		const float result=normalized*(gate[i]*sigmoid);
		if (!DataflowDeltaFinite(normalized) || !DataflowDeltaFinite(exponential)
			|| !DataflowDeltaFinite(sigmoid) || !DataflowDeltaFinite(result)) return false;
		output[i]=result;
	}
	return true;
}

template<class IO>
PIONEER_QWEN35_DELTA_SIZE_OPT
bool PioneerQwen35DataflowDeltaWorker(const DataflowDeltaInvocation& invocation,
	float* queryPage,float* keyPage,float* valuePage,
	float* stateReadPage,float* stateWritePage,float* scalarPage,
	float* headOutput,float* normPage,float* gatePage,IO& io)
{
	if (!invocation.Valid() || !queryPage || !keyPage || !valuePage
		|| !stateReadPage || !stateWritePage || !scalarPage || !headOutput
		|| !normPage || !gatePage
		|| !io.Bind(invocation)) return false;
	if (!invocation.headCount) return true;
	float* const negativeA=scalarPage+DataflowDeltaNegativeAOffset;
	float* const deltaBias=scalarPage+DataflowDeltaBiasOffset;
	float* const alpha=scalarPage+DataflowDeltaAlphaOffset;
	float* const betaLogit=scalarPage+DataflowDeltaBetaOffset;
	if (!io.ReadParameters(negativeA,deltaBias,alpha,betaLogit)
		|| !io.ReadDeltaNorm(normPage)) return false;
	for (unsigned local=0;local<invocation.headCount;local++) {
		const unsigned head=invocation.firstHead+local;
		if (!io.ReadQueryKey(head,queryPage,keyPage)
			|| !DataflowDeltaNormalizeQk(queryPage,keyPage)
			|| !io.ReadGateHead(head,gatePage)) return false;
		float decay=0,beta=0;
		if (!DataflowDeltaHeadGates(negativeA[head],deltaBias[head],
			alpha[head],betaLogit[head],decay,beta)) return false;
		for (unsigned subPanel=0;subPanel<DataflowDeltaPanelsPerHead;subPanel++) {
			const unsigned firstValue=head*DeltaWidth+subPanel*16;
			if (!io.ReadValuePanel(firstValue,valuePage)) return false;
			if (invocation.program.position) {
				if (!io.ReadStatePanel(head,subPanel,stateReadPage)) return false;
			} else {
				volatile float* zero=stateReadPage;
				for (unsigned i=0;i<DeltaWidth*16;i++) zero[i]=0.0f;
			}
			if (!DataflowDeltaUpdatePanel(queryPage,keyPage,valuePage,stateReadPage,
				stateWritePage,valuePage,decay,beta)
				|| !io.WriteStatePanel(head,subPanel,stateWritePage)) return false;
			for (unsigned lane=0;lane<16;lane++)
				headOutput[subPanel*16+lane]=valuePage[lane];
		}
		if (!DataflowDeltaNormalizeAndGate(headOutput,normPage,gatePage)) return false;
		for (unsigned subPanel=0;subPanel<DataflowDeltaPanelsPerHead;subPanel++)
			if (!io.WriteValuePanel(head*DeltaWidth+subPanel*16,
				headOutput+subPanel*16)) return false;
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

	bool ReadTensor(const TensorSpace& tensor,unsigned inputs,float* output) const
	{
		if (!fInvocation || !output || tensor.type!=0
			|| tensor.inputs!=inputs || !tensor.place.Valid()
			|| tensor.place.rows!=1
			|| tensor.place.rowBytes!=inputs*sizeof(float)
			|| tensor.place.unit!=1) return false;
		Range range;
		return tensor.place.Resolve(0,1,0,inputs*sizeof(float),range)
			&& DataflowDeltaProgram::AlignedRange(range,inputs*sizeof(float))
			&& fReader.Read(range.address,
				fInvocation->program.bankRoute[0][range.bank],
				reinterpret_cast<uint8_t*>(output),inputs*sizeof(float));
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
		return fInvocation && ReadTensor(fInvocation->program.negativeA,DeltaHeads,negativeA)
			&& ReadTensor(fInvocation->program.deltaBias,DeltaHeads,deltaBias)
			&& ReadFloatRows(fInvocation->program.alpha,0,DeltaHeads,alpha)
			&& ReadFloatRows(fInvocation->program.beta,0,DeltaHeads,beta);
	}

	bool ReadDeltaNorm(float* norm) const
	{
		return fInvocation
			&& ReadTensor(fInvocation->program.deltaNorm,DeltaWidth,norm);
	}

	bool ReadGateHead(unsigned head,float* gate) const
	{
		return fInvocation && head<DeltaHeads && gate
			&& ReadFloatRows(fInvocation->program.gate,head*DeltaWidth,
				DeltaWidth,gate);
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
