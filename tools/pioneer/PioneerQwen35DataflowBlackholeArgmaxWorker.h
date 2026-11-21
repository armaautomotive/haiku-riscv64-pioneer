/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_BLACKHOLE_ARGMAX_WORKER_H
#define PIONEER_QWEN35_DATAFLOW_BLACKHOLE_ARGMAX_WORKER_H

#include "PioneerQwen35DataflowBlackholeIO.h"
#include "PioneerQwen35DataflowArgmaxProgram.h"
#include "PioneerQwen35DataflowRms.h"

namespace PioneerQwen35 {

template<class IO>
bool PioneerQwen35DataflowArgmaxWorker(
	const DataflowArgmaxInvocation& invocation,unsigned sequence,
	float* scratch,DataflowWorkerMailbox& mailbox,IO& io)
{
	if (!invocation.Valid() || !sequence
		|| sequence>DataflowWorkerSequenceLimit || !scratch
#if !defined(PIONEER_Q35_OPERATOR_TEST)
		|| reinterpret_cast<uintptr_t>(scratch)!=DataflowActivationCacheAddress)
#else
		)
#endif
		return false;
	if (!invocation.active) return true;

	float maximum=0;
	unsigned maximumToken=0;
	bool haveValue=false;
	for (unsigned fragment=0;fragment<invocation.program.logits.count;fragment++) {
		const Fragment& part=invocation.program.logits.parts[fragment];
		unsigned offset=0;
		while (offset<part.rows) {
			unsigned rows=DataflowNocMaxTransfer/sizeof(float);
			if (rows>part.rows-offset) rows=part.rows-offset;
			rows&=~15U;
			Range range{};
			if (!rows || !invocation.program.logits.Resolve(
				part.firstRow+offset,rows,0,rows*sizeof(float),range)
				|| range.bank!=part.bank
				|| !io.ReadLogits(range,
					invocation.program.bankRoute[0][range.bank],scratch,
					rows*sizeof(float))) return false;
			for (unsigned row=0;row<rows;row++) {
				const float value=scratch[row];
				if ((PioneerAttentionBits(value)&0x7f800000U)==0x7f800000U)
					return false;
				if (!haveValue || value>maximum) {
					maximum=value;
					maximumToken=part.firstRow+offset+row;
					haveValue=true;
				}
			}
			offset+=rows;
		}
	}
	if (!haveValue || maximumToken>=Vocabulary) return false;
	const unsigned scoreBits=PioneerAttentionBits(maximum);
	volatile uint32_t* const reserved=mailbox.reserved;
	DataflowStoreRelease(reserved[1],scoreBits);
	DataflowStoreRelease(reserved[2],sequence);
	DataflowStoreRelease(reserved[3],DataflowArgmaxResultMagic);
	DataflowStoreRelease(reserved[4],~maximumToken);
	DataflowStoreRelease(reserved[5],DataflowArgmaxResultChecksum(
		maximumToken,scoreBits,sequence));
	DataflowStoreRelease(reserved[6],0);
	DataflowStoreRelease(reserved[0],maximumToken);
	return true;
}

class DataflowBlackholeArgmaxOps {
	const DataflowArgmaxInvocation* fInvocation=nullptr;
	DataflowBlackholeNoc<0> fReader;

public:
	bool Bind(const DataflowArgmaxInvocation& invocation)
	{
		fInvocation=nullptr;
		if (!invocation.Valid()) return false;
		fInvocation=&invocation;
		return true;
	}

	bool ReadLogits(const Range& range,unsigned route,float* local,
		unsigned bytes) const
	{
		if (!fInvocation || !fInvocation->active || !local
			|| range.bank>=Banks || route!=fInvocation->program.bankRoute[0][range.bank]
			|| !bytes || (bytes&63) || bytes>DataflowNocMaxTransfer
			|| range.bytes!=bytes
			|| range.address<BankBase || range.address>=BankLimit
			|| bytes>BankLimit-range.address
#if !defined(PIONEER_Q35_OPERATOR_TEST)
			|| reinterpret_cast<uintptr_t>(local)!=DataflowActivationCacheAddress)
#else
		)
#endif
			return false;
		return fReader.Read(range.address,route,
			reinterpret_cast<uint8_t*>(local),bytes);
	}
};

} // namespace PioneerQwen35

#endif
