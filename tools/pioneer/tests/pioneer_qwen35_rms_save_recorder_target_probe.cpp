/* SPDX-License-Identifier: MIT */
// Host-side Haiku/RISC-V compile probe; never publishes a hardware record.
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS 1
#define PIONEER_QWEN35_PAIRED_RECURRENCE 1
#define PIONEER_QWEN35_FUSED_RECURRENCE 1
#define PIONEER_QWEN35_ACTIVATION_SHARING_WITNESS 1
#define PIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY 1
#define PIONEER_QWEN35_FLASH_DECODE_COMPILE_ONLY 1
#define PIONEER_QWEN35_FFN_PAIR_COMPILE_ONLY 1
#define PIONEER_QWEN35_FUSED_RMS_SAVE_COMPILE_ONLY 1
#include "../PioneerQwen35DataflowStreamRecorder.h"
struct ProbeSink {
    bool StageRecord(const PioneerQwen35::DataflowStreamBinding&,const PioneerQwen35::Range&,
        unsigned,const PioneerQwen35::DataflowStreamRecord&);
};
extern "C" int q35_rms_save_record_probe(PioneerQwen35::DataflowProjectionAuthority& authority,
    const PioneerQwen35::DataflowStreamPlan& plan,ProbeSink& sink,PioneerQwen35::DataflowStreamRecord& record,
    const PioneerQwen35::DataflowRmsProgram& rms,const PioneerQwen35::DataflowResidualProgram& residual) {
    PioneerQwen35::DataflowStreamRecorder<ProbeSink> recorder(authority,plan,sink,record);
    return recorder.RunRmsSave(rms,residual,sink);
}
