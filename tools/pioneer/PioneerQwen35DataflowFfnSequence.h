/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_FFN_SEQUENCE_H
#define PIONEER_QWEN35_DATAFLOW_FFN_SEQUENCE_H

#include "PioneerQwen35DataflowAuthorization.h"
#ifdef PIONEER_QWEN35_FFN_PAIR_COMPILE_ONLY
#if !defined(PIONEER_QWEN35_RECOVERY_COMPILE_ONLY) || !defined(PIONEER_QWEN35_PAIRED_RECURRENCE) \
    || !defined(PIONEER_QWEN35_FUSED_RECURRENCE) || !defined(PIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY) \
    || !defined(PIONEER_QWEN35_FLASH_DECODE_COMPILE_ONLY) || !defined(PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS) \
    || !defined(PIONEER_QWEN35_ACTIVATION_SHARING_WITNESS) || defined(PIONEER_QWEN35_RECOVERY_QUALIFY) \
    || defined(PIONEER_QWEN35_DEVICE_DISPATCH_TRIAL)
#error Paired FFN is an isolated non-loadable hybrid graph experiment
#endif
#include <new>
#endif

namespace PioneerQwen35 {

enum DataflowFfnSequenceResult {
	DataflowFfnSequenceComplete=0,
	DataflowFfnSequenceInvalidRequest=1,
	DataflowFfnSaveBuildFailed=2,
	DataflowFfnSaveRunFailed=3,
	DataflowFfnNormBuildFailed=4,
	DataflowFfnNormRunFailed=5,
	DataflowFfnGateBuildFailed=6,
	DataflowFfnGateRunFailed=7,
	DataflowFfnUpBuildFailed=8,
	DataflowFfnUpRunFailed=9,
	DataflowFfnSiluBuildFailed=10,
	DataflowFfnSiluRunFailed=11,
	DataflowFfnDownBuildFailed=12,
	DataflowFfnDownRunFailed=13,
	DataflowFfnAddBuildFailed=14,
	DataflowFfnAddRunFailed=15
};

static_assert(DataflowFfnAddRunFailed<256,
	"FFN sequence stage fits its driver ABI field");

// Execute one complete feed-forward sublayer as checked resident worker
// commands. This owner only sequences work; it performs no tensor arithmetic.
// inputSlot contains the post-attention residual and outputSlot receives the
// completed Hidden-row residual result.
class DataflowFfnSequence {
public:
	template<class Authority,class Service,class IO>
	static DataflowFfnSequenceResult Run(unsigned layer,unsigned inputSlot,
		Authority& authority,Service& service,IO& io,unsigned& outputSlot,
		DataflowProjectionProgram& projectionScratch)
	{
		outputSlot=ActivationBuffers;
		if (layer>=Layers || inputSlot>=ActivationBuffers)
			return DataflowFfnSequenceInvalidRequest;
		const unsigned normalizedSlot=inputSlot^1U;

		DataflowResidualProgram residualSave{};
		if (!authority.BuildResidual(DataflowResidualSave,inputSlot,residualSave))
			return DataflowFfnSaveBuildFailed;
		if (service.RunResidual(residualSave,io)!=0)
			return DataflowFfnSaveRunFailed;

		DataflowRmsProgram norm{};
		if (!authority.BuildRms(layer,PostAttentionNorm,inputSlot,
			normalizedSlot,norm)) return DataflowFfnNormBuildFailed;
		if (service.RunRms(norm,io)!=0)
			return DataflowFfnNormRunFailed;

		DataflowProjectionProgram& gate=projectionScratch;
#ifdef PIONEER_QWEN35_FFN_PAIR_COMPILE_ONLY
		// Both projections read the same normalized activation and write
		// disjoint destinations. Partition the fleet instead of dispatching
		// two whole-fleet steps; the graph retains one shared completion barrier.
		auto* up=new(std::nothrow) DataflowProjectionProgram;
		if (!up) return DataflowFfnUpBuildFailed;
		if (!authority.BuildProjection(layer,Gate,normalizedSlot,inputSlot,gate,DataflowCoreCount/2)) {
			delete up;return DataflowFfnGateBuildFailed;
		}
		if (!authority.BuildProjection(layer,Up,normalizedSlot,DataflowGraphUpOutputSlot,*up,
			DataflowCoreCount-DataflowCoreCount/2)) {
			delete up;return DataflowFfnUpBuildFailed;
		}
		const bool pairOkay=service.RunIndependentProjectionPair(gate,*up,io)==0;
		delete up;
		if (!pairOkay) return DataflowFfnGateRunFailed;
#else
		if (!authority.BuildProjection(layer,Gate,normalizedSlot,inputSlot,gate))
			return DataflowFfnGateBuildFailed;
		if (service.RunProjection(gate,io)!=0)
			return DataflowFfnGateRunFailed;

		DataflowProjectionProgram& up=projectionScratch;
		if (!authority.BuildProjection(layer,Up,normalizedSlot,
			DataflowGraphUpOutputSlot,up)) return DataflowFfnUpBuildFailed;
		if (service.RunProjection(up,io)!=0)
			return DataflowFfnUpRunFailed;
#endif

		DataflowSiluProgram silu{};
		if (!authority.BuildSilu(inputSlot,silu)) return DataflowFfnSiluBuildFailed;
		if (service.RunSilu(silu,io)!=0)
			return DataflowFfnSiluRunFailed;

		DataflowProjectionProgram& down=projectionScratch;
		if (!authority.BuildProjection(layer,Down,inputSlot,normalizedSlot,down))
			return DataflowFfnDownBuildFailed;
		if (service.RunProjection(down,io)!=0)
			return DataflowFfnDownRunFailed;

		DataflowResidualProgram residualAdd{};
		if (!authority.BuildResidual(DataflowResidualAdd,normalizedSlot,residualAdd))
			return DataflowFfnAddBuildFailed;
		if (service.RunResidual(residualAdd,io)!=0)
			return DataflowFfnAddRunFailed;

		outputSlot=normalizedSlot;
		return DataflowFfnSequenceComplete;
	}
};

} // namespace PioneerQwen35

#endif
