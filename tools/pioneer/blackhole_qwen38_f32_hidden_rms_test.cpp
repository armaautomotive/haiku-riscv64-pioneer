/* SPDX-License-Identifier: MIT */
// Separate NUMERIC TEST image,not the old QRN1 native/Work implementation.
#if !defined(PIONEER_Q38_F32_HIDDEN_RMS_TEST_IMAGE) || PIONEER_Q38_F32_HIDDEN_RMS_TEST_IMAGE != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) \
 || defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE) || defined(PIONEER_Q38_GLOBAL_WORKER_IMAGE)
#error Exact isolated offline FP32 hidden RMS test image required
#endif
#define PIONEER_ATTENTION_FREESTANDING
#include "PioneerQ38F32HiddenRms.h"
// Reuse TRANSPORT geometry only,not an old image/owner capability. The old
// header was fully included above;only the call is selected here.
#define PioneerQ38LearnedRms PioneerQ38F32HiddenRms
#define scalar_main q38_f32_hidden_rms_transport_main
#include "blackhole_qwen38_learned_rms.cpp"
#undef scalar_main
#undef PioneerQ38LearnedRms
extern "C" unsigned scalar_main(){
 (void)&PioneerQ38LearnedRms;(void)&PioneerQ38F32HiddenResidualRms;return q38_f32_hidden_rms_transport_main();
}
