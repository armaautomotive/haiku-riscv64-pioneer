/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_FFN_SEQUENCE_H
#define PIONEER_QWEN35_DATAFLOW_FFN_SEQUENCE_H

#include "PioneerQwen35DataflowAuthorization.h"

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
		Authority& authority,Service& service,IO& io,unsigned& outputSlot)
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

		DataflowProjectionProgram gate{};
		if (!authority.BuildProjection(layer,Gate,normalizedSlot,inputSlot,gate))
			return DataflowFfnGateBuildFailed;
		if (service.RunProjection(gate,io)!=0)
			return DataflowFfnGateRunFailed;

		DataflowProjectionProgram up{};
		if (!authority.BuildProjection(layer,Up,normalizedSlot,
			DataflowGraphUpOutputSlot,up)) return DataflowFfnUpBuildFailed;
		if (service.RunProjection(up,io)!=0)
			return DataflowFfnUpRunFailed;

		DataflowSiluProgram silu{};
		if (!authority.BuildSilu(inputSlot,silu)) return DataflowFfnSiluBuildFailed;
		if (service.RunSilu(silu,io)!=0)
			return DataflowFfnSiluRunFailed;

		DataflowProjectionProgram down{};
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
