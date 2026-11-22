/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_CONV_PROGRESS_H
#define PIONEER_QWEN35_DATAFLOW_CONV_PROGRESS_H
namespace PioneerQwen35 {
enum DataflowConvProgressStage : unsigned {
	DataflowConvProgressQueues=1,DataflowConvProgressValidate=2,
	DataflowConvProgressBind=3,DataflowConvProgressQkvRead=4,
	DataflowConvProgressWeightRead=5,DataflowConvProgressHistoryRead=6,
	DataflowConvProgressCompute=7,DataflowConvProgressHistoryWrite=8,
	DataflowConvProgressQkvWrite=9,DataflowConvProgressFinished=10
};
}
#endif
