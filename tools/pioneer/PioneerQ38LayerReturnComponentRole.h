/* SPDX-License-Identifier: MIT */
#if defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) || defined(PIONEER_Q38_GLOBAL_CHAIN_ROOT_IMAGE) || defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE) || defined(PIONEER_Q38_GLOBAL_CHAIN_CONV_CAPSULE_IMAGE) || defined(PIONEER_Q38_GLOBAL_CHAIN_TOKEN_KV_APPEND) || defined(PIONEER_Q38_TOKEN_CANCEL_BOUNDARY) || defined(PIONEER_Q38_GLOBAL_CHAIN_F32_HIDDEN) || defined(PIONEER_Q38_GLOBAL_CHAIN_BOUNDARY_SEALS)
#error The old returning IMAGE is never the distinct global-chain role
#endif
// No guard: returning DEVICE arithmetic, not a standalone or native image.
#if !defined(PIONEER_Q38_LAYER_RETURN_COMPONENT_IMAGE) || PIONEER_Q38_LAYER_RETURN_COMPONENT_IMAGE != 1
#error Exact separately selected layer-return component required
#endif
#if defined(PIONEER_Q38_LAYER_COMPONENT_RECURRENT) == defined(PIONEER_Q38_LAYER_COMPONENT_DENSE)
#error Select exactly one returning arithmetic family, not both or neither
#endif
#ifdef PIONEER_Q38_LAYER_COMPONENT_RECURRENT
#if PIONEER_Q38_LAYER_COMPONENT_RECURRENT != 1
#error Exact recurrent component selection required, including defined0
#endif
#include "PioneerQ38MixedRecurrentBlockRootImageRole.h"
#else
#if PIONEER_Q38_LAYER_COMPONENT_DENSE != 1
#error Exact dense component selection required, including defined0
#endif
#include "PioneerQ38MixedLiveBlockRootImageRole.h"
#if !defined(PIONEER_Q38_SIGMOID_PRECISION_IMAGE) || PIONEER_Q38_SIGMOID_PRECISION_IMAGE != 1
#error Returning dense arithmetic requires the precise sigmoid implementation
#endif
#endif
#if defined(PIONEER_Q38_RECURRENT_PREFIX_SCHEDULER_IMAGE) || defined(PIONEER_Q38_RECURRENT_HISTORY_STAGING_IMAGE) \
 || defined(PIONEER_Q38_RECURRENT_PREFIX_NATIVE_CONTEXT) || defined(PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_NATIVE) \
 || defined(PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_CLIENT) || defined(PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_REFERENCE) \
 || defined(PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_CONTROL_COMPILE_ONLY) || defined(PIONEER_Q38_RECURRENT_PREFIX_FROZEN_PLAN) \
 || defined(PIONEER_Q38_RECURRENT_BLOCK_NATIVE_CONTEXT) || defined(PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_NATIVE) \
 || defined(PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_CLIENT) || defined(PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_REFERENCE) \
 || defined(PIONEER_Q38_RECURRENT_BLOCK_CONTEXT_COMPILE_ONLY) || defined(PIONEER_Q38_RECURRENT_BLOCK_EMIT_CONTEXT_BODY) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_CPU_VALIDATION)
#error Returning arithmetic creates no prefix, native, oracle, client or Work authority, including defined0
#endif
#ifdef PIONEER_Q38_LAYER_COMPONENT_DENSE_STOP
#if PIONEER_Q38_LAYER_COMPONENT_DENSE_STOP != 1 || !defined(PIONEER_Q38_LAYER_COMPONENT_DENSE) || PIONEER_Q38_LAYER_COMPONENT_DENSE != 1 \
 || !defined(PIONEER_Q38_LAYER_COMPONENT_CHAIN_LAYERS) || (PIONEER_Q38_LAYER_COMPONENT_CHAIN_LAYERS != 64 && PIONEER_Q38_LAYER_COMPONENT_CHAIN_LAYERS != 4)
#error Exact separately selected final-dense STOP profile required
#endif
#elif defined(PIONEER_Q38_LAYER_COMPONENT_CHAIN_LAYERS)
#error Chain length cannot select a STOP path without its exact component role
#endif
