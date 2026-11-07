/* SPDX-License-Identifier: MIT */
// Isolated approximate RMS image for offline instruction simulation only.
#if !defined(PIONEER_Q38_FAST_RMS_DIAGNOSTIC) || PIONEER_Q38_FAST_RMS_DIAGNOSTIC != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) \
 || defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE) || defined(PIONEER_Q38_GLOBAL_WORKER_IMAGE)
#error Exact isolated approximate RMS test image required
#endif
#define PIONEER_ATTENTION_FREESTANDING
#include "PioneerQ38FastF32HiddenRms.h"
#define PioneerQ38LearnedRms PioneerQ38FastF32HiddenRms
#define scalar_main q38_fast_f32_hidden_rms_transport_main
#include "blackhole_qwen38_learned_rms.cpp"
#undef scalar_main
#undef PioneerQ38LearnedRms
extern "C" unsigned scalar_main(){
 (void)&PioneerQ38LearnedRms;(void)&PioneerQ38F32HiddenRms;(void)&PioneerQ38F32HiddenResidualRms;
 (void)&PioneerQ38FastF32HiddenResidualRms;
 return q38_fast_f32_hidden_rms_transport_main();
}
