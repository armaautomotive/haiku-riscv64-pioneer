/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_ACTIVATION_REUSE_H
#define PIONEER_QWEN35_DATAFLOW_ACTIVATION_REUSE_H
#include "PioneerQwen35DataflowKernels.h"
namespace PioneerQwen35 {

enum : unsigned { DataflowActivationReuseAddress=0xabe40,
	DataflowActivationReuseMagic=0x31524135 };
// The graph worker publishes context before the role command; the reader is
// the only writer of cached metadata. Reset while roles are idle at each run.
struct alignas(64) DataflowActivationReuse {
	volatile uint32_t epoch,step;
	uint32_t magic,cachedEpoch,cachedStep,packing,inputs,inputBase,inputRows;
	RowSpace input;
};
static_assert(DataflowActivationReuseAddress+sizeof(DataflowActivationReuse)<=DataflowBarrierArrivalBase,
	"activation reuse metadata must fit below the barrier arrivals");

inline void DataflowActivationReuseReset(DataflowActivationReuse& cache)
{
	DataflowStoreRelease(cache.epoch,0);DataflowStoreRelease(cache.step,0);
	DataflowStoreRelease(cache.magic,0);
}
inline void DataflowActivationReuseSetStep(DataflowActivationReuse& cache,unsigned epoch,unsigned step)
{
	DataflowStoreRelease(cache.step,0);DataflowStoreRelease(cache.epoch,epoch);
	DataflowStoreRelease(cache.step,step);
}
// Q4_K, Q5_K and Q6_K all use the same Q8_K activation quantizer; Q8_0
// uses a different representation. No projection math is performed here.
inline unsigned DataflowActivationReusePacking(unsigned type)
{
	return DataflowActivationSharePacking(type);
}
inline bool DataflowActivationReuseDisjoint(const RowSpace& input,const RowSpace& output)
{
	if (!input.Valid() || !output.Valid()) return false;
	for (unsigned i=0;i<input.count;i++) for (unsigned j=0;j<output.count;j++) {
		const auto& a=input.parts[i];const auto& b=output.parts[j];
		if (a.bank!=b.bank) continue;
		const uint64_t ae=uint64_t(a.address)+uint64_t(a.rows)*input.rowBytes;
		const uint64_t be=uint64_t(b.address)+uint64_t(b.rows)*output.rowBytes;
		if (uint64_t(a.address)<be && uint64_t(b.address)<ae) return false;
	}
	return true;
}
inline bool DataflowActivationReuseHit(const DataflowActivationReuse& cache,
	const DataflowCoreInvocation& program)
{
	const uint32_t step=DataflowLoadAcquire(cache.step);
	const uint32_t epoch=DataflowLoadAcquire(cache.epoch);
	return step && epoch && DataflowLoadAcquire(cache.magic)==DataflowActivationReuseMagic
		&& cache.cachedEpoch==epoch && cache.cachedStep<UINT32_MAX
		&& cache.cachedStep+1==step && cache.packing==DataflowActivationReusePacking(program.type)
		&& cache.packing && cache.inputs==program.inputs && cache.inputBase==program.inputBase
		&& cache.inputRows==program.inputRows && DataflowRowSpaceMatches(cache.input,program.input)
		&& DataflowLoadAcquire(cache.step)==step && DataflowLoadAcquire(cache.epoch)==epoch;
}
inline void DataflowActivationReuseCommit(DataflowActivationReuse& cache,
	const DataflowCoreInvocation& program)
{
	const uint32_t step=DataflowLoadAcquire(cache.step),epoch=DataflowLoadAcquire(cache.epoch);
	DataflowStoreRelease(cache.magic,0);
	const unsigned packing=DataflowActivationReusePacking(program.type);
	if (!step || !epoch || !packing || !DataflowActivationReuseDisjoint(program.input,program.output)) return;
	cache.input=program.input;cache.cachedEpoch=epoch;cache.cachedStep=step;
	cache.packing=packing;cache.inputs=program.inputs;cache.inputBase=program.inputBase;
	cache.inputRows=program.inputRows;
	if (DataflowLoadAcquire(cache.step)==step && DataflowLoadAcquire(cache.epoch)==epoch)
		DataflowStoreRelease(cache.magic,DataflowActivationReuseMagic);
}
}
#endif
