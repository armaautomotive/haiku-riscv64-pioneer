/*
 * Copyright 2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 * Optional, root-only Pioneer experiments. Only fixed owned host-buffer DMA;
 * no arbitrary DMA, unowned host writes or firmware flash.
 */
#ifdef PIONEER_Q38_RECURRENT_PREFIX_NATIVE_ROLE_TAG_OK
#error Internal prefix NODRV role witness cannot be supplied by the caller
#endif
#ifdef PIONEER_Q38_RECURRENT_PREFIX_NATIVE_CONTEXT
#include "PioneerQ38RecurrentPrefixNativeRole.h"
#define PIONEER_Q38_RECURRENT_PREFIX_NATIVE_ROLE_TAG_OK 1
#else
#define PIONEER_Q38_RECURRENT_PREFIX_NATIVE_ROLE_TAG_OK 0
#endif
#if defined(TT_PIONEER_DRIVER_RELOAD) && PIONEER_DRIVER_BUILD_TAG == 0x5152504e4f445256ULL && !PIONEER_Q38_RECURRENT_PREFIX_NATIVE_ROLE_TAG_OK
#error Reserved QRPNODRV tag needs its exact renamed-ABI compile perimeter
#endif
#ifdef PIONEER_Q38_RECURRENT_PREFIX_SCHEDULER_IMAGE
#error Offline autonomous prefix IMAGE is not a native driver role
#endif
#ifdef PIONEER_Q38_RECURRENT_BLOCK_NATIVE_ROLE_TAG_OK
#error Internal recurrent NODRV role witness cannot be supplied by the caller
#endif
#ifdef PIONEER_Q38_RECURRENT_BLOCK_NATIVE_CONTEXT
#include "PioneerQ38RecurrentBlockNativeRole.h"
#define PIONEER_Q38_RECURRENT_BLOCK_NATIVE_ROLE_TAG_OK 1
#else
#define PIONEER_Q38_RECURRENT_BLOCK_NATIVE_ROLE_TAG_OK 0
#endif
#if defined(TT_PIONEER_DRIVER_RELOAD) && PIONEER_DRIVER_BUILD_TAG == 0x5152424e4f445256ULL && !PIONEER_Q38_RECURRENT_BLOCK_NATIVE_ROLE_TAG_OK
#error Reserved QRBNODRV tag needs its exact renamed-ABI compile perimeter
#endif
#ifdef PIONEER_Q38_RECURRENT_BLOCK_PHYSICAL_ROLE_TAG_OK
#error Internal recurrent physical-role witness cannot be supplied by the caller
#endif
#ifdef PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_NATIVE
#include "PioneerQ38RecurrentBlockValidationRole.h"
#define PIONEER_Q38_RECURRENT_BLOCK_PHYSICAL_ROLE_TAG_OK 1
#else
#define PIONEER_Q38_RECURRENT_BLOCK_PHYSICAL_ROLE_TAG_OK 0
#endif
#if defined(TT_PIONEER_DRIVER_RELOAD) && (PIONEER_DRIVER_BUILD_TAG == 0x2026102700010071ULL || PIONEER_DRIVER_BUILD_TAG == 0x2026102700010072ULL) && !PIONEER_Q38_RECURRENT_BLOCK_PHYSICAL_ROLE_TAG_OK
#error Reserved recurrent tags require the exact physical validation role;old71 cannot select new72 code
#endif
#if defined(PIONEER_Q38_MIXED_RECURRENT_PROJECTION_ROOT_IMAGE) || defined(PIONEER_Q38_RESIDENT_CONTINUE_IMAGE) || defined(PIONEER_Q38_DENSE_PREPARE_IMAGE) || defined(PIONEER_Q38_DENSE_ATTENTION_IMAGE) || defined(PIONEER_Q38_DENSE_PROJECTION_IMAGE) || defined(PIONEER_Q38_DENSE_PARTITION_IMAGE) || defined(PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_JOINED_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_PROJECTION_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_DENSE_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_WORKER_IMAGE) || defined(PIONEER_Q38_MIXED_CAPSULE_IMAGE)
#error Continuation/local dense IMAGE has no existing native/CPU/Work/component role
#endif
#ifdef PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE
#error Recurrent BLOCK IMAGE is not an existing native driver role
#endif
#if defined(PIONEER_Q38_MIXED_RECURRENT_COHORT_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_ROOT_CAPSULE_IMAGE)
#error Offline recurrent ROOT/capsule cannot select an existing native driver role
#endif
#ifdef PIONEER_Q38_RESIDENT_ADVANCE_IMAGE
#error OFFLINE advance IMAGE has no existing native or Work driver role
#endif
#if defined(PIONEER_Q38_RESIDENT_LAYER_LINK_IMAGE) || defined(PIONEER_Q38_RESIDENT_LAYER_ROOT_IMAGE) || defined(PIONEER_Q38_RESIDENT_LAYER_NORM_IMAGE) || defined(PIONEER_Q38_RESIDENT_LAYER_FFN_IMAGE) || (defined(PIONEER_Q38_RESIDENT_LAYER_NATIVE_COMPONENT) && !defined(PIONEER_Q38_RESIDENT_LAYER_CONTEXT_COMPILE_ONLY) && !defined(PIONEER_Q38_RESIDENT_LAYER_VALIDATION_NATIVE))
#error OFFLINE complete layer IMAGE has no existing native or Work driver role
#endif
#ifdef PIONEER_Q38_MIXED_LIVE_FINAL_RELEASE
#include "PioneerQ38MixedLiveFinalReleaseRole.h"
#endif
#include "tenstorrent_pci_probe.h"
#ifdef PIONEER_Q38_RECURRENT_BLOCK_FINAL_RELEASE
#include "PioneerQ38RecurrentBlockFinalReleaseRole.h"
#include "PioneerQ38RecurrentBlockFinalPciState.h"
#endif
#ifdef PIONEER_Q38_MIXED_LIVE_FINAL_RELEASE
#include "PioneerQ38MixedLiveFinalPciState.h"
#endif
#ifdef PIONEER_Q38_MIXED_LIVE_PHYSICAL_ROLE_TAG_OK
#error Internal dense physical-role witness cannot be supplied by the caller
#endif
#ifdef PIONEER_Q38_MIXED_LIVE_VALIDATION_NATIVE
#include "PioneerQ38MixedLiveValidationRole.h"
#include "PioneerQ38MixedLiveValidationProtocol.h"
#define PIONEER_Q38_MIXED_LIVE_PHYSICAL_ROLE_TAG_OK 1
#else
#define PIONEER_Q38_MIXED_LIVE_PHYSICAL_ROLE_TAG_OK 0
#endif
#if defined(TT_PIONEER_DRIVER_RELOAD) && PIONEER_DRIVER_BUILD_TAG == 0x2026102700010070ULL && !PIONEER_Q38_MIXED_LIVE_PHYSICAL_ROLE_TAG_OK
#error Reserved tag70 requires ONLY its exact separate dense physical validation role
#endif
#if defined(PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT) || defined(PIONEER_Q38_RESIDENT_LAYER_CONTEXT_COMPILE_ONLY)
#ifdef PIONEER_Q38_RESIDENT_LAYER_VALIDATION_NATIVE
#include "PioneerQ38ResidentLayerValidationRole.h"
#else
#include "PioneerQ38ResidentLayerContextCompileRole.h"
#if defined(api_version) || defined(init_hardware) || defined(init_driver) || defined(uninit_driver) || defined(publish_devices) || defined(find_device)
#error New layer context entry names cannot be supplied by the caller
#endif
// NO conventional driver ABI and NO public selector. The renamed entries
// below return inertly before any PCI/module/sem/card operation.
extern "C" {
 extern int32 pioneer_q38_layer_context_only_api_version;
 status_t pioneer_q38_layer_context_only_init_hardware();
 status_t pioneer_q38_layer_context_only_init_driver();
 void pioneer_q38_layer_context_only_uninit_driver();
 const char** pioneer_q38_layer_context_only_publish_devices();
 device_hooks* pioneer_q38_layer_context_only_find_device(const char*);
}
#define api_version pioneer_q38_layer_context_only_api_version
#define init_hardware pioneer_q38_layer_context_only_init_hardware
#define init_driver pioneer_q38_layer_context_only_init_driver
#define uninit_driver pioneer_q38_layer_context_only_uninit_driver
#define publish_devices pioneer_q38_layer_context_only_publish_devices
#define find_device pioneer_q38_layer_context_only_find_device
#endif
#endif
#ifdef PIONEER_Q38_RESIDENT_LAYER_PHYSICAL_ROLE_TAG_OK
#error Internal complete-layer physical-role witness cannot be supplied by the caller
#endif
#ifdef PIONEER_Q38_RESIDENT_LAYER_VALIDATION_NATIVE
#include "PioneerQ38ResidentLayerValidationRole.h"
#include "PioneerQ38ResidentLayerValidationProtocol.h"
#define PIONEER_Q38_RESIDENT_LAYER_PHYSICAL_ROLE_TAG_OK 1
#else
#define PIONEER_Q38_RESIDENT_LAYER_PHYSICAL_ROLE_TAG_OK 0
#endif
#if defined(TT_PIONEER_DRIVER_RELOAD) && PIONEER_DRIVER_BUILD_TAG == 0x2026102600010069ULL && !PIONEER_Q38_RESIDENT_LAYER_PHYSICAL_ROLE_TAG_OK
#error Reserved tag69 requires its exact separate complete-layer physical DEBUG role
#endif
#ifdef PIONEER_Q38_RESIDENT_OUTPUT_PHYSICAL_ROLE_TAG_OK
#error Internal combined-output physical-role witness cannot be supplied by the caller
#endif
#ifdef PIONEER_Q38_RESIDENT_OUTPUT_VALIDATION_NATIVE
#include "PioneerQ38ResidentOutputValidationRole.h"
#include "PioneerQ38ResidentOutputValidationProtocol.h"
#define PIONEER_Q38_RESIDENT_OUTPUT_PHYSICAL_ROLE_TAG_OK 1
#else
#define PIONEER_Q38_RESIDENT_OUTPUT_PHYSICAL_ROLE_TAG_OK 0
#endif
#if defined(TT_PIONEER_DRIVER_RELOAD) && PIONEER_DRIVER_BUILD_TAG == 0x2026102500010068ULL && !PIONEER_Q38_RESIDENT_OUTPUT_PHYSICAL_ROLE_TAG_OK
#error Reserved tag68 requires ONLY its exact separate combined-output physical DEBUG role
#endif
#ifdef PIONEER_Q38_RESIDENT_OUTPUT_CONTEXT_COMPILE_ONLY
#include "PioneerQ38ResidentOutputContextCompileRole.h"
#if defined(api_version) || defined(init_hardware) || defined(init_driver) || defined(uninit_driver) || defined(publish_devices) || defined(find_device)
#error Context compile witness entry names cannot be supplied by the caller
#endif
// No conventional driver ABI exports. Even the renamed entry functions
// below refuse BEFORE PCI/module/sem/host/card operations.
extern "C" {
 extern int32 pioneer_q38_output_context_only_api_version;
 status_t pioneer_q38_output_context_only_init_hardware();
 status_t pioneer_q38_output_context_only_init_driver();
 void pioneer_q38_output_context_only_uninit_driver();
 const char** pioneer_q38_output_context_only_publish_devices();
 device_hooks* pioneer_q38_output_context_only_find_device(const char*);
}
#define api_version pioneer_q38_output_context_only_api_version
#define init_hardware pioneer_q38_output_context_only_init_hardware
#define init_driver pioneer_q38_output_context_only_init_driver
#define uninit_driver pioneer_q38_output_context_only_uninit_driver
#define publish_devices pioneer_q38_output_context_only_publish_devices
#define find_device pioneer_q38_output_context_only_find_device
#endif
#ifdef PIONEER_Q38_RESIDENT_OUTPUT_OWNER
#ifndef PIONEER_Q38_RESIDENT_LAYER_CONTEXT_COMPILE_ONLY
#ifndef PIONEER_Q38_RESIDENT_OUTPUT_CONTEXT_COMPILE_ONLY
#ifndef PIONEER_Q38_RESIDENT_OUTPUT_VALIDATION_NATIVE
#ifndef PIONEER_Q38_RESIDENT_LAYER_VALIDATION_NATIVE
#error Resident-output owner is OFFLINE until its actual complete native factory and role are qualified
#endif
#endif
#endif
#endif
#endif
#if defined(TT_PIONEER_DRIVER_RELOAD) && PIONEER_DRIVER_BUILD_TAG == 0x2026102500010067ULL && !defined(PIONEER_Q38_RESIDENT_OUTPUT_CONTEXT_COMPILE_ONLY)
#error Reserved tag67 is ONLY a refusing whole-context compile witness,NOT a loadable driver
#endif
#if defined(PIONEER_Q38_RESIDENT_OPERATOR_IMAGE) || defined(PIONEER_Q38_RESIDENT_GDN_OPERATOR_IMAGE) || defined(PIONEER_Q38_RESIDENT_COHORT_ROOT_IMAGE) || defined(PIONEER_Q38_RESIDENT_GDN_OUTPUT_OPERATOR_IMAGE) || defined(PIONEER_Q38_RESIDENT_OUTPUT_ROOT_IMAGE)
#error OFFLINE resident operator IMAGE never enters an existing native or Work driver
#endif
#ifdef PIONEER_Q38_RESIDENT_SILU_PRECISION_NATIVE
#if PIONEER_Q38_RESIDENT_SILU_PRECISION_NATIVE != 1 || !defined(PIONEER_Q38_NEXT_FFN_RESIDUAL_NATIVE) || PIONEER_Q38_NEXT_FFN_RESIDUAL_NATIVE != 1 || defined(PIONEER_Q38_CONV_SILU_PRECISION_NATIVE) || defined(PIONEER_Q38_CONV_SILU_PRECISION_VALIDATION_CLIENT)
#error Exact separate native66 requires genuine SAME native65,never DEBUG64 or Work
#endif
#endif
#ifdef PIONEER_Q38_NEXT_FFN_RESIDUAL_NATIVE
#if PIONEER_Q38_NEXT_FFN_RESIDUAL_NATIVE != 1 || !defined(PIONEER_Q38_NEXT_RESIDUAL_RMS_NATIVE) || PIONEER_Q38_NEXT_RESIDUAL_RMS_NATIVE != 1 || defined(PIONEER_Q38_CONV_SILU_PRECISION_NATIVE) || defined(PIONEER_Q38_CONV_SILU_PRECISION_VALIDATION_CLIENT)
#error Exact separate native65 requires genuine SAME native63,never DEBUG64 or Work
#endif
#endif
#ifdef PIONEER_Q38_CONV_SILU_PRECISION_VALIDATION_CLIENT
#error Precision DEBUG64 CPU client never enters a kernel driver or Work selector
#endif
#ifdef PIONEER_Q38_CONV_SILU_PRECISION_NATIVE
#if PIONEER_Q38_CONV_SILU_PRECISION_NATIVE != 1 || !defined(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE) || PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE != 1
#error Exact separate DEBUG64 requires SAME native52 diagnostic parents,never Work
#endif
#endif
#if defined(PIONEER_Q38_SILU_PRECISION_IMAGE) || defined(PIONEER_Q38_SILU_LEGACY_CONTROL_IMAGE)
#error LOCAL SiLU diagnostic images never enter a native driver or Work selector
#endif
#ifdef PIONEER_Q38_NEXT_RESIDUAL_RMS_IMAGE
#error QRR2 image DATA never enters a native driver or public selector
#endif
#ifdef PIONEER_Q38_NEXT_RESIDUAL_RMS_NATIVE
#if PIONEER_Q38_NEXT_RESIDUAL_RMS_NATIVE != 1 || !defined(PIONEER_Q38_NEXT_GDN_OUTPUT_NATIVE) || PIONEER_Q38_NEXT_GDN_OUTPUT_NATIVE != 1 || defined(PIONEER_Q38_NEXT_RESIDUAL_RMS_IMAGE) || defined(PIONEER_Q38_RESIDUAL_RMS_IMAGE)
#error Exact native63 requires SAME genuine native62 output+raw,never image aliases
#endif
#endif
#ifdef PIONEER_Q38_NEXT_GDN_OUTPUT_NATIVE
#if PIONEER_Q38_NEXT_GDN_OUTPUT_NATIVE != 1 || !defined(PIONEER_Q38_NEXT_GDN_NATIVE) || PIONEER_Q38_NEXT_GDN_NATIVE != 1 || defined(PIONEER_Q38_NEXT_GDN_OUTPUT_GRAPH_ROOT) || defined(PIONEER_Q38_NEXT_GDN_OUTPUT_INPUT_IMAGE) || defined(PIONEER_Q38_NEXT_GDN_OUTPUT_GATHER_IMAGE)
#error Exact native62 requires SAME genuine native61 producer,never image aliases
#endif
#endif
#ifdef PIONEER_Q38_NEXT_GDN_NATIVE
#if PIONEER_Q38_NEXT_GDN_NATIVE != 1 || !defined(PIONEER_Q38_NEXT_CONV_NATIVE) || PIONEER_Q38_NEXT_CONV_NATIVE != 1
#error Exact native61 requires SAME actual native60 convolution producer
#endif
#endif
#ifdef PIONEER_Q38_NEXT_CONV_NATIVE
#if PIONEER_Q38_NEXT_CONV_NATIVE != 1 || !defined(PIONEER_Q38_NEXT_PROJECTION_NATIVE) || PIONEER_Q38_NEXT_PROJECTION_NATIVE != 1
#error Exact native60 requires SAME actual native59 projection producer
#endif
#endif
#ifdef PIONEER_Q38_NEXT_PROJECTION_NATIVE
#if PIONEER_Q38_NEXT_PROJECTION_NATIVE != 1 || !defined(PIONEER_Q38_NEXT_RMS_NATIVE) || PIONEER_Q38_NEXT_RMS_NATIVE != 1 || defined(PIONEER_Q38_PROJECTION_NEXT_ROOT)
#error Exact native59 requires SAME actual native58 input,never image aliases
#endif
#endif
#ifdef PIONEER_Q38_PROJECTION_NEXT_ROOT
#error QPS2 is an image-only grammar,not a native/Work/DEBUG driver selector
#endif
#ifdef PIONEER_Q38_NEXT_RMS_NATIVE
#if PIONEER_Q38_NEXT_RMS_NATIVE != 1 || !defined(PIONEER_Q38_FFN_RESIDUAL_NATIVE) || PIONEER_Q38_FFN_RESIDUAL_NATIVE != 1
#error Exact native58 requires SAME actual native57 FFN hidden,never caller inputs
#endif
#endif
#ifdef PIONEER_Q38_FFN_RESIDUAL_NATIVE
#if PIONEER_Q38_FFN_RESIDUAL_NATIVE != 1 || !defined(PIONEER_Q38_RESIDUAL_RMS_NATIVE) || PIONEER_Q38_RESIDUAL_RMS_NATIVE != 1 || defined(PIONEER_Q38_FFN_GRAPH_RESIDUAL_ROOT) || defined(PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE)
#error Separate exact native57 requires SAME actual native56 producer,not image/DEBUG
#endif
#endif
#ifdef PIONEER_Q38_RESIDUAL_RMS_NATIVE
#if PIONEER_Q38_RESIDUAL_RMS_NATIVE != 1 || !defined(PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE) || PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE != 1 || defined(PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE) || defined(PIONEER_Q38_RESIDUAL_RMS_IMAGE)
#error Separate exact native56 requires SAME native54 owner,not DEBUG/image/Work
#endif
#endif
#ifdef PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE
#if PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE != 1 || !defined(PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE) || PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE != 1 || defined(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE) || defined(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE)
#error Separate exact DEBUG55 requires SAME native54 output owner,never old DEBUG/Work
#endif
#endif
#ifdef PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE
#if PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE != 1 || !defined(PIONEER_Q38_GDN_OUTPUT_PRODUCER_NATIVE) || PIONEER_Q38_GDN_OUTPUT_PRODUCER_NATIVE != 1
#error Separate exact native54 requires SAME actual gated producer/native parents
#endif
#endif
#ifdef PIONEER_Q38_GDN_OUTPUT_PRODUCER_NATIVE
#if PIONEER_Q38_GDN_OUTPUT_PRODUCER_NATIVE != 1 || !defined(PIONEER_Q38_GDN_RENDEZVOUS_NATIVE) || PIONEER_Q38_GDN_RENDEZVOUS_NATIVE != 1 || defined(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE) || defined(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE)
#error Separate exact native53 gated-output producer requires SAME timed native owner,never DEBUG or Work
#endif
#endif
#ifdef PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE
#if PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE != 1 || !defined(PIONEER_Q38_GDN_RENDEZVOUS_NATIVE) || PIONEER_Q38_GDN_RENDEZVOUS_NATIVE != 1 || defined(PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE) || defined(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE)
#error Separate exact DEBUG52 requires SAME timed-rendezvous native51,never old DEBUG/Work
#endif
#endif
#ifdef PIONEER_Q38_GDN_RENDEZVOUS_NATIVE
#if PIONEER_Q38_GDN_RENDEZVOUS_NATIVE != 1 || !defined(PIONEER_Q38_GDN_GATED_NATIVE) || PIONEER_Q38_GDN_GATED_NATIVE != 1
#error Separate exact timed-rendezvous native51 requires SAME gated native owner
#endif
#endif
#ifdef PIONEER_Q38_GDN_GATED_NATIVE
#if PIONEER_Q38_GDN_GATED_NATIVE != 1 || !defined(PIONEER_Q38_GDN_RECURRENT_NATIVE) || PIONEER_Q38_GDN_RECURRENT_NATIVE != 1 || defined(PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE) || defined(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE)
#error Separate exact tag50 SAME-owner fused GDN/gated mode,not old DEBUG or Work
#endif
#endif
#ifdef PIONEER_Q38_RESIDENT_DECODER_OWNER
#ifndef PIONEER_Q38_RESIDENT_DECODER_NATIVE
#error Resident decoder arithmetic owner is OFFLINE until its actual separate native factory and role are qualified
#endif
#endif
#ifdef PIONEER_Q38_DECODER_RUNTIME
#ifndef PIONEER_Q38_RESIDENT_DECODER_NATIVE
#if !defined(PIONEER_Q38_RESIDENT_LAYER_CONTEXT_COMPILE_ONLY) && !defined(PIONEER_Q38_MIXED_LIVE_NATIVE_CONTEXT) && !PIONEER_Q38_RECURRENT_BLOCK_NATIVE_ROLE_TAG_OK && !PIONEER_Q38_RECURRENT_PREFIX_NATIVE_ROLE_TAG_OK
#ifndef PIONEER_Q38_RESIDENT_OUTPUT_CONTEXT_COMPILE_ONLY
#ifndef PIONEER_Q38_RESIDENT_OUTPUT_VALIDATION_NATIVE
#ifndef PIONEER_Q38_RESIDENT_LAYER_VALIDATION_NATIVE
#error Decoder runtime/history reservation is OFFLINE until its separate actual native factory and role are qualified
#endif
#endif
#endif
#endif
#endif
#endif
#ifdef PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE
#if PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE != 1 || !defined(PIONEER_Q38_GDN_RECURRENT_NATIVE) || PIONEER_Q38_GDN_RECURRENT_NATIVE != 1 || defined(PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE) || defined(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE)
#error Separate exact tag49 initialized-GDN recurrent DEBUG,not input DEBUG or Work
#endif
#endif
#ifdef PIONEER_Q38_GDN_RECURRENT_NATIVE
#if PIONEER_Q38_GDN_RECURRENT_NATIVE != 1 || !defined(PIONEER_Q38_GDN_STATE_INIT_NATIVE) || PIONEER_Q38_GDN_STATE_INIT_NATIVE != 1 || !defined(PIONEER_Q38_GDN_INPUT_OWNER) || PIONEER_Q38_GDN_INPUT_OWNER != 1 || defined(PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE) || defined(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE)
#error Separate exact tag48 SAME initialized-GDN owner,not old DEBUG/Work promotion
#endif
#endif
#ifdef PIONEER_Q38_GDN_STATE_INIT_NATIVE
#if PIONEER_Q38_GDN_STATE_INIT_NATIVE != 1 || !defined(PIONEER_Q38_GDN_INPUT_OWNER) || PIONEER_Q38_GDN_INPUT_OWNER != 1 || !defined(PIONEER_Q38_RESIDENT_DECODER_NATIVE) || PIONEER_Q38_RESIDENT_DECODER_NATIVE != 1 || defined(PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE) || defined(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE)
#error Separate exact tag47 SAME-owner GDN initialization,not old input/convolution DEBUG
#endif
#endif
#ifdef PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE
#if PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE != 1 || !defined(PIONEER_Q38_GDN_INPUT_OWNER) || PIONEER_Q38_GDN_INPUT_OWNER != 1 || defined(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE)
#error Exact separate tag46 GDN-input DEBUG requires SAME-owner role,never old convolution DEBUG
#endif
#endif
#ifdef PIONEER_Q38_GDN_INPUT_OWNER
#if PIONEER_Q38_GDN_INPUT_OWNER != 1 || !defined(PIONEER_Q38_RESIDENT_DECODER_NATIVE) || PIONEER_Q38_RESIDENT_DECODER_NATIVE != 1 || defined(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE)
#error Separate resident-input owner needs exact native role,never old convolution DEBUG
#endif
#ifdef PIONEER_Q38_RESIDENT_SILU_PRECISION_NATIVE
#define PIONEER_Q38_DECODER_EXPECTED_TAG 0x2026102500010066ULL
#elif defined(PIONEER_Q38_NEXT_FFN_RESIDUAL_NATIVE)
#define PIONEER_Q38_DECODER_EXPECTED_TAG 0x2026102500010065ULL
#elif defined(PIONEER_Q38_NEXT_RESIDUAL_RMS_NATIVE)
#define PIONEER_Q38_DECODER_EXPECTED_TAG 0x2026102400010063ULL
#elif defined(PIONEER_Q38_NEXT_GDN_OUTPUT_NATIVE)
#define PIONEER_Q38_DECODER_EXPECTED_TAG 0x2026102400010062ULL
#elif defined(PIONEER_Q38_NEXT_GDN_NATIVE)
#define PIONEER_Q38_DECODER_EXPECTED_TAG 0x2026102400010061ULL
#elif defined(PIONEER_Q38_NEXT_CONV_NATIVE)
#define PIONEER_Q38_DECODER_EXPECTED_TAG 0x2026102400010060ULL
#elif defined(PIONEER_Q38_NEXT_PROJECTION_NATIVE)
#define PIONEER_Q38_DECODER_EXPECTED_TAG 0x2026102400010059ULL
#elif defined(PIONEER_Q38_NEXT_RMS_NATIVE)
#define PIONEER_Q38_DECODER_EXPECTED_TAG 0x2026102400010058ULL
#elif defined(PIONEER_Q38_FFN_RESIDUAL_NATIVE)
#define PIONEER_Q38_DECODER_EXPECTED_TAG 0x2026102400010057ULL
#elif defined(PIONEER_Q38_RESIDUAL_RMS_NATIVE)
#define PIONEER_Q38_DECODER_EXPECTED_TAG 0x2026102400010056ULL
#elif defined(PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE)
#define PIONEER_Q38_DECODER_EXPECTED_TAG 0x2026102400010055ULL
#elif defined(PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE)
#define PIONEER_Q38_DECODER_EXPECTED_TAG 0x2026102400010054ULL
#elif defined(PIONEER_Q38_GDN_OUTPUT_PRODUCER_NATIVE)
#define PIONEER_Q38_DECODER_EXPECTED_TAG 0x2026102400010053ULL
#elif defined(PIONEER_Q38_CONV_SILU_PRECISION_NATIVE)
#define PIONEER_Q38_DECODER_EXPECTED_TAG 0x2026102400010064ULL
#elif defined(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE)
#define PIONEER_Q38_DECODER_EXPECTED_TAG 0x2026102400010052ULL
#elif defined(PIONEER_Q38_GDN_RENDEZVOUS_NATIVE)
#define PIONEER_Q38_DECODER_EXPECTED_TAG 0x2026102400010051ULL
#elif defined(PIONEER_Q38_GDN_GATED_NATIVE)
#define PIONEER_Q38_DECODER_EXPECTED_TAG 0x2026102400010050ULL
#elif defined(PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE)
#define PIONEER_Q38_DECODER_EXPECTED_TAG 0x2026102400010049ULL
#elif defined(PIONEER_Q38_GDN_RECURRENT_NATIVE)
#define PIONEER_Q38_DECODER_EXPECTED_TAG 0x2026102400010048ULL
#elif defined(PIONEER_Q38_GDN_STATE_INIT_NATIVE)
#define PIONEER_Q38_DECODER_EXPECTED_TAG 0x2026102400010047ULL
#elif defined(PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE)
#define PIONEER_Q38_DECODER_EXPECTED_TAG 0x2026102300010046ULL
#else
#define PIONEER_Q38_DECODER_EXPECTED_TAG 0x2026102300010045ULL
#endif
#elif defined(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE)
#if PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE != 1 || !defined(PIONEER_Q38_RESIDENT_DECODER_NATIVE) || PIONEER_Q38_RESIDENT_DECODER_NATIVE != 1
#error Separate fixed retained-convolution DEBUG requires exact native decoder role
#endif
#define PIONEER_Q38_DECODER_EXPECTED_TAG 0x2026102300010044ULL
#else
#define PIONEER_Q38_DECODER_EXPECTED_TAG 0x2026102300010043ULL
#endif
#ifdef PIONEER_Q38_RESIDENT_DECODER_NATIVE
#if PIONEER_Q38_RESIDENT_DECODER_NATIVE != 1 || !defined(PIONEER_Q38_RESIDENT_DECODER_OWNER) || PIONEER_Q38_RESIDENT_DECODER_OWNER != 1 || !defined(PIONEER_Q38_DECODER_RUNTIME) || PIONEER_Q38_DECODER_RUNTIME != 1 || !defined(PIONEER_Q38_PROJECTION_EXECUTION_OWNER) || PIONEER_Q38_PROJECTION_EXECUTION_OWNER != 1 || !defined(PIONEER_Q38_PROJECTION_RESULT_STASH) || PIONEER_Q38_PROJECTION_RESULT_STASH != 1 || !defined(PIONEER_Q38_LEARNED_RMS_NATIVE) || PIONEER_Q38_LEARNED_RMS_NATIVE != 1 || !defined(PIONEER_Q38_EMBEDDING_NATIVE) || PIONEER_Q38_EMBEDDING_NATIVE != 1 || !defined(TT_PIONEER_DRIVER_RELOAD) || TT_PIONEER_DRIVER_RELOAD != 1 || PIONEER_DRIVER_BUILD_TAG != PIONEER_Q38_DECODER_EXPECTED_TAG || defined(PIONEER_Q38_PROJECTION_VALIDATION_NATIVE) || defined(PIONEER_Q38_EMBEDDING_QUALIFICATION) || defined(PIONEER_Q38_EMBEDDING_VALIDATION_NATIVE) || defined(PIONEER_Q38_LEARNED_RMS_VALIDATION_NATIVE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_FFN_HOST_PROFILE) || defined(PIONEER_Q38_FFN_SEALED_PROOF)
#error Exact separate tag43 decoder,tag44 fixed DEBUG or tag45 resident-input owner required,never old DEBUG/Work/CPU-reference promotion
#endif
#define PIONEER_Q38_DECODER_ROLE_TAG_OK 1
#else
#define PIONEER_Q38_DECODER_ROLE_TAG_OK 0
#endif
#if defined(TT_PIONEER_DRIVER_RELOAD) && PIONEER_DRIVER_BUILD_TAG == 0x2026102300010043ULL && (!PIONEER_Q38_DECODER_ROLE_TAG_OK || defined(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE))
#error Reserved tag43 identifies ONLY the native retained QPS1 convolution decoder
#endif
#if defined(TT_PIONEER_DRIVER_RELOAD) && PIONEER_DRIVER_BUILD_TAG == 0x2026102300010044ULL && (!PIONEER_Q38_DECODER_ROLE_TAG_OK || !defined(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE))
#error Reserved tag44 identifies ONLY the separate fixed retained-convolution DEBUG qualification
#endif
#if defined(TT_PIONEER_DRIVER_RELOAD) && PIONEER_DRIVER_BUILD_TAG == 0x2026102300010045ULL && (!PIONEER_Q38_DECODER_ROLE_TAG_OK || !defined(PIONEER_Q38_GDN_INPUT_OWNER))
#error Reserved tag45 identifies ONLY the separate same-owner resident-input preparation,NO public selector
#endif
#if defined(TT_PIONEER_DRIVER_RELOAD) && PIONEER_DRIVER_BUILD_TAG == 0x2026102300010046ULL && (!PIONEER_Q38_DECODER_ROLE_TAG_OK || !defined(PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE))
#error Reserved tag46 identifies ONLY the separate fixed GDN-input DEBUG qualifier
#endif
#if defined(TT_PIONEER_DRIVER_RELOAD) && PIONEER_DRIVER_BUILD_TAG == 0x2026102400010047ULL && (!PIONEER_Q38_DECODER_ROLE_TAG_OK || !defined(PIONEER_Q38_GDN_STATE_INIT_NATIVE))
#error Reserved tag47 identifies ONLY same native joint-owner GDN zero+Close,NO public selector
#endif
#if defined(TT_PIONEER_DRIVER_RELOAD) && PIONEER_DRIVER_BUILD_TAG == 0x2026102400010048ULL && (!PIONEER_Q38_DECODER_ROLE_TAG_OK || !defined(PIONEER_Q38_GDN_RECURRENT_NATIVE))
#error Reserved tag48 identifies ONLY SAME native initialized-GDN recurrent worker,NO public selector
#endif
#if defined(TT_PIONEER_DRIVER_RELOAD) && PIONEER_DRIVER_BUILD_TAG == 0x2026102400010049ULL && (!PIONEER_Q38_DECODER_ROLE_TAG_OK || !defined(PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE))
#error Reserved tag49 identifies ONLY separate fixed recurrent DEBUG
#endif
#if defined(TT_PIONEER_DRIVER_RELOAD) && PIONEER_DRIVER_BUILD_TAG == 0x2026102400010050ULL && (!PIONEER_Q38_DECODER_ROLE_TAG_OK || !defined(PIONEER_Q38_GDN_GATED_NATIVE))
#error Reserved tag50 identifies ONLY SAME-owner fused QGH1,NO public selector
#endif
#if defined(TT_PIONEER_DRIVER_RELOAD) && PIONEER_DRIVER_BUILD_TAG == 0x2026102400010051ULL && (!PIONEER_Q38_DECODER_ROLE_TAG_OK || !defined(PIONEER_Q38_GDN_RENDEZVOUS_NATIVE))
#error Reserved tag51 identifies ONLY SAME-owner timed rendezvous QGT1,NO public selector
#endif
#if defined(TT_PIONEER_DRIVER_RELOAD) && PIONEER_DRIVER_BUILD_TAG == 0x2026102400010052ULL && (!PIONEER_Q38_DECODER_ROLE_TAG_OK || !defined(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE))
#error Reserved tag52 identifies ONLY separate fixed timed-rendezvous DEBUG qualification
#endif
#if defined(TT_PIONEER_DRIVER_RELOAD) && PIONEER_DRIVER_BUILD_TAG == 0x2026102400010053ULL && (!PIONEER_Q38_DECODER_ROLE_TAG_OK || !defined(PIONEER_Q38_GDN_OUTPUT_PRODUCER_NATIVE))
#error Reserved tag53 identifies ONLY SAME-owner gated-output producer,NO public selector
#endif
#if defined(TT_PIONEER_DRIVER_RELOAD) && PIONEER_DRIVER_BUILD_TAG == 0x2026102400010054ULL && (!PIONEER_Q38_DECODER_ROLE_TAG_OK || !defined(PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE))
#error Reserved tag54 identifies ONLY SAME-owner gated-output execution,NO public selector
#endif
#if defined(TT_PIONEER_DRIVER_RELOAD) && PIONEER_DRIVER_BUILD_TAG == 0x2026102400010055ULL && (!PIONEER_Q38_DECODER_ROLE_TAG_OK || !defined(PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE))
#error Reserved tag55 identifies ONLY separate fixed gated-output DEBUG qualification
#endif
#if defined(TT_PIONEER_DRIVER_RELOAD) && PIONEER_DRIVER_BUILD_TAG == 0x2026102400010056ULL && (!PIONEER_Q38_DECODER_ROLE_TAG_OK || !defined(PIONEER_Q38_RESIDUAL_RMS_NATIVE))
#error Reserved tag56 identifies ONLY SAME-owner QRR1 post-attention execution,NO public selector
#endif
#if defined(TT_PIONEER_DRIVER_RELOAD) && PIONEER_DRIVER_BUILD_TAG == 0x2026102400010057ULL && (!PIONEER_Q38_DECODER_ROLE_TAG_OK || !defined(PIONEER_Q38_FFN_RESIDUAL_NATIVE))
#error Reserved tag57 identifies ONLY SAME-owner QFG3 full FFN+residual execution,NO public selector
#endif
#if defined(TT_PIONEER_DRIVER_RELOAD) && PIONEER_DRIVER_BUILD_TAG == 0x2026102400010058ULL && (!PIONEER_Q38_DECODER_ROLE_TAG_OK || !defined(PIONEER_Q38_NEXT_RMS_NATIVE))
#error Reserved tag58 identifies ONLY SAME-owner next-layer QRN1,NO public selector
#endif
#if defined(TT_PIONEER_DRIVER_RELOAD) && PIONEER_DRIVER_BUILD_TAG == 0x2026102400010059ULL && (!PIONEER_Q38_DECODER_ROLE_TAG_OK || !defined(PIONEER_Q38_NEXT_PROJECTION_NATIVE))
#error Reserved tag59 identifies ONLY SAME-owner QPS2 next-layer projections,NO public selector
#endif
#if defined(TT_PIONEER_DRIVER_RELOAD) && PIONEER_DRIVER_BUILD_TAG == 0x2026102400010060ULL && (!PIONEER_Q38_DECODER_ROLE_TAG_OK || !defined(PIONEER_Q38_NEXT_CONV_NATIVE))
#error Reserved tag60 identifies ONLY SAME-owner next-layer QCB1,NO public selector
#endif
#if defined(TT_PIONEER_DRIVER_RELOAD) && PIONEER_DRIVER_BUILD_TAG == 0x2026102400010061ULL && (!PIONEER_Q38_DECODER_ROLE_TAG_OK || !defined(PIONEER_Q38_NEXT_GDN_NATIVE))
#error Reserved tag61 identifies ONLY SAME-owner next-layer QGT1,NO public selector
#endif
#if defined(TT_PIONEER_DRIVER_RELOAD) && PIONEER_DRIVER_BUILD_TAG == 0x2026102400010062ULL && (!PIONEER_Q38_DECODER_ROLE_TAG_OK || !defined(PIONEER_Q38_NEXT_GDN_OUTPUT_NATIVE))
#error Reserved tag62 identifies ONLY SAME-owner next-layer QOP2,NO public selector
#endif
#if defined(TT_PIONEER_DRIVER_RELOAD) && PIONEER_DRIVER_BUILD_TAG == 0x2026102400010063ULL && (!PIONEER_Q38_DECODER_ROLE_TAG_OK || !defined(PIONEER_Q38_NEXT_RESIDUAL_RMS_NATIVE))
#error Reserved tag63 identifies ONLY SAME-owner next-layer QRR2,NO public selector
#endif
#if defined(TT_PIONEER_DRIVER_RELOAD) && PIONEER_DRIVER_BUILD_TAG == 0x2026102400010064ULL && (!PIONEER_Q38_DECODER_ROLE_TAG_OK || !defined(PIONEER_Q38_CONV_SILU_PRECISION_NATIVE))
#error Reserved tag64 identifies ONLY separate precise-convolution timed DEBUG
#endif
#if defined(TT_PIONEER_DRIVER_RELOAD) && PIONEER_DRIVER_BUILD_TAG == 0x2026102500010065ULL && (!PIONEER_Q38_DECODER_ROLE_TAG_OK || !defined(PIONEER_Q38_NEXT_FFN_RESIDUAL_NATIVE))
#error Reserved tag65 identifies ONLY SAME-owner layer1 FFN,NO public selector
#endif
#if defined(TT_PIONEER_DRIVER_RELOAD) && PIONEER_DRIVER_BUILD_TAG == 0x2026102500010066ULL && (!PIONEER_Q38_DECODER_ROLE_TAG_OK || !defined(PIONEER_Q38_RESIDENT_SILU_PRECISION_NATIVE))
#error Reserved tag66 identifies ONLY SAME-owner precise resident continuation,NO public selector
#endif
#ifdef PIONEER_Q38_PROJECTION_VALIDATION_NATIVE
#if PIONEER_Q38_PROJECTION_VALIDATION_NATIVE != 1 || !defined(PIONEER_Q38_PROJECTION_RESULT_STASH) || PIONEER_Q38_PROJECTION_RESULT_STASH != 1 || !defined(PIONEER_Q38_PROJECTION_EXECUTION_OWNER) || PIONEER_Q38_PROJECTION_EXECUTION_OWNER != 1 || !defined(PIONEER_Q38_LEARNED_RMS_NATIVE) || PIONEER_Q38_LEARNED_RMS_NATIVE != 1 || !defined(PIONEER_Q38_EMBEDDING_NATIVE) || PIONEER_Q38_EMBEDDING_NATIVE != 1 || !defined(TT_PIONEER_DRIVER_RELOAD) || PIONEER_DRIVER_BUILD_TAG != 0x2026102200010042ULL || defined(PIONEER_Q38_EMBEDDING_QUALIFICATION) || defined(PIONEER_Q38_EMBEDDING_VALIDATION_NATIVE) || defined(PIONEER_Q38_LEARNED_RMS_VALIDATION_NATIVE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_FFN_HOST_PROFILE) || defined(PIONEER_Q38_FFN_SEALED_PROOF)
#error QPS1 DEBUG requires separate exact tag42 native execution/RMS role,never old/public/Work/CPU-reference promotion
#endif
#define PIONEER_Q38_PROJECTION_ROLE_TAG_OK 1
#include "PioneerQ38ProjectionValidationProtocol.h"
#else
#define PIONEER_Q38_PROJECTION_ROLE_TAG_OK 0
#endif
#if defined(TT_PIONEER_DRIVER_RELOAD) && PIONEER_DRIVER_BUILD_TAG == 0x2026102200010042ULL && !PIONEER_Q38_PROJECTION_ROLE_TAG_OK
#error Reserved tag42 is ONLY separate fixed QPS1 retained-result DEBUG qualification
#endif
#ifdef PIONEER_Q38_PROJECTION_RESULT_STASH
#if PIONEER_Q38_PROJECTION_RESULT_STASH != 1 || !defined(PIONEER_Q38_PROJECTION_EXECUTION_OWNER) || PIONEER_Q38_PROJECTION_EXECUTION_OWNER != 1 || !defined(PIONEER_Q38_LEARNED_RMS_NATIVE) || PIONEER_Q38_LEARNED_RMS_NATIVE != 1 || !defined(PIONEER_Q38_EMBEDDING_NATIVE) || PIONEER_Q38_EMBEDDING_NATIVE != 1 || defined(PIONEER_Q38_EMBEDDING_QUALIFICATION) || defined(PIONEER_Q38_EMBEDDING_VALIDATION_NATIVE) || defined(PIONEER_Q38_LEARNED_RMS_VALIDATION_NATIVE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_WORK_INFERENCE)
#error Fixed QPS1 result stashes require exact private execution/RMS owner,never old/public/CPU-reference promotion
#endif
#endif
#ifdef PIONEER_Q38_LEARNED_RMS_VALIDATION_NATIVE
#if PIONEER_Q38_LEARNED_RMS_VALIDATION_NATIVE != 1 || !defined(PIONEER_Q38_LEARNED_RMS_NATIVE) || PIONEER_Q38_LEARNED_RMS_NATIVE != 1 || !defined(PIONEER_Q38_EMBEDDING_NATIVE) || PIONEER_Q38_EMBEDDING_NATIVE != 1 || !defined(PIONEER_Q38_EMBEDDING_QUALIFICATION) || PIONEER_Q38_EMBEDDING_QUALIFICATION != 1 || !defined(TT_PIONEER_DRIVER_RELOAD) || PIONEER_DRIVER_BUILD_TAG != 0x2026102200010041ULL || defined(PIONEER_Q38_EMBEDDING_VALIDATION_NATIVE) || defined(PIONEER_Q38_PROJECTION_EXECUTION_OWNER) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_WORK_INFERENCE)
#error Learned RMS GDDR DEBUG validation requires exact isolated tag41 native role,never Work/projection/CPU-reference kernel
#endif
#endif
#ifdef PIONEER_Q38_PROJECTION_EXECUTION_OWNER
#if PIONEER_Q38_PROJECTION_EXECUTION_OWNER != 1 || !defined(PIONEER_Q38_LEARNED_RMS_NATIVE) || PIONEER_Q38_LEARNED_RMS_NATIVE != 1 || !defined(PIONEER_Q38_EMBEDDING_NATIVE) || PIONEER_Q38_EMBEDDING_NATIVE != 1 || defined(PIONEER_Q38_PROJECTION_GRAPH_MEMORY) || defined(PIONEER_Q38_EMBEDDING_QUALIFICATION) || defined(PIONEER_Q38_EMBEDDING_VALIDATION_NATIVE)
#error Private native projection EXECUTION requires exact RMS source;not old held-only/embedding qualification promotion
#endif
#endif
#ifdef PIONEER_Q38_LEARNED_RMS_NATIVE
#if PIONEER_Q38_LEARNED_RMS_NATIVE != 1 || !defined(PIONEER_Q38_EMBEDDING_NATIVE) || PIONEER_Q38_EMBEDDING_NATIVE != 1 || (defined(PIONEER_Q38_EMBEDDING_QUALIFICATION) && !defined(PIONEER_Q38_LEARNED_RMS_VALIDATION_NATIVE)) || defined(PIONEER_Q38_EMBEDDING_VALIDATION_NATIVE)
#error Private learned RMS adapter ONLY;no old embedding qualification/public selector promotion
#endif
#endif
#ifdef PIONEER_Q38_LEARNED_RMS_VALIDATION_NATIVE
#define PIONEER_Q38_EMBEDDING_EXPECTED_TAG 0x2026102200010041ULL
#elif defined(PIONEER_Q38_EMBEDDING_VALIDATION_NATIVE)
#if PIONEER_Q38_EMBEDDING_VALIDATION_NATIVE != 1 || !defined(PIONEER_Q38_EMBEDDING_QUALIFICATION) || PIONEER_Q38_EMBEDDING_QUALIFICATION != 1 || PIONEER_DRIVER_BUILD_TAG != 0x2026102200010040ULL
#error GDDR embedding validation needs its exact isolated tag40 DEBUG role
#endif
#define PIONEER_Q38_EMBEDDING_EXPECTED_TAG 0x2026102200010040ULL
#else
#define PIONEER_Q38_EMBEDDING_EXPECTED_TAG 0x2026102200010039ULL
#endif
#ifdef PIONEER_Q38_EMBEDDING_QUALIFICATION
#if PIONEER_Q38_EMBEDDING_QUALIFICATION != 1 || !defined(PIONEER_Q38_EMBEDDING_NATIVE) || PIONEER_Q38_EMBEDDING_NATIVE != 1 || !defined(TT_PIONEER_DRIVER_RELOAD) || PIONEER_DRIVER_BUILD_TAG != PIONEER_Q38_EMBEDDING_EXPECTED_TAG || defined(PIONEER_Q38_FFN_HOST_PROFILE) || defined(PIONEER_Q38_FFN_SEALED_PROOF)
#error Exact isolated tag39 embedding qualification role required,not old FFN/profile/model promotion
#endif
#define PIONEER_Q38_EMBEDDING_ROLE_TAG_OK 1
#include "PioneerQ38EmbeddingProtocol.h"
#include "PioneerQ38LearnedRmsProtocol.h"
#else
#define PIONEER_Q38_EMBEDDING_ROLE_TAG_OK 0
#endif
#if defined(TT_PIONEER_DRIVER_RELOAD) && PIONEER_DRIVER_BUILD_TAG == 0x2026102200010041ULL && (!defined(PIONEER_Q38_LEARNED_RMS_VALIDATION_NATIVE) || !PIONEER_Q38_EMBEDDING_ROLE_TAG_OK)
#error Reserved tag41 is ONLY separate fixed normalized/residual learned RMS DEBUG qualification
#endif
#if defined(TT_PIONEER_DRIVER_RELOAD) && PIONEER_DRIVER_BUILD_TAG == 0x2026102200010040ULL && (!defined(PIONEER_Q38_EMBEDDING_VALIDATION_NATIVE) || !PIONEER_Q38_EMBEDDING_ROLE_TAG_OK)
#error Reserved tag40 is ONLY the separate fixed GDDR embedding numerical DEBUG qualification
#endif
#if defined(TT_PIONEER_DRIVER_RELOAD) && PIONEER_DRIVER_BUILD_TAG == 0x2026102200010039ULL && !PIONEER_Q38_EMBEDDING_ROLE_TAG_OK
#error Reserved tag39 is ONLY the separately scoped embedding arithmetic qualification
#endif
#ifdef PIONEER_Q38_EMBEDDING_NATIVE
#if PIONEER_Q38_EMBEDDING_NATIVE != 1 || !defined(PIONEER_Q38_FFN_NATIVE_GRAPH) || !defined(PIONEER_Q38_GDN_MUTABLE_STATE) || !defined(TT_PIONEER_EXPERIMENTAL_Q38_STREAM) || defined(PIONEER_Q38_PROJECTION_GRAPH_MEMORY)
#error Embedding native adapter needs exact retained context/complete panel,NOT old memory role;NO public execution selector
#endif
#include "PioneerQ38EmbeddingNativeState.h"
#endif
#ifdef PIONEER_Q38_RESIDENT_OUTPUT_OWNER
#include "PioneerQ38ResidentOutputNativeState.h"
#endif
#ifdef PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT
#include "PioneerQ38ResidentLayerNativeState.h"
#endif
#ifdef PIONEER_Q38_RESIDENT_OUTPUT_VALIDATION_NATIVE
#include "PioneerQ38ResidentOutputLearned.h"
#endif
#ifdef PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE
#include "PioneerQ38ResidentDecoderValidationProtocol.h"
#include "PioneerQ38ProjectionValidationProtocol.h"
#endif
#ifdef PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE
#include "PioneerQ38GdnRecurrentValidationProtocol.h"
#endif
#ifdef PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE
#include "PioneerQ38GdnRendezvousValidationProtocol.h"
#endif
#ifdef PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE
#include "PioneerQ38GdnOutputValidationProtocol.h"
#endif
#ifdef PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE
#include "PioneerQ38GdnInputValidationProtocol.h"
#include "PioneerQ38ResidentDecoderValidationProtocol.h"
#include "PioneerQ38ProjectionValidationProtocol.h"
#endif
#ifdef PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE
#include "PioneerQ38GdnInputValidationProtocol.h"
#include "PioneerQ38ResidentDecoderValidationProtocol.h"
#include "PioneerQ38ProjectionValidationProtocol.h"
#endif
#ifdef PIONEER_Q38_FFN_SEALED_PROOF
#if PIONEER_Q38_FFN_SEALED_PROOF != 1 || !defined(PIONEER_Q38_FFN_HOST_PROFILE) || PIONEER_DRIVER_BUILD_TAG != 0x2026102200010038ULL || defined(PIONEER_Q38_FFN_SEALED_PROOF_CLIENT)
#error Sealed manifest proof requires its exact isolated measured native tag38 role
#endif
#endif
#if defined(TT_PIONEER_DRIVER_RELOAD) && PIONEER_DRIVER_BUILD_TAG == 0x2026102200010038ULL && !defined(PIONEER_Q38_FFN_SEALED_PROOF)
#error Reserved tag38 identifies ONLY measured sealed manifest proof
#endif
#ifdef PIONEER_Q38_FFN_HOST_PROFILE
#if PIONEER_Q38_FFN_HOST_PROFILE != 1 || !defined(PIONEER_Q38_FFN_NATIVE_GRAPH) || !defined(TT_PIONEER_DRIVER_RELOAD) || (PIONEER_DRIVER_BUILD_TAG != 0x2026102200010037ULL && !(defined(PIONEER_Q38_FFN_SEALED_PROOF) && PIONEER_DRIVER_BUILD_TAG == 0x2026102200010038ULL)) || defined(PIONEER_Q38_FFN_HOST_PROFILE_CLIENT)
#error Host-only full FFN phase profiling requires its exact isolated native tag37 role
#endif
#endif
#if defined(TT_PIONEER_DRIVER_RELOAD) && PIONEER_DRIVER_BUILD_TAG == 0x2026102200010037ULL && !defined(PIONEER_Q38_FFN_HOST_PROFILE)
#error Reserved tag37 identifies ONLY host-phase profiling of the qualified full FFN path
#endif
#if defined(PIONEER_Q38_FFN_PERSISTENT_GRAPH) && !defined(PIONEER_Q38_FFN_NATIVE_GRAPH)
#error Persistent full FFN owner is OFFLINE until its separate actual native model/input/clock/host adapter is qualified
#endif
#ifdef PIONEER_Q38_FFN_NATIVE_GRAPH
#if PIONEER_Q38_FFN_NATIVE_GRAPH != 1 || !defined(PIONEER_Q38_FFN_PERSISTENT_GRAPH) || PIONEER_Q38_FFN_PERSISTENT_GRAPH != 1 || !defined(PIONEER_Q38_FFN_GRAPH_INPUT_ROOT) || PIONEER_Q38_FFN_GRAPH_INPUT_ROOT != 1 || !defined(PIONEER_Q38_FFN_MODEL_INIT) || PIONEER_Q38_FFN_MODEL_INIT != 1 || !defined(TT_PIONEER_DRIVER_RELOAD) || (PIONEER_DRIVER_BUILD_TAG != 0x2026102200010036ULL && !PIONEER_Q38_EMBEDDING_ROLE_TAG_OK && !PIONEER_Q38_PROJECTION_ROLE_TAG_OK && !PIONEER_Q38_DECODER_ROLE_TAG_OK && !defined(PIONEER_Q38_RESIDENT_OUTPUT_CONTEXT_COMPILE_ONLY) && !defined(PIONEER_Q38_RESIDENT_LAYER_CONTEXT_COMPILE_ONLY) && !defined(PIONEER_Q38_MIXED_LIVE_NATIVE_CONTEXT) && !PIONEER_Q38_RECURRENT_BLOCK_NATIVE_ROLE_TAG_OK && !PIONEER_Q38_RECURRENT_PREFIX_NATIVE_ROLE_TAG_OK && !PIONEER_Q38_RESIDENT_OUTPUT_PHYSICAL_ROLE_TAG_OK && !PIONEER_Q38_RESIDENT_LAYER_PHYSICAL_ROLE_TAG_OK && !(defined(PIONEER_Q38_FFN_HOST_PROFILE) && (PIONEER_DRIVER_BUILD_TAG == 0x2026102200010037ULL || (defined(PIONEER_Q38_FFN_SEALED_PROOF) && PIONEER_DRIVER_BUILD_TAG == 0x2026102200010038ULL)))) || defined(PIONEER_Q38_FFN_CODE_DIAGNOSTIC) || defined(PIONEER_Q38_FFN_ROOT_LIFETIME_DIAGNOSTIC) || defined(PIONEER_Q38_FFN_PEER_LIFETIME_DIAGNOSTIC)
#error Persistent full FFN native execution requires its separate exact tag36 model-bound role
#endif
#include "PioneerQ38FfnPersistentGraphOwner.h"
#include "PioneerQ38FfnGraphProtocol.h"
#endif
#ifdef PIONEER_Q38_PROJECTION_GRAPH_MEMORY
#if PIONEER_Q38_PROJECTION_GRAPH_MEMORY != 1 || !defined(PIONEER_Q38_FFN_NATIVE_GRAPH) || !defined(TT_PIONEER_EXPERIMENTAL_Q38_STREAM)
#error Private projection memory adapter requires the retained model-bound context and complete panel;NO native execution selector
#endif
#include "PioneerQ38ProjectionGraphMemory.h"
#endif
#if defined(TT_PIONEER_DRIVER_RELOAD) && PIONEER_DRIVER_BUILD_TAG == 0x2026102200010036ULL && !defined(PIONEER_Q38_FFN_NATIVE_GRAPH)
#error Reserved tag36 identifies ONLY the persistent full FFN native adapter
#endif
#ifdef PIONEER_Q38_FFN_MODEL_INIT
#if PIONEER_Q38_FFN_MODEL_INIT != 1 || !defined(PIONEER_Q38_GDN_MUTABLE_STATE) || !defined(PIONEER_Q38_NORMAL_CLOCK) || !defined(PIONEER_Q38_SPAN_PRELOAD) || !defined(PIONEER_Q38_SPAN_FAST) || !defined(PIONEER_Q38_LOCAL_DELTA_PROBE) || !defined(TT_PIONEER_DRIVER_RELOAD) || (PIONEER_DRIVER_BUILD_TAG != 0x2026102200010035ULL && !PIONEER_Q38_EMBEDDING_ROLE_TAG_OK && !PIONEER_Q38_PROJECTION_ROLE_TAG_OK && !PIONEER_Q38_DECODER_ROLE_TAG_OK && !defined(PIONEER_Q38_RESIDENT_OUTPUT_CONTEXT_COMPILE_ONLY) && !defined(PIONEER_Q38_RESIDENT_LAYER_CONTEXT_COMPILE_ONLY) && !defined(PIONEER_Q38_MIXED_LIVE_NATIVE_CONTEXT) && !PIONEER_Q38_RECURRENT_BLOCK_NATIVE_ROLE_TAG_OK && !PIONEER_Q38_RECURRENT_PREFIX_NATIVE_ROLE_TAG_OK && !PIONEER_Q38_RESIDENT_OUTPUT_PHYSICAL_ROLE_TAG_OK && !PIONEER_Q38_RESIDENT_LAYER_PHYSICAL_ROLE_TAG_OK && !(defined(PIONEER_Q38_FFN_NATIVE_GRAPH) && (PIONEER_DRIVER_BUILD_TAG == 0x2026102200010036ULL || (defined(PIONEER_Q38_FFN_HOST_PROFILE) && (PIONEER_DRIVER_BUILD_TAG == 0x2026102200010037ULL || (defined(PIONEER_Q38_FFN_SEALED_PROOF) && PIONEER_DRIVER_BUILD_TAG == 0x2026102200010038ULL)))))) || defined(PIONEER_Q38_GDN_ZERO_DIAGNOSTIC) || defined(PIONEER_Q38_RESIDENT_GDN_ROOT_DIAGNOSTIC) || defined(PIONEER_Q38_FFN_CODE_DIAGNOSTIC) || defined(PIONEER_Q38_GRAPH_CSR_DIAGNOSTIC) || defined(TT_PIONEER_Q38_DISPATCH_PROFILE)
#error Canonical FFN model init requires its separate exact tag35 retained-owner role,not old GDN/code/Work promotion
#endif
#include "PioneerQ38GraphMutableBanks.h"
#include "PioneerQ38FfnZeroController.h"
#include "PioneerQ38FfnModelProtocol.h"
#endif
#if defined(TT_PIONEER_DRIVER_RELOAD) && PIONEER_DRIVER_BUILD_TAG == 0x2026102200010035ULL && !defined(PIONEER_Q38_FFN_MODEL_INIT)
#error Reserved tag35 identifies ONLY canonical FFN model initialization
#endif
#ifdef PIONEER_Q38_FFN_ROOT_LIFETIME_DIAGNOSTIC
#if PIONEER_Q38_FFN_ROOT_LIFETIME_DIAGNOSTIC != 1 || !defined(PIONEER_Q38_FFN_GRAPH_INPUT_ROOT) || PIONEER_Q38_FFN_GRAPH_INPUT_ROOT != 1 || !defined(PIONEER_Q38_FFN_CODE_DIAGNOSTIC) || !defined(TT_PIONEER_DRIVER_RELOAD) || PIONEER_DRIVER_BUILD_TAG != 0x2026102200010034ULL || defined(PIONEER_Q38_FFN_PEER_LIFETIME_DIAGNOSTIC) || defined(PIONEER_Q38_FFN_ROOT_LIFETIME_REFERENCE_CLIENT) || defined(PIONEER_Q38_FFN_PEER_LIFETIME_REFERENCE_CLIENT)
#error New ROOT execution lifetime requires separate exact-tag34 native STOP-only diagnostic ONLY
#endif
#include "PioneerQ38FfnRootLifetimeOwner.h"
#endif
#if defined(PIONEER_Q38_FFN_GRAPH_INPUT_ROOT) && !defined(PIONEER_Q38_FFN_ROOT_LIFETIME_DIAGNOSTIC) && !defined(PIONEER_Q38_FFN_NATIVE_GRAPH)
#error QFG2 input-root code requires the isolated native ROOT lifetime diagnostic,not model/Work promotion
#endif
#if defined(TT_PIONEER_DRIVER_RELOAD) && PIONEER_DRIVER_BUILD_TAG == 0x2026102200010034ULL && !defined(PIONEER_Q38_FFN_ROOT_LIFETIME_DIAGNOSTIC)
#error Reserved tag34 must identify ONLY the new ROOT execution-lifetime diagnostic
#endif
#ifdef PIONEER_Q38_FFN_PEER_LIFETIME_DIAGNOSTIC
#if PIONEER_Q38_FFN_PEER_LIFETIME_DIAGNOSTIC != 1 || !defined(PIONEER_Q38_FFN_CODE_DIAGNOSTIC) || !defined(TT_PIONEER_DRIVER_RELOAD) || PIONEER_DRIVER_BUILD_TAG != 0x2026102200010033ULL || defined(PIONEER_Q38_FFN_PEER_LIFETIME_REFERENCE_CLIENT)
#error New worker execution lifetime requires separate exact-tag33 native diagnostic ONLY
#endif
#include "PioneerQ38FfnPeerLifetimeOwner.h"
#endif
#if defined(TT_PIONEER_DRIVER_RELOAD) && PIONEER_DRIVER_BUILD_TAG == 0x2026102200010033ULL && !defined(PIONEER_Q38_FFN_PEER_LIFETIME_DIAGNOSTIC)
#error Reserved tag33 must identify ONLY the new worker execution-lifetime diagnostic
#endif
#ifdef PIONEER_Q38_FFN_CODE_DIAGNOSTIC
#if PIONEER_Q38_FFN_CODE_DIAGNOSTIC != 1 || !defined(TT_PIONEER_DRIVER_RELOAD) || (PIONEER_DRIVER_BUILD_TAG != 0x2026102200010032ULL && !(defined(PIONEER_Q38_FFN_PEER_LIFETIME_DIAGNOSTIC) && PIONEER_DRIVER_BUILD_TAG == 0x2026102200010033ULL) && !(defined(PIONEER_Q38_FFN_ROOT_LIFETIME_DIAGNOSTIC) && PIONEER_DRIVER_BUILD_TAG == 0x2026102200010034ULL)) || !defined(TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH) || !defined(PIONEER_Q38_NORMAL_CLOCK) || !defined(PIONEER_Q38_SENTINEL_POLL_QUALIFICATION) || !defined(PIONEER_Q38_DIRECT_SRCB_QUALIFICATION) || defined(PIONEER_Q38_GRAPH_CSR_DIAGNOSTIC) || defined(PIONEER_Q38_GDN_MUTABLE_STATE) || defined(TT_PIONEER_Q38_DISPATCH_PROFILE)
#error Held-only FFN code ownership requires separate exact-tag32 legacy direct/sentinel context ONLY
#endif
#include "PioneerQ38FfnGraphCodeOwner.h"
#endif
#if defined(TT_PIONEER_DRIVER_RELOAD) && PIONEER_DRIVER_BUILD_TAG == 0x2026102200010032ULL && !defined(PIONEER_Q38_FFN_CODE_DIAGNOSTIC)
#error Reserved tag32 must identify ONLY the never-launched FFN code ownership diagnostic
#endif
#ifdef PIONEER_Q38_GRAPH_CSR_DIAGNOSTIC
#if !defined(TT_PIONEER_DRIVER_RELOAD) || PIONEER_DRIVER_BUILD_TAG != 0x2026102200010031ULL || !defined(TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH) || !defined(PIONEER_Q38_NORMAL_CLOCK) || !defined(PIONEER_Q38_SENTINEL_POLL_QUALIFICATION) || !defined(PIONEER_Q38_DIRECT_SRCB_QUALIFICATION) || defined(PIONEER_Q38_LOCAL_Q4_QUALIFICATION) || defined(PIONEER_Q38_GDN_MUTABLE_STATE) || defined(TT_PIONEER_Q38_DISPATCH_PROFILE)
#error Fixed remote CSR qualification requires separate exact-tag31 legacy direct/sentinel context ONLY
#endif
#include "PioneerQ38GraphCsrProbe.h"
#endif
#if defined(TT_PIONEER_DRIVER_RELOAD) && PIONEER_DRIVER_BUILD_TAG == 0x2026102200010031ULL && !defined(PIONEER_Q38_GRAPH_CSR_DIAGNOSTIC)
#error Reserved tag31 must identify ONLY the fixed remote CSR diagnostic
#endif
#ifdef PIONEER_Q38_LOCAL_Q4_QUALIFICATION
#if !defined(TT_PIONEER_DRIVER_RELOAD) || PIONEER_DRIVER_BUILD_TAG != 0x2026102100010028ULL || !defined(PIONEER_Q38_NORMAL_CLOCK) || !defined(PIONEER_Q38_ROOT_CYCLE_PROFILE) || !defined(PIONEER_Q38_SENTINEL_POLL_QUALIFICATION) || !defined(PIONEER_Q38_DIRECT_SRCB_QUALIFICATION) || defined(TT_PIONEER_Q38_DISPATCH_PROFILE)
#error Local Q4 requires separate exact-tag28 retained-owner original-TC qualification ONLY
#endif
#endif
#if defined(TT_PIONEER_DRIVER_RELOAD) && PIONEER_DRIVER_BUILD_TAG == 0x2026102100010028ULL && !defined(PIONEER_Q38_LOCAL_Q4_QUALIFICATION)
#error Reserved tag28 must identify ONLY the exact block-local Q4 image qualification
#endif
#ifdef PIONEER_Q38_WORK_LOCAL_Q4
#error Local Q4 is OFFLINE peer ONLY;NO native driver/source/layout qualification
#endif
#ifdef PIONEER_Q38_Q4_DIRECT_QUALIFICATION
#if !defined(PIONEER_Q38_Q4_NIBBLE_QUALIFICATION) || !defined(TT_PIONEER_DRIVER_RELOAD) || PIONEER_DRIVER_BUILD_TAG != 0x2026102100010027ULL
#error Direct Q4 requires separate exact-tag27 retained-owner qualification ONLY
#endif
#endif
#ifdef PIONEER_Q38_WORK_Q4_DIRECT_DECODE
#error Direct Q4 decoder is OFFLINE peer ONLY;NO native driver qualification
#endif
#ifdef PIONEER_Q38_SIGNED_MAG2_QUALIFICATION
#error Signed magnitude2 image selection is OFFLINE registry proof ONLY;NO native driver qualification yet
#endif
#if defined(TT_PIONEER_DRIVER_RELOAD) && PIONEER_DRIVER_BUILD_TAG == 0x2026102100010027ULL && !defined(PIONEER_Q38_Q4_DIRECT_QUALIFICATION)
#error Reserved tag27 must identify ONLY the exact direct Q4 image qualification
#endif
#if defined(TT_PIONEER_DRIVER_RELOAD) && PIONEER_DRIVER_BUILD_TAG == 0x2026102100010026ULL && (!defined(PIONEER_Q38_Q4_NIBBLE_QUALIFICATION) || defined(PIONEER_Q38_Q4_DIRECT_QUALIFICATION))
#error Reserved tag26 must identify ONLY the exact Q4 nibble image qualification
#endif
#ifdef PIONEER_Q38_Q4_NIBBLE_QUALIFICATION
#if !defined(TT_PIONEER_DRIVER_RELOAD) || (!defined(PIONEER_Q38_Q4_DIRECT_QUALIFICATION) && PIONEER_DRIVER_BUILD_TAG != 0x2026102100010026ULL) || !defined(PIONEER_Q38_NORMAL_CLOCK) || !defined(PIONEER_Q38_ROOT_CYCLE_PROFILE) || !defined(PIONEER_Q38_SENTINEL_POLL_QUALIFICATION) || !defined(PIONEER_Q38_DIRECT_SRCB_QUALIFICATION) || defined(TT_PIONEER_Q38_DISPATCH_PROFILE)
#error Q4 nibble requires separate exact-tag26 retained-owner qualification, not ordinary Work/model promotion
#endif
#endif

#include <KernelExport.h>
#include <ByteOrder.h>
#include <PCI.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <new>
#include <vm/vm.h>
#ifdef PIONEER_Q38_PEER_PHASE_QUALIFICATION
#if !defined(TT_PIONEER_DRIVER_RELOAD) || PIONEER_DRIVER_BUILD_TAG != 0x2026102100010025ULL || !defined(PIONEER_Q38_NORMAL_CLOCK) || !defined(PIONEER_Q38_ROOT_CYCLE_PROFILE) || !defined(PIONEER_Q38_SENTINEL_POLL_QUALIFICATION) || defined(TT_PIONEER_Q38_DISPATCH_PROFILE)
#error Private timing DATA requires separate exact-tag25 retained-owner qualification,not ordinary Work
#endif
#endif
#ifdef PIONEER_Q38_COMMAND_REUSE_QUALIFICATION
#if !defined(TT_PIONEER_DRIVER_RELOAD) || PIONEER_DRIVER_BUILD_TAG != 0x2026102100010024ULL
#error Command reuse requires separate exact-tag24 retained-owner qualifier
#endif
#endif
#ifdef PIONEER_Q38_TILE_PREFETCH_QUALIFICATION
#if !defined(TT_PIONEER_DRIVER_RELOAD) || PIONEER_DRIVER_BUILD_TAG != 0x2026102100010023ULL
#error Tile prefetch requires its separate exact-tag23 retained-owner qualifier
#endif
#endif
#ifdef PIONEER_Q38_RAW_UNPACK_CACHE_QUALIFICATION
#if !defined(TT_PIONEER_DRIVER_RELOAD) || PIONEER_DRIVER_BUILD_TAG != 0x2026102100010022ULL
#error Raw-Dst cache native qualifier requires separate exact-tag22 retained-owner build
#endif
#endif
#ifdef PIONEER_Q38_FRONTEND_BARRIER_QUALIFICATION
#if !defined(TT_PIONEER_DRIVER_RELOAD) || PIONEER_DRIVER_BUILD_TAG != 0x2026102100010021ULL
#error Frontend-barrier native qualifier requires separate exact-tag21 retained-owner build
#endif
#endif
#ifdef PIONEER_Q38_BATCH4_FETCH_QUALIFICATION
#ifdef PIONEER_Q38_BATCH_RAW_QUALIFICATION
#if !defined(TT_PIONEER_DRIVER_RELOAD) || PIONEER_DRIVER_BUILD_TAG != 0x2026102100010020ULL
#error Direct batch input native qualifier requires its separate exact-tag20 retained-owner build
#endif
#else
#if !defined(TT_PIONEER_DRIVER_RELOAD) || PIONEER_DRIVER_BUILD_TAG != 0x2026102100010019ULL
#error Batch4 native qualifier requires its separate exact-tag19 retained-owner build
#endif
#endif
#endif
#ifdef PIONEER_Q38_HOST_PHASE_PROFILE
#if !defined(TT_PIONEER_Q38_DISPATCH_PROFILE) || !defined(PIONEER_Q38_SENTINEL_POLL_QUALIFICATION) || !defined(PIONEER_Q38_ROOT_CYCLE_PROFILE) || !defined(PIONEER_Q38_NORMAL_CLOCK) || !defined(TT_PIONEER_DRIVER_RELOAD) || PIONEER_DRIVER_BUILD_TAG != 0x2026102100010018ULL
#error Host phase diagnostic requires its separate exact-tag18 sentinel/root/normal-clock qualification
#endif
#include "PioneerQ38HostPhaseProfile.h"
#endif
#ifdef PIONEER_Q38_GDN_MUTABLE_STATE
#include "PioneerQ38StateZeroController.h"
#include "PioneerQ38NonRootHeld.h"
#include "PioneerQ38GdnFreshPlan.h"
#include "PioneerQ38InitializeOwnedState.h"
#if !defined(PIONEER_Q38_NORMAL_CLOCK) || !defined(PIONEER_Q38_LOCAL_DELTA_PROBE) || !defined(PIONEER_Q38_SPAN_PRELOAD)
#error Private mutable adapter requires the complete retained-owner normal-clock bank driver
#endif
#endif
#ifdef PIONEER_Q38_GDN_ZERO_DIAGNOSTIC
#ifndef PIONEER_Q38_GDN_MUTABLE_STATE
#error Fixed zero diagnostic requires the complete private mutable-state owner
#endif
#include "PioneerQ38GdnZeroProtocol.h"
#endif
#ifdef PIONEER_Q38_RESIDENT_GDN_ROOT_DIAGNOSTIC
#if !defined(PIONEER_Q38_GDN_MUTABLE_STATE) || !defined(PIONEER_Q38_GDN_ZERO_DIAGNOSTIC)
#error Resident ROOT0 qualification requires the complete fixed zero diagnostic owner
#endif
#define PIONEER_Q38_RGDN_ROOT_QUALIFICATION
#include "PioneerQ38ResidentGdnRootController.h"
#endif
#ifdef PIONEER_Q38_NORMAL_CLOCK
#if (!PIONEER_Q38_MIXED_LIVE_PHYSICAL_ROLE_TAG_OK && !PIONEER_Q38_RECURRENT_BLOCK_NATIVE_ROLE_TAG_OK && !PIONEER_Q38_RECURRENT_PREFIX_NATIVE_ROLE_TAG_OK && (!defined(PIONEER_Q38_WORK_SPARSE_EXPORT) || !defined(TT_PIONEER_Q38_WORK_MODE))) || !defined(TT_PIONEER_DRIVER_RELOAD) || !defined(PIONEER_Q38_SPAN_PRELOAD)
#error Normal Q38 clocks require the retained-owner reloadable sparse Work driver
#endif
#include "PioneerQ38ClockLease.h"
#endif
#if defined(PIONEER_Q38_LOCAL_DELTA_PROBE) && (!defined(TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR) || !defined(TT_PIONEER_DRIVER_RELOAD) || (!PIONEER_Q38_MIXED_LIVE_PHYSICAL_ROLE_TAG_OK && !PIONEER_Q38_RECURRENT_BLOCK_NATIVE_ROLE_TAG_OK && !PIONEER_Q38_RECURRENT_PREFIX_NATIVE_ROLE_TAG_OK && !defined(TT_PIONEER_Q38_WORK_MODE)))
#error LOCAL vector delta probe requires the retained-owner reloadable vector Work driver
#endif
#if defined(PIONEER_Q38_LOCAL_DELTA_GGML) && (!defined(PIONEER_Q38_LOCAL_DELTA_PROBE) || !defined(PIONEER_Q38_NORMAL_CLOCK))
#error Post-dot LOCAL GDN candidate requires the complete normal-clock local probe driver
#endif
#if defined(TT_PIONEER_EXPERIMENTAL_Q38_PANEL) && !defined(TT_PIONEER_EXPERIMENTAL_Q38_BANKS)
#error Q38 local panel probe requires the exclusive seven-bank owner
#endif
#if defined(TT_PIONEER_EXPERIMENTAL_Q38_AFFINE) && !defined(TT_PIONEER_EXPERIMENTAL_Q38_PANEL)
#error Q38 affine probe requires the guarded local panel protocol
#endif
#if defined(TT_PIONEER_EXPERIMENTAL_Q38_BLOCK) && !defined(TT_PIONEER_EXPERIMENTAL_Q38_AFFINE)
#error Q38 block probe requires the guarded affine protocol
#endif
#if defined(TT_PIONEER_EXPERIMENTAL_Q38_STREAM) && !defined(TT_PIONEER_EXPERIMENTAL_Q38_BLOCK)
#error Q38 fullK stream requires the qualified block protocol and all-bank owner
#endif
#if defined(TT_PIONEER_EXPERIMENTAL_Q38_ROWS) && !defined(TT_PIONEER_EXPERIMENTAL_Q38_STREAM)
#error Q38 batch rows require the guarded fullK/all-bank owner
#endif
#if defined(TT_PIONEER_EXPERIMENTAL_Q38_COHORT) && !defined(TT_PIONEER_EXPERIMENTAL_Q38_ROWS)
#error Q38 cohort requires full saved-view/banked row ownership
#endif
#if defined(TT_PIONEER_EXPERIMENTAL_Q38_COHORT_SEALED) && !defined(TT_PIONEER_EXPERIMENTAL_Q38_COHORT)
#error Q38 sealed cohort requires the exclusive immutable-image cohort owner
#endif
#if defined(TT_PIONEER_EXPERIMENTAL_Q38_COHORT_PAYLOAD) && !defined(TT_PIONEER_EXPERIMENTAL_Q38_COHORT_SEALED)
#error Q38 payload blocks require the exclusive boundary-code-sealed cohort owner
#endif
#if defined(TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH) && !defined(TT_PIONEER_EXPERIMENTAL_Q38_COHORT_PAYLOAD)
#error Q38 persistent dispatcher requires the guarded payload/all-bank owner
#endif
#if defined(TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR) && !defined(TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH)
#error Q38 vector peer requires the complete private persistent dispatcher lifecycle
#endif
#if defined(TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_BFLOAT) && !defined(TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR)
#error Q38 BF16 peer requires the complete guarded vector dispatcher lifecycle
#endif
#if defined(TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_FAULT_RETURN) && (!defined(TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR) || defined(TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_BFLOAT))
#error Q38 fault-return57 requires the guarded integer vector dispatcher only
#endif
#if defined(TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_TRANSFER_DIAGNOSTIC) && !defined(TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_FAULT_RETURN)
#error Q38 transfer58 diagnostics require the guarded fault-return lifecycle
#endif
#if defined(TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_PEER_PROGRESS) && !defined(TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_TRANSFER_DIAGNOSTIC)
#error Q38 progress59 diagnostics require the guarded transfer-diagnostic lifecycle
#endif

int32 api_version = B_CUR_DRIVER_API_VERSION;
static pci_module_info* sPCI;
static pci_info sCard;
static char sCookie;
struct PioneerStoreContext;
struct PioneerProbeCookie {
	uint32 magic=0x50313030;
	team_id team=-1;
	PioneerStoreContext* context=NULL;
};
static PioneerProbeCookie* sRuntimeOwner=NULL;
static sem_id sLock = -1;
#ifdef TT_PIONEER_DRIVER_RELOAD
#if !defined(TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH) || !defined(PIONEER_DRIVER_BUILD_TAG)
#error Reload support requires the guarded persistent driver and an explicit build identity
#endif
static_assert(PIONEER_DRIVER_BUILD_TAG!=0,"reload build identity must be nonzero");
// PIONEER_DRIVER_RELOAD_STATE_BEGIN
static pioneer_driver_reload_guard_info* sReloadGuard=NULL;
static uint64 sReloadGeneration=0;
static uint32 sOpenCookies=0;
static bool sReloadQuiesced=false;
struct PioneerColdRecoveryLatch {
	bool value=false;
	operator bool() const { return value || !sReloadGuard || sReloadGuard->cold(); }
	void operator=(bool failure) {
		if (failure) { value=true; if (sReloadGuard) sReloadGuard->fault(sReloadGeneration); }
	}
};
static PioneerColdRecoveryLatch sColdRecoveryRequired;
// PIONEER_DRIVER_RELOAD_STATE_END
#else
// Protected by sLock. A module reload is NOT cold recovery; after this latch
// is set, the board must be cold-recovered before changing/reloading drivers.
static bool sColdRecoveryRequired = false;
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_BANKS
static bool sBankStorageOnly = false; // sLock; blocks EVERY legacy runtime operation
#endif
static device_manager_info* sDeviceManager;
static const char* sNames[] = {TT_PCI_PROBE_DEVICE, NULL};

// PIONEER_PROBE_LOCK_BEGIN
#ifdef PIONEER_Q38_GDN_MUTABLE_STATE
// Instantiation sees the complete native context later in this TU. Keep
// this a typed presence check,not a separately maintained global hint.
template<class Cookie> static bool Q38MutableScopeOwned(const Cookie* cookie)
{
	return cookie && cookie->context && (cookie->context->q38GdnBanks || cookie->context->q38StateZero
#ifdef PIONEER_Q38_RESIDENT_OUTPUT_OWNER
  || cookie->context->q38ResidentOutputNative
#ifdef PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT
  || cookie->context->q38ResidentLayerNative
#endif
#endif
#ifdef PIONEER_Q38_EMBEDDING_NATIVE
		|| cookie->context->q38EmbeddingModel
#endif
#ifdef PIONEER_Q38_FFN_MODEL_INIT
		|| cookie->context->q38FfnModelPlan || cookie->context->q38GraphMutable || cookie->context->q38FfnZero
#endif
#ifdef PIONEER_Q38_RESIDENT_GDN_ROOT_DIAGNOSTIC
		|| cookie->context->q38ResidentGdnRoot
#endif
		);
}
#endif
#ifdef PIONEER_Q38_LOCAL_DELTA_PROBE
static const void* sQ38LocalDeltaOwner=NULL; // sLock;cleared ONLY after clean END
#endif
#ifdef PIONEER_Q38_REUSE_ACTIVATIONS
// Every OTHER successful lock invalidates resident-input hints, including
// bulk/bank operations and support queries. Saturation permanently disables
// reuse for this driver lifetime, never wraps an old hint back into use.
static uint64_t sQ38InputEpoch=1;
#endif
static status_t
AcquireProbeLock(const void* runtimeOwner=NULL,bool bankCommand=false
#ifdef PIONEER_Q38_REUSE_ACTIVATIONS
	,bool preserveQ38Input=false
#endif
#ifdef PIONEER_Q38_LOCAL_DELTA_PROBE
	,bool localDeltaCommand=false
#endif
#ifdef PIONEER_Q38_GDN_MUTABLE_STATE
	,bool gdnSessionCommand=false
#endif
	)
{
	status_t status = acquire_sem(sLock);
	if (status == B_OK && sColdRecoveryRequired) {
		release_sem(sLock);
		return B_DEV_NOT_READY;
	}
#ifdef TT_PIONEER_DRIVER_RELOAD
	if (status==B_OK && sReloadQuiesced) {
		release_sem(sLock); return B_BUSY; // EVERY hardware operation is gated
	}
#endif
	if (status==B_OK && sRuntimeOwner!=NULL && sRuntimeOwner!=runtimeOwner) {
		release_sem(sLock);
		return B_BUSY;
	}
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_BANKS
	if (status==B_OK && sBankStorageOnly && !bankCommand) {
		release_sem(sLock);
		return B_NOT_ALLOWED;
	}
#else
	(void)bankCommand;
#endif
#ifdef PIONEER_Q38_LOCAL_DELTA_PROBE
	if (status==B_OK && sQ38LocalDeltaOwner!=NULL && (!localDeltaCommand || sQ38LocalDeltaOwner!=runtimeOwner)) {
		release_sem(sLock); return B_NOT_ALLOWED; // deny overlapping ownership UNDER the lock
	}
#endif
#ifdef PIONEER_Q38_GDN_MUTABLE_STATE
	// EVERY ordinary hardware command is denied once either private owner
	// exists,even before first zero or after zero CLOSE. Only SAME-owner
	// session STATUS/explicit END use the separate private call site below.
	if (status==B_OK && Q38MutableScopeOwned(sRuntimeOwner) && (!gdnSessionCommand || sRuntimeOwner!=runtimeOwner)) {
		release_sem(sLock); return B_NOT_ALLOWED;
	}
#endif
#ifdef PIONEER_Q38_REUSE_ACTIVATIONS
	if (status==B_OK && !preserveQ38Input && sQ38InputEpoch!=UINT64_MAX) sQ38InputEpoch++;
#endif
	return status;
}
#ifdef PIONEER_Q38_LOCAL_DELTA_PROBE
static status_t AcquireQ38DeltaLock(const void* cookie)
{
	return AcquireProbeLock(cookie,true
#ifdef PIONEER_Q38_REUSE_ACTIVATIONS
		,false
#endif
		,true);
}
#endif
#ifdef PIONEER_Q38_GDN_MUTABLE_STATE
[[maybe_unused]] static status_t AcquireQ38GdnSessionLock(const void* cookie)
{
	return AcquireProbeLock(cookie,true
#ifdef PIONEER_Q38_REUSE_ACTIVATIONS
		,false
#endif
#ifdef PIONEER_Q38_LOCAL_DELTA_PROBE
		,true
#endif
		,true);
}
#endif
// PIONEER_PROBE_LOCK_END

static inline void
PioneerIoFence()
{
#if defined(__riscv)
	asm volatile("fence iorw, iorw" ::: "memory");
#else
	__sync_synchronize();
#endif
}

static inline uint32
PioneerRead32(const volatile uint32* address)
{
	PioneerIoFence();
	uint32 value = *address;
	PioneerIoFence();
	return B_LENDIAN_TO_HOST_INT32(value);
}

static inline void
PioneerWrite32(volatile uint32* address, uint32 value)
{
	PioneerIoFence();
	*address = B_HOST_TO_LENDIAN_INT32(value);
	PioneerIoFence();
}

struct PioneerHostPageIO {
	area_id area=-1;
	volatile uint32* page=NULL;
	uint64 physical=0;
	bool releaseAttempted=false,quarantined=false;
	~PioneerHostPageIO() {
		if (area>=B_OK && !releaseAttempted && !quarantined) ReleaseHostPage();
	}
	int32 AllocateHostPage() {
		if (area>=B_OK || B_PAGE_SIZE!=TT_HOST_PAGE_BYTES) return B_BAD_VALUE;
		virtual_address_restrictions virtualRestrictions={};
		virtualRestrictions.address_specification=B_ANY_KERNEL_ADDRESS;
		physical_address_restrictions physicalRestrictions={};
		physicalRestrictions.low_address=UINT64_C(0x100000000);
		physicalRestrictions.high_address=UINT64_C(0x1000000000000); // exclusive
		physicalRestrictions.alignment=TT_HOST_PAGE_BYTES;
		void* address=NULL;
		area=create_area_etc(B_SYSTEM_TEAM,"P100A dedicated high host-read page",
			TT_HOST_PAGE_BYTES,B_CONTIGUOUS,B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA,
			0,0,&virtualRestrictions,&physicalRestrictions,&address);
		if (area<B_OK) return area;
		page=(volatile uint32*)address; return B_OK;
	}
	int32 HostPageMap(uint64_t& address,uint64_t& bytes) {
		if (area<B_OK || page==NULL) return B_BAD_VALUE;
		physical_entry entry={};
		status_t status=get_memory_map((const void*)page,TT_HOST_PAGE_BYTES,&entry,1);
		if (status!=B_OK) return status;
		address=entry.address; bytes=entry.size;
		if (PioneerHostPageAddress(address,bytes)) physical=address;
		return B_OK;
	}
	int32 SetHostPageNonCached() {
#if defined(__riscv)
		if (area<B_OK || !PioneerHostPageAddress(physical,TT_HOST_PAGE_BYTES)) return B_BAD_VALUE;
		// Same ordinary non-cacheable RAM type used by this port's working
		// NVMe queue/bounce allocations, not an MMIO alias or cached buffer.
		return vm_set_area_memory_type(area,physical,B_WRITE_THROUGH_MEMORY);
#else
		return B_UNSUPPORTED;
#endif
	}
	uint32 HostPageRead(unsigned w) { return w<TT_HOST_PAGE_WORDS?PioneerRead32(page+w):UINT32_MAX; }
	void HostPageWrite(unsigned w,uint32 value) { if (w<TT_HOST_PAGE_WORDS) PioneerWrite32(page+w,value); }
	int32 ReleaseHostPage() {
		if (area<B_OK || releaseAttempted || quarantined) return B_BAD_VALUE;
		releaseAttempted=true;
		status_t status=delete_area(area);
		if (status==B_OK) { area=-1; page=NULL; }
		return status;
	}
	void QuarantineHostPage() {
		// Future DMA callers must retain a page after uncertain completion:
		// late device reads must never observe newly reused private memory.
		// A system-team area stays wired even if this module later unloads.
		quarantined=true;
	}
};

// Private contiguous host owner. Only the separately guarded bulk-upload
// controller hands this address to the device; the allocation probe never does.
struct PioneerOutputBufferIO {
	area_id outputArea=-1;
	volatile uint32* outputPage=NULL;
	uint64 outputPhysical=0;
	bool outputReleaseAttempted=false,outputQuarantined=false;
	~PioneerOutputBufferIO() {
		if (outputArea>=B_OK && !outputReleaseAttempted && !outputQuarantined) ReleaseOutputBuffer();
	}
	int32 AllocateOutputBuffer() {
		if (outputArea>=B_OK || B_PAGE_SIZE!=4096) return B_BAD_VALUE;
		virtual_address_restrictions virtualRestrictions={};
		virtualRestrictions.address_specification=B_ANY_KERNEL_ADDRESS;
		physical_address_restrictions physicalRestrictions={};
		physicalRestrictions.low_address=UINT64_C(0x100000000);
		physicalRestrictions.high_address=UINT64_C(0x1000000000000);
		physicalRestrictions.alignment=4096;
		void* address=NULL;
		outputArea=create_area_etc(B_SYSTEM_TEAM,"P100A private high host-transfer buffer",
			TT_OUTPUT_BUFFER_BYTES,B_CONTIGUOUS,B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA,
			0,0,&virtualRestrictions,&physicalRestrictions,&address);
		if (outputArea<B_OK) return outputArea;
		outputPage=(volatile uint32*)address; return B_OK;
	}
	int32 OutputBufferMap(uint64_t& address,uint64_t& bytes) {
		if (outputArea<B_OK || !outputPage) return B_BAD_VALUE;
		physical_entry entry={};
		status_t status=get_memory_map((const void*)outputPage,TT_OUTPUT_BUFFER_BYTES,&entry,1);
		if (status!=B_OK) return status;
		address=entry.address; bytes=entry.size;
		if (PioneerOutputBufferAddress(address,bytes)) outputPhysical=address;
		return B_OK;
	}
	int32 OutputBufferPageMap(unsigned page,uint64_t& address,uint64_t& bytes) {
		if (page>=4 || outputArea<B_OK || !outputPage) return B_BAD_VALUE;
		physical_entry entry={};
		status_t status=get_memory_map((const void*)(outputPage+page*1024),4096,&entry,1);
		if (status!=B_OK) return status;
		address=entry.address; bytes=entry.size; return B_OK;
	}
	int32 SetOutputBufferNonCached() {
#if defined(__riscv)
		if (outputArea<B_OK || !PioneerOutputBufferAddress(outputPhysical,TT_OUTPUT_BUFFER_BYTES)) return B_BAD_VALUE;
		return vm_set_area_memory_type(outputArea,outputPhysical,B_WRITE_THROUGH_MEMORY);
#else
		return B_UNSUPPORTED;
#endif
	}
	uint32 OutputBufferRead(unsigned w) { return outputArea>=B_OK && w<TT_OUTPUT_BUFFER_WORDS?PioneerRead32(outputPage+w):UINT32_MAX; }
	void OutputBufferWrite(unsigned w,uint32 value) { if (outputArea>=B_OK && w<TT_OUTPUT_BUFFER_WORDS) PioneerWrite32(outputPage+w,value); }
	int32 ReleaseOutputBuffer() {
		if (outputArea<B_OK || outputReleaseAttempted || outputQuarantined) return B_BAD_VALUE;
		outputReleaseAttempted=true;
		status_t status=delete_area(outputArea);
		if (status==B_OK) { outputArea=-1; outputPage=NULL; }
		return status;
	}
	void QuarantineOutputBuffer() { outputQuarantined=true; } // retain wired system-team ownership
};
struct PioneerOutputBufferProbeIO : PioneerHostPageIO,PioneerOutputBufferIO {};

struct PioneerWideOutputIO {
	area_id wideArea=-1;
	volatile uint32* widePage=NULL;
	uint64 widePhysical=0;
	bool wideReleaseAttempted=false,wideQuarantined=false;
	~PioneerWideOutputIO() { if (wideArea>=B_OK && !wideReleaseAttempted && !wideQuarantined) ReleaseWideOutput(); }
	int32 AllocateWideOutput() {
		if (wideArea>=B_OK || B_PAGE_SIZE!=4096) return B_BAD_VALUE;
		virtual_address_restrictions virtualRestrictions={}; virtualRestrictions.address_specification=B_ANY_KERNEL_ADDRESS;
		physical_address_restrictions physicalRestrictions={};
		physicalRestrictions.low_address=UINT64_C(0x100000000); physicalRestrictions.high_address=UINT64_C(0x1000000000000);
		physicalRestrictions.alignment=TT_WIDE_OUTPUT_BYTES;
		void* address=NULL;
		wideArea=create_area_etc(B_SYSTEM_TEAM,"P100A private1MiB wide result owner",TT_WIDE_OUTPUT_BYTES,B_CONTIGUOUS,
			B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA,0,0,&virtualRestrictions,&physicalRestrictions,&address);
		if (wideArea<B_OK) return wideArea;
		widePage=(volatile uint32*)address; return B_OK;
	}
	int32 WideOutputMap(uint64_t& address,uint64_t& bytes) {
		if (wideArea<B_OK || !widePage) return B_BAD_VALUE;
		physical_entry entry={}; status_t status=get_memory_map((const void*)widePage,TT_WIDE_OUTPUT_BYTES,&entry,1);
		if (status!=B_OK) return status;
		address=entry.address; bytes=entry.size;
		if (PioneerWideOutputAddress(address,bytes)) widePhysical=address;
		return B_OK;
	}
	int32 WideOutputPageMap(unsigned page,uint64_t& address,uint64_t& bytes) {
		if (page>=TT_WIDE_OUTPUT_PAGES || wideArea<B_OK || !widePage) return B_BAD_VALUE;
		physical_entry entry={}; status_t status=get_memory_map((const void*)(widePage+page*1024),4096,&entry,1);
		if (status!=B_OK) return status;
		address=entry.address; bytes=entry.size; return B_OK;
	}
	int32 SetWideOutputNonCached() {
#if defined(__riscv)
		if (wideArea<B_OK || !PioneerWideOutputAddress(widePhysical,TT_WIDE_OUTPUT_BYTES)) return B_BAD_VALUE;
		return vm_set_area_memory_type(wideArea,widePhysical,B_WRITE_THROUGH_MEMORY);
#else
		return B_UNSUPPORTED;
#endif
	}
	void WideHostFence() { PioneerIoFence(); }
	uint32 WideHostLoad(unsigned w) { return wideArea>=B_OK && widePage && w<TT_WIDE_OUTPUT_WORDS?B_LENDIAN_TO_HOST_INT32(widePage[w]):UINT32_MAX; }
	void WideHostStore(unsigned w,uint32 value) { if (wideArea>=B_OK && widePage && w<TT_WIDE_OUTPUT_WORDS) widePage[w]=B_HOST_TO_LENDIAN_INT32(value); }
	int32 ReleaseWideOutput() {
		if (wideArea<B_OK || wideReleaseAttempted || wideQuarantined) return B_BAD_VALUE;
		wideReleaseAttempted=true; status_t status=delete_area(wideArea);
		if (status==B_OK) { wideArea=-1; widePage=NULL; }
		return status;
	}
	void QuarantineWideOutput() { wideQuarantined=true; } // retain wired system-team ownership
};

static bool
FindCard(pci_module_info* pci, pci_info& card)
{
	unsigned count = 0;
	pci_info info;
	for (long i = 0; pci->get_nth_pci_info(i, &info) == B_OK; i++) {
		if (info.vendor_id == 0x1e52 && info.device_id == 0xb140
			&& info.u.h0.subsystem_vendor_id == 0x1e52
			&& info.u.h0.subsystem_id == 0x0043
			&& (info.header_type & PCI_header_type_mask) == 0) {
			card = info;
			count++;
		}
	}
	return count == 1;
}

status_t
init_hardware()
{
#if defined(PIONEER_Q38_RESIDENT_OUTPUT_CONTEXT_COMPILE_ONLY) || defined(PIONEER_Q38_RESIDENT_LAYER_CONTEXT_COMPILE_ONLY) || (defined(PIONEER_Q38_MIXED_LIVE_CONTEXT_COMPILE_ONLY) && !defined(PIONEER_Q38_MIXED_LIVE_EMIT_CONTEXT_BODY))
	return B_NOT_ALLOWED; // COMPILE witness:even renamed entry must do ZERO PCI/module IO
#endif
#ifdef TT_PIONEER_DRIVER_RELOAD
	// Even PCI enumeration must not be used as a recovery retry after an
	// UNKNOWN event. This check is ordinary resident CPU metadata only.
	pioneer_driver_reload_guard_info* guard=NULL;
	status_t guarded=get_module(PIONEER_DRIVER_RELOAD_GUARD_MODULE,(module_info**)&guard);
	if (guarded!=B_OK) return guarded;
	bool cold=guard->version!=1 || guard->cold();
	put_module(PIONEER_DRIVER_RELOAD_GUARD_MODULE);
	if (cold) return B_DEV_NOT_READY;
#endif
	dprintf("PIONEER-P100A: probe hardware PCI module begin\n");
	pci_module_info* pci;
	status_t status = get_module(B_PCI_MODULE_NAME, (module_info**)&pci);
	dprintf("PIONEER-P100A: probe hardware PCI module status %" B_PRId32 "\n", status);
	if (status != B_OK)
		return status;
	pci_info card;
	bool found = FindCard(pci, card);
	dprintf("PIONEER-P100A: probe hardware scan found %d\n", found);
	put_module(B_PCI_MODULE_NAME);
	return found ? B_OK : B_ENTRY_NOT_FOUND;
}

status_t
init_driver()
{
#if defined(PIONEER_Q38_RESIDENT_OUTPUT_CONTEXT_COMPILE_ONLY) || defined(PIONEER_Q38_RESIDENT_LAYER_CONTEXT_COMPILE_ONLY) || (defined(PIONEER_Q38_MIXED_LIVE_CONTEXT_COMPILE_ONLY) && !defined(PIONEER_Q38_MIXED_LIVE_EMIT_CONTEXT_BODY))
	return B_NOT_ALLOWED; // no reload-guard attachment,PCI module or semaphore creation
#endif
#ifdef TT_PIONEER_DRIVER_RELOAD
	status_t guarded=get_module(PIONEER_DRIVER_RELOAD_GUARD_MODULE,(module_info**)&sReloadGuard);
	if (guarded!=B_OK) return guarded;
	if (sReloadGuard->version!=1) guarded=B_BAD_VALUE;
	else guarded=sReloadGuard->attach(&sReloadGeneration);
	if (guarded!=B_OK) {
		put_module(PIONEER_DRIVER_RELOAD_GUARD_MODULE); sReloadGuard=NULL; return guarded;
	}
#endif
	dprintf("PIONEER-P100A: probe driver init begin\n");
	status_t status = get_module(B_PCI_MODULE_NAME, (module_info**)&sPCI);
	dprintf("PIONEER-P100A: probe driver PCI module status %" B_PRId32 "\n", status);
	if (status != B_OK) {
#ifdef TT_PIONEER_DRIVER_RELOAD
		sReloadGuard->detach(sReloadGeneration,true); put_module(PIONEER_DRIVER_RELOAD_GUARD_MODULE); sReloadGuard=NULL;
#endif
		return status;
	}
	if (!FindCard(sPCI, sCard)) {
		put_module(B_PCI_MODULE_NAME);
		sPCI = NULL;
#ifdef TT_PIONEER_DRIVER_RELOAD
		sReloadGuard->detach(sReloadGeneration,true); put_module(PIONEER_DRIVER_RELOAD_GUARD_MODULE); sReloadGuard=NULL;
#endif
		return B_ENTRY_NOT_FOUND;
	}
	dprintf("PIONEER-P100A: probe driver scan complete\n");
	sLock = create_sem(1, "P100A config address probe");
	if (sLock < B_OK) {
		put_module(B_PCI_MODULE_NAME);
		sPCI = NULL;
#ifdef TT_PIONEER_DRIVER_RELOAD
		sReloadGuard->detach(sReloadGeneration,true); put_module(PIONEER_DRIVER_RELOAD_GUARD_MODULE); sReloadGuard=NULL;
#endif
		return sLock;
	}
	dprintf("PIONEER-P100A: probe driver init complete\n");
	return B_OK;
}

void
uninit_driver()
{
#if defined(PIONEER_Q38_RESIDENT_OUTPUT_CONTEXT_COMPILE_ONLY) || defined(PIONEER_Q38_RESIDENT_LAYER_CONTEXT_COMPILE_ONLY) || (defined(PIONEER_Q38_MIXED_LIVE_CONTEXT_COMPILE_ONLY) && !defined(PIONEER_Q38_MIXED_LIVE_EMIT_CONTEXT_BODY))
	return; // no guard/module/sem/host teardown from a compile-only object
#endif
#ifdef TT_PIONEER_DRIVER_RELOAD
	// uninit_driver cannot veto an OS unload. Persist any unsafe state in
	// the separate KEEP_LOADED guard BEFORE dropping this driver's image.
	if (sReloadGuard) {
		bool clean=!sColdRecoveryRequired && sRuntimeOwner==NULL && sOpenCookies==0;
		sReloadGuard->detach(sReloadGeneration,clean);
		put_module(PIONEER_DRIVER_RELOAD_GUARD_MODULE); sReloadGuard=NULL;
	}
#endif
	if (sLock >= B_OK)
		delete_sem(sLock);
	if (sPCI != NULL)
		put_module(B_PCI_MODULE_NAME);
	sPCI = NULL;
	if (sDeviceManager != NULL)
		put_module(B_DEVICE_MANAGER_MODULE_NAME);
	sDeviceManager = NULL;
}

const char**
publish_devices()
{
#if defined(PIONEER_Q38_RESIDENT_OUTPUT_CONTEXT_COMPILE_ONLY) || defined(PIONEER_Q38_RESIDENT_LAYER_CONTEXT_COMPILE_ONLY) || (defined(PIONEER_Q38_MIXED_LIVE_CONTEXT_COMPILE_ONLY) && !defined(PIONEER_Q38_MIXED_LIVE_EMIT_CONTEXT_BODY))
	return NULL;
#endif
	return sNames;
}

static status_t ReleaseResidentCookie(PioneerProbeCookie* cookie);

static status_t
ProbeOpen(const char* name, uint32 flags, void** cookie)
{
	if (geteuid() != 0 || (flags & O_RWMASK) != O_RDWR)
		return B_NOT_ALLOWED;
	if (strcmp(name, TT_PCI_PROBE_DEVICE) != 0)
		return B_ENTRY_NOT_FOUND;
	thread_info thread={};
	status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	PioneerProbeCookie* instance=new(std::nothrow) PioneerProbeCookie;
	if (instance==NULL) return B_NO_MEMORY;
	instance->team=thread.team;
#ifdef TT_PIONEER_DRIVER_RELOAD
	status_t locked=acquire_sem(sLock);
	if (locked!=B_OK) { delete instance; return locked; }
	if (sOpenCookies==UINT32_MAX) { release_sem(sLock); delete instance; return B_BUSY; }
	sOpenCookies++;
	release_sem(sLock);
#endif
	*cookie=instance;
	dprintf("PIONEER-P100A: probe open ready\n");
	return B_OK;
}

static status_t ProbeClose(void*) { return B_OK; }
static status_t ProbeFree(void* cookie) {
	status_t status=ReleaseResidentCookie((PioneerProbeCookie*)cookie);
#ifdef TT_PIONEER_DRIVER_RELOAD
	if (status==B_OK) {
		status_t locked=acquire_sem(sLock);
		if (locked!=B_OK) { sColdRecoveryRequired=true; return locked; }
		if (!sOpenCookies) sColdRecoveryRequired=true;
		else sOpenCookies--;
		release_sem(sLock);
	}
#endif
	return status;
}

struct ConfigIO {
	uint32 Read(uint16 offset, uint8 size)
	{
		return sPCI->read_pci_config(sCard.bus, sCard.device, sCard.function,
			offset, size);
	}
	void Write(uint16 offset, uint8 size, uint32 value)
	{
		sPCI->write_pci_config(sCard.bus, sCard.device, sCard.function,
			offset, size, value);
	}
	uint64 Size(unsigned bar)
	{
		return (uint64)sCard.u.h0.base_register_sizes[bar]
			| ((uint64)sCard.u.h0.base_register_sizes[bar + 1] << 32);
	}
};

struct PioneerMmioIO : ConfigIO {
	const tt_pioneer_profile& profile;
	area_id area = -1;
	const volatile uint32* nodeID = NULL;

	PioneerMmioIO(const tt_pioneer_profile& p) : profile(p) {}
	PioneerMmioIO(const PioneerMmioIO&) = delete;
	PioneerMmioIO& operator=(const PioneerMmioIO&) = delete;
	~PioneerMmioIO() { if (area >= B_OK) delete_area(area); }
	const tt_pioneer_profile& Profile() { return profile; }
	uint32 CardRead(uint16 offset, uint8 size) { return Read(offset, size); }
	void CardWrite(uint16 offset, uint8 size, uint32 value) { Write(offset, size, value); }
	uint32 RootRead(uint16 offset, uint8 size)
	{
		return sPCI->read_pci_config(profile.root_bus, 0, 0, offset, size);
	}
	void RootWrite(uint16 offset, uint8 size, uint32 value)
	{
		sPCI->write_pci_config(profile.root_bus, 0, 0, offset, size, value);
	}
	bool Quiet()
	{
		if (RootRead(0x10, 4) != 0 || RootRead(0x14, 4) != 0
			|| (RootRead(0x38, 4) & 1) != 0) {
			dprintf("PIONEER-P100A: root BAR/ROM guard refused\n");
			return false;
		}
		pci_info info;
		for (long i = 0; sPCI->get_nth_pci_info(i, &info) == B_OK; i++) {
			if (info.bus != profile.root_bus || info.device != 0 || info.function != 0)
				continue;
			bool quiet = info.vendor_id == 0x1e30 && info.device_id == 0x2042
				&& (info.header_type & PCI_header_type_mask) == 1
				&& info.u.h1.base_register_sizes[0] == 0
				&& info.u.h1.base_register_sizes[1] == 0;
			if (!quiet)
				dprintf("PIONEER-P100A: root cached BAR-size guard refused\n");
			return quiet;
		}
		return false;
	}
	bool Map()
	{
		// Hardware fact from pinned Apache-2.0 tt-umd Blackhole constants:
		// BAR0 NoC2AXI node ID is at 0x1fd04044 (x = 2 or 11, y = 0).
		// Independent implementation: map only this page, no register writes.
		void* address = NULL;
		area = map_physical_memory("P100A NoC ID one-shot",
			profile.bar0_host_start + 0x1fd04000, B_PAGE_SIZE,
			B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA | B_UNCACHED_MEMORY, &address);
		if (area < B_OK) {
			dprintf("PIONEER-P100A: NoC ID map failed %" B_PRId32 "\n", area);
			return false;
		}
		nodeID = (const volatile uint32*)((const uint8*)address + 0x44);
		return true;
	}
	bool Unmap()
	{
		if (area < B_OK)
			return true;
		status_t status = delete_area(area);
		if (status != B_OK)
			return false;
		area = -1;
		nodeID = NULL;
		return true;
	}
	uint32 ReadNodeId()
	{
		return PioneerRead32(nodeID);
	}
	bool AfterNodeId(uint32) { return true; }
};

struct PioneerArcIO : PioneerMmioIO {
	tt_arc_scratch_result& arc;
	area_id tlbArea = -1, scratchArea = -1;
	volatile uint32* tlb = NULL;
	const volatile uint32* scratch = NULL;
	bool dirty = false;

	PioneerArcIO(const tt_pioneer_profile& p, tt_arc_scratch_result& result)
		: PioneerMmioIO(p), arc(result) {}
	~PioneerArcIO()
	{
		// Only virtual mappings here: the outer transaction has already
		// disabled PCI decoding. Never attempt MMIO from this destructor.
		if (scratchArea >= B_OK) delete_area(scratchArea);
		if (tlbArea >= B_OK) delete_area(tlbArea);
	}
	bool Map()
	{
		if (!PioneerMmioIO::Map())
			return false;
		void* address = NULL;
		tlbArea = map_physical_memory("P100A fixed TLB control",
			profile.bar0_host_start + 0x1fc00000, B_PAGE_SIZE,
			B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA
				| B_UNCACHED_MEMORY, &address);
		if (tlbArea < B_OK)
			return false;
		tlb = (volatile uint32*)((uint8*)address + 0x96c);
		scratchArea = map_physical_memory("P100A ARC scratch read-only",
			profile.bar0_host_start + 0x19230000, B_PAGE_SIZE,
			B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA | B_UNCACHED_MEMORY, &address);
		if (scratchArea < B_OK)
			return false;
		scratch = (const volatile uint32*)address;
		return true;
	}
	uint32 TlbRead(unsigned i) { return i < 3 ? PioneerRead32(tlb + i) : UINT32_MAX; }
	void TlbWrite(unsigned i, uint32 value)
	{
		if (i >= 3) return;
		dirty = true;
		PioneerWrite32(tlb + i, value);
	}
	uint32 ScratchRead(unsigned n)
	{
		if (n != 2 && n != 4 && n != 11 && n != 12 && n != 13)
			return UINT32_MAX;
		return PioneerRead32(scratch + (0x400 + n * 4) / 4);
	}
	bool AfterNodeId(uint32 id)
	{
		if ((id & 0xfff) != 2)
			return false;
		dprintf("PIONEER-P100A: ARC scratch attempt NoC0 (8,0), TLB 201, read-only target\n");
		return BlackholeArcScratchProbe(*this, arc);
	}
	bool Unmap()
	{
		bool okay = true;
		if (scratchArea >= B_OK) {
			if (delete_area(scratchArea) == B_OK) scratchArea = -1;
			else okay = false;
		}
		if (dirty && !arc.tlb_restored) {
			// One bounded restore retry, while the outer transaction still
			// has memory decoding enabled. Never write ARC scratch itself.
			for (unsigned i = 0; i < 3; i++) TlbWrite(i, arc.tlb_before[i]);
			arc.tlb_restored = 1;
			for (unsigned i = 0; i < 3; i++) {
				arc.tlb_after[i] = TlbRead(i);
				arc.tlb_restored &= arc.tlb_after[i] == arc.tlb_before[i];
			}
		}
		if (dirty && !arc.tlb_restored) okay = false;
		if (tlbArea >= B_OK) {
			if (delete_area(tlbArea) == B_OK) tlbArea = -1;
			else okay = false;
		}
		return PioneerMmioIO::Unmap() && okay;
	}
};

struct PioneerTelemetryIO : PioneerArcIO {
	tt_arc_telemetry_result& telemetry;
	const bool operating;
	area_id csmArea = -1;
	const volatile uint32* csm = NULL;
	bool telemetryAttempted = false;

	PioneerTelemetryIO(const tt_pioneer_profile& p, tt_arc_scratch_result& arcResult,
		tt_arc_telemetry_result& result, bool operatingSnapshot = false)
		: PioneerArcIO(p, arcResult), telemetry(result), operating(operatingSnapshot) {}
	~PioneerTelemetryIO() { if (csmArea >= B_OK) delete_area(csmArea); }
	bool Map()
	{
		if (!PioneerArcIO::Map()) return false;
		void* address = NULL;
		csmArea = map_physical_memory("P100A bounded telemetry read-only",
			profile.bar0_host_start + 0x19200000, 0x20000,
			B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA | B_UNCACHED_MEMORY, &address);
		if (csmArea < B_OK) return false;
		csm = (const volatile uint32*)address;
		return true;
	}
	uint32 CsmRead(uint32 address)
	{
		if (!BlackholeTelemetryRange(address, 4)) return UINT32_MAX;
		return PioneerRead32(csm + (address - 0x10000000) / 4);
	}
	bool AfterNodeId(uint32 id)
	{
		if (!PioneerArcIO::AfterNodeId(id)) return false;
		telemetryAttempted = true;
		dprintf("PIONEER-P100A: bounded CSM telemetry attempt, whitelisted tags only\n");
		return BlackholeArcTelemetryProbe(*this, arc, telemetry, operating);
	}
	bool Unmap()
	{
		bool okay = true;
		if (csmArea >= B_OK) {
			if (delete_area(csmArea) == B_OK) csmArea = -1;
			else okay = false;
		}
		if (telemetryAttempted && !telemetry.tlb_restored) {
			for (unsigned i = 0; i < 3; i++) TlbWrite(i, telemetry.tlb_before[i]);
			telemetry.tlb_restored = 1;
			for (unsigned i = 0; i < 3; i++) {
				telemetry.tlb_after[i] = TlbRead(i);
				telemetry.tlb_restored &= telemetry.tlb_after[i] == telemetry.tlb_before[i];
			}
		}
		if (telemetryAttempted && !telemetry.tlb_restored) okay = false;
		return PioneerArcIO::Unmap() && okay;
	}
};

struct PioneerArcQueryIO : PioneerTelemetryIO {
	tt_arc_queue_result& query;
	tt_arc_clock_guard_result* const clock;
	area_id queueWriteArea = -1, msiArea = -1;
	volatile uint32* queueWrite = NULL;
	volatile uint32* msi = NULL;
	uint32 queueBase = 0, requestAddress = 0;
	bool queryAttempted = false;
	PioneerArcQueryIO(const tt_pioneer_profile& p, tt_arc_scratch_result& a,
		tt_arc_telemetry_result& t, tt_arc_queue_result& q, tt_arc_clock_guard_result* clockGuard = NULL)
		: PioneerTelemetryIO(p,a,t), query(q), clock(clockGuard) {}
	~PioneerArcQueryIO() {
		if (queueWriteArea >= B_OK) delete_area(queueWriteArea);
		if (msiArea >= B_OK) delete_area(msiArea);
	}
	bool Map() {
		if (!PioneerTelemetryIO::Map()) return false;
		void* address = NULL;
		queueWriteArea = map_physical_memory("P100A bounded ARC host-queue writes",
			profile.bar0_host_start+0x19200000,TT_ARC_QUEUE_CSM_BYTES,B_ANY_KERNEL_ADDRESS,
			B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA | B_UNCACHED_MEMORY,&address);
		if (queueWriteArea < B_OK) return false;
		queueWrite = (volatile uint32*)address;
		msiArea = map_physical_memory("P100A inbound ARC message notification",
			profile.bar0_host_start+0x192b0000,B_PAGE_SIZE,B_ANY_KERNEL_ADDRESS,
			B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA | B_UNCACHED_MEMORY,&address);
		if (msiArea < B_OK) return false;
		msi = (volatile uint32*)address;
		return true;
	}
	uint32 CsmRead(uint32 address) {
		return BlackholeArcQueueRange(address,4) ? PioneerRead32(queueWrite+(address-0x10000000)/4) : UINT32_MAX;
	}
	void SetQueueBounds(uint32 base, uint32 request, uint32) {
		if (!BlackholeArcQueueRange(base,288) || request < base+32 || request > base+128 || (request-base-32)%32 != 0) return;
		queueBase = base; requestAddress = request;
	}
	void QueueWrite(uint32 address, uint32 value) {
		if (queueBase == 0 || !BlackholeArcQueueRange(address,4)) return;
		// Only CPU-owned request bytes and the two CPU-owned queue pointers.
		if (address != queueBase && address != queueBase+4
			&& (address < requestAddress || address >= requestAddress+32)) return;
		PioneerWrite32(queueWrite+(address-0x10000000)/4,value);
	}
	uint32 MsiStatusRead() { return PioneerRead32(msi+2); }
	void NotifyArc() { PioneerWrite32(msi,0); }
	void Pause() { spin(100); }
	bool AfterNodeId(uint32 id) {
		if (!PioneerTelemetryIO::AfterNodeId(id)) return false;
		queryAttempted = true;
		if (clock != NULL) {
			dprintf("PIONEER-P100A: normal firmware busy/idle clock cycle, no forced frequency/voltage/reset/flash/DMA\n");
			bool entered = BlackholeArcClockEnter(*this,arc,telemetry,*clock);
			bool left = BlackholeArcClockLeave(*this,arc,telemetry,*clock);
			return entered && left && clock->state == TT_ARC_CLOCK_RESTORED && !clock->recovery_required;
		}
		dprintf("PIONEER-P100A: fixed ARC GET_AICLK query, application queue only, no clock/voltage/reset/flash/DMA command\n");
		return BlackholeArcQueueProbe(*this,arc,telemetry,query);
	}
	bool Unmap() {
		bool okay = true;
		if (queryAttempted && !query.tlb_restored) {
			for (unsigned attempt = 0; attempt < 2 && !query.tlb_restored; attempt++) {
				for (unsigned w = 0; w < 3; w++) TlbWrite(w,query.tlb_before[w]);
				query.tlb_restored = 1;
				for (unsigned w = 0; w < 3; w++) { query.tlb_after[w] = TlbRead(w); query.tlb_restored &= query.tlb_after[w] == query.tlb_before[w]; }
			}
		}
		if (queryAttempted && (!query.request_restored || !query.tlb_restored || query.recovery_required)) okay = false;
		if (clock != NULL && (clock->recovery_required || !clock->restored)) okay = false;
		if (queueWriteArea >= B_OK) { if (delete_area(queueWriteArea) == B_OK) queueWriteArea = -1; else okay = false; }
		if (msiArea >= B_OK) { if (delete_area(msiArea) == B_OK) msiArea = -1; else okay = false; }
		return PioneerTelemetryIO::Unmap() && okay;
	}
};

#ifdef PIONEER_Q38_NORMAL_CLOCK
// PIONEER_Q38_CLOCK_IO_BEGIN
// Only private clock transport uses this adapter. Map creates VIRTUAL
// views; Unmap deletes VIRTUAL views ONLY. Never inherit diagnostic ARC
// rollback, which is forbidden after a Q38 ownership/completion fault.
struct PioneerQ38ClockIO : PioneerArcQueryIO {
	PioneerQ38ClockIO(const tt_pioneer_profile& p,tt_arc_scratch_result& a,
		tt_arc_telemetry_result& t,tt_arc_queue_result& unused)
		:PioneerArcQueryIO(p,a,t,unused) {}
	bool AfterNodeId(uint32)=delete;
	bool Unmap() {
		area_id* views[]={&queueWriteArea,&msiArea,&csmArea,&scratchArea,&tlbArea,&area};
		for (auto* view:views) if (*view>=B_OK) {
			if (delete_area(*view)!=B_OK) return false; // no second disposal attempt
			*view=-1;
		}
		queueWrite=msi=NULL; csm=scratch=nodeID=NULL; tlb=NULL;
		return true;
	}
};
// PIONEER_Q38_CLOCK_IO_END
#endif

struct PioneerTensixIO : PioneerTelemetryIO {
	tt_tensix_reset_result& tensix;
	area_id resetArea = -1;
	const volatile uint32* reset = NULL;
	bool resetAttempted = false;

	PioneerTensixIO(const tt_pioneer_profile& p, tt_arc_scratch_result& a,
		tt_arc_telemetry_result& t, tt_tensix_reset_result& r)
		: PioneerTelemetryIO(p, a, t), tensix(r) {}
	~PioneerTensixIO() { if (resetArea >= B_OK) delete_area(resetArea); }
	bool Map()
	{
		if (!PioneerTelemetryIO::Map()) return false;
		void* address = NULL;
		// TLB 201 -> local base ffa00000; SOFT_RESET is ffb121b0.
		resetArea = map_physical_memory("P100A Tensix reset read-only",
			profile.bar0_host_start + 0x19312000, B_PAGE_SIZE,
			B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA | B_UNCACHED_MEMORY, &address);
		if (resetArea < B_OK) return false;
		reset = (const volatile uint32*)((const uint8*)address + 0x1b0);
		return true;
	}
	uint32 NiuRead()
	{
		// Same already-mapped PCIe NIU page as NODE_ID; NIU_CFG_0 is +100.
		return PioneerRead32((const volatile uint32*)((const uint8*)nodeID + 0xbc));
	}
	uint32 ResetRead() { return PioneerRead32(reset); }
	bool AfterNodeId(uint32 id)
	{
		if (!PioneerTelemetryIO::AfterNodeId(id)) return false;
		resetAttempted = true;
		dprintf("PIONEER-P100A: enabled physical Tensix (1,2), read-only reset-state attempt\n");
		return BlackholeTensixResetProbe(*this, telemetry, tensix);
	}
	bool Unmap()
	{
		bool okay = true;
		if (resetArea >= B_OK) {
			if (delete_area(resetArea) == B_OK) resetArea = -1;
			else okay = false;
		}
		if (resetAttempted && !tensix.tlb_restored) {
			for (unsigned i = 0; i < 3; i++) TlbWrite(i, tensix.tlb_before[i]);
			tensix.tlb_restored = 1;
			for (unsigned i = 0; i < 3; i++) {
				tensix.tlb_after[i] = TlbRead(i);
				tensix.tlb_restored &= tensix.tlb_after[i] == tensix.tlb_before[i];
			}
		}
		if (resetAttempted && !tensix.tlb_restored) okay = false;
		return PioneerTelemetryIO::Unmap() && okay;
	}
};

struct PioneerL1IO : PioneerTensixIO {
	tt_l1_scratch_result& l1;
	area_id l1Area = -1;
	volatile uint32* memory = NULL;
	bool l1Attempted = false;

	PioneerL1IO(const tt_pioneer_profile& p, tt_arc_scratch_result& a,
		tt_arc_telemetry_result& t, tt_tensix_reset_result& c, tt_l1_scratch_result& r)
		: PioneerTensixIO(p, a, t, c), l1(r) {}
	~PioneerL1IO() { if (l1Area >= B_OK) delete_area(l1Area); }
	bool Map()
	{
		if (!PioneerTensixIO::Map()) return false;
		void* address = NULL;
		// TLB 200 BAR0 base 19000000; final 64 bytes of 180000-byte L1.
		l1Area = map_physical_memory("P100A stopped-core L1 scratch",
			profile.bar0_host_start + 0x1917f000, B_PAGE_SIZE,
			B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA
				| B_UNCACHED_MEMORY, &address);
		if (l1Area < B_OK) return false;
		memory = (volatile uint32*)((uint8*)address + 0xfc0);
		return true;
	}
	uint32 L1TlbRead(unsigned slot, unsigned word)
	{
		if (slot > 1 || word > 2) return UINT32_MAX;
		return PioneerRead32(tlb - 3 + slot * 3 + word);
	}
	void L1TlbWrite(unsigned slot, unsigned word, uint32 value)
	{
		if (slot > 1 || word > 2) return;
		PioneerWrite32(tlb - 3 + slot * 3 + word, value);
	}
	uint32 L1Read(unsigned word) { return word < TT_L1_WORDS ? PioneerRead32(memory + word) : UINT32_MAX; }
	void L1Write(unsigned word, uint32 value) { if (word < TT_L1_WORDS) PioneerWrite32(memory + word, value); }
	bool AfterNodeId(uint32 id)
	{
		if (!PioneerTensixIO::AfterNodeId(id)) return false;
		l1Attempted = true;
		dprintf("PIONEER-P100A: stopped Tensix (1,2), 64-byte reversible L1 data test\n");
		return BlackholeL1ScratchProbe(*this, tensix, l1);
	}
	bool Unmap()
	{
		bool okay = true;
		if (l1Area >= B_OK) {
			if (delete_area(l1Area) == B_OK) l1Area = -1;
			else okay = false;
		}
		if (l1Attempted && !l1.tlb_restored) {
			for (unsigned s = 0; s < 2; s++)
				for (unsigned w = 0; w < 3; w++) L1TlbWrite(s, w, l1.tlb_before[s][w]);
			l1.tlb_restored = 1;
			for (unsigned s = 0; s < 2; s++) {
				for (unsigned w = 0; w < 3; w++) {
					l1.tlb_after[s][w] = L1TlbRead(s, w);
					l1.tlb_restored &= l1.tlb_after[s][w] == l1.tlb_before[s][w];
				}
			}
		}
		if (l1Attempted && (!l1.tlb_restored || !l1.data_restored)) okay = false;
		return PioneerTensixIO::Unmap() && okay;
	}
};

struct PioneerBriscIO : PioneerTensixIO {
	tt_brisc_smoke_result& brisc;
	const bool trisc;
	const bool backendReset;
	area_id briscAreas[5] = {-1, -1, -1, -1, -1};
	volatile uint32* boot = NULL;
	volatile uint32* code = NULL;
	volatile uint32* results = NULL;
	volatile uint32* resetWrite = NULL;
	const volatile uint32* pc = NULL;
	bool briscAttempted = false;

	PioneerBriscIO(const tt_pioneer_profile& p, tt_arc_scratch_result& a,
		tt_arc_telemetry_result& t, tt_tensix_reset_result& c, tt_brisc_smoke_result& r, bool trisc0 = false, bool resetBackends = false)
		: PioneerTensixIO(p, a, t, c), brisc(r), trisc(trisc0), backendReset(resetBackends) {}
	~PioneerBriscIO()
	{
		// Never perform MMIO in a destructor or retry code/reset restoration.
		for (unsigned i = 0; i < 5; i++)
			if (briscAreas[i] >= B_OK) delete_area(briscAreas[i]);
	}
	bool Map()
	{
		if (!PioneerTensixIO::Map()) return false;
		const uint32 offsets[] = {trisc ? 0x19006000U : 0x19000000U, 0x19070000, 0x1917f000, 0x19312000, 0x19313000};
		void* pages[5] = {};
		for (unsigned i = 0; i < 5; i++) {
			uint32 protection = B_KERNEL_READ_AREA | B_UNCACHED_MEMORY;
			if (i != 4) protection |= B_KERNEL_WRITE_AREA;
			briscAreas[i] = map_physical_memory("P100A fixed BRISC smoke page",
				profile.bar0_host_start + offsets[i], B_PAGE_SIZE,
				B_ANY_KERNEL_ADDRESS, protection, &pages[i]);
			if (briscAreas[i] < B_OK) return false;
		}
		boot = (volatile uint32*)pages[0]; code = (volatile uint32*)pages[1];
		results = (volatile uint32*)((uint8*)pages[2] + 0xfc0);
		resetWrite = (volatile uint32*)((uint8*)pages[3] + 0x1b0);
		pc = (const volatile uint32*)((const uint8*)pages[4] + (trisc ? 0x140 : 0x138));
		return true;
	}
	uint32 BriscTlbRead(unsigned slot, unsigned word)
	{
		return slot < 2 && word < 3 ? PioneerRead32(tlb - 3 + slot * 3 + word) : UINT32_MAX;
	}
	void BriscTlbWrite(unsigned slot, unsigned word, uint32 value)
	{
		if (slot < 2 && word < 3) PioneerWrite32(tlb - 3 + slot * 3 + word, value);
	}
	volatile uint32* ImagePointer(unsigned index)
	{
		if (index == 0) return boot;
		if (index <= TT_BRISC_CODE_WORDS) return code + index - 1;
		if (index < TT_BRISC_IMAGE_WORDS) return results + index - 20;
		return NULL;
	}
	uint32 ImageRead(unsigned index)
	{
		volatile uint32* address = ImagePointer(index);
		return address != NULL ? PioneerRead32(address) : UINT32_MAX;
	}
	void ImageWrite(unsigned index, uint32 value)
	{
		volatile uint32* address = ImagePointer(index);
		if (address != NULL) PioneerWrite32(address, value);
	}
	void ResetWrite(uint32 value)
	{
		if (value == (trisc ? 0x46800 : 0x47000) || value == 0x47800 || (trisc && backendReset && value == 0x87ffff)) PioneerWrite32(resetWrite, value);
	}
	uint32 ResetEntryOverride() { return PioneerRead32(resetWrite + (0x234-0x1b0)/4); }
	uint32 PcRead() { return PioneerRead32(pc); }
	void Pause() { spin(100); } // fixed 100 us; policy bounds the iteration count
	bool AfterNodeId(uint32 id)
	{
		if (!PioneerTensixIO::AfterNodeId(id)) return false;
		briscAttempted = true;
		dprintf("PIONEER-P100A: fixed %s scalar checksum, sole tile (1,2), no coprocessor/DMA/flash\n", trisc ? "TRISC0" : "BRISC");
		return BlackholeBriscSmokeProbe(*this, tensix, brisc, trisc, backendReset);
	}
	bool Unmap()
	{
		bool okay = true;
		for (unsigned i = 0; i < 5; i++) {
			if (briscAreas[i] < B_OK) continue;
			if (delete_area(briscAreas[i]) == B_OK) briscAreas[i] = -1;
			else okay = false;
		}
		boot = code = results = resetWrite = NULL; pc = NULL;
		if (briscAttempted && !brisc.tlb_restored) {
			// Only TLB control cleanup here. Never retry a core start/stop or
			// rewrite code after the policy has declared execution uncertain.
			for (unsigned s = 0; s < 2; s++)
				for (unsigned w = 0; w < 3; w++) BriscTlbWrite(s, w, brisc.tlb_before[s][w]);
			brisc.tlb_restored = 1;
			for (unsigned s = 0; s < 2; s++) {
				for (unsigned w = 0; w < 3; w++) {
					brisc.tlb_after[s][w] = BriscTlbRead(s, w);
					brisc.tlb_restored &= brisc.tlb_after[s][w] == brisc.tlb_before[s][w];
				}
			}
		}
		if (briscAttempted && (!brisc.l1_restored || !brisc.tlb_restored || brisc.recovery_required)) okay = false;
		return PioneerTensixIO::Unmap() && okay;
	}
};

struct PioneerDramRouteIO : PioneerTensixIO {
	tt_dram_route_result& dram;
	area_id dramArea = -1;
	const volatile uint32* dramNiu = NULL;
	bool dramAttempted = false;
	PioneerDramRouteIO(const tt_pioneer_profile& p, tt_arc_scratch_result& a,
		tt_arc_telemetry_result& t, tt_tensix_reset_result& c, tt_dram_route_result& r)
		: PioneerTensixIO(p, a, t, c), dram(r) {}
	~PioneerDramRouteIO() { if (dramArea >= B_OK) delete_area(dramArea); }
	bool Map()
	{
		if (!PioneerTensixIO::Map()) return false;
		void* address = NULL;
		// TLB 201, local ffa00000 + 120000 => ffb20000. Only one NIU
		// page, read-only and uncached, not GDDR payload memory.
		dramArea = map_physical_memory("P100A fixed GDDR bank-0 NIU read-only",
			profile.bar0_host_start + 0x19320000, B_PAGE_SIZE,
			B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA | B_UNCACHED_MEMORY, &address);
		if (dramArea < B_OK) return false;
		dramNiu = (const volatile uint32*)address;
		return true;
	}
	uint32 SourceRead(unsigned word)
	{
		uint32 offset = BlackholeDramSourceOffset(word);
		return offset == UINT32_MAX ? UINT32_MAX
			: PioneerRead32((const volatile uint32*)((const uint8*)nodeID - 0x44 + offset));
	}
	uint32 DramIdRead(unsigned word)
	{
		uint32 offset = BlackholeDramIdOffset(word);
		return offset == UINT32_MAX ? UINT32_MAX : PioneerRead32(dramNiu + offset / 4);
	}
	bool AfterNodeId(uint32 id)
	{
		if (!PioneerTensixIO::AfterNodeId(id)) return false;
		dramAttempted = true;
		dprintf("PIONEER-P100A: GDDR bank-0 NIU route proof, read-only target\n");
		return BlackholeDramRouteProbe(*this, telemetry, tensix, dram);
	}
	bool Unmap()
	{
		bool okay = true;
		if (dramArea >= B_OK) {
			if (delete_area(dramArea) == B_OK) dramArea = -1;
			else okay = false;
		}
		if (dramAttempted && !dram.tlb_restored) {
			for (unsigned i = 0; i < 3; i++) TlbWrite(i, dram.tlb_before[i]);
			dram.tlb_restored = 1;
			for (unsigned i = 0; i < 3; i++) {
				dram.tlb_after[i] = TlbRead(i);
				dram.tlb_restored &= dram.tlb_after[i] == dram.tlb_before[i];
			}
		}
		if (dramAttempted && (!dram.tlb_restored || dram.recovery_required)) okay = false;
		return PioneerTensixIO::Unmap() && okay;
	}
};

struct PioneerDramSamplesIO : PioneerTensixIO {
	tt_dram_samples_result& dram;
	const bool writeSamples;
	area_id dramArea = -1, dataArea = -1;
	const volatile uint32* dramNiu = NULL;
	volatile uint32* data = NULL;
	bool dramAttempted = false;
	PioneerDramSamplesIO(const tt_pioneer_profile& p, tt_arc_scratch_result& a,
		tt_arc_telemetry_result& t, tt_tensix_reset_result& c, tt_dram_samples_result& r, bool writes)
		: PioneerTensixIO(p, a, t, c), dram(r), writeSamples(writes) {}
	~PioneerDramSamplesIO() {
		if (dataArea >= B_OK) delete_area(dataArea);
		if (dramArea >= B_OK) delete_area(dramArea);
	}
	bool Map()
	{
		if (!PioneerTensixIO::Map()) return false;
		void* address = NULL;
		dramArea = map_physical_memory("P100A enabled GDDR NIU read-only",
			profile.bar0_host_start + 0x19320000, B_PAGE_SIZE,
			B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA | B_UNCACHED_MEMORY, &address);
		if (dramArea < B_OK) return false;
		dramNiu = (const volatile uint32*)address;
		if (writeSamples) {
			dataArea = map_physical_memory("P100A fixed reversible GDDR samples",
				profile.bar0_host_start + 0x19000000, B_PAGE_SIZE,
				B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA | B_UNCACHED_MEMORY, &address);
			if (dataArea < B_OK) return false;
			data = (volatile uint32*)address;
		}
		return true;
	}
	uint32 SourceRead(unsigned word) {
		uint32 offset = BlackholeDramSourceOffset(word);
		return offset == UINT32_MAX ? UINT32_MAX
			: PioneerRead32((const volatile uint32*)((const uint8*)nodeID - 0x44 + offset));
	}
	uint32 DramIdRead(unsigned word) {
		uint32 offset = BlackholeDramIdOffset(word);
		return offset == UINT32_MAX ? UINT32_MAX : PioneerRead32(dramNiu + offset / 4);
	}
	uint32 DramTlbRead(unsigned slot, unsigned word) {
		return slot < 2 && word < 3 ? PioneerRead32(tlb - 3 + slot * 3 + word) : UINT32_MAX;
	}
	void DramTlbWrite(unsigned slot, unsigned word, uint32 value) {
		if (slot < 2 && word < 3) PioneerWrite32(tlb - 3 + slot * 3 + word, value);
	}
	uint32 DramRead(unsigned word) { return data != NULL && word < TT_DRAM_WORDS ? PioneerRead32(data + word) : UINT32_MAX; }
	void DramWrite(unsigned word, uint32 value) { if (data != NULL && word < TT_DRAM_WORDS) PioneerWrite32(data + word, value); }
	bool AfterNodeId(uint32 id) {
		if (!PioneerTensixIO::AfterNodeId(id)) return false;
		dramAttempted = true;
		dprintf("PIONEER-P100A: seven-bank DRAM %s transaction, no core start/DMA/flash\n", writeSamples ? "reversible sample" : "read-only route");
		return BlackholeDramSamplesProbe(*this, telemetry, tensix, writeSamples, dram);
	}
	bool Unmap()
	{
		bool okay = true;
		if (dataArea >= B_OK) {
			if (delete_area(dataArea) == B_OK) dataArea = -1;
			else okay = false;
		}
		if (dramArea >= B_OK) {
			if (delete_area(dramArea) == B_OK) dramArea = -1;
			else okay = false;
		}
		if (dramAttempted && !dram.tlb_restored) {
			for (unsigned slot = 0; slot < 2; slot++) for (unsigned word = 0; word < 3; word++) DramTlbWrite(slot,word,dram.tlb_before[slot][word]);
			dram.tlb_restored = 1;
			for (unsigned slot = 0; slot < 2; slot++) for (unsigned word = 0; word < 3; word++) {
				dram.tlb_after[slot][word] = DramTlbRead(slot,word);
				dram.tlb_restored &= dram.tlb_after[slot][word] == dram.tlb_before[slot][word];
			}
		}
		// Never restore payload in cleanup after ownership/route uncertainty.
		if (dramAttempted && (!dram.data_restored || !dram.tlb_restored || dram.recovery_required)) okay = false;
		return PioneerTensixIO::Unmap() && okay;
	}
};

struct PioneerComputeNocIO : PioneerDramSamplesIO {
	tt_compute_noc_result& noc;
	area_id nocArea=-1;
	const volatile uint32* nocRegisters=NULL;
	bool nocAttempted=false;
	const unsigned targetY;
	PioneerComputeNocIO(const tt_pioneer_profile& p,tt_arc_scratch_result& a,
		tt_arc_telemetry_result& t,tt_tensix_reset_result& c,tt_dram_samples_result& d,
		tt_compute_noc_result& n,unsigned y=2) : PioneerDramSamplesIO(p,a,t,c,d,false),noc(n),targetY(y) {}
	~PioneerComputeNocIO() { if (nocArea>=B_OK) delete_area(nocArea); }
	bool Map() {
		if (targetY!=2 && targetY!=3) return false;
		if (!PioneerDramSamplesIO::Map()) return false;
		void* address=NULL;
		// Both NIU command-register pages; target reads only. TLB 201 is
		// saved/restored by policy, forced to the fixed selected (1,2)/(1,3).
		nocArea=map_physical_memory("P100A compute NoC0 read-only state",
			profile.bar0_host_start+0x19320000,2*B_PAGE_SIZE,B_ANY_KERNEL_ADDRESS,
			B_KERNEL_READ_AREA|B_UNCACHED_MEMORY,&address);
		if (nocArea<B_OK) return false;
		nocRegisters=(const volatile uint32*)address; return true;
	}
	uint32 ComputeNocRead(uint32 offset) {
		return BlackholeComputeNocReadOffset(offset) ? PioneerRead32(nocRegisters+offset/4) : UINT32_MAX;
	}
	bool AfterNodeId(uint32 id) {
		if (!PioneerDramSamplesIO::AfterNodeId(id)) return false;
		nocAttempted=true;
		return BlackholeComputeNocProbe(*this,dram,noc,targetY);
	}
	bool Unmap() {
		bool okay=true;
		if (nocArea>=B_OK) {
			if (delete_area(nocArea)==B_OK) nocArea=-1; else okay=false;
		}
		if (nocAttempted && !noc.tlb_restored) {
			for (unsigned w=0;w<3;w++) DramTlbWrite(1,w,noc.tlb_before[w]);
			noc.tlb_restored=1;
			for (unsigned w=0;w<3;w++) noc.tlb_restored &= DramTlbRead(1,w)==noc.tlb_before[w];
		}
		if (nocAttempted && (!noc.tlb_restored || noc.recovery_required)) okay=false;
		return PioneerDramSamplesIO::Unmap() && okay;
	}
};

struct PioneerFleetNocIO : PioneerComputeNocIO {
	tt_pioneer_fleet_noc_result& fleet;
	PioneerFleetNocIO(const tt_pioneer_profile& p,tt_pioneer_fleet_noc_result& r)
		: PioneerComputeNocIO(p,r.baseline.control.control.control.arc,r.baseline.control.control.telemetry,
			r.baseline.control.tensix,r.baseline.dram,r.worker[0]),fleet(r) {}
	bool AfterNodeId(uint32 id) {
		fleet.version=1;
		if (!PioneerDramSamplesIO::AfterNodeId(id) || !BlackholeDramAllCoresHeld(*this,dram)) return false;
		for (unsigned worker=0;worker<TT_PIONEER_WORKERS;worker++) {
			fleet.checked++;
			bool okay=PioneerWorkerNocProbe(*this,dram,worker,fleet.worker[worker]);
			fleet.recovery_required |= fleet.worker[worker].recovery_required;
			if (!okay) return false;
			fleet.passed++;
		}
		return true;
	}
};

struct PioneerResidentReadIO : PioneerComputeNocIO {
	tt_resident_read_result& read;
	area_id sourceArea=-1,scratchArea=-1,writeArea=-1;
	const volatile uint32* sourceData=NULL;
	volatile uint32* scratchData=NULL;
	volatile uint32* niuWrite=NULL;
	bool readAttempted=false;
	PioneerResidentReadIO(const tt_pioneer_profile& p,tt_arc_scratch_result& a,
		tt_arc_telemetry_result& t,tt_tensix_reset_result& c,tt_dram_samples_result& d,
		tt_compute_noc_result& n,tt_resident_read_result& r)
		: PioneerComputeNocIO(p,a,t,c,d,n),read(r) {}
	~PioneerResidentReadIO() {
		if (sourceArea>=B_OK) delete_area(sourceArea);
		if (scratchArea>=B_OK) delete_area(scratchArea);
		if (writeArea>=B_OK) delete_area(writeArea);
	}
	bool Map() {
		if (!PioneerComputeNocIO::Map()) return false;
		void* address=NULL;
		sourceArea=map_physical_memory("P100A fixed GDDR read-only source",
			profile.bar0_host_start+0x19000000,B_PAGE_SIZE,B_ANY_KERNEL_ADDRESS,
			B_KERNEL_READ_AREA|B_UNCACHED_MEMORY,&address);
		if (sourceArea<B_OK) return false;
		sourceData=(const volatile uint32*)address;
		scratchArea=map_physical_memory("P100A sole tile final 64-byte L1 destination",
			profile.bar0_host_start+0x1917f000,B_PAGE_SIZE,B_ANY_KERNEL_ADDRESS,
			B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA|B_UNCACHED_MEMORY,&address);
		if (scratchArea<B_OK) return false;
		scratchData=(volatile uint32*)((uint8*)address+0xfc0);
		writeArea=map_physical_memory("P100A fixed NIU0 initiator 0 control",
			profile.bar0_host_start+0x19320000,B_PAGE_SIZE,B_ANY_KERNEL_ADDRESS,
			B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA|B_UNCACHED_MEMORY,&address);
		if (writeArea<B_OK) return false;
		niuWrite=(volatile uint32*)address; return true;
	}
	uint32 ResidentNocRead(uint32 offset) {
		return BlackholeResidentReadRegister(offset)?PioneerRead32(nocRegisters+offset/4):UINT32_MAX;
	}
	void ResidentNocWrite(uint32 offset,uint32 value) {
		if (BlackholeResidentWriteRegister(offset)) PioneerWrite32(niuWrite+offset/4,value);
	}
	uint32 ResidentGddrRead(unsigned word) { return word<16?PioneerRead32(sourceData+word):UINT32_MAX; }
	uint32 ResidentL1Read(unsigned word) { return word<16?PioneerRead32(scratchData+word):UINT32_MAX; }
	void ResidentL1Write(unsigned word,uint32 value) { if (word<16) PioneerWrite32(scratchData+word,value); }
	void Pause() { spin(100); }
	bool AfterNodeId(uint32 id) {
		if (!PioneerComputeNocIO::AfterNodeId(id)) return false;
		readAttempted=true;
		dprintf("PIONEER-P100A: single fixed 64-byte GDDR-to-L1 read, cores held, no host DMA\n");
		return BlackholeResidentReadProbe(*this,dram,noc,read);
	}
	bool Unmap() {
		bool okay=true;
		area_id* areas[]={&sourceArea,&scratchArea,&writeArea};
		for (area_id* area:areas) if (*area>=B_OK) {
			if (delete_area(*area)==B_OK) *area=-1; else okay=false;
		}
		// The policy alone owns NIU/L1 cleanup. Uncertain completion permits
		// only TLB control rollback, never a request/reset/payload retry.
		if (readAttempted && (!read.l1_restored || !read.niu_restored || !read.tlb_restored || read.recovery_required)) okay=false;
		return PioneerComputeNocIO::Unmap() && okay;
	}
};

struct PioneerHostPathIO : PioneerComputeNocIO {
	tt_host_path_result& path;
	area_id inboundArea=-1,siiArea=-1;
	const volatile uint32* inbound=NULL;
	const volatile uint32* sii=NULL;
	bool pathAttempted=false;
	PioneerHostPathIO(const tt_pioneer_profile& p,tt_arc_scratch_result& a,
		tt_arc_telemetry_result& t,tt_tensix_reset_result& c,tt_dram_samples_result& d,
		tt_compute_noc_result& n,tt_host_path_result& r)
		: PioneerComputeNocIO(p,a,t,c,d,n),path(r) {}
	~PioneerHostPathIO() {
		if (inboundArea>=B_OK) delete_area(inboundArea);
		if (siiArea>=B_OK) delete_area(siiArea);
	}
	bool Map() {
		if (!PioneerComputeNocIO::Map()) return false;
		void* address=NULL;
		// Same Cadence AT page already configured by ECAM, strictly read-only.
		inboundArea=map_physical_memory("P100A root inbound read-only audit",
			profile.controller+0x00400000,B_PAGE_SIZE,B_ANY_KERNEL_ADDRESS,
			B_KERNEL_READ_AREA|B_UNCACHED_MEMORY,&address);
		if (inboundArea<B_OK) return false;
		inbound=(const volatile uint32*)((uint8*)address+0x810);
		siiArea=map_physical_memory("P100A fixed PCIe SII read-only audit",
			profile.bar0_host_start+0x19000000,B_PAGE_SIZE,B_ANY_KERNEL_ADDRESS,
			B_KERNEL_READ_AREA|B_UNCACHED_MEMORY,&address);
		if (siiArea<B_OK) return false;
		sii=(const volatile uint32*)((uint8*)address+0x134); return true;
	}
	uint32 HostInboundRead(unsigned w) { return w<2?PioneerRead32(inbound+w):UINT32_MAX; }
	uint32 HostSiiRead(unsigned w) { return w<8?PioneerRead32(sii+w):UINT32_MAX; }
	bool AfterNodeId(uint32 id) {
		if (!PioneerComputeNocIO::AfterNodeId(id)) return false;
		pathAttempted=true;
		dprintf("PIONEER-P100A: read-only host-transfer route/inbound/SII audit; no DMA or bus mastering\n");
		return BlackholeHostPathProbe(*this,dram,noc,id,path);
	}
	bool Unmap() {
		bool okay=true;
		area_id* areas[]={&inboundArea,&siiArea};
		for (area_id* area:areas) if (*area>=B_OK) {
			if (delete_area(*area)==B_OK) *area=-1; else okay=false;
		}
		if (pathAttempted && (!path.tlb_restored || path.recovery_required)) okay=false;
		return PioneerComputeNocIO::Unmap() && okay;
	}
};

struct PioneerHostDmaAuditIO : PioneerHostPathIO {
	tt_host_dma_audit_result& audit;
	area_id hdmaArea=-1;
	const volatile uint32* hdma=NULL;
	bool auditAttempted=false;
	PioneerHostDmaAuditIO(const tt_pioneer_profile& p,tt_arc_scratch_result& a,
		tt_arc_telemetry_result& t,tt_tensix_reset_result& c,tt_dram_samples_result& d,
		tt_compute_noc_result& n,tt_host_path_result& h,tt_host_dma_audit_result& r)
		: PioneerHostPathIO(p,a,t,c,d,n,h),audit(r) {}
	~PioneerHostDmaAuditIO() { if (hdmaArea>=B_OK) delete_area(hdmaArea); }
	bool Map() {
		if (!PioneerHostPathIO::Map()) return false;
		void* address=NULL;
		hdmaArea=map_physical_memory("P100A firmware HDMA ch0 read-only audit",
			profile.bar0_host_start+0x19180000,B_PAGE_SIZE,B_ANY_KERNEL_ADDRESS,
			B_KERNEL_READ_AREA|B_UNCACHED_MEMORY,&address);
		if (hdmaArea<B_OK) return false;
		hdma=(const volatile uint32*)address; return true;
	}
	uint32 HostPciRead(unsigned peer,uint16 offset,uint8 size) {
		return peer==0?CardRead(offset,size):peer==1?RootRead(offset,size):UINT32_MAX;
	}
	uint32 HostHdmaRead(unsigned w) { return w<4?PioneerRead32(hdma+BlackholeHostHdmaOffset(w)/4):UINT32_MAX; }
	bool AfterNodeId(uint32 id) {
		if (!PioneerHostPathIO::AfterNodeId(id)) return false;
		auditAttempted=true;
		dprintf("PIONEER-P100A: read-only PCIe pending/firmware HDMA ch0 audit; no DMA or bus mastering\n");
		return BlackholeHostDmaAudit(*this,dram,noc,path,audit);
	}
	bool Unmap() {
		bool okay=true;
		if (hdmaArea>=B_OK) { if (delete_area(hdmaArea)==B_OK) hdmaArea=-1; else okay=false; }
		if (auditAttempted && (!audit.tlb_restored || audit.recovery_required)) okay=false;
		return PioneerHostPathIO::Unmap() && okay;
	}
};

struct PioneerHostReadIO : PioneerHostDmaAuditIO, PioneerHostPageIO {
	tt_host_read_result& read;
	const unsigned payloadBytes;
	uint32* backup=NULL;
	area_id scratchArea=-1,writeArea=-1;
	volatile uint32* scratchData=NULL;
	volatile uint32* niuWrite=NULL;
	bool readAttempted=false;
	PioneerHostReadIO(const tt_pioneer_profile& p,tt_arc_scratch_result& a,
		tt_arc_telemetry_result& t,tt_tensix_reset_result& c,tt_dram_samples_result& d,
		tt_compute_noc_result& n,tt_host_path_result& h,tt_host_dma_audit_result& auditResult,tt_host_read_result& r,unsigned bytes)
		: PioneerHostDmaAuditIO(p,a,t,c,d,n,h,auditResult),read(r),payloadBytes(bytes) {}
	~PioneerHostReadIO() {
		free(backup);
		if (scratchArea>=B_OK) delete_area(scratchArea);
		if (writeArea>=B_OK) delete_area(writeArea);
	}
	bool Map() {
		// 128/8192 are only the separate fixed write/readback policies:
		// two distinct L1 regions, never a user-selected size.
		if (payloadBytes!=64 && payloadBytes!=128 && payloadBytes!=4096 && payloadBytes!=8192) return false;
		if (!PioneerHostDmaAuditIO::Map()) return false;
		void* address=NULL;
		unsigned mappedBytes=payloadBytes==8192?8192:4096;
		scratchArea=map_physical_memory("P100A owned host transfer fixed L1 scratch",
			profile.bar0_host_start+0x19180000-mappedBytes,mappedBytes,B_ANY_KERNEL_ADDRESS,
			B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA|B_UNCACHED_MEMORY,&address);
		if (scratchArea<B_OK) return false;
		scratchData=(volatile uint32*)((uint8*)address+(mappedBytes-payloadBytes));
		writeArea=map_physical_memory("P100A owned host read fixed NIU0 initiator",
			profile.bar0_host_start+0x19320000,B_PAGE_SIZE,B_ANY_KERNEL_ADDRESS,
			B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA|B_UNCACHED_MEMORY,&address);
		if (writeArea<B_OK) return false;
		niuWrite=(volatile uint32*)address; return true;
	}
	void HostPciWrite(unsigned peer,uint16 offset,uint8 size,uint32 value) {
		if (offset!=4 || size!=2 || (value!=0x402 && value!=0x406)) return;
		if (peer==0) CardWrite(offset,size,value); else if (peer==1) RootWrite(offset,size,value);
	}
	uint32 ResidentNocRead(uint32 offset) {
		return BlackholeResidentReadRegister(offset)?PioneerRead32(nocRegisters+offset/4):UINT32_MAX;
	}
	void ResidentNocWrite(uint32 offset,uint32 value) {
		if (BlackholeResidentWriteRegister(offset)) PioneerWrite32(niuWrite+offset/4,value);
	}
	uint32* HostReadBackup(unsigned words) {
		if (backup!=NULL || words!=payloadBytes/4 || (words!=16 && words!=32 && words!=1024 && words!=2048)) return NULL;
		backup=(uint32*)malloc(words*sizeof(uint32)); return backup;
	}
	uint32 ResidentL1Read(unsigned w) { return w<payloadBytes/4?PioneerRead32(scratchData+w):UINT32_MAX; }
	void ResidentL1Write(unsigned w,uint32 value) { if (w<payloadBytes/4) PioneerWrite32(scratchData+w,value); }
	void Pause() { spin(100); }
	bool AfterNodeId(uint32 id) {
		if (!PioneerHostDmaAuditIO::AfterNodeId(id)) return false;
		readAttempted=true;
		dprintf("PIONEER-P100A: single %u-byte owned high host-page read; all cores held, no host writes\n",payloadBytes);
		if (payloadBytes==4096) return BlackholeHostReadProbe<PioneerHostReadIO,4096>(*this,dram,noc,path,audit,read);
		return payloadBytes==64 && BlackholeHostReadProbe(*this,dram,noc,path,audit,read);
	}
	bool Unmap() {
		bool okay=true;
		area_id* areas[]={&scratchArea,&writeArea};
		for (area_id* mapped:areas) if (*mapped>=B_OK) {
			if (delete_area(*mapped)==B_OK) *mapped=-1; else okay=false;
		}
		if (readAttempted && (!read.bme_restored || !read.l1_restored || !read.niu_restored
			|| !read.tlb_restored || read.recovery_required)) okay=false;
		return PioneerHostDmaAuditIO::Unmap() && okay;
	}
};

struct PioneerHostWriteIO : PioneerHostReadIO {
	tt_host_write_result& write;
	const unsigned writeBytes;
	PioneerHostWriteIO(const tt_pioneer_profile& p,tt_arc_scratch_result& a,
		tt_arc_telemetry_result& t,tt_tensix_reset_result& c,tt_dram_samples_result& d,
		tt_compute_noc_result& n,tt_host_path_result& h,tt_host_dma_audit_result& auditResult,tt_host_write_result& r,unsigned bytes)
		: PioneerHostReadIO(p,a,t,c,d,n,h,auditResult,r.transfer,2*bytes),write(r),writeBytes(bytes) {}
	uint32 ResidentNocRead(uint32 offset) {
		return BlackholeHostWriteReadRegister(offset)?PioneerRead32(nocRegisters+offset/4):UINT32_MAX;
	}
	bool AfterNodeId(uint32 id) {
		if (!PioneerHostDmaAuditIO::AfterNodeId(id)) return false;
		readAttempted=true;
		dprintf("PIONEER-P100A: single owned host %u-byte write + distinct L1 readback; all cores held\n",writeBytes);
		if (writeBytes==4096) return BlackholeHostWriteProbe<PioneerHostWriteIO,4096>(*this,dram,noc,path,audit,write);
		return writeBytes==64 && BlackholeHostWriteProbe(*this,dram,noc,path,audit,write);
	}
};
#include "PioneerQ38MixedLiveNativeState.h"
#include "PioneerQ38RecurrentBlockNativeState.h"
#include "PioneerQ38RecurrentPrefixNativeState.h"
struct PioneerSessionIO : PioneerHostWriteIO,PioneerOutputBufferIO,PioneerWideOutputIO {
#ifdef PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT
 bool ResidentLayerCallScopeMatches();bool ResidentLayerBindSourceScope();
 bool ResidentLayerInitializationBaseScope();bool ResidentLayerInitializationSourceScope(bool,bool);
 bool ResidentLayerInitializationOtherCoresHeld(bool);bool ResidentLayerPrefixInitializationScope();
 bool ResidentLayerSourceScope(bool);bool ResidentLayerMappingSourceScope();
 bool ResidentLayerMemoryScope(const PioneerQ38ResidentLayerMemory&,const PioneerQ38CanonicalModelPlan&,
  const PioneerQ38GdnIdentity&,const uint8_t*,unsigned);
 bool ResidentLayerExecutionScope(const PioneerQ38ResidentLayerOwner&,const PioneerQ38ProjectionGraphOwner&,const PioneerQ38ResidentLayerMemory&,
  const PioneerQ38ProjectionMutableBanks&,const PioneerQ38CanonicalModelPlan&,const PioneerQ38GdnIdentity&,const uint8_t*,unsigned);
 void ResidentLayerRetainHostOwners();
#include "PioneerQ38ResidentLayerNativeMapping.inc"
#endif
#ifdef PIONEER_Q38_RESIDENT_OUTPUT_OWNER
 bool ResidentOutputCallScopeMatches();bool ResidentOutputBindSourceScope();
 bool ResidentOutputInitializationBaseScope();bool ResidentOutputInitializationSourceScope(bool,bool);
 bool ResidentOutputInitializationOtherCoresHeld(bool);
 bool ResidentOutputPrefixInitializationScope();
 bool ConvHistoryZeroBeforeCohort();bool ConvHistoryZeroOtherCoresHeld();
 bool ResidentOutputSourceScope(bool);bool ResidentOutputMappingSourceScope();
 bool ResidentOutputMemoryScope(const PioneerQ38ResidentOutputMemory&,const PioneerQ38CanonicalModelPlan&,
  const PioneerQ38GdnIdentity&,const uint8_t*,unsigned);
 bool ResidentOutputExecutionScope(const PioneerQ38ResidentOutputOwner&,const PioneerQ38ProjectionGraphOwner&,const PioneerQ38ResidentOutputMemory&,
  const PioneerQ38ProjectionMutableBanks&,const PioneerQ38CanonicalModelPlan&,const PioneerQ38GdnIdentity&,const uint8_t*,unsigned);
 void ResidentOutputRetainHostOwners();
#include "PioneerQ38ResidentOutputNativeMapping.inc"
#endif
#ifdef PIONEER_Q38_RESIDENT_DECODER_NATIVE
 bool DecoderCallScopeMatches();
#ifdef PIONEER_Q38_GDN_STATE_INIT_NATIVE
 bool RecurrentZeroSourceScope();
#endif
#ifdef PIONEER_Q38_GDN_RECURRENT_NATIVE
 bool ResidentGdnRecurrentScope(const PioneerQ38ResidentDecoderOwner&,const PioneerQ38ProjectionGraphOwner&,const PioneerQ38GdnBanks&);
#endif
 bool ConvHistoryZeroBeforeCohort();bool ConvHistoryZeroOtherCoresHeld();
 bool ResidentDecoderScope(const PioneerQ38ResidentDecoderOwner&,const PioneerQ38ProjectionGraphOwner&,const PioneerQ38ConvHistoryBanks&);
#endif
#ifdef PIONEER_Q38_EMBEDDING_NATIVE
	bool EmbeddingSourceScope(bool controller);
	bool ProjectionZeroBeforeCohort();bool ProjectionZeroOtherCoresHeld();
	bool EmbeddingNativeScope(const PioneerQ38EmbeddingController&,const PioneerQ38ProjectionMutableBanks&,
		const PioneerQ38CanonicalModelPlan&,const PioneerQ38ProjectionIdentity&,const uint8_t*,unsigned);
	bool EmbeddingOtherCoresHeld();void EmbeddingRetainHostOwners();
#ifdef PIONEER_Q38_LEARNED_RMS_NATIVE
	bool LearnedRmsSourceScope(bool controller);
	bool LearnedRmsNativeScope(const PioneerQ38LearnedRmsController&,const PioneerQ38ProjectionMutableBanks&,
		const PioneerQ38CanonicalModelPlan&,const PioneerQ38EmbeddingController&,const PioneerQ38ProjectionIdentity&,const uint8_t*,unsigned);
	bool LearnedRmsOtherCoresHeld();
#endif
	area_id q38EmbeddingMemoryArea=-1;volatile uint32* q38EmbeddingMemoryView=NULL;
	bool q38EmbeddingMemoryMapAttempted=false;uint64 q38EmbeddingMemoryBar=0;
#endif
#if defined(PIONEER_Q38_PROJECTION_GRAPH_MEMORY) || defined(PIONEER_Q38_PROJECTION_EXECUTION_OWNER)
	bool ProjectionGraphMemoryScope(const PioneerQ38ProjectionGraphMemory&,const PioneerQ38CanonicalModelPlan&,
		const PioneerQ38GdnIdentity&,const uint8_t*,unsigned);
	void ProjectionGraphRetainHostOwners();
	area_id q38ProjectionMemoryArea[3]={-1,-1,-1};
	volatile uint32* q38ProjectionMemoryView[3]={NULL,NULL,NULL};
	bool q38ProjectionMemoryMapAttempted=false;
	uint64 q38ProjectionMemoryBar=0;
#endif
#ifdef PIONEER_Q38_PROJECTION_EXECUTION_OWNER
	bool ProjectionExecutionSourceScope(bool controller);
	bool ProjectionGraphExecutionScope(const PioneerQ38ProjectionGraphOwner&,const PioneerQ38ProjectionGraphMemory&,const PioneerQ38ProjectionMutableBanks&,
		const PioneerQ38CanonicalModelPlan&,const PioneerQ38LearnedRmsController&,const PioneerQ38ProjectionIdentity&,const uint8_t*,unsigned);
#endif
#ifdef PIONEER_Q38_FFN_NATIVE_GRAPH
	bool FfnGraphScope(const PioneerQ38FfnPersistentGraphOwner&,const PioneerQ38GraphMutableBanks&,
		const PioneerQ38CanonicalModelPlan&,const PioneerQ38FfnIdentity&,const uint8_t*,unsigned);
	void FfnGraphRetainHostOwners();
#endif
#ifdef PIONEER_Q38_HOST_PHASE_PROFILE
	PioneerQ38HostPhaseProfile q38HostPhaseProfile;
	bool q38HostPhaseReportAllowed=false; // set ONLY by validated explicit BANK_END
#endif
#ifdef PIONEER_Q38_GDN_MUTABLE_STATE
	PioneerStoreContext* q38MutableContext=NULL;
	bool StateZeroBeforeCohort();
	bool StateZeroOtherCoresHeld();
	bool NonRootHeldOwner();
	bool NonRootHeldMappingStable();
#endif
#ifdef PIONEER_Q38_RESIDENT_GDN_ROOT_DIAGNOSTIC
	bool ResidentGdnBeforeRoot();
	bool ResidentGdnOtherCoresHeld();
#endif
#ifdef PIONEER_Q38_SPAN_PRELOAD
	bool q38SpanAllowed=true;
	bool SpanBeforeCohort() { return q38SpanAllowed && !fleetEnabled; }
	bool SpanViewCapacity(unsigned words) { return words<=TT_Q38_PANEL_VIEW_WORDS+1; }
	bool SpanOtherCoresHeld() { return BlackholeDramAllCoresHeld(*this,dram); }
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_PANEL
	area_id q38PanelArea=-1;
	volatile uint32* q38PanelView=NULL;
	uint32* q38PanelSaved=NULL;
	bool q38PanelMapAttempted=false;
#if defined(PIONEER_Q38_FFN_CODE_DIAGNOSTIC) || defined(PIONEER_Q38_FFN_NATIVE_GRAPH)
	area_id q38FfnCodeArea=-1;
	volatile uint32* q38FfnCodeView=NULL;
	bool q38FfnCodeMapAttempted=false;
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_COHORT
	uint32* q38CohortSaved[TT_Q38_COHORT_WORKERS]={};
#endif
#endif
	PioneerRuntimeProfile runtimeProfile;
	area_id runtimeAreas[5]={-1,-1,-1,-1,-1};
	volatile uint32* runtimeEntry=NULL;
	volatile uint32* runtimeCode=NULL;
	volatile uint32* runtimeBuffers=NULL;
	volatile uint32* runtimeControl=NULL;
	const volatile uint32* runtimePc=NULL;
	uint32* runtimeSaved=NULL;
	uint32* secondRuntimeSaved=NULL;
	uint32* secondLeaseSaved=NULL;
	uint32* runtimeStage=NULL;
	area_id attentionArea=-1;
	volatile uint32* attentionView=NULL;
	uint32* attentionSaved=NULL;
	uint32* peerAttentionSaved[8]={};
	bool peerAttentionEnabled=false;
	bool peerRopeEnabled=false; // selected once before ANY peer code owner
	bool attentionMapAttempted=false;
	area_id bulkArea=-1;
	volatile uint32* bulkView=NULL;
	uint32* bulkSaved=NULL;
	bool bulkMapAttempted=false;
	unsigned runtimeCore=0;
	bool fleetEnabled=false;
	bool persistentHostStages=false;
	bool payloadBlocks=false;
	area_id coordinatorArea=-1;
	volatile uint32* coordinatorView=NULL;
	uint32* coordinatorSaved=NULL;
	bool coordinatorMapAttempted=false;
	area_id layerArea=-1;
	volatile uint32* layerView=NULL;
	uint32* layerSaved=NULL;
	bool layerMapAttempted=false;
	uint32* decoderSaved=NULL;
	bool decoderMapAttempted=false;
	area_id fleetArea=-1;
	volatile uint32* fleetView=NULL;
	uint32* fleetRuntimeSaved[TT_PIONEER_WORKERS]={};
	uint32* fleetLeaseSaved[TT_PIONEER_WORKERS]={};
	uint32* fleetCodeSaved[TT_PIONEER_WORKERS]={};
	bool runtimeMapAttempted=false,runtimeMapped=false;
	unsigned runtimeBufferWords=8192;
	PioneerSessionIO(const tt_pioneer_profile& p,tt_arc_scratch_result& a,
		tt_arc_telemetry_result& t,tt_tensix_reset_result& c,tt_dram_samples_result& d,
		tt_compute_noc_result& n,tt_host_path_result& h,tt_host_dma_audit_result& auditResult,tt_host_write_result& r)
		: PioneerHostWriteIO(p,a,t,c,d,n,h,auditResult,r,4096) {}
	~PioneerSessionIO() {
#ifdef PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT
  for(auto area:q38ResidentLayerMemoryArea)if(area>=B_OK)delete_area(area); // known-clean context destruction ONLY
#endif
#ifdef PIONEER_Q38_RESIDENT_OUTPUT_OWNER
  for(auto area:q38ResidentOutputMemoryArea)if(area>=B_OK)delete_area(area); // known-clean context destruction ONLY
#endif
#if defined(PIONEER_Q38_PROJECTION_GRAPH_MEMORY) || defined(PIONEER_Q38_PROJECTION_EXECUTION_OWNER)
		// An uncertain projection context is retained before destruction.
		for (auto area:q38ProjectionMemoryArea) if (area>=B_OK) delete_area(area);
#endif
#if defined(PIONEER_Q38_FFN_CODE_DIAGNOSTIC) || defined(PIONEER_Q38_FFN_NATIVE_GRAPH)
		if (q38FfnCodeArea>=B_OK) delete_area(q38FfnCodeArea);
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_PANEL
		if (q38PanelArea>=B_OK) delete_area(q38PanelArea);
		free(q38PanelSaved);
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_COHORT
		for (unsigned w=1;w<TT_Q38_COHORT_WORKERS;w++) free(q38CohortSaved[w]);
#endif
#endif
#include "PioneerQ38MixedLiveNativeMapDestroy.inc"
#ifdef PIONEER_Q38_RECURRENT_BLOCK_NATIVE_CONTEXT
  for(auto area:q38RecurrentBlockMemoryArea)if(area>=B_OK)delete_area(area); // known-clean destruction ONLY
#endif
#ifdef PIONEER_Q38_RECURRENT_PREFIX_NATIVE_CONTEXT
  for(auto area:q38RecurrentPrefixMemoryArea)if(area>=B_OK)delete_area(area); // known-clean destruction ONLY
#endif
		if (attentionArea>=B_OK) delete_area(attentionArea);
		if (bulkArea>=B_OK) delete_area(bulkArea);
		if (coordinatorArea>=B_OK) delete_area(coordinatorArea);
		free(coordinatorSaved);
		if (layerArea>=B_OK) delete_area(layerArea);
		free(layerSaved);
		free(decoderSaved);
		free(runtimeSaved); free(secondRuntimeSaved); free(secondLeaseSaved); free(runtimeStage);
		free(attentionSaved);
		for (unsigned i=0;i<8;i++) free(peerAttentionSaved[i]);
		free(bulkSaved);
		if (fleetArea>=B_OK) delete_area(fleetArea);
		for (unsigned w=0;w<TT_PIONEER_WORKERS;w++) { free(fleetRuntimeSaved[w]); free(fleetLeaseSaved[w]); free(fleetCodeSaved[w]); }
	}
	void QuarantineHostPage() {
		PioneerHostPageIO::QuarantineHostPage();
		if (outputArea>=B_OK) QuarantineOutputBuffer();
		if (wideArea>=B_OK) QuarantineWideOutput();
	}
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_PANEL
	bool MapQ38Panel() {
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_COHORT
		if (runtimeCore>=120 || (runtimeCore && !fleetEnabled) || !MapRuntime()) return false;
		uint32*& saved=runtimeCore?q38CohortSaved[runtimeCore]:q38PanelSaved;
		if (!saved) saved=(uint32*)malloc((TT_Q38_PANEL_VIEW_WORDS+1)*4);
		if (!saved) return false;
#else
		if (runtimeCore || !MapRuntime()) return false;
#endif
		if (q38PanelView) return Q38PanelMappingStable();
		if (q38PanelMapAttempted) return false;
		q38PanelMapAttempted=true;
#ifndef TT_PIONEER_EXPERIMENTAL_Q38_COHORT
		q38PanelSaved=(uint32*)malloc((TT_Q38_PANEL_VIEW_WORDS+1)*sizeof(uint32));
		if (!q38PanelSaved) return false;
#endif
		void* view=NULL;
		q38PanelArea=map_physical_memory("P100A exact local Q38 panel probe saved L1",profile.bar0_host_start+0x1906e000,
			TT_Q38_PANEL_VIEW_BYTES,B_ANY_KERNEL_ADDRESS,B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA|B_UNCACHED_MEMORY,&view);
		if (q38PanelArea<B_OK) return false;
		q38PanelView=(volatile uint32*)view; return true;
	}
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_COHORT
	bool Q38PanelMappingStable() { return runtimeCore<120 && (!runtimeCore || fleetEnabled) && q38PanelArea>=B_OK && q38PanelView && Q38PanelSavedWords(); }
	uint32* Q38PanelSavedWords() { return runtimeCore<120?(runtimeCore?q38CohortSaved[runtimeCore]:q38PanelSaved):NULL; }
	uint64 Q38CohortNow() { return uint64(system_time()); }
#ifdef TT_PIONEER_Q38_DISPATCH_PROFILE
	// PIONEER_Q38_HOST_PHASE_ADD_BEGIN
	void Q38DispatchProfile(unsigned type,unsigned inputs,unsigned peers,bool valid,const uint64_t* t) {
#ifdef PIONEER_Q38_HOST_PHASE_PROFILE
		q38HostPhaseProfile.Add(type,inputs,peers,valid,t); // CPU memory ONLY,NO per-projection log/card IO
#else
		// The caller has already completed ALL proofs/hold/nonce commit.
		// One diagnostic line,no card reads or additional operations.
		dprintf("P100A_Q38_PROFILE type%u K%u peers%u valid%u us authority%llu stage%llu arm%llu execute%llu proof%llu output%llu hold%llu commit%llu;CPU_WALL_NOT_PURE_GPU\n",
			type,inputs,peers,unsigned(valid),(unsigned long long)t[0],(unsigned long long)t[1],
			(unsigned long long)t[2],(unsigned long long)t[3],(unsigned long long)t[4],
			(unsigned long long)t[5],(unsigned long long)t[6],(unsigned long long)t[7]);
#endif
	}
	// PIONEER_Q38_HOST_PHASE_ADD_END
#ifdef PIONEER_Q38_HOST_PHASE_PROFILE
	// PIONEER_Q38_HOST_PHASE_REPORT_BEGIN
	void Q38HostPhaseReport() {
		for(unsigned type=0;type<3;type++)for(unsigned k=0;k<3;k++) {
			const auto& b=q38HostPhaseProfile.At(type,k);if(!b.count && b.valid)continue;
			const auto* t=b.warm;
			dprintf("P100A_Q38_HOST_PHASE type%u K%u peers119 valid%u total%llu warm%llu us authority%llu stage%llu arm%llu execute%llu proof%llu output%llu hold%llu commit%llu;CPU_WALL_SUMS_FIRST_OF_EACH_SHAPE_EXCLUDED_NOT_PURE_GPU_OR_TOKEN_RATE_NO_PER_PROJECTION_LOG\n",
				type==0?8:type==1?12:14,k==0?5120:k==1?6144:17408,unsigned(b.valid),
				(unsigned long long)b.count,(unsigned long long)(b.count?b.count-1:0),
				(unsigned long long)t[0],(unsigned long long)t[1],(unsigned long long)t[2],(unsigned long long)t[3],
				(unsigned long long)t[4],(unsigned long long)t[5],(unsigned long long)t[6],(unsigned long long)t[7]);
		}
		dprintf("P100A_Q38_HOST_PHASE_END ignored%llu;CPU_METADATA_ONLY_AFTER_EXPLICIT_CLEAN_END\n",(unsigned long long)q38HostPhaseProfile.Ignored());
	}
	// PIONEER_Q38_HOST_PHASE_REPORT_END
#endif
#endif
#else
	bool Q38PanelMappingStable() { return !runtimeCore && q38PanelArea>=B_OK && q38PanelView && q38PanelSaved; }
	uint32* Q38PanelSavedWords() { return q38PanelSaved; }
#endif
	uint32 Q38PanelRead(uint32 a) {
		return q38PanelView && !(a&3) && a>=TT_Q38_PANEL_VIEW_BASE && a-TT_Q38_PANEL_VIEW_BASE<TT_Q38_PANEL_VIEW_BYTES
			?PioneerRead32(q38PanelView+(a-TT_Q38_PANEL_VIEW_BASE)/4):UINT32_MAX;
	}
	void Q38PanelWrite(uint32 a,uint32 value) {
		if (q38PanelView && !(a&3) && a>=TT_Q38_PANEL_VIEW_BASE && a-TT_Q38_PANEL_VIEW_BASE<TT_Q38_PANEL_VIEW_BYTES)
			PioneerWrite32(q38PanelView+(a-TT_Q38_PANEL_VIEW_BASE)/4,value);
	}
#if defined(PIONEER_Q38_FFN_CODE_DIAGNOSTIC) || defined(PIONEER_Q38_FFN_NATIVE_GRAPH)
	// PIONEER_Q38_FFN_CODE_NATIVE_ADAPTER_BEGIN
	bool MapFfnGraphCodePrefix() {
		if (runtimeCore || !fleetEnabled || !runtimeMapped || !Q38PanelMappingStable()) return false;
		if (q38FfnCodeView) return FfnGraphCodeMappingStable(0);
		if (q38FfnCodeMapAttempted) return false;
		q38FfnCodeMapAttempted=true;
		void* view=NULL;
		q38FfnCodeArea=map_physical_memory(
#ifdef PIONEER_Q38_FFN_NATIVE_GRAPH
			"P100A owned persistent full FFN prefix",
#elif defined(PIONEER_Q38_FFN_ROOT_LIFETIME_DIAGNOSTIC)
			"P100A owned ROOT and worker-lifetime prefix",
#elif defined(PIONEER_Q38_FFN_PEER_LIFETIME_DIAGNOSTIC)
			"P100A owned FFN worker-lifetime prefix",
#else
			"P100A owned complete FFN prefix (never-launched diagnostic)",
#endif
			profile.bar0_host_start+0x19064000,40960,B_ANY_KERNEL_ADDRESS,
			B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA|B_UNCACHED_MEMORY,&view);
		if (q38FfnCodeArea<B_OK || !view) return false;
		q38FfnCodeView=(volatile uint32*)view;return true;
	}
	bool FfnGraphCodeMappingStable(unsigned w) {
		return w<120 && runtimeCore==w && fleetEnabled && runtimeMapped && q38FfnCodeArea>=B_OK
			&& q38FfnCodeView && Q38PanelMappingStable();
	}
	uint32 FfnGraphProgramRead(unsigned w,uint32 a) {
		if (!FfnGraphCodeMappingStable(w) || !PioneerQ38FfnGraphProgramAddress(w,a)) return UINT32_MAX;
		if (a==0x6000) return RuntimeCodeRead(0);
		return a>=0x70000?Q38PanelRead(a):PioneerRead32(q38FfnCodeView+(a-0x64000)/4);
	}
	bool FfnGraphProgramReadChecked(unsigned w,uint32 a,uint32& output) {
		// Arbitrary saved prefix bytes may legitimately be UINT32_MAX.
		// Report mapping/address failure SEPARATELY,never as saved data.
		// A false result leaves output untouched and authorizes NO retry.
		if (!PioneerQ38FfnGraphProgramAddress(w,a) || !FfnGraphCodeMappingStable(w)) return false;
		output=a==0x6000?RuntimeCodeRead(0):a>=0x70000?Q38PanelRead(a):PioneerRead32(q38FfnCodeView+(a-0x64000)/4);
		return true;
	}
	bool FfnGraphProgramWrite(unsigned w,uint32 a,uint32 value) {
		if (!FfnGraphCodeMappingStable(w) || !PioneerQ38FfnGraphProgramAddress(w,a)) return false;
		if (a==0x6000) RuntimeCodeWrite(0,value);
		else if (a>=0x70000) Q38PanelWrite(a,value);
		else PioneerWrite32(q38FfnCodeView+(a-0x64000)/4,value);
		return true;
	}
	// PIONEER_Q38_FFN_CODE_NATIVE_ADAPTER_END
#endif
#ifdef PIONEER_Q38_EMBEDDING_NATIVE
	// PIONEER_Q38_EMBEDDING_NATIVE_MAPPING_BEGIN
	bool MapEmbeddingMemory() {
		if (!EmbeddingSourceScope(false) || runtimeCore || fleetEnabled || !runtimeMapped || !Q38PanelMappingStable()
			|| !runtimeEntry || !scratchData || payloadBytes!=8192 || TT_Q38_PANEL_VIEW_BASE!=0x6e000
			|| TT_Q38_PANEL_VIEW_BYTES<0x3e000 || profile.bar0_host_start>UINT64_MAX-0x1906e000) return false;
		if (q38EmbeddingMemoryMapAttempted) return EmbeddingMemoryMappingStable();
		if (q38EmbeddingMemoryArea>=B_OK || q38EmbeddingMemoryView) return false;
		q38EmbeddingMemoryMapAttempted=true;q38EmbeddingMemoryBar=profile.bar0_host_start;void* view=NULL;
		q38EmbeddingMemoryArea=map_physical_memory("P100A retained embedding ROOT prefix",profile.bar0_host_start+0x19064000,
			40960,B_ANY_KERNEL_ADDRESS,B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA|B_UNCACHED_MEMORY,&view);
		if (q38EmbeddingMemoryArea<B_OK || !view) return false; // retain failed/partial map,NO retry
		q38EmbeddingMemoryView=(volatile uint32*)view;return EmbeddingMemoryMappingStable();
	}
	bool EmbeddingMemoryMappingStable() {
		return !runtimeCore && !fleetEnabled && runtimeMapped && runtimeEntry && scratchData && payloadBytes==8192
			&& Q38PanelMappingStable() && q38EmbeddingMemoryMapAttempted && q38EmbeddingMemoryArea>=B_OK && q38EmbeddingMemoryView
			&& profile.bar0_host_start==q38EmbeddingMemoryBar && profile.bar0_host_start<=UINT64_MAX-0x1906e000
			&& TT_Q38_PANEL_VIEW_BASE==0x6e000 && TT_Q38_PANEL_VIEW_BYTES>=0x3e000;
	}
	bool EmbeddingReadChecked(uint32 a,uint32& output) {
		if (!PioneerQ38EmbeddingSnapshotContains(a) || !EmbeddingMemoryMappingStable()) return false;
		uint32 value=a==0x6000?RuntimeCodeRead(0):a<0x6e000?PioneerRead32(q38EmbeddingMemoryView+(a-0x64000)/4)
			:a<0xac000?Q38PanelRead(a):ResidentL1Read(0x1fc0/4+(a-0x17ffc0)/4);
		output=value;return true; // UINT32_MAX saved DATA is valid;NO unowned padding
	}
	bool EmbeddingWrite(uint32 a,uint32 value) {
		if (!PioneerQ38EmbeddingSnapshotContains(a) || !EmbeddingMemoryMappingStable()) return false;
		if (a==0x6000) RuntimeCodeWrite(0,value);
		else if (a<0x6e000) PioneerWrite32(q38EmbeddingMemoryView+(a-0x64000)/4,value);
		else if (a<0xac000) Q38PanelWrite(a,value);
		else ResidentL1Write(0x1fc0/4+(a-0x17ffc0)/4,value);
		return true;
	}
	// PIONEER_Q38_EMBEDDING_NATIVE_MAPPING_END
#endif
#if defined(PIONEER_Q38_PROJECTION_GRAPH_MEMORY) || defined(PIONEER_Q38_PROJECTION_EXECUTION_OWNER)
	// PIONEER_Q38_PROJECTION_MEMORY_NATIVE_ADAPTER_BEGIN
	bool MapProjectionGraphMemory() {
#ifdef PIONEER_Q38_PROJECTION_EXECUTION_OWNER
		if (!ProjectionExecutionSourceScope(false)) return false;
#endif
		if (runtimeCore || !fleetEnabled || !runtimeMapped || !Q38PanelMappingStable()
			|| TT_Q38_PANEL_VIEW_BASE!=0x6e000 || TT_Q38_PANEL_VIEW_BYTES<0x3e000
			|| !runtimeEntry || !scratchData || payloadBytes!=8192
			|| profile.bar0_host_start>UINT64_MAX-0x190c9000) return false;
		if (q38ProjectionMemoryMapAttempted) return ProjectionGraphMemoryMappingStable(0);
		for (unsigned i=0;i<3;i++) if (q38ProjectionMemoryArea[i]>=B_OK || q38ProjectionMemoryView[i]) return false;
		q38ProjectionMemoryMapAttempted=true;
		q38ProjectionMemoryBar=profile.bar0_host_start;
		const uint32 offset[]={0x19064000,0x190b0000,0x190c0000},bytes[]={40960,36864,36864};
		for (unsigned i=0;i<3;i++) {
			void* view=NULL;
			q38ProjectionMemoryArea[i]=map_physical_memory("P100A complete retained projection L1",
				profile.bar0_host_start+offset[i],bytes[i],B_ANY_KERNEL_ADDRESS,
				B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA|B_UNCACHED_MEMORY,&view);
			if (q38ProjectionMemoryArea[i]<B_OK || !view) return false; // retain partial maps,NO retry
			q38ProjectionMemoryView[i]=(volatile uint32*)view;
		}
		return ProjectionGraphMemoryMappingStable(0);
	}
	bool ProjectionGraphMemoryMappingStable(unsigned w) {
		if (w>=120 || runtimeCore!=w || !fleetEnabled || !runtimeMapped || !runtimeEntry || !scratchData
			|| payloadBytes!=8192 || !Q38PanelMappingStable() || !q38ProjectionMemoryMapAttempted
			|| profile.bar0_host_start!=q38ProjectionMemoryBar || profile.bar0_host_start>UINT64_MAX-0x190c9000
			|| TT_Q38_PANEL_VIEW_BASE!=0x6e000 || TT_Q38_PANEL_VIEW_BYTES<0x3e000) return false;
		for (unsigned i=0;i<3;i++) if (q38ProjectionMemoryArea[i]<B_OK || !q38ProjectionMemoryView[i]) return false;
		return true; // CPU mapping presence,NOT source/cookie/physical/launch authority
	}
	bool ProjectionGraphMemoryReadChecked(unsigned w,uint32 a,uint32& output) {
		if (!PioneerQ38ProjectionGraphSnapshotContains(w,a) || !ProjectionGraphMemoryMappingStable(w)) return false;
		uint32 value;
		if (a==0x6000) value=RuntimeCodeRead(0);
		else if (a<0x6e000) value=PioneerRead32(q38ProjectionMemoryView[0]+(a-0x64000)/4);
		else if (a<0xac000) value=Q38PanelRead(a);
		else if (a>=0x17ffc0) value=ResidentL1Read(0x1fc0/4+(a-0x17ffc0)/4);
		else {unsigned i=a>=0xc0000?2:1;value=PioneerRead32(q38ProjectionMemoryView[i]+(a-(i==1?0xb0000:0xc0000))/4);}
		output=value;return true; // UINT32_MAX is valid saved DATA,not a failure sentinel
	}
	bool ProjectionGraphMemoryWrite(unsigned w,uint32 a,uint32 value) {
		if (!PioneerQ38ProjectionGraphSnapshotContains(w,a) || !ProjectionGraphMemoryMappingStable(w)) return false;
		if (a==0x6000) RuntimeCodeWrite(0,value);
		else if (a<0x6e000) PioneerWrite32(q38ProjectionMemoryView[0]+(a-0x64000)/4,value);
		else if (a<0xac000) Q38PanelWrite(a,value);
		else if (a>=0x17ffc0) ResidentL1Write(0x1fc0/4+(a-0x17ffc0)/4,value);
		else {unsigned i=a>=0xc0000?2:1;PioneerWrite32(q38ProjectionMemoryView[i]+(a-(i==1?0xb0000:0xc0000))/4,value);}
		return true;
	}
	// PIONEER_Q38_PROJECTION_MEMORY_NATIVE_ADAPTER_END
#endif
#endif
	bool MapCoordinator() {
		if (runtimeCore!=0) return false;
		if (coordinatorView) return true;
		if (coordinatorMapAttempted) return false;
		coordinatorMapAttempted=true;
		coordinatorSaved=(uint32*)malloc(TT_COORDINATOR_SAVED_WORDS*4);
		if (!coordinatorSaved) return false;
		void* view=NULL;
		coordinatorArea=map_physical_memory("P100A fixed backed-up coordinator code/stack",profile.bar0_host_start+0x1906c000,
			TT_COORDINATOR_SAVED_WORDS*4,B_ANY_KERNEL_ADDRESS,B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA|B_UNCACHED_MEMORY,&view);
		if (coordinatorArea<B_OK) return false;
		coordinatorView=(volatile uint32*)view; return true;
	}
	bool CoordinatorMappingStable() { return runtimeCore==0 && coordinatorArea>=B_OK && coordinatorView && coordinatorSaved; }
	uint32* CoordinatorSavedWords() { return coordinatorSaved; }
	uint32 CoordinatorRead(unsigned w) { return w<TT_COORDINATOR_SAVED_WORDS && coordinatorView?PioneerRead32(coordinatorView+w):UINT32_MAX; }
	void CoordinatorWrite(unsigned w,uint32 value) { if (w<TT_COORDINATOR_SAVED_WORDS && coordinatorView) PioneerWrite32(coordinatorView+w,value); }
	bool MapLayer() {
		if (runtimeCore!=0) return false;
		if (layerView) return true;
		if (layerMapAttempted) return false;
		layerMapAttempted=true; layerSaved=(uint32*)malloc(TT_LAYER_CODE_WORDS*4);
		if (!layerSaved) return false;
		void* view=NULL;
		layerArea=map_physical_memory("P100A fixed backed-up32KiB FFN code",profile.bar0_host_start+0x19064000,
			TT_LAYER_CODE_WORDS*4,B_ANY_KERNEL_ADDRESS,B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA|B_UNCACHED_MEMORY,&view);
		if (layerArea<B_OK) return false;
		layerView=(volatile uint32*)view; return true;
	}
	bool LayerMappingStable() { return runtimeCore==0 && layerArea>=B_OK && layerView && layerSaved; }
	uint32* LayerSavedWords() { return layerSaved; }
	uint32 LayerCodeRead(unsigned w) { return w<TT_LAYER_CODE_WORDS && layerView?PioneerRead32(layerView+w):UINT32_MAX; }
	void LayerCodeWrite(unsigned w,uint32 value) { if (w<TT_LAYER_CODE_WORDS && layerView) PioneerWrite32(layerView+w,value); }
	bool MapDecoder() {
		if (runtimeCore!=0) return false;
		if (decoderSaved) return LayerMappingStable();
		if (decoderMapAttempted) return false;
		decoderMapAttempted=true;
		if (!MapLayer()) return false;
		decoderSaved=(uint32*)malloc(TT_DECODER_CODE_WORDS*4);
		return decoderSaved!=NULL;
	}
	bool DecoderMappingStable() { return decoderSaved && LayerMappingStable(); }
	bool DecoderAttentionMappingStable() { return runtimeCore==0 && attentionArea>=B_OK && attentionView && attentionSaved; }
	bool DecoderPeerMappingsStable() {
		if (!peerAttentionEnabled || !DecoderAttentionMappingStable()) return false;
		for (unsigned i=0;i<8;i++) if (!peerAttentionSaved[i]) return false;
		return true;
	}
	bool DecoderRopeMappingsStable() { return peerRopeEnabled && DecoderPeerMappingsStable(); }
	uint32* DecoderSavedWords() { return decoderSaved; }
	bool MapBulk() {
		if (runtimeCore!=0) return false;
		if (bulkView) return true;
		if (bulkMapAttempted) return false;
		bulkMapAttempted=true;
		bulkSaved=(uint32*)malloc(TT_BULK_UPLOAD_SAVED_WORDS*4);
		if (!bulkSaved) return false;
		void* view=NULL;
		bulkArea=map_physical_memory("P100A fixed owned bulk-upload L1",profile.bar0_host_start+0x19140000,
			TT_BULK_UPLOAD_SAVED_WORDS*4,B_ANY_KERNEL_ADDRESS,B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA|B_UNCACHED_MEMORY,&view);
		if (bulkArea<B_OK) return false;
		bulkView=(volatile uint32*)view; return true;
	}
	bool BulkMappingStable() { return runtimeCore==0 && bulkArea>=B_OK && bulkView && bulkSaved; }
	uint32* BulkSavedWords() { return bulkSaved; }
	uint32 BulkL1Read(unsigned w) { return w<TT_BULK_UPLOAD_SAVED_WORDS && bulkView?PioneerRead32(bulkView+w):UINT32_MAX; }
	void BulkL1Write(unsigned w,uint32 value) { if (w<TT_BULK_UPLOAD_SAVED_WORDS && bulkView) PioneerWrite32(bulkView+w,value); }
	void BulkHostFence() { PioneerIoFence(); }
	uint32 BulkHostLoad(unsigned w) {
		return w<TT_BULK_UPLOAD_WORDS && outputArea>=B_OK && outputPage?B_LENDIAN_TO_HOST_INT32(outputPage[w]):UINT32_MAX;
	}
	void BulkHostStore(unsigned w,uint32 value) {
		if (w<TT_BULK_UPLOAD_WORDS && outputArea>=B_OK && outputPage) outputPage[w]=B_HOST_TO_LENDIAN_INT32(value);
	}
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_COHORT_PAYLOAD
	uint64 BulkHostPairLoad(unsigned w) {
		typedef uint64 Alias64 __attribute__((__may_alias__));
		return !(w&1) && w<TT_BULK_UPLOAD_WORDS-1 && outputArea>=B_OK && outputPage
			?B_LENDIAN_TO_HOST_INT64(*reinterpret_cast<const volatile Alias64*>(outputPage+w)):UINT64_MAX;
	}
	void BulkHostPairStore(unsigned w,uint32 low,uint32 high) {
		typedef uint64 Alias64 __attribute__((__may_alias__));
		if (!(w&1) && w<TT_BULK_UPLOAD_WORDS-1 && outputArea>=B_OK && outputPage)
			*reinterpret_cast<volatile Alias64*>(outputPage+w)=B_HOST_TO_LENDIAN_INT64(uint64(low)|(uint64(high)<<32));
	}
#endif
	struct PayloadIO {
		volatile uint32* view;
		void PayloadFence() { PioneerIoFence(); }
		uint32 PayloadRead(unsigned w) { return B_LENDIAN_TO_HOST_INT32(view[w]); }
		void PayloadWrite(unsigned w,uint32 value) { view[w]=B_HOST_TO_LENDIAN_INT32(value); }
		uint64 PayloadReadPair(unsigned w) {
			// All mapped bases are page aligned; generic block helpers align
			// the word index and never read a pair beyond the checked span.
			typedef uint64 Alias64 __attribute__((__may_alias__));
			return B_LENDIAN_TO_HOST_INT64(*reinterpret_cast<const volatile Alias64*>(view+w));
		}
		void PayloadWritePair(unsigned w,uint32 low,uint32 high) {
			typedef uint64 Alias64 __attribute__((__may_alias__));
			*reinterpret_cast<volatile Alias64*>(view+w)=B_HOST_TO_LENDIAN_INT64(uint64(low)|(uint64(high)<<32));
		}
	};
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_COHORT_PAYLOAD
	PayloadIO Q38PayloadData(uint32 a) { return PayloadIO{q38PanelView+(a-TT_Q38_PANEL_VIEW_BASE)/4}; }
#endif
	bool RuntimePayloadRead(unsigned first,unsigned words,uint32* output) {
		if (!output || !runtimeMapped || !runtimeBuffers || !words || first!=4096 || words>4096
			|| first>=runtimeBufferWords || words>runtimeBufferWords-first) return false;
		if (payloadBlocks) { PayloadIO data={runtimeBuffers}; return PioneerPayloadReadBlock(data,first,words,runtimeBufferWords,output); }
		for (unsigned w=0;w<words;w++) output[w]=RuntimeBufferRead(first+w);
		return true;
	}
	bool CoordinatorPayloadRead(unsigned words,uint32* output) {
		if (!output || runtimeCore!=0 || !runtimeMapped || !runtimeBuffers || !payloadBlocks
			|| !words || words>8192 || runtimeBufferWords!=TT_Q8_BULK_SAVED_BUFFER_WORDS) return false;
		PayloadIO data={runtimeBuffers}; return PioneerPayloadReadBlock(data,4096,words,runtimeBufferWords,output);
	}
	bool BulkPayloadStage(const uint32* input,unsigned words) {
		if (!input || !BulkMappingStable() || !words || words>3072) return false;
		if (payloadBlocks) { PayloadIO data={bulkView}; return PioneerPayloadStageBlock(data,words,3072,input); }
		for (unsigned w=0;w<words;w++) BulkL1Write(w,input[w]);
		bool exact=true;
		for (unsigned w=0;w<words;w++) exact &= BulkL1Read(w)==input[w];
		return exact;
	}
	bool BulkPayloadRead(unsigned first,unsigned words,uint32* output) {
		if (!output || !BulkMappingStable() || first!=4096 || words!=2048) return false;
		if (payloadBlocks) { PayloadIO data={bulkView}; return PioneerPayloadReadBlock(data,first,words,8192,output); }
		for (unsigned w=0;w<words;w++) output[w]=BulkL1Read(first+w);
		return true;
	}
	bool MapRuntime(bool bulk=false) {
		unsigned words=bulk?TT_Q8_BULK_SAVED_BUFFER_WORDS:8192;
		if (runtimeMapped) return runtimeBufferWords==words;
		if (runtimeMapAttempted) return false;
		runtimeMapAttempted=true;
		runtimeBufferWords=words;
		runtimeSaved=(uint32*)malloc((1025+words)*sizeof(uint32)); runtimeStage=(uint32*)malloc(4096);
		if (!runtimeSaved || !runtimeStage) return false;
		const uint32 offsets[]={0x19006000,0x19070000,0x19080000,0x19312000,0x19313000};
		void* views[5]={};
		for (unsigned i=0;i<5;i++) {
			uint32 protection=B_KERNEL_READ_AREA|B_UNCACHED_MEMORY;
			if (i!=4) protection|=B_KERNEL_WRITE_AREA;
			runtimeAreas[i]=map_physical_memory("P100A owned runtime fixed compute view",profile.bar0_host_start+offsets[i],
				i==2?words*sizeof(uint32):B_PAGE_SIZE,B_ANY_KERNEL_ADDRESS,protection,&views[i]);
			if (runtimeAreas[i]<B_OK) return false;
		}
		runtimeEntry=(volatile uint32*)views[0]; runtimeCode=(volatile uint32*)views[1];
		runtimeBuffers=(volatile uint32*)views[2]; runtimeControl=(volatile uint32*)views[3];
		runtimePc=(const volatile uint32*)((const uint8*)views[4]+0x140);
		runtimeMapped=true; return true;
	}
#include "PioneerQ38MixedLiveNativeMapping.inc"
#include "PioneerQ38RecurrentBlockNativeMapping.inc"
#include "PioneerQ38RecurrentPrefixNativeMapping.inc"
	// views is not a core reset or NIU transaction, even during rollback.
	bool SelectRuntimeCore(unsigned core) {
		if (core>=TT_PIONEER_WORKERS || (!fleetEnabled && core>1)) return false;
		unsigned x=fleetEnabled?PioneerWorkerX(core):1,y=fleetEnabled?PioneerWorkerY(core):2+core;
		bool okay=BlackholeDramConfigure(*this,0,0,x,y,false)
			&& BlackholeDramConfigure(*this,1,0xffa00000,x,y,false);
		if (okay) runtimeCore=core;
		return okay;
	}
	bool AllocateFleetWorker(unsigned worker) {
		if (!fleetEnabled || worker>=120 || !runtimeMapped || runtimeBufferWords!=TT_Q8_BULK_SAVED_BUFFER_WORDS) return false;
		if (fleetArea<B_OK) {
			void* view=NULL;
			fleetArea=map_physical_memory("P100A owned fleet prelude page",profile.bar0_host_start+0x19071000,
				B_PAGE_SIZE,B_ANY_KERNEL_ADDRESS,B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA|B_UNCACHED_MEMORY,&view);
			if (fleetArea<B_OK) return false;
			fleetView=(volatile uint32*)view;
		}
		if (!fleetCodeSaved[worker]) fleetCodeSaved[worker]=(uint32*)malloc(4096);
		if (worker && !fleetRuntimeSaved[worker]) fleetRuntimeSaved[worker]=(uint32*)malloc((1025+runtimeBufferWords)*4);
		return fleetCodeSaved[worker] && (!worker || fleetRuntimeSaved[worker]);
	}
	uint32* FleetSavedWords() { return runtimeCore<120?fleetCodeSaved[runtimeCore]:NULL; }
	// Fleet polls still require the same status/PC/counter proofs. Shorter
	// bounded backoff avoids multiplying the old100us delay by120 workers.
	void Pause() { spin(fleetEnabled?1:100); }
	uint32 FleetCodeRead(unsigned word) { return word<1024 && fleetView?PioneerRead32(fleetView+word):UINT32_MAX; }
	void FleetCodeWrite(unsigned word,uint32 value) { if (word<1024 && fleetView) PioneerWrite32(fleetView+word,value); }
	bool AllocateSecondRuntime() {
		if (!runtimeMapped || runtimeBufferWords!=TT_Q8_BULK_SAVED_BUFFER_WORDS) return false;
		if (!secondRuntimeSaved) secondRuntimeSaved=(uint32*)malloc((1025+runtimeBufferWords)*sizeof(uint32));
		return secondRuntimeSaved!=NULL;
	}
	bool MapAttention(bool parallelRope=false) {
		if (parallelRope && (!peerRopeEnabled || !runtimeCore || runtimeCore>8)) return false;
		if ((runtimeCore && (!peerAttentionEnabled || runtimeCore>8)) || (!runtimeMapped && !MapRuntime())) return false;
		uint32*& saved=runtimeCore?peerAttentionSaved[runtimeCore-1]:attentionSaved;
		if (!saved) saved=(uint32*)malloc((parallelRope?TT_ATTN_TILE_ROPE_SAVED_WORDS:TT_ATTN_TILE_SAVED_WORDS)*sizeof(uint32));
		if (!saved) return false;
		if (attentionView) return attentionArea>=B_OK;
		if (attentionMapAttempted) return false;
		attentionMapAttempted=true;
		void* view=NULL;
		attentionArea=map_physical_memory("P100A fixed attention qualification L1",profile.bar0_host_start+0x19072000,
			0x42000,B_ANY_KERNEL_ADDRESS,B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA|B_UNCACHED_MEMORY,&view);
		if (attentionArea<B_OK) return false;
		attentionView=(volatile uint32*)view; return true;
	}
	uint32* AttentionSavedWords() { return !runtimeCore?attentionSaved:peerAttentionEnabled && runtimeCore<=8?peerAttentionSaved[runtimeCore-1]:NULL; }
	bool AttentionRopeMappingStable() { return peerRopeEnabled && runtimeCore && runtimeCore<=8 && attentionArea>=B_OK && attentionView && peerAttentionSaved[runtimeCore-1]; }
	uint32 AttentionReadWord(unsigned w) {
		uint32 address=PioneerAttentionTileWordAddress(w,peerRopeEnabled && runtimeCore && runtimeCore<=8);
		if (address==0x6000) return RuntimeCodeRead(0);
		return address!=UINT32_MAX && attentionView?PioneerRead32(attentionView+(address-0x72000)/4):UINT32_MAX;
	}
	void AttentionWriteWord(unsigned w,uint32 value) {
		uint32 address=PioneerAttentionTileWordAddress(w,peerRopeEnabled && runtimeCore && runtimeCore<=8);
		if (address==0x6000) RuntimeCodeWrite(0,value);
		else if (address!=UINT32_MAX && attentionView) PioneerWrite32(attentionView+(address-0x72000)/4,value);
	}
	uint32* HostReadBackup(unsigned words) {
		if (!runtimeCore) return PioneerHostWriteIO::HostReadBackup(words);
		if (fleetEnabled) {
			if (words!=2048 || runtimeCore>=120 || fleetLeaseSaved[runtimeCore]) return NULL;
			fleetLeaseSaved[runtimeCore]=(uint32*)malloc(8192); return fleetLeaseSaved[runtimeCore];
		}
		if (words!=2048 || secondLeaseSaved) return NULL;
		secondLeaseSaved=(uint32*)malloc(words*sizeof(uint32)); return secondLeaseSaved;
	}
	uint32* RuntimeSavedWords() { return !runtimeCore?runtimeSaved:fleetEnabled?fleetRuntimeSaved[runtimeCore]:secondRuntimeSaved; }
	unsigned RuntimeBufferWords() const { return runtimeBufferWords; }
	uint32* RuntimeStageWords() { return runtimeStage; }
	void RuntimeHostStageFence() { PioneerIoFence(); }
	void RuntimeHostStageStore(unsigned w,uint32 value) {
		if (w<1024 && PioneerHostPageIO::area>=B_OK && PioneerHostPageIO::page)
			PioneerHostPageIO::page[w]=B_HOST_TO_LENDIAN_INT32(value);
	}
	uint32 RuntimeHostStageLoad(unsigned w) {
		return w<1024 && PioneerHostPageIO::area>=B_OK && PioneerHostPageIO::page
			?B_LENDIAN_TO_HOST_INT32(PioneerHostPageIO::page[w]):UINT32_MAX;
	}
	bool RuntimeHostFillVerify(const uint32* data) {
		// Keep the old checked/Assistant paths' transport unchanged. Only
		// the separate persistent session selects boundary-fenced host RAM.
		return persistentHostStages?PioneerRuntimeHostStageBlock(*this,data):PioneerRuntimeHostStage(*this,data,0L);
	}
	void RuntimeProfileMark(unsigned slot) { runtimeProfile.Mark(uint64(system_time()),slot); }
	uint32 RuntimeCodeRead(unsigned w) { return w<=1024?PioneerRead32(w?runtimeCode+w-1:runtimeEntry):UINT32_MAX; }
	void RuntimeCodeWrite(unsigned w,uint32 value) { if (w<=1024) PioneerWrite32(w?runtimeCode+w-1:runtimeEntry,value); }
	uint32 RuntimeBufferRead(unsigned w) { return w<runtimeBufferWords?PioneerRead32(runtimeBuffers+w):UINT32_MAX; }
	void RuntimeBufferWrite(unsigned w,uint32 value) { if (w<runtimeBufferWords) PioneerWrite32(runtimeBuffers+w,value); }
	uint32 RuntimeControlRead(unsigned w) {
		const uint32 offsets[]={0x234,0xa0,0x90}; return w<3?PioneerRead32(runtimeControl+offsets[w]/4):UINT32_MAX;
	}
	void RuntimeResetWrite(uint32 value) {
		if (value==0x87ffff || value==0x46800 || value==0x47800) PioneerWrite32(runtimeControl+0x1b0/4,value);
	}
	uint32 RuntimePcRead() { return PioneerRead32(runtimePc); }
	uint32 RuntimeStatusRead(unsigned w) { return w<3?ResidentL1Read((0x1fc0/4)+w):UINT32_MAX; }
	void RuntimeStatusWrite(unsigned w,uint32 value) { if (w<3) ResidentL1Write((0x1fc0/4)+w,value); }
	bool Unmap() {
		bool okay=true;
#ifdef PIONEER_Q38_RECURRENT_PREFIX_NATIVE_CONTEXT
  // No prefix final-release perimeter is qualified yet. Never dispose
  // its aliases via an old native END/Unmap path, even after STOP/restore.
  if(q38RecurrentPrefixMemoryMapAttempted)return false;
#endif
#ifdef PIONEER_Q38_RECURRENT_BLOCK_FINAL_RELEASE
  if(q38RecurrentBlockMemoryMapAttempted || q38RecurrentBlockFinalUnmapAttempted)return RecurrentBlockFinalUnmap();
#endif
#ifdef PIONEER_Q38_EMBEDDING_NATIVE
#ifdef PIONEER_Q38_MIXED_LIVE_FINAL_RELEASE
  if(q38MixedLiveMemoryMapAttempted || q38MixedLiveFinalUnmapAttempted)return MixedLiveFinalUnmap();
#endif
		if (q38EmbeddingMemoryArea>=B_OK) {
			if (delete_area(q38EmbeddingMemoryArea)==B_OK) q38EmbeddingMemoryArea=-1;
			else okay=false;
		}
#endif
		if (attentionArea>=B_OK) {
			if (delete_area(attentionArea)==B_OK) { attentionArea=-1; attentionView=NULL; } else okay=false;
		}
		for (unsigned i=0;i<5;i++) if (runtimeAreas[i]>=B_OK) {
			if (delete_area(runtimeAreas[i])==B_OK) runtimeAreas[i]=-1; else okay=false;
		}
		return PioneerHostWriteIO::Unmap() && okay;
	}
};

struct PioneerResidentMathIO : PioneerComputeNocIO {
	tt_resident_math_result& compute;
	const unsigned gemvInputs;
	area_id mathAreas[7]={-1,-1,-1,-1,-1,-1,-1};
	volatile uint32* entry=NULL;
	volatile uint32* code=NULL;
	volatile uint32* tail=NULL;
	volatile uint32* controls=NULL;
	volatile uint32* gddrData=NULL;
	volatile uint32* niuWrite=NULL;
	const volatile uint32* pc=NULL;
	uint32* savedWords=NULL;
	bool computeAttempted=false;
	PioneerResidentMathIO(const tt_pioneer_profile& p,tt_arc_scratch_result& a,
		tt_arc_telemetry_result& t,tt_tensix_reset_result& c,tt_dram_samples_result& d,
		tt_compute_noc_result& n,tt_resident_math_result& r,unsigned inputs=0)
		: PioneerComputeNocIO(p,a,t,c,d,n),compute(r),gemvInputs(inputs) {}
	~PioneerResidentMathIO() {
		free(savedWords);
		for (unsigned i=0;i<7;i++) if (mathAreas[i]>=B_OK) delete_area(mathAreas[i]);
	}
	bool Map() {
		if (gemvInputs!=0 && !BlackholeResidentGemvShape(gemvInputs)) return false;
		if (!PioneerComputeNocIO::Map()) return false;
		savedWords=(uint32*)malloc((TT_RESIDENT_MATH_IMAGE_WORDS+GddrWordCount())*sizeof(uint32));
		if (savedWords==NULL) return false;
		const uint32 offsets[]={0x19006000,0x19070000,0x1917f000,0x19312000,0x19313000,0x19000000,0x19320000};
		void* pages[7]={};
		for (unsigned i=0;i<7;i++) {
			uint32 protection=B_KERNEL_READ_AREA|B_UNCACHED_MEMORY;
			if (i!=4) protection|=B_KERNEL_WRITE_AREA;
			size_t bytes=i==5?((GddrWordCount()*sizeof(uint32)+B_PAGE_SIZE-1)&~(B_PAGE_SIZE-1)):B_PAGE_SIZE;
			mathAreas[i]=map_physical_memory("P100A fixed resident-matrix experiment page",
				profile.bar0_host_start+offsets[i],bytes,B_ANY_KERNEL_ADDRESS,protection,&pages[i]);
			if (mathAreas[i]<B_OK) return false;
		}
		entry=(volatile uint32*)pages[0]; code=(volatile uint32*)pages[1]; tail=(volatile uint32*)pages[2];
		controls=(volatile uint32*)pages[3]; pc=(const volatile uint32*)((uint8*)pages[4]+0x140);
		gddrData=(volatile uint32*)pages[5]; niuWrite=(volatile uint32*)pages[6]; return true;
	}
	uint32* ResidentMathSavedWords() { return savedWords; }
	unsigned GddrWordCount() const { return (gemvInputs!=0?gemvInputs/16:1)*TT_RESIDENT_MATH_GDDR_WORDS; }
	volatile uint32* MathPointer(unsigned index) {
		if (index>=TT_RESIDENT_MATH_IMAGE_WORDS) return NULL;
		if (index==0) return entry;
		uint32 address=BlackholeResidentMathAddress(index);
		return index<=TT_RESIDENT_MATH_CODE_WORDS?code+(address-0x70000)/4:tail+(address-0x17f000)/4;
	}
	uint32 MathImageRead(unsigned index) { volatile uint32* p=MathPointer(index); return p!=NULL?PioneerRead32(p):UINT32_MAX; }
	void MathImageWrite(unsigned index,uint32 value) { volatile uint32* p=MathPointer(index); if (p!=NULL) PioneerWrite32(p,value); }
	uint32 MathControlRead(unsigned index) {
		const uint32 offsets[]={0x234,0xa0,0x90};
		return index<3?PioneerRead32(controls+offsets[index]/4):UINT32_MAX;
	}
	void MathResetWrite(uint32 value) {
		if (value==0x87ffff || value==0x46800 || value==0x47800) PioneerWrite32(controls+0x1b0/4,value);
	}
	uint32 MathPcRead() { return PioneerRead32(pc); }
	uint32 ResidentGddrRead(unsigned word) { return word<GddrWordCount()?PioneerRead32(gddrData+word):UINT32_MAX; }
	void ResidentGddrWrite(unsigned word,uint32 value) { if (word<GddrWordCount()) PioneerWrite32(gddrData+word,value); }
	uint32 ResidentNocRead(uint32 offset) { return BlackholeResidentReadRegister(offset)?PioneerRead32(nocRegisters+offset/4):UINT32_MAX; }
	void ResidentNocWrite(uint32 offset,uint32 value) {
		// This operation's HOST cannot ring any NoC doorbell. Fixed TRISC0
		// alone issues its bounded reads after the full launch proof.
		if (offset!=0x40 && BlackholeResidentWriteRegister(offset)) PioneerWrite32(niuWrite+offset/4,value);
	}
	void Pause() { spin(100); }
	bool AfterNodeId(uint32 id) {
		if (!PioneerComputeNocIO::AfterNodeId(id)) return false;
		computeAttempted=true;
		if (gemvInputs!=0) {
			dprintf("PIONEER-P100A: fixed 16-output resident GEMV K=%u, device partial sums, no host DMA/flash\n",gemvInputs);
			return BlackholeResidentMathProbe(*this,dram,noc,compute,BlackholeResidentGemvProgram(gemvInputs));
		}
		dprintf("PIONEER-P100A: fixed TRISC0 resident GDDR inputs, two device-issued reads, no host DMA/flash\n");
		return BlackholeResidentMathProbe(*this,dram,noc,compute);
	}
	bool Unmap() {
		bool okay=true;
		for (unsigned i=0;i<7;i++) if (mathAreas[i]>=B_OK) {
			if (delete_area(mathAreas[i])==B_OK) mathAreas[i]=-1; else okay=false;
		}
		if (computeAttempted && (compute.recovery_required || !compute.gddr_restored || !compute.niu_restored
			|| !compute.math.l1_restored || !compute.math.tlb_restored)) okay=false;
		return PioneerComputeNocIO::Unmap() && okay;
	}
};

struct PioneerMatmulIO : PioneerDramSamplesIO {
	tt_tensix_matmul_result& math;
	const bool configSmoke;
	const unsigned sfpuSmoke;
	tt_arc_queue_result unusedQuery = {};
	PioneerArcQueryIO clockIO;
	tt_arc_clock_guard_result* const clock;
	area_id mathAreas[5] = {-1,-1,-1,-1,-1};
	volatile uint32* entry = NULL;
	volatile uint32* code = NULL;
	volatile uint32* tail = NULL;
	volatile uint32* controls = NULL;
	const volatile uint32* pc = NULL;
	bool mathAttempted = false;
	uint32* savedWords = NULL;
	const tt_tensix_tile_data* const tileInput;
	uint32* const tileOutput;
	const unsigned tileCount;
	uint32* const completedJobs;
	PioneerMatmulIO(const tt_pioneer_profile& p, tt_arc_scratch_result& a,
		tt_arc_telemetry_result& t, tt_tensix_reset_result& c, tt_dram_samples_result& d, tt_tensix_matmul_result& m,
		bool configOnly = false, tt_arc_clock_guard_result* clockGuard = NULL, unsigned sfpuOnly = 0,
		const tt_tensix_tile_data* input = NULL, uint32* output = NULL,
		unsigned count = 1, uint32* completed = NULL)
		: PioneerDramSamplesIO(p,a,t,c,d,false), math(m), configSmoke(configOnly), sfpuSmoke(sfpuOnly),
		clockIO(p,a,t,clockGuard != NULL ? clockGuard->last : unusedQuery,clockGuard), clock(clockGuard),
		tileInput(input), tileOutput(output), tileCount(count), completedJobs(completed) {}
	~PioneerMatmulIO() { free(savedWords); for (unsigned i = 0; i < 5; i++) if (mathAreas[i] >= B_OK) delete_area(mathAreas[i]); }
	bool Map()
	{
		if (!PioneerDramSamplesIO::Map()) return false;
		savedWords = (uint32*)malloc(BlackholeTensixImageWords(sfpuSmoke)*sizeof(uint32));
		if (savedWords == NULL) return false;
		const uint32 offsets[] = {0x19006000,0x19070000,0x1917f000,0x19312000,0x19313000};
		void* pages[5] = {};
		for (unsigned i = 0; i < 5; i++) {
			uint32 protection = B_KERNEL_READ_AREA | B_UNCACHED_MEMORY;
			if (i != 4) protection |= B_KERNEL_WRITE_AREA;
			mathAreas[i] = map_physical_memory("P100A fixed TRISC0 matrix-test page",
				profile.bar0_host_start + offsets[i],B_PAGE_SIZE,B_ANY_KERNEL_ADDRESS,protection,&pages[i]);
			if (mathAreas[i] < B_OK) return false;
		}
		entry = (volatile uint32*)pages[0]; code = (volatile uint32*)pages[1];
		tail = (volatile uint32*)pages[2]; controls = (volatile uint32*)pages[3];
		pc = (const volatile uint32*)((const uint8*)pages[4]+0x140); // TRISC0 snapshot
		// Additional virtual views only; no nested PCI configuration transaction.
		if (clock != NULL && !clockIO.Map()) return false;
		return true;
	}
	volatile uint32* MathPointer(unsigned index) {
		if (index >= BlackholeTensixImageWords(sfpuSmoke)) return NULL;
		uint32 address = BlackholeTensixImageAddress(index,sfpuSmoke);
		if (index == 0) return entry;
		if (index <= BlackholeTensixCodeWords(sfpuSmoke)) return code + (address-0x70000)/4;
		return tail + (address-0x17f000)/4;
	}
	uint32 MathImageRead(unsigned index) { volatile uint32* p = MathPointer(index); return p != NULL ? PioneerRead32(p) : UINT32_MAX; }
	uint32* MathSavedWords() { return savedWords; }
	void MathImageWrite(unsigned index, uint32 value) { volatile uint32* p = MathPointer(index); if (p != NULL) PioneerWrite32(p,value); }
	uint32 MathControlRead(unsigned index) {
		const uint32 offsets[] = {0x234,0xa0,0x90};
		return index < 3 ? PioneerRead32(controls+offsets[index]/4) : UINT32_MAX;
	}
	void MathResetWrite(uint32 value) {
		if (value == 0x87ffff || value == 0x46800 || value == 0x47800) PioneerWrite32(controls+0x1b0/4,value);
	}
	uint32 MathPcRead() { return PioneerRead32(pc); }
	void Pause() { spin(100); }
	bool EnterNormalClock(tt_arc_clock_guard_result& r) {
		clockIO.queryAttempted = true;
		return BlackholeArcClockEnter(clockIO,arc,telemetry,r);
	}
	bool LeaveNormalClock(tt_arc_clock_guard_result& r) { return BlackholeArcClockLeave(clockIO,arc,telemetry,r); }
	bool AfterNodeId(uint32 id) {
		if (!PioneerDramSamplesIO::AfterNodeId(id)) return false;
		mathAttempted = true;
		dprintf("PIONEER-P100A: fixed TRISC0 matrix fixtures, one idle tile context reset, no NoC/DMA commands\n");
		if (clock != NULL) return BlackholeClockedMatmulProbe(*this,dram,math,*clock);
		return BlackholeTensixMatmulProbe(*this,dram,math,configSmoke,sfpuSmoke,tileInput,tileOutput,tileCount,completedJobs);
	}
	bool Unmap()
	{
		bool okay = true;
		for (unsigned i = 0; i < 5; i++) {
			if (mathAreas[i] < B_OK) continue;
			if (delete_area(mathAreas[i]) == B_OK) mathAreas[i] = -1;
			else okay = false;
		}
		if (mathAttempted && !math.tlb_restored) {
			for (unsigned s = 0; s < 2; s++) for (unsigned w = 0; w < 3; w++) DramTlbWrite(s,w,math.tlb_before[s][w]);
			math.tlb_restored = 1;
			for (unsigned s = 0; s < 2; s++) for (unsigned w = 0; w < 3; w++) {
				math.tlb_after[s][w] = DramTlbRead(s,w);
				math.tlb_restored &= math.tlb_after[s][w] == math.tlb_before[s][w];
			}
		}
		if (mathAttempted && (!math.l1_restored || !math.tlb_restored || math.recovery_required)) okay = false;
		if (clock != NULL && !clockIO.Unmap()) okay = false;
		return PioneerDramSamplesIO::Unmap() && okay;
	}
};

// The session owns the PCI reservation, mappings and high host page until
// explicit END or the final close of this FD. No firmware/core start is
// performed here. Storage/compute commands will use this same owner.
struct PioneerStoreContext {
#ifdef PIONEER_Q38_RECURRENT_PREFIX_NATIVE_CONTEXT
 PioneerQ38RecurrentPrefixNativeState* q38RecurrentPrefixNative=NULL;
#endif
#ifdef PIONEER_Q38_RECURRENT_BLOCK_NATIVE_CONTEXT
 PioneerQ38RecurrentBlockNativeState* q38RecurrentBlockNative=NULL;
#endif
#ifdef PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT
 PioneerQ38ResidentLayerNativeState* q38ResidentLayerNative=NULL; // independent NEW full-layer lifetime
#endif
#ifdef PIONEER_Q38_RESIDENT_OUTPUT_OWNER
 PioneerQ38ResidentOutputNativeState* q38ResidentOutputNative=NULL; // separate private retained lifetime,not old prefix CLOSED
#endif
#ifdef PIONEER_Q38_EMBEDDING_NATIVE
	PioneerQ38EmbeddingNativeState* q38EmbeddingModel=NULL; // exact retained heap model/workspace/producer owners
#endif
#ifdef PIONEER_Q38_PROJECTION_GRAPH_MEMORY
	PioneerQ38ProjectionGraphMemory* q38ProjectionMemory=NULL; // retained complete HEAP owner,NO public opcode
#endif
#ifdef PIONEER_Q38_FFN_MODEL_INIT
#include "PioneerQ38MixedLiveNativeField.inc"
	// NEVER enter the old individually deleted q38GdnBanks/q38StateZero.
	PioneerQ38CanonicalModelPlan* q38FfnModelPlan=NULL;
	PioneerQ38GraphMutableBanks* q38GraphMutable=NULL;
	PioneerQ38FfnZeroLease* q38FfnZero=NULL;
	PioneerQ38GdnIdentity q38FfnModelIdentity={};
	const uint8* q38FfnModelLedger=NULL; // exact actual native initialized bitmap
	unsigned q38FfnModelLayer=64;
#endif
#ifdef PIONEER_Q38_GDN_MUTABLE_STATE
	// Private heap owners:separate fixed-zero diagnostic ONLY,not model compute.
	PioneerQ38GdnBanks* q38GdnBanks=NULL;
	PioneerQ38StateZeroLease* q38StateZero=NULL;
	PioneerQ38GdnIdentity q38GdnIdentity={};
	// Only the private diagnostic guard may bind this seal under the global lock,then clear it
	// before releasing the lock. Identity/presence alone never grants IO.
	thread_id q38GdnCallThread=-1;
#endif
#ifdef PIONEER_Q38_RESIDENT_GDN_ROOT_DIAGNOSTIC
	PioneerQ38ResidentGdnRootLease* q38ResidentGdnRoot=NULL;
#endif
#ifdef PIONEER_Q38_LOCAL_DELTA_PROBE
	PioneerQ38DeltaProbeLease q38Delta;
#endif
#ifdef PIONEER_Q38_SPAN_PRELOAD
	PioneerQ38SpanLease* q38Span=NULL;
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH
	PioneerQ38DispatchLease* q38Dispatch=NULL;
#ifdef PIONEER_Q38_FFN_NATIVE_GRAPH
	PioneerQ38FfnPersistentGraphOwner* q38FfnGraph=NULL;
	PioneerQ38FfnCanonicalJobs* q38FfnGraphJobs[2]={};
#endif
#if defined(PIONEER_Q38_FFN_CODE_DIAGNOSTIC) || defined(PIONEER_Q38_FFN_NATIVE_GRAPH)
	PioneerQ38FfnGraphCodeOwner* q38FfnCode=NULL;
#ifdef PIONEER_Q38_FFN_ROOT_LIFETIME_DIAGNOSTIC
	PioneerQ38FfnRootLifetimeOwner* q38FfnRootLifetime=NULL;
#endif
#ifdef PIONEER_Q38_FFN_PEER_LIFETIME_DIAGNOSTIC
	PioneerQ38FfnPeerLifetimeOwner* q38FfnPeerLifetime=NULL;
#endif
#endif
	bool q38DispatchRoutes=false,dispatchOwnerRetained=false;
	unsigned q38EndPhase=0;
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_COHORT
	PioneerQ38CohortLease* q38Cohort=NULL;
	bool q38CohortRoutes=false;
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_STREAM
	PioneerQ38StreamLease streamProbe;
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_PANEL
	PioneerQ38PanelLease panelProbe;
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_BANKS
	PioneerResidentBanks banks;
	bool bankStorageOnly=false;
#endif
	tt_pioneer_profile profile={};
	tt_pioneer_host_dma_audit_result baseline={};
	tt_host_write_result unused={};
	tt_host_page_result host={};
	tt_resident_session_idle_result idle={};
	tt_pioneer_mmio_transaction transaction={};
	PioneerResidentLease lease;
	PioneerRuntimeMatvecLease compute;
	PioneerRuntimeMatvecLease secondCompute;
	PioneerAttentionTileLease attention;
	PioneerAttentionTileLease peerAttention[8];
	PioneerResidentAttentionLease residentAttention;
	PioneerCoordinatorLease coordinator;
	PioneerLayerLease layer;
	PioneerDecoderLease decoder;
	PioneerDecoderJob decoderModel={};
	bool decoderReady=false;
	PioneerBulkUploadLease bulk;
	PioneerOutputBufferLease bulkHost;
	PioneerWideOutputLease wideHost;
	PioneerRuntimeActivationCache activation[2];
	PioneerResidentLease secondLease;
	tt_compute_noc_result secondNoc={};
	PioneerFleetWorker* fleet=NULL;
	bool fleetRoutes=false;
	int runtimeCoreMode=-1; // mode8 may ONE-WAY seal a fixed immutable model into decoder mode9
	uint64 bytes;
	uint8* initialized=NULL;
	uint32* hashes=NULL;
	uint32 uploadedPages=0;
	PioneerSessionIO io;
#ifdef PIONEER_Q38_NORMAL_CLOCK
	tt_arc_queue_result q38ClockMappingResult={}; // never used for protocol completion
	PioneerQ38ClockIO q38ClockIO;
	PioneerQ38ClockLease<PioneerQ38ClockIO> q38Clock;
#endif
	explicit PioneerStoreContext(uint64 capacity) : bytes(capacity),
		io(profile,baseline.baseline.baseline.baseline.control.control.control.arc,
			baseline.baseline.baseline.baseline.control.control.telemetry,
			baseline.baseline.baseline.baseline.control.tensix,
			baseline.baseline.baseline.baseline.dram,baseline.baseline.baseline.noc,
			baseline.baseline.path,baseline.audit,unused)
#ifdef PIONEER_Q38_NORMAL_CLOCK
		,q38ClockIO(profile,baseline.baseline.baseline.baseline.control.control.control.arc,
			baseline.baseline.baseline.baseline.control.control.telemetry,q38ClockMappingResult),q38Clock(q38ClockIO)
#endif
		{
#ifdef PIONEER_Q38_GDN_MUTABLE_STATE
		io.q38MutableContext=this;
#endif
		initialized=(uint8*)calloc(capacity/4096,1);
		hashes=(uint32*)calloc(capacity/4096,sizeof(uint32));
	}
	~PioneerStoreContext() {
#ifdef PIONEER_Q38_RECURRENT_PREFIX_NATIVE_CONTEXT
  delete q38RecurrentPrefixNative; // known-clean END only,before borrowed embedding/graph/hosts
#endif
#ifdef PIONEER_Q38_RECURRENT_BLOCK_NATIVE_CONTEXT
  delete q38RecurrentBlockNative; // clean END only,before borrowed prefix/graph/hosts
#endif
#ifdef PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT
  delete q38ResidentLayerNative; // known-clean END ONLY,before its borrowed prefix/graph/banks/hosts
#endif
#ifdef PIONEER_Q38_RESIDENT_OUTPUT_OWNER
  delete q38ResidentOutputNative; // known-clean END ONLY,before borrowed prefix/graph/banks/host owners
#endif
#ifdef PIONEER_Q38_EMBEDDING_NATIVE
#include "PioneerQ38MixedLiveNativeDestroy.inc"
#endif
#ifdef PIONEER_Q38_PROJECTION_GRAPH_MEMORY
		delete q38ProjectionMemory; // borrows actual model/source/cohort;destroy FIRST on known-clean close ONLY
#endif
#ifdef PIONEER_Q38_FFN_MODEL_INIT
		delete q38FfnZero;
		delete q38GraphMutable;
		delete q38FfnModelPlan; // borrowed by the joint owner;destroy LAST
#endif
#ifdef PIONEER_Q38_GDN_MUTABLE_STATE
#ifdef PIONEER_Q38_RESIDENT_GDN_ROOT_DIAGNOSTIC
		delete q38ResidentGdnRoot;
#endif
		delete q38StateZero;
		delete q38GdnBanks;
#endif
#ifdef PIONEER_Q38_SPAN_PRELOAD
		delete q38Span;
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH
	#ifdef PIONEER_Q38_FFN_NATIVE_GRAPH
		delete q38FfnGraph; // borrows the exact canonical/code/cohort owners
		for (auto* jobs:q38FfnGraphJobs) delete jobs;
	#endif
	#if defined(PIONEER_Q38_FFN_CODE_DIAGNOSTIC) || defined(PIONEER_Q38_FFN_NATIVE_GRAPH)
	#ifdef PIONEER_Q38_FFN_ROOT_LIFETIME_DIAGNOSTIC
		delete q38FfnRootLifetime; // borrows code owner;NEVER owns/deletes it
	#endif
	#ifdef PIONEER_Q38_FFN_PEER_LIFETIME_DIAGNOSTIC
		delete q38FfnPeerLifetime; // borrows code owner;NEVER owns/deletes it
	#endif
		delete q38FfnCode;
	#endif
		delete q38Dispatch;
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_COHORT
		delete q38Cohort;
#endif
		delete[] fleet; free(initialized); free(hashes);
	}
	tt_pioneer_mmio_result& Pci() { return baseline.baseline.baseline.baseline.control.control.control.pci; }
	bool Unsafe(
#ifdef PIONEER_Q38_GDN_MUTABLE_STATE
		bool controlledZero=false
#endif
#ifdef PIONEER_Q38_FFN_NATIVE_GRAPH
		,bool controlledGraph=false
#endif
#ifdef PIONEER_Q38_PROJECTION_GRAPH_MEMORY
		,bool controlledProjectionMemory=false
#endif
#ifdef PIONEER_Q38_EMBEDDING_NATIVE
		,bool controlledEmbedding=false
#endif
		) {
#ifdef PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT
#include "PioneerQ38ResidentLayerNativeUnsafe.inc"
#endif
#ifdef PIONEER_Q38_RESIDENT_OUTPUT_OWNER
#include "PioneerQ38ResidentOutputNativeUnsafe.inc"
#endif
#ifdef PIONEER_Q38_EMBEDDING_NATIVE
#include "PioneerQ38MixedLiveNativeUnsafe.inc" // PIONEER_Q38_EMBEDDING_NATIVE_UNSAFE_BEGIN
#ifdef PIONEER_Q38_RECURRENT_PREFIX_NATIVE_CONTEXT
  if(io.q38RecurrentPrefixMemoryMapAttempted && (!q38RecurrentPrefixNative || !q38RecurrentPrefixNative->memory))return true;
  if(q38RecurrentPrefixNative && (q38RecurrentPrefixNative->Unsafe() || (!controlledEmbedding && !q38RecurrentPrefixNative->Closed())))return true;
#endif
#ifdef PIONEER_Q38_RECURRENT_BLOCK_NATIVE_CONTEXT
  if(io.q38RecurrentBlockMemoryMapAttempted && (!q38RecurrentBlockNative || !q38RecurrentBlockNative->memory))return true;
  if(q38RecurrentBlockNative && (q38RecurrentBlockNative->Unsafe() || (!controlledEmbedding && !q38RecurrentBlockNative->Closed())))return true;
#endif
		if (q38EmbeddingModel) {
			if (q38EmbeddingModel->Unsafe() || (!controlledEmbedding && !q38EmbeddingModel->Closed())) return true;
		} else if (io.q38EmbeddingMemoryMapAttempted
#ifdef PIONEER_Q38_PROJECTION_EXECUTION_OWNER
			|| io.q38ProjectionMemoryMapAttempted
#endif
			) return true;
		// PIONEER_Q38_EMBEDDING_NATIVE_UNSAFE_END
#endif
#ifdef PIONEER_Q38_PROJECTION_GRAPH_MEMORY
		// PIONEER_Q38_PROJECTION_MEMORY_UNSAFE_BEGIN
		if (q38ProjectionMemory && !q38ProjectionMemory->Closed()) {
			if (!controlledProjectionMemory || !q38Dispatch || !q38FfnModelPlan
				|| !q38ProjectionMemory->MatchesNativeOwner(baseline.baseline.baseline.baseline.dram,banks,*q38Dispatch,
					*q38FfnModelPlan,q38FfnModelIdentity,initialized,PioneerResidentBanks::Pages)) return true;
		} else if ((q38ProjectionMemory && q38ProjectionMemory->Unsafe())
			|| (io.q38ProjectionMemoryMapAttempted && !q38ProjectionMemory)) return true;
		// PIONEER_Q38_PROJECTION_MEMORY_UNSAFE_END
#endif
#ifdef PIONEER_Q38_GDN_MUTABLE_STATE
	#ifdef PIONEER_Q38_FFN_MODEL_INIT
		if ((q38GraphMutable && q38GraphMutable->Unsafe())
			|| (q38FfnZero && (q38FfnZero->unsafe || (!controlledZero && q38FfnZero->running)))) return true;
	#endif
		if ((q38GdnBanks && q38GdnBanks->Unsafe())
			|| (q38StateZero && (q38StateZero->unsafe || (!controlledZero && q38StateZero->running)))) return true;
#ifdef PIONEER_Q38_RESIDENT_GDN_ROOT_DIAGNOSTIC
		if (q38ResidentGdnRoot && (q38ResidentGdnRoot->unsafe || (!controlledZero && q38ResidentGdnRoot->running))) return true;
#endif
#endif
#ifdef PIONEER_Q38_NORMAL_CLOCK
		if (q38Clock.Unsafe()) return true;
#endif
#ifdef PIONEER_Q38_LOCAL_DELTA_PROBE
		if (q38Delta.unsafe || q38Delta.running) return true;
#endif
#ifdef PIONEER_Q38_SPAN_PRELOAD
		if (q38Span && (q38Span->unsafe || q38Span->running)) return true;
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH
#ifdef PIONEER_Q38_FFN_ROOT_LIFETIME_DIAGNOSTIC
		if (q38FfnRootLifetime && q38FfnRootLifetime->Unsafe()) return true;
#endif
#ifdef PIONEER_Q38_FFN_PEER_LIFETIME_DIAGNOSTIC
		if (q38FfnPeerLifetime && q38FfnPeerLifetime->Unsafe()) return true;
#endif
#ifdef PIONEER_Q38_FFN_NATIVE_GRAPH
		if (q38FfnGraph && !q38FfnGraph->Closed()) {
			if (!controlledGraph || !q38Dispatch || !q38FfnCode || !q38GraphMutable || !q38FfnModelPlan
				|| !q38FfnGraph->MatchesNativeOwner(baseline.baseline.baseline.baseline.dram,banks,*q38Dispatch,
					*q38FfnCode,*q38GraphMutable,*q38FfnModelPlan,q38FfnModelIdentity,initialized,PioneerResidentBanks::Pages)) return true;
		} else
#endif
#ifdef PIONEER_Q38_PROJECTION_EXECUTION_OWNER
		// PIONEER_Q38_PROJECTION_EXECUTION_NATIVE_UNSAFE_BEGIN
#ifdef PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT
  if(q38ResidentLayerNative && q38ResidentLayerNative->ControlsLayerCohort(banks,baseline.baseline.baseline.baseline.dram,
   q38Dispatch,initialized,PioneerResidentBanks::Pages)){
   if(!controlledEmbedding)return true;
  }else
#endif
#ifdef PIONEER_Q38_RESIDENT_OUTPUT_OWNER
  if(q38ResidentOutputNative && q38ResidentOutputNative->ControlsOutputCohort(banks,baseline.baseline.baseline.baseline.dram,
   q38Dispatch,initialized,PioneerResidentBanks::Pages)){
   if(!controlledEmbedding)return true;
  }else
#endif
		if (q38Dispatch && q38EmbeddingModel && q38EmbeddingModel->projectionCohort && !q38EmbeddingModel->projection.Closed()) {
			if (!controlledEmbedding || !q38EmbeddingModel->ControlsProjectionCohort(baseline.baseline.baseline.baseline.dram,
				banks,q38Dispatch,initialized,PioneerResidentBanks::Pages)) return true;
		} else
		// PIONEER_Q38_PROJECTION_EXECUTION_NATIVE_UNSAFE_END
#endif
#include "PioneerQ38MixedLiveNativeCohortUnsafe.inc"
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_COHORT
		if (q38Cohort) {
			if (q38Cohort->unsafe) return true;
			for (unsigned w=0;w<120;w++) {
				auto& n=q38Cohort->worker[w];
				if (n.noc.recovery_required || n.image.running || n.image.unsafe || n.Store().pending || n.Store().recovery_required) return true;
			}
		}
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_STREAM
		if (streamProbe.running || streamProbe.unsafe) return true;
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_PANEL
		if (panelProbe.running || panelProbe.unsafe) return true;
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_BANKS
		if (banks.Quarantined()) return true;
#endif
		for (unsigned i=0;i<8;i++) if (peerAttention[i].running || peerAttention[i].unsafe) return true;
		if (fleet) for (unsigned w=0;w<TT_PIONEER_WORKERS;w++) if (fleet[w].Compute().unsafe || fleet[w].Compute().running
			|| fleet[w].Store().pending || fleet[w].Store().recovery_required || fleet[w].noc.recovery_required) return true;
		return decoder.running || decoder.unsafe || wideHost.pending || wideHost.quarantined || io.wideQuarantined || layer.running || layer.unsafe || coordinator.running || coordinator.unsafe || bulk.unsafe || bulkHost.pending || bulkHost.quarantined || attention.running || attention.unsafe || compute.running || compute.unsafe || secondCompute.running || secondCompute.unsafe
			|| secondLease.pending || secondLease.recovery_required || secondNoc.recovery_required
			|| lease.pending || lease.recovery_required || idle.recovery_required || baseline.audit.recovery_required || baseline.baseline.path.recovery_required
			|| baseline.baseline.baseline.noc.recovery_required || baseline.baseline.baseline.baseline.dram.recovery_required
#ifdef PIONEER_Q38_MIXED_LIVE_FINAL_RELEASE
			|| PioneerQ38MixedLiveFinalPciUncertain(*this);
#elif defined(PIONEER_Q38_RECURRENT_BLOCK_FINAL_RELEASE)
   || PioneerQ38RecurrentBlockFinalPciUncertain(*this);
#else
			|| Pci().state==TT_PIONEER_MMIO_RESTORE_FAILED;
#endif
	}
	void Snapshot(tt_resident_session_result& r) {
		r={}; r.version=1; r.state=TT_RESIDENT_SESSION_READY; r.active=1; r.bytes=bytes;
		r.host_ready=host.state==TT_HOST_PAGE_READY; r.host_address_bits=host.address_bits;
		r.host_released=host.released; r.pci_restored=Pci().restored;
		r.route_state=baseline.baseline.path.state; r.audit_state=baseline.audit.state;
		r.capabilities=TT_PIONEER_RESIDENT_CAPABILITIES; r.uploaded_pages=uploadedPages;
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_BANKS
		if (bankStorageOnly) r.capabilities=0; // no legacy compute/storage authority
#endif
		r.cold=sColdRecoveryRequired;
	}
};

#ifdef PIONEER_Q38_NORMAL_CLOCK
// PIONEER_Q38_CLOCK_SERVICE_BEGIN
static bool
BeginQ38NormalClock(PioneerStoreContext& c)
{
	if (!c.bankStorageOnly || c.bytes!=PioneerResidentBanks::Total || c.Unsafe()
#ifdef PIONEER_Q38_RESIDENT_OUTPUT_OWNER
  || c.q38ResidentOutputNative
#ifdef PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT
  || c.q38ResidentLayerNative
#endif
#endif
#ifdef PIONEER_Q38_EMBEDDING_NATIVE
		|| c.q38EmbeddingModel
#endif
#ifdef PIONEER_Q38_GDN_MUTABLE_STATE
		|| c.q38GdnBanks || c.q38StateZero
#ifdef PIONEER_Q38_FFN_MODEL_INIT
		|| c.q38FfnModelPlan || c.q38GraphMutable || c.q38FfnZero
#endif
#endif
		|| c.q38Clock.RequiresExplicitEnd() || c.q38Dispatch || c.q38Cohort || c.q38Span
		|| c.panelProbe.saved || c.streamProbe.saved || c.fleet || c.runtimeCoreMode!=-1) return false;
	auto& dram=c.baseline.baseline.baseline.baseline.dram;
	const auto& audit=c.baseline.audit;
	// Fresh BEGIN's existing physical ALL120-held/quiet-DMA audit, under
	// the SAME global lock/reservation/bank owner. No caller-supplied proof.
	if (!c.banks.Matches(dram) || dram.cores_checked!=120 || audit.version!=1
		|| audit.state!=TT_HOST_DMA_AUDIT_PASSED || audit.tlb_restored!=1 || audit.recovery_required) return false;
	bool ready=c.q38Clock.Begin(c.baseline.baseline.baseline.baseline.control.control.control.arc,
		c.baseline.baseline.baseline.baseline.control.control.telemetry);
	const auto& r=c.q38Clock.Snapshot(); // CPU metadata ONLY,even after failure
	dprintf("P100A_Q38_CLOCK begin%u state%u mhz%u/%u mode%u/%u messages%u uncertain%u;NORMAL_FIRMWARE_ONLY\n",
		unsigned(ready),r.state,r.before_mhz,r.busy_mhz,r.before_mode,r.busy_mode,r.messages,unsigned(c.q38Clock.Unsafe()));
	return ready;
}
static bool
FinishQ38NormalClock(PioneerStoreContext& c,bool knownClean)
{
	// Native END calls this ONLY AFTER explicit span END, ALL peer STOPs,
	// independent physical audits/holds and complete bulk/root restoration.
	// It is BEFORE final PCI disable and before releasing ANY host owner.
	knownClean=knownClean && !c.Unsafe() && (!c.q38Span || c.q38Span->closed)
		&& (!c.q38Dispatch || c.q38Dispatch->closed);
#ifdef PIONEER_Q38_EMBEDDING_NATIVE
	knownClean=knownClean && (!c.q38EmbeddingModel || c.q38EmbeddingModel->Closed());
#endif
#ifdef PIONEER_Q38_GDN_MUTABLE_STATE
	knownClean=knownClean && (!c.q38StateZero || c.q38StateZero->closed);
#ifdef PIONEER_Q38_FFN_MODEL_INIT
	knownClean=knownClean && (!c.q38FfnZero || c.q38FfnZero->closed);
#endif
#endif
	bool ended=c.q38Clock.End(c.baseline.baseline.baseline.baseline.control.control.control.arc,
		c.baseline.baseline.baseline.baseline.control.control.telemetry,knownClean);
	const auto& r=c.q38Clock.Snapshot(); // CPU metadata ONLY;not another ARC query
	dprintf("P100A_Q38_CLOCK end%u state%u mhz%u/%u/%u restored%u messages%u uncertain%u;NORMAL_FIRMWARE_ONLY\n",
		unsigned(ended),r.state,r.before_mhz,r.busy_mhz,r.after_mhz,r.restored,r.messages,unsigned(c.q38Clock.Unsafe()));
	return ended;
}
// PIONEER_Q38_CLOCK_SERVICE_END
#endif

#ifdef TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH
// PIONEER_Q38_DISPATCH_RETAIN_BEGIN
// Memory bookkeeping ONLY. In particular,NO legacy PCI/TLB/SRAM rollback,
// mapping/host release,unreserve,context destruction or cookie destruction.
// PIONEER_Q38_RETAIN_MEMORY_BEGIN
static void
RetainQ38DispatchOwner(PioneerStoreContext& c,tt_resident_session_result& result)
{
	c.dispatchOwnerRetained=true;
#ifdef PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT
 if(c.q38ResidentLayerNative)c.q38ResidentLayerNative->Fault(); // MEMORY ONLY;retain journal and all aliases
#endif
#ifdef PIONEER_Q38_RESIDENT_OUTPUT_OWNER
 if(c.q38ResidentOutputNative)c.q38ResidentOutputNative->Fault(); // MEMORY ONLY;never delete on unknown
#endif
#ifdef PIONEER_Q38_EMBEDDING_NATIVE
	if (c.q38EmbeddingModel) c.q38EmbeddingModel->Fault(); // MEMORY ONLY;ALL snapshots/leases/maps retained
#endif
#ifdef PIONEER_Q38_PROJECTION_GRAPH_MEMORY
	if (c.q38ProjectionMemory) c.q38ProjectionMemory->Fault(); // MEMORY ONLY;retain full snapshot and ALL maps
#endif
#ifdef PIONEER_Q38_FFN_MODEL_INIT
	if (c.q38FfnZero) c.q38FfnZero->unsafe=true;
	if (c.q38GraphMutable) c.q38GraphMutable->Fault(); // MEMORY ONLY;retain ALL borrowed roles
#endif
#ifdef PIONEER_Q38_GDN_MUTABLE_STATE
	if (c.q38StateZero) c.q38StateZero->unsafe=true;
	if (c.q38GdnBanks) c.q38GdnBanks->Fault(); // MEMORY ONLY
#ifdef PIONEER_Q38_RESIDENT_GDN_ROOT_DIAGNOSTIC
	if (c.q38ResidentGdnRoot) c.q38ResidentGdnRoot->unsafe=true; // MEMORY ONLY
#endif
#endif
#ifdef PIONEER_Q38_NORMAL_CLOCK
	c.q38Clock.Quarantine(); // MEMORY ONLY; never issue IDLE after ANY unknown
#endif
	if (c.q38Dispatch) PioneerQ38DispatchQuarantine(*c.q38Dispatch,c.banks);
	c.banks.Quarantine(); c.lease.recovery_required=true; c.io.QuarantineHostPage();
#ifdef PIONEER_Q38_RECURRENT_BLOCK_NATIVE_CONTEXT
 if(c.q38RecurrentBlockNative)c.q38RecurrentBlockNative->Fault(); // MEMORY ONLY;never clean up unknown aliases
#endif
#ifdef PIONEER_Q38_RECURRENT_PREFIX_NATIVE_CONTEXT
 if(c.q38RecurrentPrefixNative)c.q38RecurrentPrefixNative->Fault(); // MEMORY ONLY;retain every source/clock/host/map owner
#endif
#if defined(PIONEER_Q38_RECURRENT_PREFIX_NATIVE_CONTEXT) || defined(PIONEER_Q38_RECURRENT_BLOCK_NATIVE_CONTEXT) || defined(PIONEER_Q38_RESIDENT_DECODER_NATIVE) || defined(PIONEER_Q38_RESIDENT_OUTPUT_OWNER)
 c.bulk.unsafe=true; // same retained bulk snapshot/lease;NEVER restore/reuse after decoder uncertainty
#endif
	if (c.bulkHost.allocated && !c.bulkHost.released) { c.bulkHost.ready=false; c.bulkHost.quarantined=true; }
	if (c.wideHost.allocated && !c.wideHost.released) { c.wideHost.ready=false; c.wideHost.quarantined=true; }
	sColdRecoveryRequired=true; result.active=1; result.host_released=0; result.cold=1;
	result.state=TT_RESIDENT_SESSION_CLEANUP_FAILED;
#ifdef _KERNEL_MODE
	const auto* d=c.q38Dispatch;
	dprintf("PIONEER-P100A:54 RETAIN end=%u close=%u worker=%u status=%08x/%08x/%08x prepared=%u booted=%u closed=%u;NO_LATER_CARD_IO\n",
		c.q38EndPhase,d?d->closePhase:0,d?d->closeWorker:0,d?d->closeStatus[0]:0,d?d->closeStatus[1]:0,d?d->closeStatus[2]:0,
		d?d->prepared:0,unsigned(d && d->booted),unsigned(d && d->closed));
	if (d) dprintf("PIONEER-P100A:54 DETAIL %08x/%08x/%08x/%08x;ALREADY_READ_VALUES_ONLY\n",
		d->closeDetail[0],d->closeDetail[1],d->closeDetail[2],d->closeDetail[3]);
#endif
}
// PIONEER_Q38_RETAIN_MEMORY_END
static bool
EndQ38DispatchOwnership(PioneerStoreContext& c,tt_resident_session_result& result,bool& clean)
{
#ifdef PIONEER_Q38_RECURRENT_PREFIX_NATIVE_CONTEXT
 if(c.q38RecurrentPrefixNative || c.io.q38RecurrentPrefixMemoryMapAttempted){
  auto* state=c.q38RecurrentPrefixNative;auto* m=c.q38EmbeddingModel;
  if(!state || !m || !sRuntimeOwner || c.bytes!=PioneerResidentBanks::Total || c.runtimeCoreMode!=1
   || !c.io.RecurrentPrefixCallScopeMatches() || !state->Closed()
   || !state->MatchesBinding(c,*m,sRuntimeOwner,c.hashes,sRuntimeOwner->team,c.initialized,sReloadGeneration)
   || !c.io.RecurrentPrefixMemoryMappingStable(0)){
   clean=false;RetainQ38DispatchOwner(c,result);return false;
  }
 }
#endif
#ifdef PIONEER_Q38_RECURRENT_BLOCK_NATIVE_CONTEXT
 if(c.q38RecurrentBlockNative || c.io.q38RecurrentBlockMemoryMapAttempted){
  auto* state=c.q38RecurrentBlockNative;auto* m=c.q38EmbeddingModel;
  if(!state || !m || !sRuntimeOwner || c.bytes!=PioneerResidentBanks::Total || c.runtimeCoreMode!=1
   || !c.io.RecurrentBlockCallScopeMatches() || !state->Closed()
   || !state->MatchesBinding(c,*m,sRuntimeOwner,c.hashes,sRuntimeOwner->team,c.initialized,sReloadGeneration)
   || !c.io.RecurrentBlockMemoryMappingStable(0)){
   clean=false;RetainQ38DispatchOwner(c,result);return false;
  }
 }
#endif
#ifdef PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT
#include "PioneerQ38ResidentLayerNativeEnd.inc"
#endif
#ifdef PIONEER_Q38_RESIDENT_OUTPUT_OWNER
#include "PioneerQ38ResidentOutputNativeEnd.inc"
#endif
#ifdef PIONEER_Q38_EMBEDDING_NATIVE
#include "PioneerQ38MixedLiveNativeEnd.inc" // PIONEER_Q38_EMBEDDING_NATIVE_END_BEGIN
	if (c.q38EmbeddingModel || c.io.q38EmbeddingMemoryMapAttempted
#ifdef PIONEER_Q38_PROJECTION_EXECUTION_OWNER
		|| c.io.q38ProjectionMemoryMapAttempted
#endif
		) {
		auto* model=c.q38EmbeddingModel;
		if (!model || !model->Closed() || !c.io.q38EmbeddingMemoryMapAttempted || model->ledger!=c.initialized
			|| model->identity.driverGeneration!=sReloadGeneration || !c.bankStorageOnly
			|| !model->graph.MatchesInitializationOwner(c.banks,c.baseline.baseline.baseline.baseline.dram,model->identity)
			|| !model->embedding.MatchesNativeOwner(c.baseline.baseline.baseline.baseline.dram,c.baseline.baseline.baseline.noc,
				c.banks,model->graph,model->plan,model->identity,c.initialized,PioneerResidentBanks::Pages)
			|| model->zero.owner!=model->graph.Projection(model->identity) || model->zero.banks!=&c.banks
			|| model->zero.proof!=&c.baseline.baseline.baseline.baseline.dram || !(model->zero.identity==model->identity)
#ifdef PIONEER_Q38_LEARNED_RMS_NATIVE
#ifdef PIONEER_Q38_PROJECTION_EXECUTION_OWNER
			|| !model->rms.Completed()
			|| (model->phase!=PioneerQ38EmbeddingNativeState::RmsComplete && model->phase!=
#ifdef PIONEER_Q38_RESIDENT_DECODER_NATIVE
    PioneerQ38EmbeddingNativeState::DecoderComplete)
   || !model->MatchesInitializedHistory(c.banks,c.baseline.baseline.baseline.baseline.dram)
#ifdef PIONEER_Q38_GDN_STATE_INIT_NATIVE
   || !model->MatchesInitializedGdn(c.banks,c.baseline.baseline.baseline.baseline.dram)
#endif
#else
    PioneerQ38EmbeddingNativeState::ProjectionComplete)
#endif
#else
			|| model->phase!=PioneerQ38EmbeddingNativeState::RmsComplete || !model->rms.Completed()
#endif
			|| !model->rms.MatchesNativeOwner(c.baseline.baseline.baseline.baseline.dram,c.baseline.baseline.baseline.noc,
				c.banks,model->graph,model->plan,model->embedding,model->identity,c.initialized,PioneerResidentBanks::Pages)
#endif
			|| model->zero.nonce!=2 || !model->zero.saved || model->zero.dirty || model->zero.ready || model->zero.running) {
			clean=false;RetainQ38DispatchOwner(c,result);return false; // BEFORE old STOP/restore/ARC/host/map release
		}
	}
	// PIONEER_Q38_EMBEDDING_NATIVE_END_END
#endif
#ifdef PIONEER_Q38_PROJECTION_EXECUTION_OWNER
	// PIONEER_Q38_PROJECTION_EXECUTION_NATIVE_END_BEGIN
	if (c.q38Dispatch || c.io.q38ProjectionMemoryMapAttempted || (c.q38EmbeddingModel && c.q38EmbeddingModel->projectionCohort)) {
		auto* m=c.q38EmbeddingModel;
#ifdef PIONEER_Q38_RESIDENT_DECODER_NATIVE
  auto* history=m?m->graph.ConvHistory(m->identity):NULL;
#endif
		if (!m || !m->Closed() || m->phase!=
#ifdef PIONEER_Q38_RESIDENT_DECODER_NATIVE
   PioneerQ38EmbeddingNativeState::DecoderComplete || !history || !m->MatchesInitializedHistory(c.banks,c.baseline.baseline.baseline.baseline.dram)
   || !m->decoder.MatchesClosedNativeOwner(m->projection,*history)
#else
   PioneerQ38EmbeddingNativeState::ProjectionComplete
#endif
   || !c.q38Dispatch
			|| m->projectionCohort!=c.q38Dispatch || !c.q38DispatchRoutes || c.q38Dispatch->worker[0].master!=&c.lease
			|| !c.io.ProjectionGraphMemoryMappingStable(0)
			|| !m->projection.MatchesClosedNativeOwner(c.baseline.baseline.baseline.baseline.dram,c.banks,*c.q38Dispatch,
				m->projectionMemory,m->graph,m->plan,m->rms,m->identity,c.initialized,PioneerResidentBanks::Pages)) {
			clean=false;RetainQ38DispatchOwner(c,result);return false; // BEFORE ANY legacy STOP/restore/clock/host/map release
		}
	}
	// PIONEER_Q38_PROJECTION_EXECUTION_NATIVE_END_END
#endif
#ifdef PIONEER_Q38_PROJECTION_GRAPH_MEMORY
	// PIONEER_Q38_PROJECTION_MEMORY_END_GATE_BEGIN
	if ((c.q38ProjectionMemory && (!c.q38ProjectionMemory->Closed() || c.q38ProjectionMemory->Unsafe()))
		|| (c.io.q38ProjectionMemoryMapAttempted && !c.q38ProjectionMemory)) {
		clean=false; RetainQ38DispatchOwner(c,result); return false; // BEFORE any STOP/restore/ARC/release
	}
	// PIONEER_Q38_PROJECTION_MEMORY_END_GATE_END
#endif
#ifdef PIONEER_Q38_FFN_MODEL_INIT
	// PIONEER_Q38_FFN_MODEL_END_GATE_BEGIN
	// BEFORE ANY STOP/restore/clock/release. Neither page count nor a
	// pristine lease proves that this exact joint owner completed zero CLOSE.
	if (c.q38FfnModelPlan || c.q38GraphMutable || c.q38FfnZero) {
		const auto* graph=c.q38GraphMutable;
		auto* ffn=graph?c.q38GraphMutable->Ffn(c.q38FfnModelIdentity):NULL;
		if (!c.bankStorageOnly || !c.q38FfnModelPlan || !graph || !ffn || !c.q38FfnZero
			|| graph->Unsafe() || graph->CanonicalModel(c.q38FfnModelIdentity)!=c.q38FfnModelPlan
			|| c.q38FfnModelLedger!=c.initialized || c.q38FfnModelLayer>=64
			|| !graph->MatchesInitializationOwner(c.banks,c.baseline.baseline.baseline.baseline.dram,c.q38FfnModelIdentity)
			|| c.q38FfnZero->owner!=ffn || c.q38FfnZero->banks!=&c.banks
			|| c.q38FfnZero->proof!=&c.baseline.baseline.baseline.baseline.dram
			|| !(c.q38FfnZero->identity==c.q38FfnModelIdentity) || !c.q38FfnZero->closed
			|| !c.q38FfnZero->saved || c.q38FfnZero->nonce!=1
			|| c.q38FfnZero->unsafe || c.q38FfnZero->running || c.q38FfnZero->dirty || c.q38FfnZero->ready
			|| !ffn->InitializationPublished(c.q38FfnModelIdentity)) {
			clean=false; RetainQ38DispatchOwner(c,result); return false;
		}
	}
	// PIONEER_Q38_FFN_MODEL_END_GATE_END
#endif
#ifdef PIONEER_Q38_GDN_MUTABLE_STATE
	if (!c.bankStorageOnly && (c.q38GdnBanks || c.q38StateZero
#ifdef PIONEER_Q38_RESIDENT_GDN_ROOT_DIAGNOSTIC
		|| c.q38ResidentGdnRoot
#endif
		)) {
		clean=false; RetainQ38DispatchOwner(c,result); return false; // misbound private scope:NEVER legacy rollback
	}
#endif
	if (!c.bankStorageOnly) return true;
#ifdef PIONEER_Q38_FFN_NATIVE_GRAPH
	// PIONEER_Q38_FFN_GRAPH_END_GATE_BEGIN
	// CPU checks BEFORE old STOP/delta/ARC/restore/release. Graph CLOSE is
	// explicit;never let END pretend that NEW running peers are old booted.
	if ((c.q38FfnGraph || c.q38FfnCode) && (!c.q38FfnGraph || !c.q38FfnCode
		|| !c.q38FfnGraph->Closed() || c.q38FfnGraph->Unsafe()
		|| !c.q38FfnCode->Closed() || c.q38FfnCode->Unsafe())) {
		clean=false; RetainQ38DispatchOwner(c,result); return false;
	}
	// PIONEER_Q38_FFN_GRAPH_END_GATE_END
#endif
#ifdef PIONEER_Q38_FFN_ROOT_LIFETIME_DIAGNOSTIC
	// PIONEER_Q38_FFN_ROOT_LIFETIME_END_GATE_BEGIN
	// A published ROOT heap can NEVER be implicitly accepted by BANK_END.
	// Check CPU ownership BEFORE delta/STOP/ARC/restore/release paths.
	if ((c.q38FfnRootLifetime || c.q38FfnCode) && (!c.q38FfnRootLifetime || !c.q38FfnCode
		|| !c.q38FfnRootLifetime->Closed() || c.q38FfnRootLifetime->Unsafe()
		|| !c.q38FfnCode->Closed() || c.q38FfnCode->Unsafe())) {
		clean=false; RetainQ38DispatchOwner(c,result); return false;
	}
	// PIONEER_Q38_FFN_ROOT_LIFETIME_END_GATE_END
#endif
#ifdef PIONEER_Q38_RESIDENT_GDN_ROOT_DIAGNOSTIC
	if (c.q38ResidentGdnRoot && (!c.q38ResidentGdnRoot->closed || c.q38ResidentGdnRoot->unsafe
		|| c.q38ResidentGdnRoot->running || c.q38ResidentGdnRoot->awaiting || !c.q38ResidentGdnRoot->saved
		|| c.q38ResidentGdnRoot->dirty || c.q38ResidentGdnRoot->ready || !c.q38GdnBanks || !c.q38StateZero
		|| !c.q38StateZero->closed || !(c.q38StateZero->identity==c.q38GdnIdentity) || !c.q38GdnIdentity.Valid()
		|| !(c.q38ResidentGdnRoot->identity==c.q38GdnIdentity) || c.q38ResidentGdnRoot->owner!=c.q38GdnBanks
		|| c.q38ResidentGdnRoot->banks!=&c.banks || c.q38ResidentGdnRoot->proof!=&c.baseline.baseline.baseline.baseline.dram)) {
		clean=false; RetainQ38DispatchOwner(c,result); return false; // NO implicit debug ACCEPT/CLOSE
	}
#endif
#ifdef PIONEER_Q38_GDN_MUTABLE_STATE
	// Explicit state-zero CLOSE must precede BANK_END. Refuse BEFORE
	// delta/STOP/ARC/restore paths, even if the last device job passed.
	if ((c.q38GdnBanks && c.q38GdnBanks->Unsafe())
		|| (c.q38StateZero && (!c.q38StateZero->closed || c.q38StateZero->unsafe || c.q38StateZero->running))) {
		clean=false; RetainQ38DispatchOwner(c,result); return false;
	}
#endif
#ifdef PIONEER_Q38_LOCAL_DELTA_PROBE
	if (c.q38Delta.saved) clean=clean && PioneerQ38DeltaProbeClose(c.io,
		c.baseline.baseline.baseline.baseline.dram,c.baseline.baseline.baseline.noc,c.lease,c.banks,c.q38Delta,true);
#endif
#ifdef PIONEER_Q38_SPAN_PRELOAD
	// SPAN_END is an explicit prerequisite, never an implicit cleanup path.
	// A caller-only failure may be invisible to transport completion metadata.
	if (c.q38Span && !c.q38Span->closed) clean=false;
#endif
	if (clean && c.q38Dispatch) clean=PioneerQ38DispatchClose(c.io,c.baseline.baseline.baseline.baseline.dram,c.banks,*c.q38Dispatch,true);
	if (clean) return true;
	RetainQ38DispatchOwner(c,result); return false;
}
// PIONEER_Q38_DISPATCH_RETAIN_END
#endif
#include "PioneerQ38MixedLiveNative.inc"
#include "PioneerQ38RecurrentBlockNative.inc"
#include "PioneerQ38RecurrentPrefixNative.inc"
#ifdef PIONEER_Q38_GDN_MUTABLE_STATE
// PIONEER_Q38_STATE_ZERO_NATIVE_ADAPTER_BEGIN
// Private native adapter,selected ONLY by the separate fixed zero diagnostic
// flag. Ordinary mutable/model compute remains REFUSED. No production call
// binds this owner/identity/seal. Existing owned mappings are BORROWED,not copied
// or allocated;no new reset address/host page/BME or public data capability.
#ifdef PIONEER_Q38_EMBEDDING_NATIVE
// PIONEER_Q38_EMBEDDING_NATIVE_SCOPE_BEGIN
bool PioneerSessionIO::EmbeddingSourceScope(bool controller)
{
	auto* c=q38MutableContext;auto* m=c?c->q38EmbeddingModel:NULL;
#ifdef PIONEER_Q38_RESIDENT_OUTPUT_OWNER
 if(c && c->q38ResidentOutputNative && !ResidentOutputPrefixInitializationScope())return false;
#ifdef PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT
 if(c && c->q38ResidentLayerNative && !ResidentLayerPrefixInitializationScope())return false;
#endif
#endif
	if (!c || &c->io!=this || !m || sColdRecoveryRequired || sReloadQuiesced || !sReloadGeneration || !sRuntimeOwner
		|| sRuntimeOwner->magic!=0x50313030 || sRuntimeOwner->context!=c || !sBankStorageOnly || !c->bankStorageOnly
		|| c->bytes!=PioneerResidentBanks::Total || c->runtimeCoreMode!=1 || runtimeCore || fleetEnabled || !runtimeMapped || q38SpanAllowed
		|| c->q38GdnCallThread<0 || c->q38GdnCallThread!=find_thread(NULL) || c->dispatchOwnerRetained
		|| !m->identity.Valid() || m->identity.driverGeneration!=sReloadGeneration || !c->initialized || !c->hashes || m->ledger!=c->initialized
		|| c->q38FfnModelPlan || c->q38GraphMutable || c->q38FfnZero || c->q38FfnGraph || c->q38FfnCode
		|| c->q38FfnModelIdentity.session || c->q38FfnModelIdentity.sequence || c->q38FfnModelIdentity.modelEpoch || c->q38FfnModelIdentity.driverGeneration
		|| c->q38GdnBanks || c->q38StateZero || c->q38GdnIdentity.session || c->q38GdnIdentity.sequence
		|| c->q38GdnIdentity.modelEpoch || c->q38GdnIdentity.driverGeneration || c->q38Dispatch || c->q38Cohort || c->fleet || sQ38LocalDeltaOwner
		|| c->q38Delta.saved || c->q38Delta.dirty || c->q38Delta.ready || c->q38Delta.running || c->q38Delta.awaiting
		|| c->panelProbe.saved || c->panelProbe.dirty || c->panelProbe.ready || c->streamProbe.saved || c->streamProbe.dirty || c->streamProbe.ready
		|| !c->q38Span || !c->q38Span->closed || c->q38Span->dirty || c->q38Span->ready || c->q38Span->running || c->q38Span->unsafe
		|| !c->lease.saved || !c->lease.ready || c->lease.pending || c->lease.recovery_required
		|| !c->bulk.saved || !c->bulk.ready || (m->phase!=PioneerQ38EmbeddingNativeState::Zeroing
#ifdef PIONEER_Q38_RESIDENT_DECODER_NATIVE
   && m->phase!=PioneerQ38EmbeddingNativeState::HistoryZeroing
#endif
   && c->bulk.dirty) || c->bulk.unsafe
		|| c->host.state!=TT_HOST_PAGE_READY || c->host.released || PioneerHostPageIO::quarantined
		|| !c->bulkHost.allocated || !c->bulkHost.ready || c->bulkHost.pending || c->bulkHost.quarantined || c->bulkHost.released
		|| !c->wideHost.allocated || !c->wideHost.ready || !c->wideHost.memory_type_set || c->wideHost.pending
		|| c->wideHost.quarantined || c->wideHost.released || wideQuarantined || m->Unsafe()
		|| m->graph.CanonicalModel(m->identity)!=&m->plan || !m->graph.MatchesInitializationOwner(c->banks,dram,m->identity)) return false;
	auto* state=m->graph.Projection(m->identity);if (!state) return false;
#ifdef PIONEER_Q38_RESIDENT_DECODER_NATIVE
 if(!DecoderCallScopeMatches() || m->decoder.Claimed())return false;
#ifdef PIONEER_Q38_GDN_STATE_INIT_NATIVE
 if(!m->MatchesInitializedGdn(c->banks,dram))return false;
#endif
 if(m->phase==PioneerQ38EmbeddingNativeState::HistoryZeroing){
  auto* h=m->graph.ConvHistory(m->identity);const auto& z=m->historyZero;
  if(controller || !h || h->InitializationPublished(m->identity) || !PioneerQ38ProjectionZeroPristine(m->zero) || m->zero.closed
   || m->embedding.Completed() || m->embedding.Running() || m->rms.Claimed() || m->projection.Claimed() || m->projectionCohort
   || !h->MatchesInitializationOwner(c->banks,dram,m->identity))return false;
  if(z.identity.Valid()){
   if(z.owner!=h || z.banks!=&c->banks || z.proof!=&dram || !(z.identity==m->identity) || z.closed || z.unsafe
    || !h->OwnsInitializationLease(&z))return false;
  }else if(!PioneerQ38ConvHistoryZeroPristine(z) || z.closed)return false;
 }else if(!m->MatchesInitializedHistory(c->banks,dram))return false;
#endif
	const auto& z=m->zero;
	if (z.identity.Valid()) {
		if (!(z.identity==m->identity) || z.owner!=state || z.banks!=&c->banks || z.proof!=&dram) return false;
	} else if (!PioneerQ38ProjectionZeroPristine(z) || z.closed) return false;
	if (m->phase==PioneerQ38EmbeddingNativeState::Zeroing
#ifdef PIONEER_Q38_RESIDENT_DECODER_NATIVE
  || m->phase==PioneerQ38EmbeddingNativeState::HistoryZeroing
#endif
  ) {if (controller || m->embedding.Running()) return false;}
	else if (m->phase==PioneerQ38EmbeddingNativeState::Embedding) {
		if (!z.saved || !z.closed || z.nonce!=2 || z.ready || z.running || z.dirty || !state->InitializationPublished(m->identity)) return false;
		if (controller && !m->embedding.MatchesNativeOwner(dram,noc,c->banks,m->graph,m->plan,m->identity,c->initialized,PioneerResidentBanks::Pages)) return false;
	} else return false;
	const auto& clock=c->q38Clock.Snapshot();
	return c->q38Clock.RequiresExplicitEnd() && clock.state==TT_ARC_CLOCK_BUSY_VERIFIED && clock.busy_requested
		&& clock.busy_acknowledged && !clock.restored && !c->Unsafe(true,false,true);
}
bool PioneerSessionIO::ProjectionZeroBeforeCohort()
{
	return q38MutableContext && q38MutableContext->q38EmbeddingModel
		&& q38MutableContext->q38EmbeddingModel->phase==PioneerQ38EmbeddingNativeState::Zeroing && EmbeddingSourceScope(false);
}
bool PioneerSessionIO::ProjectionZeroOtherCoresHeld()
{
	if (!ProjectionZeroBeforeCohort() || !NonRootHeldMappingStable()) return false;
	if (PioneerQ38NonRootHeld(*this,dram)) return true;
	tt_resident_session_result result={};RetainQ38DispatchOwner(*q38MutableContext,result);return false;
}
bool PioneerSessionIO::EmbeddingNativeScope(const PioneerQ38EmbeddingController& controller,const PioneerQ38ProjectionMutableBanks& graph,
	const PioneerQ38CanonicalModelPlan& plan,const PioneerQ38ProjectionIdentity& id,const uint8_t* ledger,unsigned pages)
{
	auto* m=q38MutableContext?q38MutableContext->q38EmbeddingModel:NULL;
	return m && &controller==&m->embedding && &graph==&m->graph && &plan==&m->plan && id==m->identity
		&& ledger==m->ledger && pages==PioneerResidentBanks::Pages && EmbeddingSourceScope(true);
}
bool PioneerSessionIO::EmbeddingOtherCoresHeld()
{
	if (!EmbeddingSourceScope(true) || !NonRootHeldMappingStable()) return false;
	if (PioneerQ38NonRootHeld(*this,dram)) return true;
	tt_resident_session_result result={};RetainQ38DispatchOwner(*q38MutableContext,result);return false;
}
void PioneerSessionIO::EmbeddingRetainHostOwners()
{
	if (q38MutableContext) {tt_resident_session_result result={};RetainQ38DispatchOwner(*q38MutableContext,result);} // MEMORY ONLY
}
// PIONEER_Q38_EMBEDDING_NATIVE_SCOPE_END
#ifdef PIONEER_Q38_LEARNED_RMS_NATIVE
// PIONEER_Q38_LEARNED_RMS_NATIVE_SCOPE_BEGIN
bool PioneerSessionIO::LearnedRmsSourceScope(bool controller)
{
	auto* c=q38MutableContext;auto* m=c?c->q38EmbeddingModel:NULL;
#ifdef PIONEER_Q38_RESIDENT_OUTPUT_OWNER
 if(c && c->q38ResidentOutputNative && !ResidentOutputPrefixInitializationScope())return false;
#ifdef PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT
 if(c && c->q38ResidentLayerNative && !ResidentLayerPrefixInitializationScope())return false;
#endif
#endif
	if (!c || &c->io!=this || !m || m->phase!=PioneerQ38EmbeddingNativeState::Rms
		|| sColdRecoveryRequired || sReloadQuiesced || !sReloadGeneration || !sRuntimeOwner
		|| sRuntimeOwner->magic!=0x50313030 || sRuntimeOwner->context!=c || !sBankStorageOnly || !c->bankStorageOnly
		|| c->bytes!=PioneerResidentBanks::Total || c->runtimeCoreMode!=1 || runtimeCore || fleetEnabled || !runtimeMapped || q38SpanAllowed
		|| c->q38GdnCallThread<0 || c->q38GdnCallThread!=find_thread(NULL) || c->dispatchOwnerRetained
		|| !m->identity.Valid() || m->identity.driverGeneration!=sReloadGeneration || !c->initialized || !c->hashes || m->ledger!=c->initialized
		|| c->q38FfnModelPlan || c->q38GraphMutable || c->q38FfnZero || c->q38FfnGraph || c->q38FfnCode
		|| c->q38FfnModelIdentity.session || c->q38FfnModelIdentity.sequence || c->q38FfnModelIdentity.modelEpoch || c->q38FfnModelIdentity.driverGeneration
		|| c->q38GdnBanks || c->q38StateZero || c->q38GdnIdentity.session || c->q38GdnIdentity.sequence
		|| c->q38GdnIdentity.modelEpoch || c->q38GdnIdentity.driverGeneration || c->q38Dispatch || c->q38Cohort || c->fleet || sQ38LocalDeltaOwner
		|| c->q38Delta.saved || c->q38Delta.dirty || c->q38Delta.ready || c->q38Delta.running || c->q38Delta.awaiting
		|| c->panelProbe.saved || c->panelProbe.dirty || c->panelProbe.ready || c->streamProbe.saved || c->streamProbe.dirty || c->streamProbe.ready
		|| !c->q38Span || !c->q38Span->closed || c->q38Span->dirty || c->q38Span->ready || c->q38Span->running || c->q38Span->unsafe
		|| !c->lease.saved || !c->lease.ready || c->lease.pending || c->lease.recovery_required
		|| !c->bulk.saved || !c->bulk.ready || c->bulk.dirty || c->bulk.unsafe
		|| c->host.state!=TT_HOST_PAGE_READY || c->host.released || PioneerHostPageIO::quarantined
		|| !c->bulkHost.allocated || !c->bulkHost.ready || c->bulkHost.pending || c->bulkHost.quarantined || c->bulkHost.released
		|| !c->wideHost.allocated || !c->wideHost.ready || !c->wideHost.memory_type_set || c->wideHost.pending
		|| c->wideHost.quarantined || c->wideHost.released || wideQuarantined || m->Unsafe()
		|| m->graph.CanonicalModel(m->identity)!=&m->plan || !m->graph.MatchesInitializationOwner(c->banks,dram,m->identity)) return false;
#ifdef PIONEER_Q38_RESIDENT_DECODER_NATIVE
 if(!DecoderCallScopeMatches() || !m->MatchesInitializedHistory(c->banks,dram) || m->decoder.Claimed())return false;
#ifdef PIONEER_Q38_GDN_STATE_INIT_NATIVE
 if(!m->MatchesInitializedGdn(c->banks,dram))return false;
#endif
#endif
	auto* state=m->graph.Projection(m->identity);const auto& z=m->zero;
	if (!state || !z.saved || !z.closed || z.nonce!=2 || z.ready || z.running || z.dirty
		|| z.owner!=state || z.banks!=&c->banks || z.proof!=&dram || !(z.identity==m->identity)
		|| !state->InitializationPublished(m->identity) || !m->embedding.Completed()
		|| !m->embedding.MatchesNativeOwner(dram,noc,c->banks,m->graph,m->plan,m->identity,c->initialized,PioneerResidentBanks::Pages)
		|| (controller && !m->rms.MatchesNativeOwner(dram,noc,c->banks,m->graph,m->plan,m->embedding,m->identity,c->initialized,PioneerResidentBanks::Pages))) return false;
	const auto& clock=c->q38Clock.Snapshot();
	return c->q38Clock.RequiresExplicitEnd() && clock.state==TT_ARC_CLOCK_BUSY_VERIFIED && clock.busy_requested
		&& clock.busy_acknowledged && !clock.restored && !c->Unsafe(true,false,true);
}
bool PioneerSessionIO::LearnedRmsNativeScope(const PioneerQ38LearnedRmsController& controller,const PioneerQ38ProjectionMutableBanks& graph,
	const PioneerQ38CanonicalModelPlan& plan,const PioneerQ38EmbeddingController& input,const PioneerQ38ProjectionIdentity& id,const uint8_t* ledger,unsigned pages)
{
	auto* m=q38MutableContext?q38MutableContext->q38EmbeddingModel:NULL;
	return m && &controller==&m->rms && &graph==&m->graph && &plan==&m->plan && &input==&m->embedding && id==m->identity
		&& ledger==m->ledger && pages==PioneerResidentBanks::Pages && LearnedRmsSourceScope(true);
}
bool PioneerSessionIO::LearnedRmsOtherCoresHeld()
{
	if (!LearnedRmsSourceScope(true) || !NonRootHeldMappingStable()) return false;
	if (PioneerQ38NonRootHeld(*this,dram)) return true;
	tt_resident_session_result result={};RetainQ38DispatchOwner(*q38MutableContext,result);return false;
}
// PIONEER_Q38_LEARNED_RMS_NATIVE_SCOPE_END
#endif
#ifdef PIONEER_Q38_PROJECTION_EXECUTION_OWNER
// PIONEER_Q38_PROJECTION_EXECUTION_NATIVE_SCOPE_BEGIN
bool PioneerSessionIO::ProjectionExecutionSourceScope(bool controller)
{
	auto* c=q38MutableContext;auto* m=c?c->q38EmbeddingModel:NULL;
#ifdef PIONEER_Q38_RESIDENT_OUTPUT_OWNER
 if(c && c->q38ResidentOutputNative && !ResidentOutputPrefixInitializationScope())return false;
#ifdef PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT
 if(c && c->q38ResidentLayerNative && !ResidentLayerPrefixInitializationScope())return false;
#endif
#endif
	if (!c || &c->io!=this || !m || sColdRecoveryRequired || sReloadQuiesced || !sReloadGeneration || !sRuntimeOwner
		|| sRuntimeOwner->magic!=0x50313030 || sRuntimeOwner->context!=c || !sBankStorageOnly || !c->bankStorageOnly
		|| c->bytes!=PioneerResidentBanks::Total || c->runtimeCoreMode!=1 || runtimeCore>=120 || !runtimeMapped || q38SpanAllowed
		|| c->q38GdnCallThread<0 || c->q38GdnCallThread!=find_thread(NULL) || c->dispatchOwnerRetained
		|| !m->identity.Valid() || m->identity.driverGeneration!=sReloadGeneration || !c->initialized || !c->hashes || m->ledger!=c->initialized
		|| c->q38FfnModelPlan || c->q38GraphMutable || c->q38FfnZero || c->q38FfnGraph || c->q38FfnCode
		|| c->q38FfnModelIdentity.session || c->q38FfnModelIdentity.sequence || c->q38FfnModelIdentity.modelEpoch || c->q38FfnModelIdentity.driverGeneration
		|| c->q38GdnBanks || c->q38StateZero || c->q38GdnIdentity.session || c->q38GdnIdentity.sequence
		|| c->q38GdnIdentity.modelEpoch || c->q38GdnIdentity.driverGeneration || c->q38Cohort || c->fleet || sQ38LocalDeltaOwner
		|| c->q38Delta.saved || c->q38Delta.dirty || c->q38Delta.ready || c->q38Delta.running || c->q38Delta.awaiting
		|| c->panelProbe.saved || c->panelProbe.dirty || c->panelProbe.ready || c->streamProbe.saved || c->streamProbe.dirty || c->streamProbe.ready
		|| !c->q38Span || !c->q38Span->closed || c->q38Span->dirty || c->q38Span->ready || c->q38Span->running || c->q38Span->unsafe
		|| !c->lease.saved || !c->lease.ready || c->lease.pending || c->lease.recovery_required
		|| !c->bulk.saved || !c->bulk.ready || c->bulk.dirty || c->bulk.unsafe
		|| c->host.state!=TT_HOST_PAGE_READY || c->host.released || PioneerHostPageIO::quarantined
		|| !c->bulkHost.allocated || !c->bulkHost.ready || c->bulkHost.pending || c->bulkHost.quarantined || c->bulkHost.released
		|| !c->wideHost.allocated || !c->wideHost.ready || !c->wideHost.memory_type_set || c->wideHost.pending
		|| c->wideHost.quarantined || c->wideHost.released || wideQuarantined || m->Unsafe()
		|| m->graph.CanonicalModel(m->identity)!=&m->plan || !m->graph.MatchesInitializationOwner(c->banks,dram,m->identity)) return false;
#ifdef PIONEER_Q38_RESIDENT_DECODER_NATIVE
 if(!DecoderCallScopeMatches() || !m->MatchesInitializedHistory(c->banks,dram))return false;
#ifdef PIONEER_Q38_GDN_STATE_INIT_NATIVE
 if(!m->MatchesInitializedGdn(c->banks,dram))return false;
#endif
 if(m->phase!=PioneerQ38EmbeddingNativeState::Decoder && m->phase!=PioneerQ38EmbeddingNativeState::DecoderHeld && m->decoder.Claimed())return false;
#endif
	auto* state=m->graph.Projection(m->identity);const auto& z=m->zero;
	if (!state || !z.saved || !z.closed || z.nonce!=2 || z.ready || z.running || z.dirty || z.owner!=state || z.banks!=&c->banks
		|| z.proof!=&dram || !(z.identity==m->identity) || !state->InitializationPublished(m->identity) || !m->embedding.Completed() || !m->rms.Completed()
		|| !m->rms.MatchesNativeOwner(dram,noc,c->banks,m->graph,m->plan,m->embedding,m->identity,c->initialized,PioneerResidentBanks::Pages)) return false;
	if (m->phase==PioneerQ38EmbeddingNativeState::RmsComplete) {
		if (controller || runtimeCore || fleetEnabled || c->q38Dispatch || c->q38DispatchRoutes || m->projectionCohort
			|| m->projection.Claimed() || q38ProjectionMemoryMapAttempted) return false;
	} else {
		if ((m->phase!=PioneerQ38EmbeddingNativeState::ProjectionPreparing && m->phase!=PioneerQ38EmbeddingNativeState::Projection
#ifdef PIONEER_Q38_RESIDENT_DECODER_NATIVE
   && m->phase!=PioneerQ38EmbeddingNativeState::Decoder && m->phase!=PioneerQ38EmbeddingNativeState::DecoderHeld
#endif
   )
			|| !fleetEnabled || !c->q38Dispatch || m->projectionCohort!=c->q38Dispatch || c->q38Dispatch->worker[0].master!=&c->lease
			|| !m->ControlsProjectionCohort(dram,c->banks,c->q38Dispatch,c->initialized,PioneerResidentBanks::Pages)
			|| (controller && (!c->q38DispatchRoutes || !m->projection.MatchesNativeOwner(dram,c->banks,*c->q38Dispatch,m->projectionMemory,
				m->graph,m->plan,m->rms,m->identity,c->initialized,PioneerResidentBanks::Pages)))) return false;
	}
	const auto& clock=c->q38Clock.Snapshot();
	return c->q38Clock.RequiresExplicitEnd() && clock.state==TT_ARC_CLOCK_BUSY_VERIFIED && clock.busy_requested
		&& clock.busy_acknowledged && !clock.restored && !c->Unsafe(true,false,true);
}
bool PioneerSessionIO::ProjectionGraphMemoryScope(const PioneerQ38ProjectionGraphMemory& owner,const PioneerQ38CanonicalModelPlan& plan,
	const PioneerQ38GdnIdentity& id,const uint8_t* learned,unsigned pages)
{
	auto* m=q38MutableContext?q38MutableContext->q38EmbeddingModel:NULL;
	return m && &owner==&m->projectionMemory && &plan==&m->plan && id==m->identity && learned==m->ledger && pages==PioneerResidentBanks::Pages
		&& m->phase==PioneerQ38EmbeddingNativeState::ProjectionPreparing && ProjectionExecutionSourceScope(false)
		&& q38MutableContext->q38DispatchRoutes && ProjectionGraphMemoryMappingStable(runtimeCore)
		&& owner.MatchesNativeOwner(dram,q38MutableContext->banks,*m->projectionCohort,m->plan,id,learned,pages);
}
bool PioneerSessionIO::ProjectionGraphExecutionScope(const PioneerQ38ProjectionGraphOwner& owner,const PioneerQ38ProjectionGraphMemory& memory,
	const PioneerQ38ProjectionMutableBanks& graph,const PioneerQ38CanonicalModelPlan& plan,const PioneerQ38LearnedRmsController& input,
	const PioneerQ38ProjectionIdentity& id,const uint8_t* learned,unsigned pages)
{
	auto* m=q38MutableContext?q38MutableContext->q38EmbeddingModel:NULL;
	return m && &owner==&m->projection && &memory==&m->projectionMemory && &graph==&m->graph && &plan==&m->plan && &input==&m->rms
		&& id==m->identity && learned==m->ledger && pages==PioneerResidentBanks::Pages && ProjectionExecutionSourceScope(true);
}
void PioneerSessionIO::ProjectionGraphRetainHostOwners()
{
	if (q38MutableContext) {tt_resident_session_result result={};RetainQ38DispatchOwner(*q38MutableContext,result);} // MEMORY ONLY
}
// PIONEER_Q38_PROJECTION_EXECUTION_NATIVE_SCOPE_END
#endif
#ifdef PIONEER_Q38_RESIDENT_OUTPUT_OWNER
#include "PioneerQ38ResidentOutputNativeBind.inc"
#include "PioneerQ38ResidentOutputNativeInitialize.inc"
#include "PioneerQ38ResidentOutputNativeExecution.inc"
#endif
#ifdef PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT
#include "PioneerQ38ResidentLayerNativeBind.inc"
#include "PioneerQ38ResidentLayerNativeInitialize.inc"
#include "PioneerQ38ResidentLayerNativeExecution.inc"
#endif
#ifdef PIONEER_Q38_RESIDENT_DECODER_NATIVE
// PIONEER_Q38_RESIDENT_DECODER_NATIVE_SCOPE_BEGIN
bool PioneerSessionIO::DecoderCallScopeMatches()
{
 auto* c=q38MutableContext;
 if(!c || &c->io!=this || !sRuntimeOwner || sRuntimeOwner->magic!=0x50313030 || sRuntimeOwner->context!=c
  || c->q38GdnCallThread<0 || c->q38GdnCallThread!=find_thread(NULL) || sRuntimeOwner->team<0)return false;
 thread_info info={};
 return get_thread_info(c->q38GdnCallThread,&info)==B_OK && info.team==sRuntimeOwner->team;
} // actual FD/team/thread seal;the complete surrounding source/clock/host checks remain mandatory
#ifdef PIONEER_Q38_GDN_STATE_INIT_NATIVE
// PIONEER_Q38_GDN_STATE_INIT_NATIVE_SCOPE_BEGIN
bool PioneerSessionIO::RecurrentZeroSourceScope()
{
 auto* c=q38MutableContext;auto* m=c?c->q38EmbeddingModel:NULL;
 if(!c || &c->io!=this || !m || m->phase!=PioneerQ38EmbeddingNativeState::GdnZeroing
  || sColdRecoveryRequired || sReloadQuiesced || !sReloadGeneration || !DecoderCallScopeMatches()
  || !sBankStorageOnly || !c->bankStorageOnly || c->bytes!=PioneerResidentBanks::Total
  || c->runtimeCoreMode!=1 || runtimeCore || fleetEnabled || !runtimeMapped || q38SpanAllowed || c->dispatchOwnerRetained
  || !m->identity.Valid() || m->identity.driverGeneration!=sReloadGeneration || !c->initialized || !c->hashes || m->ledger!=c->initialized
  || m->position || m->token>=248320 || m->bank>=7 || m->Unsafe() || m->plan.RecurrentCount()!=672
  || c->q38FfnModelPlan || c->q38GraphMutable || c->q38FfnZero || c->q38FfnGraph || c->q38FfnCode
  || c->q38FfnModelIdentity.session || c->q38FfnModelIdentity.sequence || c->q38FfnModelIdentity.modelEpoch || c->q38FfnModelIdentity.driverGeneration
  || c->q38GdnBanks || c->q38StateZero || c->q38GdnIdentity.session || c->q38GdnIdentity.sequence
  || c->q38GdnIdentity.modelEpoch || c->q38GdnIdentity.driverGeneration || c->q38Dispatch || c->q38DispatchRoutes || c->q38Cohort || c->fleet || sQ38LocalDeltaOwner
  || c->q38Delta.saved || c->q38Delta.dirty || c->q38Delta.ready || c->q38Delta.running || c->q38Delta.awaiting
  || c->panelProbe.saved || c->panelProbe.dirty || c->panelProbe.ready || c->streamProbe.saved || c->streamProbe.dirty || c->streamProbe.ready
  || !c->q38Span || !c->q38Span->closed || c->q38Span->dirty || c->q38Span->ready || c->q38Span->running || c->q38Span->unsafe
  || !c->lease.saved || !c->lease.ready || c->lease.pending || c->lease.recovery_required
  || !c->bulk.saved || !c->bulk.ready || c->bulk.unsafe
  || c->host.state!=TT_HOST_PAGE_READY || c->host.released || PioneerHostPageIO::quarantined
  || !c->bulkHost.allocated || !c->bulkHost.ready || c->bulkHost.pending || c->bulkHost.quarantined || c->bulkHost.released
  || !c->wideHost.allocated || !c->wideHost.ready || !c->wideHost.memory_type_set || c->wideHost.pending
  || c->wideHost.quarantined || c->wideHost.released || wideQuarantined
  || !PioneerQ38ProjectionZeroPristine(m->zero) || m->zero.closed
  || !PioneerQ38ConvHistoryZeroPristine(m->historyZero) || m->historyZero.closed
  || m->embedding.Completed() || m->embedding.Running() || m->rms.Claimed() || m->projection.Claimed() || m->projectionCohort || m->decoder.Claimed()
  || m->graph.CanonicalModel(m->identity)!=&m->plan || !m->graph.MatchesInitializationOwner(c->banks,dram,m->identity))return false;
 auto* g=m->graph.Recurrent(m->identity);const auto& z=m->gdnZero;
 if(!g || !g->MatchesInitializationOwner(c->banks,dram,m->identity) || g->InitializationPublished(m->identity) || z.closed)return false;
 if(z.identity.Valid()){if(!(z.identity==m->identity))return false;}
 else if(z.identity.session || z.identity.sequence || z.identity.modelEpoch || z.identity.driverGeneration
  || z.saved || z.dirty || z.ready || z.running || z.nonce)return false;
 const auto& clock=c->q38Clock.Snapshot();
 return c->q38Clock.RequiresExplicitEnd() && clock.state==TT_ARC_CLOCK_BUSY_VERIFIED && clock.busy_requested
  && clock.busy_acknowledged && !clock.restored && !c->Unsafe(true,false,true);
} // CPU scope ONLY;actual ALL119 physical holds/code/NoC/data/CLOSE proofs stay mandatory
// PIONEER_Q38_GDN_STATE_INIT_NATIVE_SCOPE_END
#endif
bool PioneerSessionIO::ConvHistoryZeroBeforeCohort()
{
 auto* m=q38MutableContext?q38MutableContext->q38EmbeddingModel:NULL;
 return m && m->phase==PioneerQ38EmbeddingNativeState::HistoryZeroing && EmbeddingSourceScope(false);
}
bool PioneerSessionIO::ConvHistoryZeroOtherCoresHeld()
{
 if(!ConvHistoryZeroBeforeCohort() || !NonRootHeldMappingStable())return false;
 if(PioneerQ38NonRootHeld(*this,dram))return true;
 tt_resident_session_result result={};RetainQ38DispatchOwner(*q38MutableContext,result);return false;
}
bool PioneerSessionIO::ResidentDecoderScope(const PioneerQ38ResidentDecoderOwner& owner,
 const PioneerQ38ProjectionGraphOwner& projection,const PioneerQ38ConvHistoryBanks& history)
{
 auto* c=q38MutableContext;auto* m=c?c->q38EmbeddingModel:NULL;
 return m && &owner==&m->decoder && &projection==&m->projection && &history==m->graph.ConvHistory(m->identity)
  && (m->phase==PioneerQ38EmbeddingNativeState::Decoder || m->phase==PioneerQ38EmbeddingNativeState::DecoderHeld)
  && m->MatchesInitializedHistory(c->banks,dram) && owner.MatchesNativeOwner(projection,history)
  && ProjectionExecutionSourceScope(true); // SAME global context/clock/ALL hosts/current source,not a second owner
}
// PIONEER_Q38_RESIDENT_DECODER_NATIVE_SCOPE_END
#ifdef PIONEER_Q38_GDN_RECURRENT_NATIVE
// PIONEER_Q38_GDN_RECURRENT_NATIVE_SCOPE_BEGIN
bool PioneerSessionIO::ResidentGdnRecurrentScope(const PioneerQ38ResidentDecoderOwner& owner,
 const PioneerQ38ProjectionGraphOwner& projection,const PioneerQ38GdnBanks& state)
{
 auto* c=q38MutableContext;auto* m=c?c->q38EmbeddingModel:NULL;
 return m && &owner==&m->decoder && &projection==&m->projection && m->phase==PioneerQ38EmbeddingNativeState::DecoderHeld
  && &state==m->graph.Recurrent(m->identity) && m->MatchesInitializedGdn(c->banks,dram)
  && owner.MatchesRecurrentNativeOwner(state) && ProjectionExecutionSourceScope(true);
} // SAME real initialized producer/call/context/clock/ALLhosts,not metadata-only zero publication
// PIONEER_Q38_GDN_RECURRENT_NATIVE_SCOPE_END
#endif
#endif
// PIONEER_Q38_EMBEDDING_NATIVE_FACTORY_BEGIN
static uint64 sQ38EmbeddingModelEpoch=0; // global-lock protected,never wrap/reuse
[[maybe_unused]] static bool BindQ38EmbeddingModel(PioneerStoreContext& c,unsigned token,unsigned position,unsigned bank)
{
#ifdef PIONEER_Q38_RESIDENT_DECODER_NATIVE
 if(position || !c.io.DecoderCallScopeMatches())return false; // first-token implementation,not a guessed history epoch
#endif
	if (sColdRecoveryRequired || sReloadQuiesced || !sReloadGeneration || !sRuntimeOwner || sRuntimeOwner->context!=&c
		|| sRuntimeOwner->magic!=0x50313030 || !sBankStorageOnly || !c.bankStorageOnly || c.bytes!=PioneerResidentBanks::Total
		|| c.io.q38MutableContext!=&c || c.q38GdnCallThread<0 || c.q38GdnCallThread!=find_thread(NULL) || c.dispatchOwnerRetained
		|| c.runtimeCoreMode!=-1 || c.io.runtimeCore || c.io.fleetEnabled || !c.io.runtimeMapped || !c.io.q38SpanAllowed || c.Unsafe()
		|| c.q38EmbeddingModel || c.io.q38EmbeddingMemoryMapAttempted || !c.initialized || !c.hashes || sQ38EmbeddingModelEpoch==UINT64_MAX
		|| token>=248320 || position>=8192 || bank>=7 || c.q38FfnModelPlan || c.q38GraphMutable || c.q38FfnZero
		|| c.q38FfnGraph || c.q38FfnCode || c.q38GdnBanks || c.q38StateZero || c.q38Dispatch || c.q38Cohort || c.fleet || sQ38LocalDeltaOwner
		|| c.q38Delta.saved || c.q38Delta.dirty || c.q38Delta.ready || c.q38Delta.running || c.q38Delta.awaiting
		|| c.panelProbe.saved || c.panelProbe.dirty || c.panelProbe.ready || c.streamProbe.saved || c.streamProbe.dirty || c.streamProbe.ready
		|| c.q38FfnModelIdentity.session || c.q38FfnModelIdentity.sequence || c.q38FfnModelIdentity.modelEpoch || c.q38FfnModelIdentity.driverGeneration
		|| c.q38GdnIdentity.session || c.q38GdnIdentity.sequence || c.q38GdnIdentity.modelEpoch || c.q38GdnIdentity.driverGeneration
		|| !c.q38Span || !c.q38Span->closed || c.q38Span->dirty || c.q38Span->ready || c.q38Span->running || c.q38Span->unsafe
		|| !c.lease.saved || !c.lease.ready || c.lease.pending || c.lease.recovery_required || !c.bulk.saved || !c.bulk.ready || c.bulk.dirty || c.bulk.unsafe
		|| c.host.state!=TT_HOST_PAGE_READY || c.host.released || c.io.PioneerHostPageIO::quarantined
		|| !c.bulkHost.allocated || !c.bulkHost.ready || c.bulkHost.pending || c.bulkHost.quarantined || c.bulkHost.released
		|| !c.wideHost.allocated || !c.wideHost.ready || !c.wideHost.memory_type_set || c.wideHost.pending || c.wideHost.quarantined
		|| c.wideHost.released || c.io.wideQuarantined) return false;
	const auto& clock=c.q38Clock.Snapshot();
	if (!c.q38Clock.RequiresExplicitEnd() || clock.state!=TT_ARC_CLOCK_BUSY_VERIFIED || !clock.busy_requested || !clock.busy_acknowledged || clock.restored) return false;
	auto* m=new(std::nothrow) PioneerQ38EmbeddingNativeState;if (!m) return false;
	m->identity={sQ38EmbeddingModelEpoch+1,1,1,sReloadGeneration};m->ledger=c.initialized;m->token=token;m->position=position;m->bank=bank;
	uint32 source[16];const auto& noc=c.baseline.baseline.baseline.noc;
	for (unsigned i=0;i<16;i++) source[i]=i?noc.source[0][i]:c.lease.config|0x10000;
	PioneerQ38EmbeddingPlan check;
	if (!m->plan.Build() || !check.Build(m->plan,token,position,bank,m->identity,source,1)
		|| !check.LearnedPagesPresent(c.initialized,PioneerResidentBanks::Pages)) {delete m;return false;} // NO claim/IO
#ifdef PIONEER_Q38_RECURRENT_PREFIX_NATIVE_CONTEXT
 // This fixed prefix must refuse missing later-layer learned pages BEFORE
 // any epoch/reservation publication or embedding/state execution.
 if(token || position || !PioneerQ38RecurrentPrefixLearned(m->plan,c.initialized,PioneerResidentBanks::Pages)){delete m;return false;}
#endif
#ifdef PIONEER_Q38_MIXED_LIVE_VALIDATION_NATIVE
 // ALL original layer3 learned pages must exist BEFORE the embedding
 // producer can execute,not only when the later dense bind is attempted.
 unsigned denseTensors=0,denseParts=0;
 for(unsigned i=0;i<m->plan.Count();i++){
  const auto* p=m->plan.Part(i);if(p->kind!=m->plan.WeightKind || p->layer!=3)continue;
  if(!p->firstRow)denseTensors++;
  denseParts++;unsigned first=p->bank*unsigned(m->plan.Capacity/4096)+p->offset/4096;
  for(unsigned j=0;j<p->allocated/4096;j++)if(c.initialized[first+j]!=1){delete m;return false;}
 }
 if(denseTensors!=11 || denseParts!=53){delete m;return false;}
#endif
#if defined(PIONEER_Q38_RECURRENT_BLOCK_NATIVE_CONTEXT) || defined(PIONEER_Q38_RESIDENT_OUTPUT_VALIDATION_NATIVE) || defined(PIONEER_Q38_RESIDENT_LAYER_VALIDATION_NATIVE)
 // Match the existing native handoff's COMPLETE layer0 upload contract
 // BEFORE epoch/reservation/model publication. A partial prefix must not
 // execute and only then discover missing norm/FFN/output/Conv/GDN pages.
 if(!PioneerQ38ResidentOutputLearned(m->plan,c.initialized,PioneerResidentBanks::Pages)){delete m;return false;}
#endif
#if defined(PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE) || defined(PIONEER_Q38_RESIDUAL_RMS_NATIVE)
 if(!PioneerQ38GdnOutputValidationLearned(m->plan,c.initialized,PioneerResidentBanks::Pages)){delete m;return false;}
#endif
#ifdef PIONEER_Q38_RESIDUAL_RMS_NATIVE
 if(!PioneerQ38ResidualRmsLearned(m->plan,c.initialized,PioneerResidentBanks::Pages,token,bank,m->identity,source)){delete m;return false;}
#endif
#ifdef PIONEER_Q38_FFN_RESIDUAL_NATIVE
 if(!PioneerQ38FfnResidualLearned(m->plan,c.initialized,PioneerResidentBanks::Pages)){delete m;return false;}
#endif
#ifdef PIONEER_Q38_NEXT_RMS_NATIVE
 if(!PioneerQ38NextRmsLearned(m->plan,c.initialized,PioneerResidentBanks::Pages,bank,m->identity,source)){delete m;return false;}
#endif
#ifdef PIONEER_Q38_NEXT_PROJECTION_NATIVE
 if(!PioneerQ38NextProjectionLearned(m->plan,c.initialized,PioneerResidentBanks::Pages)){delete m;return false;}
#endif
#ifdef PIONEER_Q38_NEXT_CONV_NATIVE
 if(!PioneerQ38NextConvLearned(m->plan,c.initialized,PioneerResidentBanks::Pages)){delete m;return false;}
#endif
#ifdef PIONEER_Q38_NEXT_GDN_NATIVE
 if(!PioneerQ38NextGdnLearned(m->plan,c.initialized,PioneerResidentBanks::Pages)){delete m;return false;}
#endif
#ifdef PIONEER_Q38_NEXT_GDN_OUTPUT_NATIVE
 if(!PioneerQ38NextGdnOutputLearned(m->plan,c.initialized,PioneerResidentBanks::Pages)){delete m;return false;}
#endif
#ifdef PIONEER_Q38_NEXT_RESIDUAL_RMS_NATIVE
 if(!PioneerQ38NextResidualRmsLearned(m->plan,c.initialized,PioneerResidentBanks::Pages)){delete m;return false;}
#endif
#ifdef PIONEER_Q38_NEXT_FFN_RESIDUAL_NATIVE
 if(!PioneerQ38NextFfnResidualLearned(m->plan,c.initialized,PioneerResidentBanks::Pages)){delete m;return false;}
#endif
#ifdef PIONEER_Q38_RESIDENT_DECODER_NATIVE
 unsigned convStripes=0;
 for(unsigned i=0;i<m->plan.Count();i++){
  const auto* p=m->plan.Part(i);if(p->kind!=m->plan.WeightKind || p->layer || p->role!=12)continue;
  if(p->type || p->rowBytes!=16 || p->bank>=7 || p->allocated<4096){delete m;return false;}
  unsigned first=p->bank*unsigned(m->plan.Capacity/4096)+p->offset/4096;
  for(unsigned page=0;page<p->allocated/4096;page++)if(c.initialized[first+page]!=1){delete m;return false;}
  convStripes++;
 }
 if(convStripes!=7){delete m;return false;} // exact original learned Conv4 pages BEFORE epoch/claims/IO
#ifdef PIONEER_Q38_GDN_INPUT_OWNER
 PioneerQ38GdnInputPlan learnedGdn;
 if(!learnedGdn.Build(m->plan,0,0,0,bank,m->identity,source,3)
  || !learnedGdn.LearnedPagesPresent(c.initialized,PioneerResidentBanks::Pages)){delete m;return false;}
#ifdef PIONEER_Q38_GDN_GATED_NATIVE
 if(!PioneerQ38GdnGatedPlan::LearnedPagePresentBeforeState(m->plan,0,c.initialized,PioneerResidentBanks::Pages)){delete m;return false;}
#endif
#endif
#endif
#ifdef PIONEER_Q38_LEARNED_RMS_NATIVE
	PioneerQ38LearnedRmsPlan norm;
	if (!norm.Build(m->plan,0,0,position,bank,m->identity,source,2)
		|| !norm.LearnedPagesPresent(c.initialized,PioneerResidentBanks::Pages)) {delete m;return false;} // BEFORE publish/claim/IO
#endif
#ifdef PIONEER_Q38_PROJECTION_EXECUTION_OWNER
	const unsigned projectionRoles[]={5U,6U,10U,11U};
	for (unsigned role:projectionRoles)if (!m->plan.ProjectionPagesPresent(0,role,c.initialized,PioneerResidentBanks::Pages)) {delete m;return false;}
#endif
	// Host SAME-FILE model provenance remains mandatory;kernel epoch denotes
	// this actual retained layout/source binding,not caller SHA authentication.
	sQ38EmbeddingModelEpoch++;c.q38EmbeddingModel=m;c.runtimeCoreMode=1;c.io.q38SpanAllowed=false;
	bool okay=m->graph.BindCanonical(c.banks,c.baseline.baseline.baseline.baseline.dram,m->identity,c.initialized,PioneerResidentBanks::Pages,m->plan,bank);
	if (!okay) {tt_resident_session_result result={};RetainQ38DispatchOwner(c,result);}return okay;
}
[[maybe_unused]] static bool InitializeQ38EmbeddingModel(PioneerStoreContext& c,PioneerQ38ProjectionZeroResult& result)
{
#ifdef PIONEER_Q38_RESIDENT_OUTPUT_OWNER
 if(c.q38ResidentOutputNative && !c.io.ResidentOutputPrefixInitializationScope())return false;
#ifdef PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT
 if(c.q38ResidentLayerNative && !c.io.ResidentLayerPrefixInitializationScope())return false;
#endif
#endif
	auto* m=c.q38EmbeddingModel;if (!m || m->phase!=PioneerQ38EmbeddingNativeState::Bound || m->Unsafe()) return false;
#ifdef PIONEER_Q38_RESIDENT_DECODER_NATIVE
 if(!m->MatchesInitializedHistory(c.banks,c.baseline.baseline.baseline.baseline.dram))return false;
#endif
	m->phase=PioneerQ38EmbeddingNativeState::Zeroing;
	auto* state=m->graph.Projection(m->identity);if (!state) {tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);return false;}
	auto& dram=c.baseline.baseline.baseline.baseline.dram;auto& noc=c.baseline.baseline.baseline.noc;
	bool okay=PioneerQ38ProjectionZeroPrepare(c.io,dram,noc,*state,c.banks,m->identity,c.host,c.lease,c.bulk,c.bulkHost,c.wideHost,m->zero)
		&& PioneerQ38ProjectionZeroRun(c.io,dram,noc,*state,c.banks,c.host,c.lease,c.bulk,c.bulkHost,c.wideHost,m->zero,m->identity,0,0,256,result)
		&& result.pages==256 && result.bytes==1048576 && result.nonce==1 && result.completed && result.held && !result.recovery_required
		&& PioneerQ38ProjectionZeroRun(c.io,dram,noc,*state,c.banks,c.host,c.lease,c.bulk,c.bulkHost,c.wideHost,m->zero,m->identity,0,256,256,result)
		&& result.pages==256 && result.bytes==1048576 && result.nonce==2 && result.completed && result.held && !result.recovery_required
		&& PioneerQ38ProjectionZeroClose(c.io,dram,noc,*state,c.banks,c.host,c.lease,c.bulk,c.bulkHost,c.wideHost,m->zero,true)
		&& state->InitializationPublished(m->identity);
	if (okay) m->phase=PioneerQ38EmbeddingNativeState::Embedding;
	else {tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}return okay;
}
[[maybe_unused]] static bool RunQ38EmbeddingModel(PioneerStoreContext& c,PioneerQ38EmbeddingResult& result
#ifdef PIONEER_Q38_EMBEDDING_VALIDATION_NATIVE
	,uint32* validation=NULL
#endif
)
{
	auto* m=c.q38EmbeddingModel;if (!m || m->phase!=PioneerQ38EmbeddingNativeState::Embedding || m->Unsafe()) return false;
	bool okay=c.io.EmbeddingSourceScope(false) && c.io.MapEmbeddingMemory()
		&& m->embedding.RunOnce(c.io,c.baseline.baseline.baseline.baseline.dram,c.baseline.baseline.baseline.noc,c.banks,m->graph,m->plan,
			m->identity,c.initialized,PioneerResidentBanks::Pages,c.lease,m->token,m->position,m->bank,result
#ifdef PIONEER_Q38_EMBEDDING_VALIDATION_NATIVE
			,validation
#endif
			);
	if (okay) m->phase=PioneerQ38EmbeddingNativeState::Complete;
	else {tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}return okay;
}
// PIONEER_Q38_EMBEDDING_NATIVE_FACTORY_END
#ifdef PIONEER_Q38_LEARNED_RMS_NATIVE
// PIONEER_Q38_LEARNED_RMS_NATIVE_FACTORY_BEGIN
[[maybe_unused]] static bool RunQ38LearnedRmsModel(PioneerStoreContext& c,PioneerQ38LearnedRmsResult& result
#ifdef PIONEER_Q38_LEARNED_RMS_VALIDATION_NATIVE
	,uint32* normalized=NULL,uint32* residual=NULL
#endif
)
{
	auto* m=c.q38EmbeddingModel;
	if (!m || m->phase!=PioneerQ38EmbeddingNativeState::Complete || !m->Closed() || m->rms.Claimed() || m->Unsafe()) return false;
	m->phase=PioneerQ38EmbeddingNativeState::Rms;
	bool okay=c.io.LearnedRmsSourceScope(false) && c.io.EmbeddingMemoryMappingStable()
		&& m->rms.RunOnce(c.io,c.baseline.baseline.baseline.baseline.dram,c.baseline.baseline.baseline.noc,c.banks,m->graph,m->plan,
			m->embedding,m->identity,c.initialized,PioneerResidentBanks::Pages,c.lease,m->token,m->position,m->bank,result
#ifdef PIONEER_Q38_LEARNED_RMS_VALIDATION_NATIVE
			,normalized,residual
#endif
			);
	if (okay) m->phase=PioneerQ38EmbeddingNativeState::RmsComplete;
	else {tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}return okay;
}
// PIONEER_Q38_LEARNED_RMS_NATIVE_FACTORY_END
#endif
#ifdef PIONEER_Q38_PROJECTION_EXECUTION_OWNER
// PIONEER_Q38_PROJECTION_EXECUTION_NATIVE_FACTORY_BEGIN
[[maybe_unused]] static bool PrepareQ38ProjectionExecution(PioneerStoreContext& c,PioneerQ38ProjectionGraphResult& r)
{
	auto* m=c.q38EmbeddingModel;
	if (!m || m->phase!=PioneerQ38EmbeddingNativeState::RmsComplete || !m->Closed() || c.q38Dispatch || m->projectionCohort
		|| m->projection.Claimed() || c.io.q38ProjectionMemoryMapAttempted || !c.io.ProjectionExecutionSourceScope(false)
		|| r.version!=1 || r.phase || r.worker || r.word || r.unsafe || r.prepared || r.runs || r.closed || r.armed || r.released
		|| r.ready || r.stopped || r.held || r.rootStarts || r.rootHolds || r.rootReads || r.rootWrites || r.rootReadFlits || r.rootWriteFlits
		|| r.memory.version!=1 || r.memory.started || r.memory.phase || r.memory.worker || r.memory.word || r.memory.unsafe || r.memory.installed
		|| r.memory.closed || r.memory.savedWords || r.memory.verifiedOriginalWords || r.memory.installedWords || r.memory.restoredWords) return false;
#ifdef PIONEER_Q38_PROJECTION_VALIDATION_NATIVE
	if (r.validationPages || r.validationWords) return false;
#endif
	auto* participants=new(std::nothrow) PioneerQ38DispatchLease;if (!participants) return false; // NO new IO yet
	c.q38Dispatch=participants;m->projectionCohort=participants;m->phase=PioneerQ38EmbeddingNativeState::ProjectionPreparing;
	auto& dram=c.baseline.baseline.baseline.baseline.dram;auto& noc=c.baseline.baseline.baseline.noc;
	auto& lease=*participants;lease.worker[0].master=&c.lease;lease.worker[0].noc=noc;c.io.fleetEnabled=true;
	bool okay=c.io.SelectRuntimeCore(0) && c.lease.ready && c.io.MapRuntime() && BlackholeDramAllCoresHeld(c.io,dram);
	for (unsigned w=1;w<120 && okay;w++) okay=PioneerWorkerNocProbe(c.io,dram,w,lease.worker[w].noc);
	c.q38DispatchRoutes=okay;
	for (unsigned w=0;w<120 && okay;w++) {
		auto& node=lease.worker[w];okay=c.io.SelectRuntimeCore(w) && c.io.MapQ38Panel();
		if (okay && w) okay=PioneerQ38CohortPeerLeaseBegin(c.io,dram,w,node);
		if (okay) okay=PioneerQ38DispatchPrepareRole(c.io,dram,c.banks,lease,w);
		if (!okay) lease.prepared=w+1;
	}
	if (okay) okay=c.io.SelectRuntimeCore(0) && c.io.MapProjectionGraphMemory()
		&& m->projection.Prepare(c.io,dram,c.banks,lease,m->projectionMemory,m->graph,m->plan,m->rms,m->identity,
			c.initialized,PioneerResidentBanks::Pages,m->token,m->position,m->bank,10,r);
	if (okay) m->phase=PioneerQ38EmbeddingNativeState::Projection;
	else {tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}return okay;
}
[[maybe_unused]] static bool RunQ38ProjectionExecution(PioneerStoreContext& c,unsigned role,PioneerQ38ProjectionGraphResult& r)
{
	if (role!=5 && role!=6 && role!=10 && role!=11) return false; // configuration,NO IO or ownership mutation
	auto* m=c.q38EmbeddingModel;if (!m || m->phase!=PioneerQ38EmbeddingNativeState::Projection || m->Unsafe()) return false;
	bool okay=c.io.ProjectionExecutionSourceScope(true) && m->projection.Run(c.io,role,r);
	if (!okay) {tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}return okay;
}
[[maybe_unused]] static bool CloseQ38ProjectionExecution(PioneerStoreContext& c,PioneerQ38ProjectionGraphResult& r
#ifdef PIONEER_Q38_PROJECTION_VALIDATION_NATIVE
	,uint32* validation=NULL
#endif
)
{
#ifdef PIONEER_Q38_RESIDENT_DECODER_NATIVE
 (void)c;(void)r;return false; // transferred snapshot must close through the actual decoder,never the old restore path
#else
	auto* m=c.q38EmbeddingModel;if (!m || m->phase!=PioneerQ38EmbeddingNativeState::Projection || m->Unsafe()) return false;
	bool okay=c.io.ProjectionExecutionSourceScope(true) && m->projection.Close(c.io,r
#ifdef PIONEER_Q38_PROJECTION_VALIDATION_NATIVE
		,validation
#endif
		);
	if (okay) m->phase=PioneerQ38EmbeddingNativeState::ProjectionComplete;
	else {tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}return okay;
#endif
}
// PIONEER_Q38_PROJECTION_EXECUTION_NATIVE_FACTORY_END
#endif
#ifdef PIONEER_Q38_RESIDENT_DECODER_NATIVE
// PIONEER_Q38_RESIDENT_DECODER_NATIVE_FACTORY_BEGIN
#ifdef PIONEER_Q38_GDN_STATE_INIT_NATIVE
// PIONEER_Q38_GDN_STATE_INIT_NATIVE_FACTORY_BEGIN
[[maybe_unused]] static bool InitializeQ38GdnStateModel(PioneerStoreContext& c,PioneerQ38StateZeroResult& r)
{
 auto* m=c.q38EmbeddingModel;
 if(!m || m->phase!=PioneerQ38EmbeddingNativeState::Bound || m->Unsafe() || m->position || m->decoder.Claimed()
  || m->gdnZero.saved || m->gdnZero.dirty || m->gdnZero.ready || m->gdnZero.running || m->gdnZero.closed
  || m->gdnZero.nonce || m->gdnZero.identity.session || m->gdnZero.identity.sequence
  || m->gdnZero.identity.modelEpoch || m->gdnZero.identity.driverGeneration)return false;
 auto* g=m->graph.Recurrent(m->identity);unsigned regions=0,pages=0;
 if(!g || !g->InitializationRegions(m->identity,regions) || regions!=672
  || !g->InitializationProgress(m->identity,pages) || pages || g->InitializationPublished(m->identity))return false;
 m->phase=PioneerQ38EmbeddingNativeState::GdnZeroing;
 auto& dram=c.baseline.baseline.baseline.baseline.dram;auto& noc=c.baseline.baseline.baseline.noc;
 bool okay=PioneerQ38StateZeroPrepare(c.io,dram,noc,*g,c.banks,m->identity,c.host,c.lease,c.bulk,c.bulkHost,c.wideHost,m->gdnZero);
 for(unsigned region=0;region<regions && okay;region++){
  unsigned count=0;
  okay=g->InitializationPages(m->identity,region,count) && (count==96 || count==112)
   && PioneerQ38StateZeroRun(c.io,dram,noc,*g,c.banks,c.host,c.lease,c.bulk,c.bulkHost,c.wideHost,m->gdnZero,m->identity,region,0,count,r)
   && r.state==TT_RUNTIME_PASSED && r.pages==count && r.bytes==count*4096 && r.nonce==region+1
   && r.started && r.completed && r.held && !r.recovery_required;
 }
 okay=okay && PioneerQ38StateZeroClose(c.io,dram,noc,*g,c.banks,c.host,c.lease,c.bulk,c.bulkHost,c.wideHost,m->gdnZero,true)
  && m->MatchesInitializedGdn(c.banks,dram);
 if(okay)m->phase=PioneerQ38EmbeddingNativeState::Bound;
 else{tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}
 return okay; // BOTH slots actually zero+CLOSE,not a recurrent read/worker/token capability
}
// PIONEER_Q38_GDN_STATE_INIT_NATIVE_FACTORY_END
#endif
[[maybe_unused]] static bool InitializeQ38ConvHistoryModel(PioneerStoreContext& c,PioneerQ38ConvHistoryZeroResult& r)
{
 auto* m=c.q38EmbeddingModel;
 if(!m || m->phase!=PioneerQ38EmbeddingNativeState::Bound || m->Unsafe() || m->position || m->decoder.Claimed()
  || !PioneerQ38ConvHistoryZeroPristine(m->historyZero) || m->historyZero.closed)return false;
#ifdef PIONEER_Q38_GDN_STATE_INIT_NATIVE
 if(!m->MatchesInitializedGdn(c.banks,c.baseline.baseline.baseline.baseline.dram))return false;
#endif
 auto* h=m->graph.ConvHistory(m->identity);if(!h)return false;
 m->phase=PioneerQ38EmbeddingNativeState::HistoryZeroing;
 auto& dram=c.baseline.baseline.baseline.baseline.dram;auto& noc=c.baseline.baseline.baseline.noc;
 bool okay=PioneerQ38ConvHistoryZeroPrepare(c.io,dram,noc,*h,c.banks,m->identity,c.host,c.lease,c.bulk,c.bulkHost,c.wideHost,m->historyZero);
 for(unsigned region=0;region<336 && okay;region++){
  okay=PioneerQ38ConvHistoryZeroRun(c.io,dram,noc,*h,c.banks,c.host,c.lease,c.bulk,c.bulkHost,c.wideHost,m->historyZero,m->identity,region,0,5,r)
   && r.pages==5 && r.bytes==20480 && r.nonce==region+1 && r.completed && r.held && !r.recovery_required;
 }
 okay=okay && PioneerQ38ConvHistoryZeroClose(c.io,dram,noc,*h,c.banks,c.host,c.lease,c.bulk,c.bulkHost,c.wideHost,m->historyZero,true)
  && m->MatchesInitializedHistory(c.banks,dram);
 if(okay)m->phase=PioneerQ38EmbeddingNativeState::Bound;
 else{tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}
 return okay;
}
[[maybe_unused]] static bool RunQ38ResidentDecoderModel(PioneerStoreContext& c,PioneerQ38ResidentDecoderResult& r)
{
 auto* m=c.q38EmbeddingModel;
 if(!m || m->phase!=PioneerQ38EmbeddingNativeState::Projection || m->Unsafe() || m->decoder.Claimed())return false;
 if(!c.io.ProjectionExecutionSourceScope(true)){tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);return false;}
 auto* h=m->graph.ConvHistory(m->identity);if(!h)return false;
 m->phase=PioneerQ38EmbeddingNativeState::Decoder;
 bool okay=m->decoder.RunOnce(c.io,m->projection,*h,r);
 if(okay)m->phase=PioneerQ38EmbeddingNativeState::DecoderHeld;
 else if(!m->decoder.Claimed() && !m->Unsafe())m->phase=PioneerQ38EmbeddingNativeState::Projection; // inert DATA refusal BEFORE claims/IO
 else{tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}
 return okay;
}
#ifdef PIONEER_Q38_GDN_INPUT_OWNER
#ifdef PIONEER_Q38_GDN_RECURRENT_NATIVE
// PIONEER_Q38_GDN_RECURRENT_NATIVE_FACTORY_BEGIN
[[maybe_unused]] static bool RunQ38ResidentGdnRecurrent(PioneerStoreContext& c,PioneerQ38ResidentDecoderResult& r)
{
 auto* m=c.q38EmbeddingModel;
 if(!m || m->phase!=PioneerQ38EmbeddingNativeState::DecoderHeld || m->Unsafe() || m->decoder.GdnInputsClaimed())return false;
 auto* g=m->graph.Recurrent(m->identity);
 bool okay=g && m->MatchesInitializedGdn(c.banks,c.baseline.baseline.baseline.baseline.dram)
  && c.io.ProjectionExecutionSourceScope(true) && m->decoder.RunGdnRecurrent(c.io,*g,r);
 if(!okay){tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}
 return okay;
}
// PIONEER_Q38_GDN_RECURRENT_NATIVE_FACTORY_END
#endif
[[maybe_unused]] static bool PrepareQ38ResidentGdnInputs(PioneerStoreContext& c,PioneerQ38ResidentDecoderResult& r)
{
#ifdef PIONEER_Q38_GDN_RECURRENT_NATIVE
 (void)c;(void)r;return false; // old prep-only entry is INERT in combined mode,not a retention fault
#else
 auto* m=c.q38EmbeddingModel;
 if(!m || m->phase!=PioneerQ38EmbeddingNativeState::DecoderHeld || m->Unsafe() || m->decoder.GdnInputsClaimed())return false;
 bool okay=c.io.ProjectionExecutionSourceScope(true) && m->decoder.PrepareGdnInputs(c.io,r);
 if(!okay){tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}
 return okay; // SAME native call/context/owner. NO public opcode or new producer grant.
#endif
}
#endif
#ifdef PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE
[[maybe_unused]] static bool RunQ38ResidentGdnOutput(PioneerStoreContext& c,PioneerQ38ResidentDecoderResult& r)
{
 auto* m=c.q38EmbeddingModel;
 if(!m || m->phase!=PioneerQ38EmbeddingNativeState::DecoderHeld || m->Unsafe() || !m->decoder.GatedOutput())return false;
 bool okay=c.io.ProjectionExecutionSourceScope(true) && m->decoder.RunGdnOutput(c.io,r);
 if(!okay && m->Unsafe()){tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}
 return okay; // SAME actual source/call/thread/clock/hosts,NO public selector
}
#endif
#ifdef PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE
[[maybe_unused]] static bool ReadQ38ResidentGdnOutputValidationInput(PioneerStoreContext& c,PioneerQ38ResidentDecoderResult& r,uint32* values)
{
 auto* m=c.q38EmbeddingModel;
 if(!m || m->phase!=PioneerQ38EmbeddingNativeState::DecoderHeld || m->Unsafe() || !m->decoder.GatedOutput())return false;
 bool okay=c.io.ProjectionExecutionSourceScope(true) && m->decoder.ReadGdnOutputValidationInput(c.io,values,r);
 if(!okay && m->Unsafe()){tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}
 return okay;
}
#endif
#ifdef PIONEER_Q38_RESIDUAL_RMS_NATIVE
[[maybe_unused]] static bool RunQ38ResidentResidualRms(PioneerStoreContext& c,PioneerQ38ResidentDecoderResult& r)
{
 auto* m=c.q38EmbeddingModel;
 if(!m || m->phase!=PioneerQ38EmbeddingNativeState::DecoderHeld || m->Unsafe() || !m->decoder.GdnOutputCompleted())return false;
 bool okay=c.io.ProjectionExecutionSourceScope(true) && m->decoder.RunResidualRms(c.io,r);
 if(!okay && m->Unsafe()){tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}
 return okay; // SAME global call/team/thread/source/clock/ALL hosts;NO public selector
}
#endif
#ifdef PIONEER_Q38_FFN_RESIDUAL_NATIVE
[[maybe_unused]] static bool RunQ38ResidentFfn(PioneerStoreContext& c,PioneerQ38ResidentDecoderResult& r)
{
 auto* m=c.q38EmbeddingModel;
 if(!m || m->phase!=PioneerQ38EmbeddingNativeState::DecoderHeld || m->Unsafe() || !m->decoder.PostAttentionInput())return false;
 bool okay=c.io.ProjectionExecutionSourceScope(true) && m->decoder.RunFfn(c.io,r);
 if(!okay && m->Unsafe()){tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}
 return okay; // no CPU vector/result/address or public opcode accepted
}
#endif
#ifdef PIONEER_Q38_NEXT_RMS_NATIVE
[[maybe_unused]] static bool RunQ38NextRms(PioneerStoreContext& c,PioneerQ38ResidentDecoderResult& r)
{
 auto* m=c.q38EmbeddingModel;
 if(!m || m->phase!=PioneerQ38EmbeddingNativeState::DecoderHeld || m->Unsafe() || !m->decoder.FfnHiddenInput())return false;
 bool okay=c.io.ProjectionExecutionSourceScope(true) && m->decoder.RunNextRms(c.io,r);
 if(!okay && m->Unsafe()){tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}
 return okay; // actual private hidden only,no caller layer/address/CPU product
}
#endif
#ifdef PIONEER_Q38_NEXT_PROJECTION_NATIVE
[[maybe_unused]] static bool RunQ38NextProjections(PioneerStoreContext& c,PioneerQ38ResidentDecoderResult& r)
{
 auto* m=c.q38EmbeddingModel;
 if(!m || m->phase!=PioneerQ38EmbeddingNativeState::DecoderHeld || m->Unsafe() || !m->decoder.NextLayerInput())return false;
 bool okay=c.io.ProjectionExecutionSourceScope(true) && m->decoder.RunNextProjections(c.io,r);
 if(!okay && m->Unsafe()){tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}
 return okay; // SAME private producer and all existing ownership;NO public selector
}
#endif
#ifdef PIONEER_Q38_NEXT_CONV_NATIVE
[[maybe_unused]] static bool RunQ38NextConv(PioneerStoreContext& c,PioneerQ38ResidentDecoderResult& r)
{
 auto* m=c.q38EmbeddingModel;
 if(!m || m->phase!=PioneerQ38EmbeddingNativeState::DecoderHeld || m->Unsafe() || !m->decoder.NextProjectionInput())return false;
 bool okay=c.io.ProjectionExecutionSourceScope(true) && m->decoder.RunNextConv(c.io,r);
 if(!okay && m->Unsafe()){tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}
 return okay; // logical genuine producer only;no host products/current-history flip
}
#endif
#ifdef PIONEER_Q38_NEXT_GDN_NATIVE
[[maybe_unused]] static bool RunQ38NextGdn(PioneerStoreContext& c,PioneerQ38ResidentDecoderResult& r)
{
 auto* m=c.q38EmbeddingModel;
 if(!m || m->phase!=PioneerQ38EmbeddingNativeState::DecoderHeld || m->Unsafe() || !m->decoder.NextConvolutionInput())return false;
 bool okay=c.io.ProjectionExecutionSourceScope(true) && m->decoder.RunNextGdn(c.io,r);
 if(!okay && m->Unsafe()){tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}
 return okay; // SAME private GDN NEXT only,not current-state/token publication
}
#endif
#ifdef PIONEER_Q38_NEXT_GDN_OUTPUT_NATIVE
[[maybe_unused]] static bool RunQ38NextGdnOutput(PioneerStoreContext& c,PioneerQ38ResidentDecoderResult& r)
{
 auto* m=c.q38EmbeddingModel;
 if(!m || m->phase!=PioneerQ38EmbeddingNativeState::DecoderHeld || m->Unsafe() || !m->decoder.NextAttentionInput())return false;
 bool okay=c.io.ProjectionExecutionSourceScope(true) && m->decoder.RunNextGdnOutput(c.io,r);
 if(!okay && m->Unsafe()){tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}
 return okay; // actual private layer1 output ONLY,not caller-ready/full model/Work
}
#endif
#ifdef PIONEER_Q38_NEXT_RESIDUAL_RMS_NATIVE
[[maybe_unused]] static bool RunQ38NextResidualRms(PioneerStoreContext& c,PioneerQ38ResidentDecoderResult& r)
{
 auto* m=c.q38EmbeddingModel;
 if(!m || m->phase!=PioneerQ38EmbeddingNativeState::DecoderHeld || m->Unsafe() || !m->decoder.NextPostAttentionInput())return false;
 bool okay=c.io.ProjectionExecutionSourceScope(true) && m->decoder.RunNextResidualRms(c.io,r);
 if(!okay && m->Unsafe()){tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}
 return okay; // genuine SAME layer1 output+raw only,no host products/public selector
}
#endif
#ifdef PIONEER_Q38_NEXT_FFN_RESIDUAL_NATIVE
[[maybe_unused]] static bool RunQ38NextFfnResidual(PioneerStoreContext& c,PioneerQ38ResidentDecoderResult& r)
{
 auto* m=c.q38EmbeddingModel;
 if(!m || m->phase!=PioneerQ38EmbeddingNativeState::DecoderHeld || m->Unsafe() || !m->decoder.NextFfnInput())return false;
 bool okay=c.io.ProjectionExecutionSourceScope(true) && m->decoder.RunNextFfn(c.io,r);
 if(!okay && m->Unsafe()){tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}
 return okay; // completed layer1 hidden stays private in GDDR;NO host products
}
#endif
[[maybe_unused]] static bool CloseQ38ResidentDecoderModel(PioneerStoreContext& c,PioneerQ38ResidentDecoderResult& r
#if defined(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE)
 ,uint32* validation=NULL
#endif
)
{
 auto* m=c.q38EmbeddingModel;
 if(!m || m->phase!=PioneerQ38EmbeddingNativeState::DecoderHeld || m->Unsafe())return false;
 bool okay=c.io.ProjectionExecutionSourceScope(true) && m->decoder.Close(c.io,r
#if defined(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE)
  ,validation
#endif
 );
 if(okay)m->phase=PioneerQ38EmbeddingNativeState::DecoderComplete;
 else{tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);}
 return okay;
}
// PIONEER_Q38_RESIDENT_DECODER_NATIVE_FACTORY_END
#endif
#endif
// PIONEER_Q38_STATE_ZERO_NATIVE_SCOPE_BEGIN
bool PioneerSessionIO::StateZeroBeforeCohort()
{
#ifdef PIONEER_Q38_RECURRENT_PREFIX_NATIVE_CONTEXT
 return RecurrentPrefixInitializationScope(false,false);
#elif defined(PIONEER_Q38_RECURRENT_BLOCK_NATIVE_CONTEXT)
 return RecurrentBlockInitializationScope(false,false);
#else
#ifdef PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT
 if(q38MutableContext && q38MutableContext->q38ResidentLayerNative)return ResidentLayerInitializationSourceScope(false,false);
#endif
#ifdef PIONEER_Q38_RESIDENT_OUTPUT_OWNER
 return ResidentOutputInitializationSourceScope(false,false);
#elif defined(PIONEER_Q38_GDN_STATE_INIT_NATIVE)
 return RecurrentZeroSourceScope();
#elif defined(PIONEER_Q38_FFN_MODEL_INIT)
	// PIONEER_Q38_FFN_MODEL_NATIVE_SCOPE_BEGIN
	// Same real FD/cookie/global-lock call seal,actual upload bitmap,joint
	// owner and clock. A host SHA/geometry/ready Boolean cannot grant this.
	PioneerStoreContext* c=q38MutableContext;
	if (!c || &c->io!=this || sColdRecoveryRequired || sReloadQuiesced || !sRuntimeOwner
		|| sRuntimeOwner->magic!=0x50313030 || sRuntimeOwner->context!=c || !sBankStorageOnly || !c->bankStorageOnly
		|| c->bytes!=PioneerResidentBanks::Total || c->runtimeCoreMode!=1 || runtimeCore || fleetEnabled || q38SpanAllowed
		|| c->q38GdnCallThread<0 || c->q38GdnCallThread!=find_thread(NULL)
		|| c->q38GdnBanks || c->q38StateZero || !c->q38FfnModelPlan || !c->q38FfnModelPlan->Ready()
		|| !c->q38GraphMutable || !c->q38FfnZero || !c->q38FfnModelIdentity.Valid()
		|| c->q38FfnModelIdentity.driverGeneration!=sReloadGeneration || c->q38FfnModelLayer>=64
		|| !c->initialized || c->q38FfnModelLedger!=c->initialized
		|| c->q38Dispatch || c->q38Cohort || c->fleet || sQ38LocalDeltaOwner
		|| c->q38Delta.saved || c->q38Delta.dirty || c->q38Delta.ready || c->q38Delta.running || c->q38Delta.awaiting
		|| c->panelProbe.saved || c->panelProbe.dirty || c->panelProbe.ready
		|| c->streamProbe.saved || c->streamProbe.dirty || c->streamProbe.ready
		|| !c->q38Span || !c->q38Span->closed || c->q38Span->dirty || c->q38Span->ready
		|| c->q38Span->running || c->q38Span->unsafe) return false;
	auto& graph=*c->q38GraphMutable;
	auto* ffn=graph.Ffn(c->q38FfnModelIdentity);
	if (!ffn || graph.CanonicalModel(c->q38FfnModelIdentity)!=c->q38FfnModelPlan
		|| !graph.MatchesInitializationOwner(c->banks,dram,c->q38FfnModelIdentity)) return false;
	const auto& zero=*c->q38FfnZero;
	if (zero.identity.Valid()) {
		if (!(zero.identity==c->q38FfnModelIdentity) || zero.owner!=ffn || zero.banks!=&c->banks || zero.proof!=&dram) return false;
	} else if (!PioneerQ38FfnZeroPristine(zero) || zero.closed) return false;
	const auto& clock=c->q38Clock.Snapshot();
	PioneerQ38FfnExtent probe;
	return c->q38Clock.RequiresExplicitEnd() && clock.state==TT_ARC_CLOCK_BUSY_VERIFIED
		&& clock.busy_requested && clock.busy_acknowledged && !clock.restored && !c->Unsafe(true)
		&& ffn->MatchesInitializationOwner(c->banks,dram,c->q38FfnModelIdentity)
		&& ffn->InitializeSpan(c->q38FfnModelIdentity,0,0,1,probe);
	// PIONEER_Q38_FFN_MODEL_NATIVE_SCOPE_END
#else
	PioneerStoreContext* c=q38MutableContext;
	if (!c || &c->io!=this || sColdRecoveryRequired || sReloadQuiesced || !sRuntimeOwner
		|| sRuntimeOwner->magic!=0x50313030 || sRuntimeOwner->context!=c || !sBankStorageOnly || !c->bankStorageOnly
		|| c->bytes!=PioneerResidentBanks::Total || c->runtimeCoreMode!=1 || runtimeCore || fleetEnabled
		|| q38SpanAllowed || c->q38GdnCallThread<0 || c->q38GdnCallThread!=find_thread(NULL)
		|| !c->q38GdnBanks || !c->q38StateZero || !c->q38GdnIdentity.Valid()
		|| c->q38GdnIdentity.driverGeneration!=sReloadGeneration
		|| c->q38Dispatch || c->q38Cohort || c->fleet || sQ38LocalDeltaOwner
		|| c->q38Delta.saved || c->q38Delta.dirty || c->q38Delta.ready || c->q38Delta.running || c->q38Delta.awaiting
		|| c->panelProbe.saved || c->panelProbe.dirty || c->panelProbe.ready
		|| c->streamProbe.saved || c->streamProbe.dirty || c->streamProbe.ready) return false;
	if (c->q38Span && (!c->q38Span->closed || c->q38Span->dirty || c->q38Span->ready
		|| c->q38Span->running || c->q38Span->unsafe)) return false;
	const auto& zero=*c->q38StateZero;
#ifdef PIONEER_Q38_RESIDENT_GDN_ROOT_DIAGNOSTIC
	if (c->q38ResidentGdnRoot) return false; // mutually exclusive root role,even after its clean CLOSE
#endif
	if (zero.identity.Valid()) { if (!(zero.identity==c->q38GdnIdentity)) return false; }
	else if (zero.identity.session || zero.identity.sequence || zero.identity.modelEpoch || zero.identity.driverGeneration
		|| zero.saved || zero.dirty || zero.ready || zero.running || zero.nonce) return false;
	const auto& clock=c->q38Clock.Snapshot();
	if (!c->q38Clock.RequiresExplicitEnd() || clock.state!=TT_ARC_CLOCK_BUSY_VERIFIED
		|| !clock.busy_requested || !clock.busy_acknowledged || clock.restored) return false;
	PioneerQ38GdnBankExtent probe;
	// Exempt ONLY this controlled root's running bit after proving the
	// exact context/identity/thread/clock and absence of competing owners.
	// ALL other uncertainty remains fatal;ordinary Unsafe() still rejects
	// this running bit for every cleanup/status/lost-caller path.
	return !c->Unsafe(true) && c->q38GdnBanks->MatchesInitializationOwner(c->banks,dram,c->q38GdnIdentity)
		&& c->q38GdnBanks->InitializeSpan(c->q38GdnIdentity,0,0,1,probe);
#endif
#endif
}
#ifdef PIONEER_Q38_RESIDENT_GDN_ROOT_DIAGNOSTIC
// PIONEER_Q38_RGDN_ROOT_NATIVE_SCOPE_BEGIN
// CPU-only exact context/owner gate. Never a model/peer compute capability.
bool PioneerSessionIO::ResidentGdnBeforeRoot()
{
	PioneerStoreContext* c=q38MutableContext;
	if (!c || &c->io!=this || sColdRecoveryRequired || sReloadQuiesced || !sRuntimeOwner
		|| sRuntimeOwner->magic!=0x50313030 || sRuntimeOwner->context!=c || !sBankStorageOnly || !c->bankStorageOnly
		|| c->bytes!=PioneerResidentBanks::Total || c->runtimeCoreMode!=1 || runtimeCore || fleetEnabled || q38SpanAllowed
		|| c->q38GdnCallThread<0 || c->q38GdnCallThread!=find_thread(NULL) || !c->q38GdnBanks || !c->q38StateZero
		|| !c->q38ResidentGdnRoot || !c->q38GdnIdentity.Valid() || c->q38GdnIdentity.driverGeneration!=sReloadGeneration
		|| c->q38Dispatch || c->q38Cohort || c->fleet || sQ38LocalDeltaOwner
		|| c->q38Delta.saved || c->q38Delta.dirty || c->q38Delta.ready || c->q38Delta.running || c->q38Delta.awaiting
		|| c->panelProbe.saved || c->panelProbe.dirty || c->panelProbe.ready
		|| c->streamProbe.saved || c->streamProbe.dirty || c->streamProbe.ready) return false;
	if (c->q38Span && (!c->q38Span->closed || c->q38Span->dirty || c->q38Span->ready || c->q38Span->running || c->q38Span->unsafe)) return false;
	const auto& zero=*c->q38StateZero;
	if (!zero.closed || zero.running || zero.unsafe || zero.dirty || zero.ready || !(zero.identity==c->q38GdnIdentity)) return false;
	const auto& root=*c->q38ResidentGdnRoot;
	if (root.identity.Valid()) {
		if (!(root.identity==c->q38GdnIdentity) || root.owner!=c->q38GdnBanks || root.banks!=&c->banks || root.proof!=&dram) return false;
		if ((root.running && (!root.saved || !root.dirty || !root.ready || root.closed || root.awaiting))
			|| (root.awaiting && (!root.saved || !root.dirty || !root.ready || root.closed || root.running))
			|| (root.closed && (root.running || root.awaiting || root.dirty || root.ready))) return false;
	} else if (root.identity.session || root.identity.sequence || root.identity.modelEpoch || root.identity.driverGeneration
		|| root.saved || root.dirty || root.ready || root.running || root.unsafe || root.awaiting || root.closed || root.nonce
		|| root.readSlot || root.position || root.owner || root.banks || root.proof) return false;
	const auto& clock=c->q38Clock.Snapshot();
	if (!c->q38Clock.RequiresExplicitEnd() || clock.state!=TT_ARC_CLOCK_BUSY_VERIFIED
		|| !clock.busy_requested || !clock.busy_acknowledged || clock.restored) return false;
	// Exempt ONLY this exact controlled root's running bit. Every other
	// uncertainty is still fatal;ordinary Unsafe() blocks cleanup/END.
	return !c->Unsafe(true) && c->q38GdnBanks->MatchesInitializationOwner(c->banks,dram,c->q38GdnIdentity)
		&& c->q38GdnBanks->InitializationPublished(c->q38GdnIdentity);
}
// PIONEER_Q38_RGDN_ROOT_NATIVE_SCOPE_END
#endif
// PIONEER_Q38_NONROOT_NATIVE_OWNER_BEGIN
bool PioneerSessionIO::NonRootHeldOwner() {
	return StateZeroBeforeCohort()
#ifdef PIONEER_Q38_RECURRENT_PREFIX_NATIVE_CONTEXT
  || RecurrentPrefixInitializationScope(true,false)
#endif
#ifdef PIONEER_Q38_RECURRENT_BLOCK_NATIVE_CONTEXT
  || RecurrentBlockInitializationScope(true,false)
#endif
#ifdef PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT
  || ResidentLayerInitializationSourceScope(true,false)
#endif
#ifdef PIONEER_Q38_RESIDENT_OUTPUT_OWNER
  || ResidentOutputInitializationSourceScope(true,false)
#endif
#ifdef PIONEER_Q38_EMBEDDING_NATIVE
		|| EmbeddingSourceScope(false)
#endif
#ifdef PIONEER_Q38_LEARNED_RMS_NATIVE
		|| LearnedRmsSourceScope(false)
#endif
#ifdef PIONEER_Q38_RESIDENT_GDN_ROOT_DIAGNOSTIC
		|| ResidentGdnBeforeRoot()
#endif
		;
}
bool PioneerSessionIO::NonRootHeldMappingStable()
{
	return NonRootHeldOwner() && PioneerMmioIO::area>=B_OK && nodeID && tlbArea>=B_OK && tlb
		&& resetArea>=B_OK && reset;
}
// PIONEER_Q38_NONROOT_NATIVE_OWNER_END
bool PioneerSessionIO::StateZeroOtherCoresHeld()
{
#ifdef PIONEER_Q38_RECURRENT_PREFIX_NATIVE_CONTEXT
 return RecurrentPrefixInitializationOtherCoresHeld(false);
#elif defined(PIONEER_Q38_RECURRENT_BLOCK_NATIVE_CONTEXT)
 return RecurrentBlockInitializationOtherCoresHeld(false);
#else
#ifdef PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT
 if(q38MutableContext && q38MutableContext->q38ResidentLayerNative)return ResidentLayerInitializationOtherCoresHeld(false);
#endif
#ifdef PIONEER_Q38_RESIDENT_OUTPUT_OWNER
 return ResidentOutputInitializationOtherCoresHeld(false);
#else
	if (!StateZeroBeforeCohort() || !NonRootHeldMappingStable()) return false; // CPU-only refusal
	if (PioneerQ38NonRootHeld(*this,dram)) return true;
	// First physical readback mismatch:full MEMORY-ONLY context quarantine,
	// including clock/cookie/PCI/ALL host leases,BEFORE returning to caller.
	tt_resident_session_result result={}; RetainQ38DispatchOwner(*q38MutableContext,result); return false;
#endif
#endif
}
#ifdef PIONEER_Q38_RESIDENT_GDN_ROOT_DIAGNOSTIC
// PIONEER_Q38_RGDN_ROOT_NATIVE_HELD_BEGIN
bool PioneerSessionIO::ResidentGdnOtherCoresHeld()
{
	if (!ResidentGdnBeforeRoot() || !NonRootHeldMappingStable()) return false;
	if (PioneerQ38NonRootHeld(*this,dram)) return true;
	tt_resident_session_result result={}; RetainQ38DispatchOwner(*q38MutableContext,result); return false; // MEMORY ONLY
}
// PIONEER_Q38_RGDN_ROOT_NATIVE_HELD_END
#endif
// PIONEER_Q38_STATE_ZERO_NATIVE_SCOPE_END
// PIONEER_Q38_GDN_CALL_SCOPE_BEGIN
// PRIVATE fixed zero diagnostic ONLY. Exactly ONE
// non-reentrant stack scope belongs at its control entry,not inside
// a controller/helper. FD lifetime keeps the cookie alive until return;
// context is captured/rechecked ONLY after acquiring the global lock.
// Destruction clears a CPU seal and releases that lock,NEVER END/STOP/
// restore/reset/ARC/unmap/free. All owners survive for explicit protocol END.
class PioneerQ38GdnCallScope {
	PioneerProbeCookie* cookie=NULL;
	PioneerStoreContext* context=NULL;
	thread_id thread=-1;
	status_t status=B_BAD_VALUE;
	bool held=false;
public:
	explicit PioneerQ38GdnCallScope(PioneerProbeCookie* owner) {
		if (!owner || owner->magic!=0x50313030) return;
		thread_info info={}; thread=find_thread(NULL);
		if (thread<0 || get_thread_info(thread,&info)!=B_OK || info.team!=owner->team) { status=B_NOT_ALLOWED; return; }
		status=AcquireQ38GdnSessionLock(owner); if (status!=B_OK) return;
		held=true; context=owner->context;
		if (owner->magic!=0x50313030 || sRuntimeOwner!=owner || !context || !sBankStorageOnly
			|| !context->bankStorageOnly || context->bytes!=PioneerResidentBanks::Total
			|| context->io.q38MutableContext!=context || context->q38GdnCallThread!=-1) {
			context=NULL; status=B_NOT_ALLOWED; held=false; release_sem(sLock); return;
		}
		if (context->Unsafe()) {
			tt_resident_session_result result={}; RetainQ38DispatchOwner(*context,result);
			context=NULL; status=B_DEV_NOT_READY; held=false; release_sem(sLock); return;
		}
		cookie=owner; context->q38GdnCallThread=thread;
	}
	PioneerQ38GdnCallScope(const PioneerQ38GdnCallScope&)=delete;
	PioneerQ38GdnCallScope& operator=(const PioneerQ38GdnCallScope&)=delete;
	~PioneerQ38GdnCallScope() {
		if (!held) return;
		if (!context || !cookie || cookie->context!=context || cookie->magic!=0x50313030
			|| sRuntimeOwner!=cookie || context->q38GdnCallThread!=thread || find_thread(NULL)!=thread) {
			if (context) { tt_resident_session_result result={}; RetainQ38DispatchOwner(*context,result); }
		} else context->q38GdnCallThread=-1;
		release_sem(sLock);
	}
	status_t Status() const { return status; }
	PioneerStoreContext* Context() const { return held && status==B_OK?context:NULL; }
};
// PIONEER_Q38_GDN_CALL_SCOPE_END
#ifdef PIONEER_Q38_RESIDENT_OUTPUT_VALIDATION_NATIVE
#include "PioneerQ38ResidentOutputValidationControl.h"
#endif
#ifdef PIONEER_Q38_RESIDENT_LAYER_VALIDATION_NATIVE
#include "PioneerQ38ResidentLayerValidationControl.h"
#endif
#ifdef PIONEER_Q38_MIXED_LIVE_VALIDATION_NATIVE
#include "PioneerQ38MixedLiveValidationControl.h"
#endif
#ifdef PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_NATIVE
#include "PioneerQ38RecurrentBlockValidationControl.h"
#endif
#ifdef PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE
// PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE_CONTROL_BEGIN
struct PioneerQ38GdnRecurrentValidationResults {
 PioneerQ38StateZeroResult state;PioneerQ38ConvHistoryZeroResult history;PioneerQ38ProjectionZeroResult zero;
 PioneerQ38EmbeddingResult embedding;PioneerQ38LearnedRmsResult norm;
 PioneerQ38ProjectionGraphResult projection;PioneerQ38ResidentDecoderResult decoder;
}; // HEAP only:the combined zero+compute results must not grow a kernel stack frame
static status_t Q38GdnRecurrentValidationControl(PioneerProbeCookie* cookie,uint32 operation,void* data,size_t length)
{
 if(!cookie || cookie->magic!=0x50313030 || !data)return B_BAD_VALUE;
 thread_info thread={};status_t status=get_thread_info(find_thread(NULL),&thread);
 if(status!=B_OK || thread.team!=cookie->team)return status!=B_OK?status:B_NOT_ALLOWED;
 if(operation==TT_PCI_PROBE_Q38_GDN_RECURRENT_VALIDATION_SUPPORT){
  if(length!=sizeof(tt_q38_embedding_support))return B_BAD_VALUE;
  status=AcquireQ38GdnSessionLock(cookie);if(status!=B_OK)return status;
  const tt_q38_embedding_support support={1,55,16,TT_Q38_GDN_RECURRENT_VALIDATION_WORDS,248320,512,0,0};
  status=user_memcpy(data,&support,sizeof(support));release_sem(sLock);return status;
 }
 if(operation!=TT_PCI_PROBE_Q38_GDN_RECURRENT_VALIDATE || length!=sizeof(tt_q38_gdn_recurrent_validation))return B_BAD_VALUE;
 auto* reply=new(std::nothrow) tt_q38_gdn_recurrent_validation;if(!reply)return B_NO_MEMORY;
 status=user_memcpy(reply,data,sizeof(*reply));
 bool shape=status==B_OK && PioneerQ38EmbeddingRequest(reply->metadata) && !reply->metadata.position;
 if(shape){const auto* bytes=reinterpret_cast<const uint8*>(reply);
  for(unsigned i=sizeof(reply->metadata);i<sizeof(*reply);i++)if(bytes[i]){shape=false;break;}
 }
 if(!shape){delete reply;return status!=B_OK?status:B_BAD_VALUE;}
 auto* results=new(std::nothrow) PioneerQ38GdnRecurrentValidationResults;
 if(!results){delete reply;return B_NO_MEMORY;}
 PioneerQ38GdnCallScope scope(cookie);auto* c=scope.Context();
 if(!c || !BindQ38EmbeddingModel(*c,reply->metadata.token,0,0)){
  delete results;delete reply;return scope.Status()!=B_OK?scope.Status():B_NOT_ALLOWED;
 }
 auto& state=results->state;auto& history=results->history;auto& zero=results->zero;
 auto& embedding=results->embedding;auto& norm=results->norm;auto& projection=results->projection;auto& decoder=results->decoder;
 uint64 began=uint64(system_time());
 bool okay=InitializeQ38GdnStateModel(*c,state) && InitializeQ38ConvHistoryModel(*c,history)
  && InitializeQ38EmbeddingModel(*c,zero) && RunQ38EmbeddingModel(*c,embedding)
  && RunQ38LearnedRmsModel(*c,norm) && PrepareQ38ProjectionExecution(*c,projection);
 const unsigned roles[]={10,11,5,6};for(unsigned role:roles)if(okay)okay=RunQ38ProjectionExecution(*c,role,projection);
 if(okay)okay=RunQ38ResidentDecoderModel(*c,decoder) && RunQ38ResidentGdnRecurrent(*c,decoder)
  && CloseQ38ResidentDecoderModel(*c,decoder,reply->values);
 uint64 ended=uint64(system_time());auto* m=c->q38EmbeddingModel;
 okay=okay && ended>=began && m && m->Closed() && m->embedding.Completed() && m->rms.Completed() && !c->Unsafe()
  && m->MatchesInitializedGdn(c->banks,c->baseline.baseline.baseline.baseline.dram)
  && m->gdnZero.closed && m->gdnZero.nonce==672 && state.nonce==672
  && m->historyZero.closed && m->historyZero.nonce==336 && history.nonce==336 && history.pages==5
  && projection.runs==4 && projection.armed==119 && projection.released==119 && projection.rootStarts==4 && projection.rootHolds==4
  && projection.memory.savedWords==8134072 && projection.memory.verifiedOriginalWords==8134072 && projection.memory.installedWords==989304
  && decoder.closed && !decoder.unsafe && decoder.installed==120 && decoder.armed==120 && decoder.released==120
  && decoder.completed==120 && decoder.held==120 && decoder.stop.stopped==119 && decoder.stop.held==119
  && decoder.stop.rootStarts==1 && decoder.stop.rootHolds==1 && decoder.gdnInstalled==96 && decoder.gdnArmed==96
  && decoder.gdnReleased==96 && decoder.gdnCompleted==96 && decoder.gdnHeld==96 && decoder.gdnLayerPublished==1
  && decoder.gdnValidationWorkers==96 && decoder.gdnValidationWords==TT_Q38_GDN_RECURRENT_VALIDATION_WORDS
  && decoder.memory.restoredWords==8134072 && decoder.memory.verifiedOriginalWords==8134072;
 auto& r=reply->metadata;r.state=okay?TT_RUNTIME_PASSED:TT_RUNTIME_COMPLETION_FAILED;
 r.closed=r.input_ready=okay?1:0;r.recovery_required=okay?0:1;
 r.session=m?m->identity.session:0;r.driver_generation=m?m->identity.driverGeneration:0;
 r.elapsed_us=okay?ended-began:0;r.source_pages=okay?23237:0;r.workspace_pages=okay?512:0;
 r.saved_words=okay?embedding.savedWords+norm.savedWords+projection.memory.savedWords:0;
 r.restored_words=okay?embedding.restoredWords+norm.restoredWords+decoder.memory.restoredWords:0;
 if(okay){
  reply->state_runs=state.nonce;reply->state_pages=73728;reply->history_runs=history.nonce;reply->history_pages=1680;
  reply->projections=projection.runs;reply->peer_boots=projection.released;reply->peer_stopped=decoder.stop.stopped;reply->peer_held=decoder.stop.held;
  reply->root_starts=projection.rootStarts+decoder.stop.rootStarts;reply->root_holds=projection.rootHolds+decoder.stop.rootHolds;
  reply->convolution_workers=decoder.completed;reply->convolution_jobs=640;reply->convolution_channels=10240;reply->convolution_held=decoder.held;
  reply->input_workers=96;reply->input_installed=decoder.gdnInstalled;reply->input_armed=decoder.gdnArmed;
  reply->input_released=decoder.gdnReleased;reply->input_completed=decoder.gdnCompleted;reply->input_held=decoder.gdnHeld;
  reply->layer_published=decoder.gdnLayerPublished;reply->readback_workers=decoder.gdnValidationWorkers;reply->readback_words=decoder.gdnValidationWords;
  reply->saved_words=projection.memory.savedWords;reply->verified_words=projection.memory.verifiedOriginalWords+decoder.memory.verifiedOriginalWords;
  reply->installed_words=projection.memory.installedWords;reply->restored_words=decoder.memory.restoredWords;
 }else{
  dprintf("Q38-GDN-RECURRENT-DEBUG: FAIL state%u history%u zero%u embedding%u norm%u projection%u decoder%u worker%u word%u readback%u/%u restored%u;NO_POST_FAILURE_DEVICE_IO\n",
   state.phase,history.phase,zero.phase,embedding.phase,norm.phase,projection.phase,decoder.phase,decoder.worker,decoder.word,
   decoder.gdnValidationWorkers,decoder.gdnValidationWords,decoder.memory.restoredWords);
  dprintf("Q38-GDN-RECURRENT-DEBUG: MEMORY_ONLY closed%u unsafe%u init%u gdn%u/%u/%u history%u/%u/%u/%u projection%u/%u/%u/%u/%u memory%u/%u/%u decoder%u/%u/%u/%u/%u/%u stop%u/%u/%u/%u gdn%u/%u/%u/%u/%u/%u verified%u\n",
   unsigned(m && m->Closed()),unsigned(c->Unsafe()),unsigned(m && m->MatchesInitializedGdn(c->banks,c->baseline.baseline.baseline.baseline.dram)),
   unsigned(m && m->gdnZero.closed),m?m->gdnZero.nonce:0,state.nonce,unsigned(m && m->historyZero.closed),m?m->historyZero.nonce:0,history.nonce,history.pages,
   projection.runs,projection.armed,projection.released,projection.rootStarts,projection.rootHolds,
   projection.memory.savedWords,projection.memory.verifiedOriginalWords,projection.memory.installedWords,
   decoder.closed,decoder.installed,decoder.armed,decoder.released,decoder.completed,decoder.held,
   decoder.stop.stopped,decoder.stop.held,decoder.stop.rootStarts,decoder.stop.rootHolds,
   decoder.gdnInstalled,decoder.gdnArmed,decoder.gdnReleased,decoder.gdnCompleted,decoder.gdnHeld,decoder.gdnLayerPublished,decoder.memory.verifiedOriginalWords);
  memset(reinterpret_cast<uint8*>(reply)+sizeof(reply->metadata),0,sizeof(*reply)-sizeof(reply->metadata));
 }
 status=user_memcpy(data,reply,sizeof(*reply));delete reply;
 if(!okay || status!=B_OK || c->Unsafe()){tt_resident_session_result retained={};RetainQ38DispatchOwner(*c,retained);}
 delete results;
 return status; // independent user DEBUG observer checks ALL823488 words BEFORE explicit END
}
// PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE_CONTROL_END
#endif
#ifdef PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE
#include "PioneerQ38GdnOutputValidationControl.h"
#endif
#ifdef PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE
// PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE_CONTROL_BEGIN
struct PioneerQ38GdnRendezvousValidationResults {
 PioneerQ38StateZeroResult state;PioneerQ38ConvHistoryZeroResult history;PioneerQ38ProjectionZeroResult zero;
 PioneerQ38EmbeddingResult embedding;PioneerQ38LearnedRmsResult norm;
 PioneerQ38ProjectionGraphResult projection;PioneerQ38ResidentDecoderResult decoder;
}; // HEAP only:the combined zero+compute results must not grow a kernel stack frame
static status_t Q38GdnRendezvousValidationControl(PioneerProbeCookie* cookie,uint32 operation,void* data,size_t length)
{
 if(!cookie || cookie->magic!=0x50313030 || !data)return B_BAD_VALUE;
 thread_info thread={};status_t status=get_thread_info(find_thread(NULL),&thread);
 if(status!=B_OK || thread.team!=cookie->team)return status!=B_OK?status:B_NOT_ALLOWED;
 if(operation==TT_PCI_PROBE_Q38_GDN_RENDEZVOUS_VALIDATION_SUPPORT){
  if(length!=sizeof(tt_q38_embedding_support))return B_BAD_VALUE;
  status=AcquireQ38GdnSessionLock(cookie);if(status!=B_OK)return status;
  const tt_q38_embedding_support support={1,55,17,TT_Q38_GDN_RENDEZVOUS_VALIDATION_WORDS,248320,512,0,0};
  status=user_memcpy(data,&support,sizeof(support));release_sem(sLock);return status;
 }
 if(operation!=TT_PCI_PROBE_Q38_GDN_RENDEZVOUS_VALIDATE || length!=sizeof(tt_q38_gdn_rendezvous_validation))return B_BAD_VALUE;
 auto* reply=new(std::nothrow) tt_q38_gdn_rendezvous_validation;if(!reply)return B_NO_MEMORY;
 status=user_memcpy(reply,data,sizeof(*reply));
 bool shape=status==B_OK && PioneerQ38EmbeddingRequest(reply->metadata) && !reply->metadata.position;
 if(shape){const auto* bytes=reinterpret_cast<const uint8*>(reply);
  for(unsigned i=sizeof(reply->metadata);i<sizeof(*reply);i++)if(bytes[i]){shape=false;break;}
 }
 if(!shape){delete reply;return status!=B_OK?status:B_BAD_VALUE;}
 auto* results=new(std::nothrow) PioneerQ38GdnRendezvousValidationResults();
 if(!results){delete reply;return B_NO_MEMORY;}
 PioneerQ38GdnCallScope scope(cookie);auto* c=scope.Context();
 if(!c || !BindQ38EmbeddingModel(*c,reply->metadata.token,0,0)){
  delete results;delete reply;return scope.Status()!=B_OK?scope.Status():B_NOT_ALLOWED;
 }
 auto& state=results->state;auto& history=results->history;auto& zero=results->zero;
 auto& embedding=results->embedding;auto& norm=results->norm;auto& projection=results->projection;auto& decoder=results->decoder;
 uint64 began=uint64(system_time());
 bool okay=InitializeQ38GdnStateModel(*c,state) && InitializeQ38ConvHistoryModel(*c,history)
  && InitializeQ38EmbeddingModel(*c,zero) && RunQ38EmbeddingModel(*c,embedding)
  && RunQ38LearnedRmsModel(*c,norm) && PrepareQ38ProjectionExecution(*c,projection);
 const unsigned roles[]={10,11,5,6};for(unsigned role:roles)if(okay)okay=RunQ38ProjectionExecution(*c,role,projection);
 if(okay)okay=RunQ38ResidentDecoderModel(*c,decoder) && RunQ38ResidentGdnRecurrent(*c,decoder)
  && CloseQ38ResidentDecoderModel(*c,decoder,reply->values);
 uint64 ended=uint64(system_time());auto* m=c->q38EmbeddingModel;
 okay=okay && ended>=began && m && m->Closed() && m->embedding.Completed() && m->rms.Completed() && !c->Unsafe()
  && m->MatchesInitializedGdn(c->banks,c->baseline.baseline.baseline.baseline.dram)
  && m->gdnZero.closed && m->gdnZero.nonce==672 && state.nonce==672
  && m->historyZero.closed && m->historyZero.nonce==336 && history.nonce==336 && history.pages==5
  && projection.runs==4 && projection.armed==119 && projection.released==119 && projection.rootStarts==4 && projection.rootHolds==4
  && projection.memory.savedWords==8330680 && projection.memory.verifiedOriginalWords==8330680
  // This ledger counts the initial projection images, not subsequent
  // checked decoder overlays (including the precise convolution image).
  && projection.memory.installedWords==1185912
  && decoder.closed && !decoder.unsafe && decoder.installed==120 && decoder.armed==120 && decoder.released==120
  && decoder.completed==120 && decoder.held==120 && decoder.stop.stopped==119 && decoder.stop.held==119
  && decoder.stop.rootStarts==1 && decoder.stop.rootHolds==1 && decoder.gdnInstalled==96 && decoder.gdnArmed==96
  && decoder.gdnReleased==96 && decoder.gdnCompleted==96 && decoder.gdnHeld==96 && decoder.gdnLayerPublished==1 && decoder.gdnGatedPublished==1
  && decoder.gdnValidationWorkers==96 && decoder.gdnValidationWords==TT_Q38_GDN_RENDEZVOUS_VALIDATION_WORDS
  && decoder.memory.restoredWords==8330680 && decoder.memory.verifiedOriginalWords==8330680;
 auto& r=reply->metadata;r.state=okay?TT_RUNTIME_PASSED:TT_RUNTIME_COMPLETION_FAILED;
 r.closed=r.input_ready=okay?1:0;r.recovery_required=okay?0:1;
 r.session=m?m->identity.session:0;r.driver_generation=m?m->identity.driverGeneration:0;
 r.elapsed_us=okay?ended-began:0;r.source_pages=okay?23238:0;r.workspace_pages=okay?512:0;
 r.saved_words=okay?embedding.savedWords+norm.savedWords+projection.memory.savedWords:0;
 r.restored_words=okay?embedding.restoredWords+norm.restoredWords+decoder.memory.restoredWords:0;
 if(okay){
  reply->gated_published=decoder.gdnGatedPublished;reply->readback_half_words=TT_Q38_GDN_RENDEZVOUS_VALIDATION_HALF_WORDS;
  reply->state_runs=state.nonce;reply->state_pages=73728;reply->history_runs=history.nonce;reply->history_pages=1680;
  reply->projections=projection.runs;reply->peer_boots=projection.released;reply->peer_stopped=decoder.stop.stopped;reply->peer_held=decoder.stop.held;
  reply->root_starts=projection.rootStarts+decoder.stop.rootStarts;reply->root_holds=projection.rootHolds+decoder.stop.rootHolds;
  reply->convolution_workers=decoder.completed;reply->convolution_jobs=640;reply->convolution_channels=10240;reply->convolution_held=decoder.held;
  reply->input_workers=96;reply->input_installed=decoder.gdnInstalled;reply->input_armed=decoder.gdnArmed;
  reply->input_released=decoder.gdnReleased;reply->input_completed=decoder.gdnCompleted;reply->input_held=decoder.gdnHeld;
  reply->layer_published=decoder.gdnLayerPublished;reply->readback_workers=decoder.gdnValidationWorkers;reply->readback_words=decoder.gdnValidationWords;
  reply->saved_words=projection.memory.savedWords;reply->verified_words=projection.memory.verifiedOriginalWords+decoder.memory.verifiedOriginalWords;
  reply->installed_words=projection.memory.installedWords;reply->restored_words=decoder.memory.restoredWords;
 }else{
  dprintf("Q38-GDN-TIMED-DEBUG: FAIL state%u history%u zero%u embedding%u norm%u projection%u decoder%u worker%u word%u readback%u/%u restored%u;NO_POST_FAILURE_DEVICE_IO\n",
   state.phase,history.phase,zero.phase,embedding.phase,norm.phase,projection.phase,decoder.phase,decoder.worker,decoder.word,
   decoder.gdnValidationWorkers,decoder.gdnValidationWords,decoder.memory.restoredWords);
  dprintf("Q38-GDN-TIMED-DEBUG: MEMORY_ONLY closed%u unsafe%u init%u gdn%u/%u/%u history%u/%u/%u/%u projection%u/%u/%u/%u/%u memory%u/%u/%u decoder%u/%u/%u/%u/%u/%u stop%u/%u/%u/%u gdn%u/%u/%u/%u/%u/%u verified%u\n",
   unsigned(m && m->Closed()),unsigned(c->Unsafe()),unsigned(m && m->MatchesInitializedGdn(c->banks,c->baseline.baseline.baseline.baseline.dram)),
   unsigned(m && m->gdnZero.closed),m?m->gdnZero.nonce:0,state.nonce,unsigned(m && m->historyZero.closed),m?m->historyZero.nonce:0,history.nonce,history.pages,
   projection.runs,projection.armed,projection.released,projection.rootStarts,projection.rootHolds,
   projection.memory.savedWords,projection.memory.verifiedOriginalWords,projection.memory.installedWords,
   decoder.closed,decoder.installed,decoder.armed,decoder.released,decoder.completed,decoder.held,
   decoder.stop.stopped,decoder.stop.held,decoder.stop.rootStarts,decoder.stop.rootHolds,
   decoder.gdnInstalled,decoder.gdnArmed,decoder.gdnReleased,decoder.gdnCompleted,decoder.gdnHeld,decoder.gdnLayerPublished,decoder.memory.verifiedOriginalWords);
  memset(reinterpret_cast<uint8*>(reply)+sizeof(reply->metadata),0,sizeof(*reply)-sizeof(reply->metadata));
 }
 status=user_memcpy(data,reply,sizeof(*reply));delete reply;
 if(!okay || status!=B_OK || c->Unsafe()){tt_resident_session_result retained={};RetainQ38DispatchOwner(*c,retained);}
 delete results;
 return status; // independent user DEBUG observer checks ALL829632 words BEFORE explicit END
}
// PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE_CONTROL_END
#endif
#ifdef PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE
// PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE_CONTROL_BEGIN
static status_t Q38GdnInputValidationControl(PioneerProbeCookie* cookie,uint32 operation,void* data,size_t length)
{
 if(!cookie || cookie->magic!=0x50313030 || !data)return B_BAD_VALUE;
 thread_info thread={};status_t status=get_thread_info(find_thread(NULL),&thread);
 if(status!=B_OK || thread.team!=cookie->team)return status!=B_OK?status:B_NOT_ALLOWED;
 if(operation==TT_PCI_PROBE_Q38_GDN_INPUT_VALIDATION_SUPPORT){
  if(length!=sizeof(tt_q38_embedding_support))return B_BAD_VALUE;
  status=AcquireQ38GdnSessionLock(cookie);if(status!=B_OK)return status;
  const tt_q38_embedding_support support={1,55,15,30912,248320,512,0,0};
  status=user_memcpy(data,&support,sizeof(support));release_sem(sLock);return status;
 }
 if(operation!=TT_PCI_PROBE_Q38_GDN_INPUT_VALIDATE || length!=sizeof(tt_q38_gdn_input_validation))return B_BAD_VALUE;
 auto* reply=new(std::nothrow) tt_q38_gdn_input_validation;if(!reply)return B_NO_MEMORY;
 status=user_memcpy(reply,data,sizeof(*reply));
 bool shape=status==B_OK && PioneerQ38EmbeddingRequest(reply->metadata) && !reply->metadata.position;
 if(shape){const auto* bytes=reinterpret_cast<const uint8*>(reply);
  for(unsigned i=sizeof(reply->metadata);i<sizeof(*reply);i++)if(bytes[i]){shape=false;break;}
 }
 if(!shape){delete reply;return status!=B_OK?status:B_BAD_VALUE;}
 PioneerQ38GdnCallScope scope(cookie);auto* c=scope.Context();
 if(!c || !BindQ38EmbeddingModel(*c,reply->metadata.token,0,0)){
  delete reply;return scope.Status()!=B_OK?scope.Status():B_NOT_ALLOWED;
 }
 PioneerQ38ConvHistoryZeroResult history;PioneerQ38ProjectionZeroResult zero;
 PioneerQ38EmbeddingResult embedding;PioneerQ38LearnedRmsResult norm;
 PioneerQ38ProjectionGraphResult projection;PioneerQ38ResidentDecoderResult decoder;
 uint64 began=uint64(system_time());
 bool okay=InitializeQ38ConvHistoryModel(*c,history) && InitializeQ38EmbeddingModel(*c,zero)
  && RunQ38EmbeddingModel(*c,embedding) && RunQ38LearnedRmsModel(*c,norm) && PrepareQ38ProjectionExecution(*c,projection);
 const unsigned roles[]={10,11,5,6};for(unsigned role:roles)if(okay)okay=RunQ38ProjectionExecution(*c,role,projection);
 if(okay)okay=RunQ38ResidentDecoderModel(*c,decoder) && PrepareQ38ResidentGdnInputs(*c,decoder)
  && CloseQ38ResidentDecoderModel(*c,decoder,reply->values);
 uint64 ended=uint64(system_time());auto* m=c->q38EmbeddingModel;
 okay=okay && ended>=began && m && m->Closed() && m->embedding.Completed() && m->rms.Completed() && !c->Unsafe()
  && m->historyZero.closed && m->historyZero.nonce==336 && history.nonce==336 && history.pages==5
  && projection.runs==4 && projection.armed==119 && projection.released==119 && projection.rootStarts==4 && projection.rootHolds==4
  && projection.memory.savedWords==8134072 && projection.memory.verifiedOriginalWords==8134072 && projection.memory.installedWords==989304
  && decoder.closed && !decoder.unsafe && decoder.installed==120 && decoder.armed==120 && decoder.released==120
  && decoder.completed==120 && decoder.held==120 && decoder.stop.stopped==119 && decoder.stop.held==119
  && decoder.stop.rootStarts==1 && decoder.stop.rootHolds==1 && decoder.gdnInstalled==96 && decoder.gdnArmed==96
  && decoder.gdnReleased==96 && decoder.gdnCompleted==96 && decoder.gdnHeld==96
  && decoder.gdnReadbackWorkers==96 && decoder.gdnReadbackWords==30912
  && decoder.memory.restoredWords==8134072 && decoder.memory.verifiedOriginalWords==8134072;
 auto& r=reply->metadata;r.state=okay?TT_RUNTIME_PASSED:TT_RUNTIME_COMPLETION_FAILED;
 r.closed=r.input_ready=okay?1:0;r.recovery_required=okay?0:1;
 r.session=m?m->identity.session:0;r.driver_generation=m?m->identity.driverGeneration:0;
 r.elapsed_us=okay?ended-began:0;r.source_pages=okay?23237:0;r.workspace_pages=okay?512:0;
 r.saved_words=okay?embedding.savedWords+norm.savedWords+projection.memory.savedWords:0;
 r.restored_words=okay?embedding.restoredWords+norm.restoredWords+decoder.memory.restoredWords:0;
 if(okay){
  reply->history_runs=history.nonce;reply->history_pages=1680;
  reply->projections=projection.runs;reply->peer_boots=projection.released;reply->peer_stopped=decoder.stop.stopped;reply->peer_held=decoder.stop.held;
  reply->root_starts=projection.rootStarts+decoder.stop.rootStarts;reply->root_holds=projection.rootHolds+decoder.stop.rootHolds;
  reply->convolution_workers=decoder.completed;reply->convolution_jobs=640;reply->convolution_channels=10240;reply->convolution_held=decoder.held;
  reply->input_workers=96;reply->input_installed=decoder.gdnInstalled;reply->input_armed=decoder.gdnArmed;
  reply->input_released=decoder.gdnReleased;reply->input_completed=decoder.gdnCompleted;reply->input_held=decoder.gdnHeld;
  reply->readback_workers=decoder.gdnReadbackWorkers;reply->readback_words=decoder.gdnReadbackWords;
  reply->saved_words=projection.memory.savedWords;reply->verified_words=projection.memory.verifiedOriginalWords+decoder.memory.verifiedOriginalWords;
  reply->installed_words=projection.memory.installedWords;reply->restored_words=decoder.memory.restoredWords;
 }else{
  dprintf("Q38-GDN-INPUT-DEBUG: FAIL history%u zero%u embedding%u norm%u projection%u decoder%u worker%u word%u readback%u/%u restored%u\n",
   history.phase,zero.phase,embedding.phase,norm.phase,projection.phase,decoder.phase,decoder.worker,decoder.word,
   decoder.gdnReadbackWorkers,decoder.gdnReadbackWords,decoder.memory.restoredWords);
  dprintf("Q38-GDN-INPUT-DEBUG: CAPTURED worker%u status%08x/%08x/%08x pc%08x/%08x;EXISTING_READS_ONLY_NO_POST_FAILURE_DEVICE_IO\n",
   decoder.observedWorker,decoder.observedStatus[0],decoder.observedStatus[1],decoder.observedStatus[2],decoder.observedPc[0],decoder.observedPc[1]);
  memset(reinterpret_cast<uint8*>(reply)+sizeof(reply->metadata),0,sizeof(*reply)-sizeof(reply->metadata));
 }
 status=user_memcpy(data,reply,sizeof(*reply));delete reply;
 if(!okay || status!=B_OK || c->Unsafe()){tt_resident_session_result retained={};RetainQ38DispatchOwner(*c,retained);}
 return status; // separate DEBUG observer compares ALL values BEFORE explicit END
}
// PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE_CONTROL_END
#endif
#ifdef PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE
// PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE_CONTROL_BEGIN
static status_t Q38ResidentDecoderValidationControl(PioneerProbeCookie* cookie,uint32 operation,void* data,size_t length)
{
 if(!cookie || cookie->magic!=0x50313030 || !data)return B_BAD_VALUE;
 thread_info thread={};status_t status=get_thread_info(find_thread(NULL),&thread);
 if(status!=B_OK || thread.team!=cookie->team)return status!=B_OK?status:B_NOT_ALLOWED;
 if(operation==TT_PCI_PROBE_Q38_DECODER_VALIDATION_SUPPORT){
  if(length!=sizeof(tt_q38_embedding_support))return B_BAD_VALUE;
  status=AcquireQ38GdnSessionLock(cookie);if(status!=B_OK)return status;
  const tt_q38_embedding_support support={1,55,14,10240,248320,512,0,0};
  status=user_memcpy(data,&support,sizeof(support));release_sem(sLock);return status;
 }
 if(operation!=TT_PCI_PROBE_Q38_DECODER_VALIDATE || length!=sizeof(tt_q38_resident_decoder_validation))return B_BAD_VALUE;
 auto* reply=new(std::nothrow) tt_q38_resident_decoder_validation;if(!reply)return B_NO_MEMORY;
 status=user_memcpy(reply,data,sizeof(*reply));
 bool shape=status==B_OK && PioneerQ38EmbeddingRequest(reply->metadata) && !reply->metadata.position;
 if(shape){const auto* bytes=reinterpret_cast<const uint8*>(reply);
  for(unsigned i=sizeof(reply->metadata);i<sizeof(*reply);i++)if(bytes[i]){shape=false;break;}
 }
 if(!shape){delete reply;return status!=B_OK?status:B_BAD_VALUE;}
 PioneerQ38GdnCallScope scope(cookie);auto* c=scope.Context();
 if(!c || !BindQ38EmbeddingModel(*c,reply->metadata.token,0,0)){
  delete reply;return scope.Status()!=B_OK?scope.Status():B_NOT_ALLOWED;
 }
 PioneerQ38ConvHistoryZeroResult history;PioneerQ38ProjectionZeroResult zero;
 PioneerQ38EmbeddingResult embedding;PioneerQ38LearnedRmsResult norm;
 PioneerQ38ProjectionGraphResult projection;PioneerQ38ResidentDecoderResult decoder;
 uint64 began=uint64(system_time());
 bool okay=InitializeQ38ConvHistoryModel(*c,history) && InitializeQ38EmbeddingModel(*c,zero)
  && RunQ38EmbeddingModel(*c,embedding) && RunQ38LearnedRmsModel(*c,norm) && PrepareQ38ProjectionExecution(*c,projection);
 const unsigned roles[]={10,11,5,6};for(unsigned role:roles)if(okay)okay=RunQ38ProjectionExecution(*c,role,projection);
 if(okay)okay=RunQ38ResidentDecoderModel(*c,decoder) && CloseQ38ResidentDecoderModel(*c,decoder,reply->values);
 uint64 ended=uint64(system_time());auto* m=c->q38EmbeddingModel;
 okay=okay && ended>=began && m && m->Closed() && m->embedding.Completed() && m->rms.Completed() && !c->Unsafe()
  && m->historyZero.closed && m->historyZero.nonce==336 && history.nonce==336 && history.pages==5
  && projection.runs==4 && projection.armed==119 && projection.released==119 && projection.rootStarts==4 && projection.rootHolds==4
  && projection.memory.savedWords==8134072 && projection.memory.verifiedOriginalWords==8134072 && projection.memory.installedWords==989304
  && decoder.closed && !decoder.unsafe && decoder.installed==120 && decoder.armed==120 && decoder.released==120
  && decoder.completed==120 && decoder.held==120 && decoder.stop.stopped==119 && decoder.stop.held==119
  && decoder.stop.rootStarts==1 && decoder.stop.rootHolds==1 && decoder.validationPages==40 && decoder.validationWords==40960
  && decoder.memory.restoredWords==8134072 && decoder.memory.verifiedOriginalWords==8134072;
 auto& r=reply->metadata;r.state=okay?TT_RUNTIME_PASSED:TT_RUNTIME_COMPLETION_FAILED;
 r.closed=r.input_ready=okay?1:0;r.recovery_required=okay?0:1;
 r.session=m?m->identity.session:0;r.driver_generation=m?m->identity.driverGeneration:0;
 r.elapsed_us=okay?ended-began:0;r.source_pages=okay?23235:0;r.workspace_pages=okay?512:0;
 // Snapshot accounting for embedding/RMS/projection;elapsed also includes
 // independently verified history initialization and its code restoration.
 r.saved_words=okay?embedding.savedWords+norm.savedWords+projection.memory.savedWords:0;
 r.restored_words=okay?embedding.restoredWords+norm.restoredWords+decoder.memory.restoredWords:0;
 if(okay){
  reply->history_runs=history.nonce;reply->history_pages=1680;
  reply->projections=projection.runs;reply->peer_boots=projection.released;reply->peer_stopped=decoder.stop.stopped;reply->peer_held=decoder.stop.held;
  reply->root_starts=projection.rootStarts+decoder.stop.rootStarts;reply->root_holds=projection.rootHolds+decoder.stop.rootHolds;
  reply->convolution_workers=decoder.completed;reply->convolution_jobs=640;reply->convolution_channels=10240;reply->convolution_held=decoder.held;
  reply->readback_pages=decoder.validationPages;reply->readback_words=decoder.validationWords;
  reply->saved_words=projection.memory.savedWords;reply->verified_words=projection.memory.verifiedOriginalWords+decoder.memory.verifiedOriginalWords;
  reply->installed_words=projection.memory.installedWords;reply->restored_words=decoder.memory.restoredWords;
 }else{
  dprintf("Q38-DECODER-VALIDATION: FAIL history%u zero%u embedding%u norm%u projection%u decoder%u worker%u word%u readback%u/%u restored%u\n",
   history.phase,zero.phase,embedding.phase,norm.phase,projection.phase,decoder.phase,decoder.worker,decoder.word,
   decoder.validationPages,decoder.validationWords,decoder.memory.restoredWords);
  dprintf("Q38-DECODER-VALIDATION: CAPTURED worker%u status%08x/%08x/%08x pc%08x/%08x;EXISTING_READS_ONLY_NO_POST_FAILURE_DEVICE_IO\n",
   decoder.observedWorker,decoder.observedStatus[0],decoder.observedStatus[1],decoder.observedStatus[2],decoder.observedPc[0],decoder.observedPc[1]);
  memset(reinterpret_cast<uint8*>(reply)+sizeof(reply->metadata),0,sizeof(*reply)-sizeof(reply->metadata));
 }
 status=user_memcpy(data,reply,sizeof(*reply));delete reply;
 if(!okay || status!=B_OK || c->Unsafe()){tt_resident_session_result retained={};RetainQ38DispatchOwner(*c,retained);}
 return status; // CPU comparison is separate DEBUG client;failure CLOSE ONLY,never automatic END
}
// PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE_CONTROL_END
#endif
#ifdef PIONEER_Q38_PROJECTION_VALIDATION_NATIVE
// PIONEER_Q38_PROJECTION_VALIDATION_NATIVE_CONTROL_BEGIN
static status_t Q38ProjectionValidationControl(PioneerProbeCookie* cookie,uint32 operation,void* data,size_t length)
{
	if (!cookie || cookie->magic!=0x50313030 || !data) return B_BAD_VALUE;
	thread_info thread={};status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK || thread.team!=cookie->team) return status!=B_OK?status:B_NOT_ALLOWED;
	if (operation==TT_PCI_PROBE_Q38_PROJECTION_VALIDATION_SUPPORT) {
		if (length!=sizeof(tt_q38_embedding_support)) return B_BAD_VALUE;
		status=AcquireQ38GdnSessionLock(cookie);if (status!=B_OK) return status;
		const tt_q38_embedding_support support={1,55,13,16480,248320,512,0,0};
		status=user_memcpy(data,&support,sizeof(support));release_sem(sLock);return status;
	}
	if (operation!=TT_PCI_PROBE_Q38_PROJECTION_VALIDATE || length!=sizeof(tt_q38_projection_validation)) return B_BAD_VALUE;
	auto* reply=new(std::nothrow) tt_q38_projection_validation;
	if (!reply) return B_NO_MEMORY;
	status=user_memcpy(reply,data,sizeof(*reply));
	bool shape=status==B_OK && PioneerQ38EmbeddingRequest(reply->metadata);
	if (shape) {
		const auto* bytes=reinterpret_cast<const uint8*>(reply);
		for (unsigned i=sizeof(reply->metadata);i<sizeof(*reply);i++) if (bytes[i]) {shape=false;break;}
	}
	if (!shape) {delete reply;return status!=B_OK?status:B_BAD_VALUE;}
	PioneerQ38GdnCallScope scope(cookie);auto* c=scope.Context();
	if (!c || !BindQ38EmbeddingModel(*c,reply->metadata.token,reply->metadata.position,0)) {
		delete reply;return scope.Status()!=B_OK?scope.Status():B_NOT_ALLOWED;
	}
	PioneerQ38ProjectionZeroResult zero;PioneerQ38EmbeddingResult embedding;PioneerQ38LearnedRmsResult norm;
	PioneerQ38ProjectionGraphResult projection;
	uint64 began=uint64(system_time());
	bool okay=InitializeQ38EmbeddingModel(*c,zero) && RunQ38EmbeddingModel(*c,embedding) && RunQ38LearnedRmsModel(*c,norm)
		&& PrepareQ38ProjectionExecution(*c,projection);
	const unsigned roles[]={10,11,5,6};
	for (unsigned role:roles) if (okay) okay=RunQ38ProjectionExecution(*c,role,projection);
	if (okay) okay=CloseQ38ProjectionExecution(*c,projection,reply->values);
	uint64 ended=uint64(system_time());auto* m=c->q38EmbeddingModel;
	okay=okay && ended>=began && m && m->Closed() && m->embedding.Completed() && m->rms.Completed() && !c->Unsafe()
		&& projection.closed && !projection.unsafe && projection.runs==4 && projection.armed==119 && projection.released==119
		&& projection.stopped==119 && projection.held==119 && projection.rootStarts==5 && projection.rootHolds==5
		&& projection.memory.closed && projection.memory.savedWords==8134072 && projection.memory.verifiedOriginalWords==16268144
		&& projection.memory.installedWords==989304 && projection.memory.restoredWords==8134072
		&& projection.validationPages==18 && projection.validationWords==18432;
	auto& r=reply->metadata;r.state=okay?TT_RUNTIME_PASSED:TT_RUNTIME_COMPLETION_FAILED;
	r.closed=r.input_ready=okay?1:0;r.recovery_required=okay?0:1;
	r.session=m?m->identity.session:0;r.driver_generation=m?m->identity.driverGeneration:0;
	r.elapsed_us=okay?ended-began:0;r.source_pages=okay?23193:0;r.workspace_pages=okay?512:0;
	r.saved_words=okay?embedding.savedWords+norm.savedWords+projection.memory.savedWords:0;
	r.restored_words=okay?embedding.restoredWords+norm.restoredWords+projection.memory.restoredWords:0;
	if (okay) {
		reply->projections=projection.runs;reply->peer_boots=projection.released;reply->peer_stopped=projection.stopped;reply->peer_held=projection.held;
		reply->root_starts=projection.rootStarts;reply->root_holds=projection.rootHolds;
		reply->readback_pages=projection.validationPages;reply->readback_words=projection.validationWords;
		reply->saved_words=projection.memory.savedWords;reply->verified_words=projection.memory.verifiedOriginalWords;
		reply->installed_words=projection.memory.installedWords;reply->restored_words=projection.memory.restoredWords;
	} else {
		dprintf("Q38-PROJECTION-VALIDATION: FAIL zero%u embedding%u norm%u projection_phase%u worker%u word%u runs%u readback%u/%u saved%u restored%u\n",
			zero.phase,embedding.phase,norm.phase,projection.phase,projection.worker,projection.word,projection.runs,
			projection.validationPages,projection.validationWords,projection.memory.savedWords,projection.memory.restoredWords);
		memset(reinterpret_cast<uint8*>(reply)+sizeof(reply->metadata),0,sizeof(*reply)-sizeof(reply->metadata));
	}
	status=user_memcpy(data,reply,sizeof(*reply));delete reply;
	if (!okay || status!=B_OK || c->Unsafe()) {tt_resident_session_result retained={};RetainQ38DispatchOwner(*c,retained);}
	return status; // separate DEBUG client mismatch CLOSE ONLY;never a later END/device I/O
}
// PIONEER_Q38_PROJECTION_VALIDATION_NATIVE_CONTROL_END
#endif
#ifdef PIONEER_Q38_EMBEDDING_QUALIFICATION
// PIONEER_Q38_EMBEDDING_NATIVE_CONTROL_BEGIN
#ifdef PIONEER_Q38_LEARNED_RMS_VALIDATION_NATIVE
static status_t Q38EmbeddingControl(PioneerProbeCookie* cookie,uint32 operation,void* data,size_t length)
{
	if (!cookie || cookie->magic!=0x50313030 || !data) return B_BAD_VALUE;
	thread_info thread={};status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK || thread.team!=cookie->team) return status!=B_OK?status:B_NOT_ALLOWED;
	if (operation==TT_PCI_PROBE_Q38_RMS_VALIDATION_SUPPORT) {
		if (length!=sizeof(tt_q38_embedding_support)) return B_BAD_VALUE;
		status=AcquireQ38GdnSessionLock(cookie);if (status!=B_OK) return status;
		const tt_q38_embedding_support support={1,55,12,5120,248320,512,0,0};
		status=user_memcpy(data,&support,sizeof(support));release_sem(sLock);return status;
	}
	if (operation!=TT_PCI_PROBE_Q38_RMS_VALIDATE || length!=sizeof(tt_q38_learned_rms_validation)) return B_BAD_VALUE;
	auto* reply=new(std::nothrow) tt_q38_learned_rms_validation;
	if (!reply) return B_NO_MEMORY;
	status=user_memcpy(reply,data,sizeof(*reply));
	bool shape=status==B_OK && PioneerQ38EmbeddingRequest(reply->metadata);
	if (shape) for (unsigned i=0;i<5120;i++) if (reply->normalized[i] || reply->residual[i]) {shape=false;break;}
	if (!shape) {delete reply;return status!=B_OK?status:B_BAD_VALUE;}
	PioneerQ38GdnCallScope scope(cookie);auto* c=scope.Context();
	if (!c || !BindQ38EmbeddingModel(*c,reply->metadata.token,reply->metadata.position,0)) {
		delete reply;return scope.Status()!=B_OK?scope.Status():B_NOT_ALLOWED;
	}
	PioneerQ38ProjectionZeroResult zero;PioneerQ38EmbeddingResult embedding;PioneerQ38LearnedRmsResult norm;
	uint64 began=uint64(system_time());
	bool okay=InitializeQ38EmbeddingModel(*c,zero) && RunQ38EmbeddingModel(*c,embedding)
		&& RunQ38LearnedRmsModel(*c,norm,reply->normalized,reply->residual);
	uint64 ended=uint64(system_time());auto* m=c->q38EmbeddingModel;
	okay=okay && ended>=began && m && m->Closed() && m->embedding.Completed() && m->rms.Completed() && !c->Unsafe()
		&& norm.validationPages==10 && norm.validationWords==10240;
	auto& r=reply->metadata;r.state=okay?TT_RUNTIME_PASSED:TT_RUNTIME_COMPLETION_FAILED;
	r.closed=r.input_ready=okay?1:0;r.recovery_required=okay?0:1;
	r.session=m?m->identity.session:0;r.driver_generation=m?m->identity.driverGeneration:0;
	r.elapsed_us=okay?ended-began:0;r.source_pages=okay?17:0;r.workspace_pages=okay?512:0;
	r.saved_words=okay?embedding.savedWords+norm.savedWords:0;r.restored_words=okay?embedding.restoredWords+norm.restoredWords:0;
	if (!okay) {
		dprintf("Q38-RMS-VALIDATION: FAIL zero_phase%u embedding_phase%u norm_phase%u word%u readback_pages%u words%u saved%u restored%u\n",
			zero.phase,embedding.phase,norm.phase,norm.word,norm.validationPages,norm.validationWords,norm.savedWords,norm.restoredWords);
		memset(reply->normalized,0,sizeof(reply->normalized));memset(reply->residual,0,sizeof(reply->residual));
	}
	status=user_memcpy(data,reply,sizeof(*reply));delete reply;
	if (!okay || status!=B_OK || c->Unsafe()) {tt_resident_session_result retained={};RetainQ38DispatchOwner(*c,retained);}
	return status; // CPU mismatch in separate DEBUG client must CLOSE ONLY,no later END/device I/O
}
#elif defined(PIONEER_Q38_EMBEDDING_VALIDATION_NATIVE)
static status_t Q38EmbeddingControl(PioneerProbeCookie* cookie,uint32 operation,void* data,size_t length)
{
	if (!cookie || cookie->magic!=0x50313030 || !data) return B_BAD_VALUE;
	thread_info thread={};status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK || thread.team!=cookie->team) return status!=B_OK?status:B_NOT_ALLOWED;
	if (operation==TT_PCI_PROBE_Q38_EMBEDDING_VALIDATION_SUPPORT) {
		if (length!=sizeof(tt_q38_embedding_support)) return B_BAD_VALUE;
		status=AcquireQ38GdnSessionLock(cookie);if (status!=B_OK) return status;
		const tt_q38_embedding_support support={1,55,11,5120,248320,512,0,0};
		status=user_memcpy(data,&support,sizeof(support));release_sem(sLock);return status;
	}
	if (operation!=TT_PCI_PROBE_Q38_EMBEDDING_VALIDATE || length!=sizeof(tt_q38_embedding_validation)) return B_BAD_VALUE;
	auto* reply=new(std::nothrow) tt_q38_embedding_validation;
	if (!reply) return B_NO_MEMORY;
	status=user_memcpy(reply,data,sizeof(*reply));
	bool shape=status==B_OK && PioneerQ38EmbeddingRequest(reply->metadata);
	if (shape) for (unsigned i=0;i<5120;i++) if (reply->values[i]) {shape=false;break;}
	if (!shape) {delete reply;return status!=B_OK?status:B_BAD_VALUE;}
	PioneerQ38GdnCallScope scope(cookie);auto* c=scope.Context();
	if (!c || !BindQ38EmbeddingModel(*c,reply->metadata.token,reply->metadata.position,0)) {
		delete reply;return scope.Status()!=B_OK?scope.Status():B_NOT_ALLOWED;
	}
	PioneerQ38ProjectionZeroResult zero;PioneerQ38EmbeddingResult result;
	uint64 began=uint64(system_time());
	bool okay=InitializeQ38EmbeddingModel(*c,zero) && RunQ38EmbeddingModel(*c,result,reply->values);
	uint64 ended=uint64(system_time());auto* m=c->q38EmbeddingModel;
	okay=okay && ended>=began && m && m->Closed() && m->embedding.Completed() && !c->Unsafe()
		&& result.validationPages==5 && result.validationWords==5120;
	auto& r=reply->metadata;r.state=okay?TT_RUNTIME_PASSED:TT_RUNTIME_COMPLETION_FAILED;
	r.closed=r.input_ready=okay?1:0;r.recovery_required=okay?0:1;
	r.session=m?m->identity.session:0;r.driver_generation=m?m->identity.driverGeneration:0;
	r.elapsed_us=okay?ended-began:0;r.source_pages=okay?12:0;r.workspace_pages=okay?512:0;
	r.saved_words=okay?result.savedWords:0;r.restored_words=okay?result.restoredWords:0;
	if (!okay) {
		// CPU-side captured diagnostics ONLY;do not read/stop/restore a failed device.
		dprintf("Q38-EMBEDDING-VALIDATION: FAIL zero_phase%u zero_state%u embedding_phase%u word%u readback_pages%u words%u saved%u restored%u\n",
			zero.phase,zero.state,result.phase,result.word,result.validationPages,result.validationWords,result.savedWords,result.restoredWords);
		memset(reply->values,0,sizeof(reply->values)); // never publish a partial/unrestored numeric result
	}
	status=user_memcpy(data,reply,sizeof(*reply));delete reply;
	if (!okay || status!=B_OK || c->Unsafe()) {tt_resident_session_result retained={};RetainQ38DispatchOwner(*c,retained);}
	return status; // DEBUG client mismatch must CLOSE ONLY,not explicit END
}
#else
static status_t Q38EmbeddingControl(PioneerProbeCookie* cookie,uint32 operation,void* data,size_t length)
{
	if (!cookie || cookie->magic!=0x50313030 || !data) return B_BAD_VALUE;
	thread_info thread={};status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK || thread.team!=cookie->team) return status!=B_OK?status:B_NOT_ALLOWED;
	if (operation==TT_PCI_PROBE_Q38_EMBEDDING_SUPPORT) {
		if (length!=sizeof(tt_q38_embedding_support)) return B_BAD_VALUE;
		status=AcquireQ38GdnSessionLock(cookie);if (status!=B_OK) return status;
		const tt_q38_embedding_support support={1,55,10,5120,248320,512,1,0};
		status=user_memcpy(data,&support,sizeof(support));release_sem(sLock);return status;
	}
	if (operation!=TT_PCI_PROBE_Q38_EMBEDDING_PRODUCE || length!=sizeof(tt_q38_embedding_produce)) return B_BAD_VALUE;
	tt_q38_embedding_produce request={};status=user_memcpy(&request,data,sizeof(request));
	if (status!=B_OK || !PioneerQ38EmbeddingRequest(request)) return status!=B_OK?status:B_BAD_VALUE;
	PioneerQ38GdnCallScope scope(cookie);auto* c=scope.Context();
	if (!c || !BindQ38EmbeddingModel(*c,request.token,request.position,0)) return scope.Status()!=B_OK?scope.Status():B_NOT_ALLOWED;
	PioneerQ38ProjectionZeroResult zero;PioneerQ38EmbeddingResult result;
	uint64 began=uint64(system_time());
	bool okay=InitializeQ38EmbeddingModel(*c,zero) && RunQ38EmbeddingModel(*c,result);
	uint64 ended=uint64(system_time());auto* m=c->q38EmbeddingModel;
	okay=okay && ended>=began && m && m->Closed() && m->embedding.Completed() && !c->Unsafe();
	request.state=okay?TT_RUNTIME_PASSED:TT_RUNTIME_COMPLETION_FAILED;
	request.closed=request.input_ready=okay?1:0;request.recovery_required=okay?0:1;
	request.session=m?m->identity.session:0;request.driver_generation=m?m->identity.driverGeneration:0;
	request.elapsed_us=okay?ended-began:0; // initialization+execution+restore,NOT model inference time
	request.source_pages=okay?12:0;request.workspace_pages=okay?512:0;
	request.saved_words=okay?result.savedWords:0;request.restored_words=okay?result.restoredWords:0;
	status=user_memcpy(data,&request,sizeof(request));
	if (!okay || status!=B_OK || c->Unsafe()) {tt_resident_session_result retained={};RetainQ38DispatchOwner(*c,retained);}
	return status; // explicit BANK_END still mandatory;close alone retains context
}
#endif
// PIONEER_Q38_EMBEDDING_NATIVE_CONTROL_END
#endif
#ifdef PIONEER_Q38_FFN_NATIVE_GRAPH
// PIONEER_Q38_FFN_GRAPH_NATIVE_SCOPE_BEGIN
bool PioneerSessionIO::FfnGraphScope(const PioneerQ38FfnPersistentGraphOwner& owner,
	const PioneerQ38GraphMutableBanks& graph,const PioneerQ38CanonicalModelPlan& plan,
	const PioneerQ38FfnIdentity& identity,const uint8_t* learned,unsigned pages)
{
	auto* c=q38MutableContext;
	if (!c || &c->io!=this || sColdRecoveryRequired || sReloadQuiesced || !sReloadGeneration || !sRuntimeOwner
		|| sRuntimeOwner->magic!=0x50313030 || sRuntimeOwner->context!=c || !sBankStorageOnly || !c->bankStorageOnly
		|| c->bytes!=PioneerResidentBanks::Total || c->runtimeCoreMode!=1 || !fleetEnabled || !runtimeMapped || q38SpanAllowed
		|| c->q38GdnCallThread<0 || c->q38GdnCallThread!=find_thread(NULL) || c->dispatchOwnerRetained
		|| c->q38FfnGraph!=&owner || c->q38GraphMutable!=&graph || c->q38FfnModelPlan!=&plan
		|| !(identity==c->q38FfnModelIdentity) || identity.driverGeneration!=sReloadGeneration
		|| !learned || learned!=c->initialized || learned!=c->q38FfnModelLedger || pages!=PioneerResidentBanks::Pages
		|| !c->hashes || !c->q38Dispatch || !c->q38DispatchRoutes || !c->q38FfnCode || !c->q38FfnZero
		|| c->q38GdnBanks || c->q38StateZero || c->q38GdnIdentity.Valid() || c->q38Cohort || c->fleet || sQ38LocalDeltaOwner
		|| c->q38Delta.saved || c->q38Delta.dirty || c->q38Delta.ready || c->q38Delta.running || c->q38Delta.awaiting
		|| c->panelProbe.saved || c->panelProbe.dirty || c->panelProbe.ready
		|| c->streamProbe.saved || c->streamProbe.dirty || c->streamProbe.ready
		|| !c->q38Span || !c->q38Span->closed || c->q38Span->dirty || c->q38Span->ready || c->q38Span->running || c->q38Span->unsafe
		|| !c->lease.saved || !c->lease.ready || c->lease.pending || c->lease.recovery_required
		|| !c->bulk.saved || !c->bulk.ready || c->bulk.dirty || c->bulk.unsafe
		|| c->host.state!=TT_HOST_PAGE_READY || c->host.released || PioneerHostPageIO::quarantined
		|| !c->bulkHost.allocated || !c->bulkHost.ready || c->bulkHost.pending || c->bulkHost.quarantined || c->bulkHost.released
		|| !c->wideHost.allocated || !c->wideHost.ready || !c->wideHost.memory_type_set
		|| c->wideHost.pending || c->wideHost.quarantined || c->wideHost.released || wideQuarantined
		|| !FfnGraphCodeMappingStable(runtimeCore)) return false;
	auto* ffn=c->q38GraphMutable->Ffn(identity);const auto& zero=*c->q38FfnZero;
	if (!ffn || zero.owner!=ffn || zero.banks!=&c->banks || zero.proof!=&dram || !(zero.identity==identity)
		|| !zero.saved || !zero.closed || zero.nonce!=1 || zero.ready || zero.running || zero.unsafe || zero.dirty
		|| !owner.MatchesNativeOwner(dram,c->banks,*c->q38Dispatch,*c->q38FfnCode,graph,plan,identity,learned,pages)) return false;
	const auto& clock=c->q38Clock.Snapshot();
	return c->q38Clock.RequiresExplicitEnd() && clock.state==TT_ARC_CLOCK_BUSY_VERIFIED
		&& clock.busy_requested && clock.busy_acknowledged && !clock.restored && !c->Unsafe(false,true);
}
void PioneerSessionIO::FfnGraphRetainHostOwners()
{
	// MEMORY ONLY;called after the first observed failure. No destructor,
	// ARC,STOP,END,reset,unmap or retry is permitted here.
	if (q38MutableContext) { tt_resident_session_result retained={}; RetainQ38DispatchOwner(*q38MutableContext,retained); }
}
// PIONEER_Q38_FFN_GRAPH_NATIVE_SCOPE_END
#ifdef PIONEER_Q38_PROJECTION_GRAPH_MEMORY
// PIONEER_Q38_PROJECTION_MEMORY_NATIVE_SCOPE_BEGIN
bool PioneerSessionIO::ProjectionGraphMemoryScope(const PioneerQ38ProjectionGraphMemory& owner,
	const PioneerQ38CanonicalModelPlan& plan,const PioneerQ38GdnIdentity& identity,const uint8_t* learned,unsigned pages)
{
	auto* c=q38MutableContext;
	if (!c || &c->io!=this || sColdRecoveryRequired || sReloadQuiesced || !sReloadGeneration || !sRuntimeOwner
		|| sRuntimeOwner->magic!=0x50313030 || sRuntimeOwner->context!=c || !sBankStorageOnly || !c->bankStorageOnly
		|| c->bytes!=PioneerResidentBanks::Total || c->runtimeCoreMode!=1 || !fleetEnabled || !runtimeMapped || q38SpanAllowed
		|| c->q38GdnCallThread<0 || c->q38GdnCallThread!=find_thread(NULL) || c->dispatchOwnerRetained
		|| c->q38ProjectionMemory!=&owner || c->q38FfnModelPlan!=&plan || !c->q38GraphMutable || !c->q38FfnZero
		|| c->q38FfnGraph || c->q38FfnCode || !(identity==c->q38FfnModelIdentity) || identity.driverGeneration!=sReloadGeneration
		|| !learned || learned!=c->initialized || learned!=c->q38FfnModelLedger || pages!=PioneerResidentBanks::Pages
		|| !c->hashes || !c->q38Dispatch || !c->q38DispatchRoutes || c->q38GdnBanks || c->q38StateZero
		|| c->q38GdnIdentity.Valid() || c->q38Cohort || c->fleet || sQ38LocalDeltaOwner
		|| c->q38Delta.saved || c->q38Delta.dirty || c->q38Delta.ready || c->q38Delta.running || c->q38Delta.awaiting
		|| c->panelProbe.saved || c->panelProbe.dirty || c->panelProbe.ready
		|| c->streamProbe.saved || c->streamProbe.dirty || c->streamProbe.ready
		|| !c->q38Span || !c->q38Span->closed || c->q38Span->dirty || c->q38Span->ready || c->q38Span->running || c->q38Span->unsafe
		|| !c->lease.saved || !c->lease.ready || c->lease.pending || c->lease.recovery_required
		|| !c->bulk.saved || !c->bulk.ready || c->bulk.dirty || c->bulk.unsafe
		|| c->host.state!=TT_HOST_PAGE_READY || c->host.released || PioneerHostPageIO::quarantined
		|| !c->bulkHost.allocated || !c->bulkHost.ready || c->bulkHost.pending || c->bulkHost.quarantined || c->bulkHost.released
		|| !c->wideHost.allocated || !c->wideHost.ready || !c->wideHost.memory_type_set
		|| c->wideHost.pending || c->wideHost.quarantined || c->wideHost.released || wideQuarantined
		|| !ProjectionGraphMemoryMappingStable(runtimeCore)) return false;
	const auto& graph=*c->q38GraphMutable;const auto& zero=*c->q38FfnZero;auto* ffn=c->q38GraphMutable->Ffn(identity);
	if (graph.CanonicalModel(identity)!=&plan || !graph.MatchesInitializationOwner(c->banks,dram,identity)
		|| !ffn || !ffn->InitializationPublished(identity) || zero.owner!=ffn || zero.banks!=&c->banks || zero.proof!=&dram || !(zero.identity==identity)
		|| !zero.saved || !zero.closed || zero.nonce!=1 || zero.ready || zero.running || zero.unsafe || zero.dirty
		|| !owner.MatchesNativeOwner(dram,c->banks,*c->q38Dispatch,plan,identity,learned,pages)) return false;
	const auto& clock=c->q38Clock.Snapshot();
	// OLD FFN zero is a retained SOURCE/MODEL binding ONLY here. It does
	// NOT initialize the NEW >=2MiB generic workspace or grant a producer.
	return c->q38Clock.RequiresExplicitEnd() && clock.state==TT_ARC_CLOCK_BUSY_VERIFIED
		&& clock.busy_requested && clock.busy_acknowledged && !clock.restored && !c->Unsafe(false,false,true);
}
void PioneerSessionIO::ProjectionGraphRetainHostOwners()
{
	if (q38MutableContext) { tt_resident_session_result retained={}; RetainQ38DispatchOwner(*q38MutableContext,retained); }
}
// PIONEER_Q38_PROJECTION_MEMORY_NATIVE_SCOPE_END
#endif
// PIONEER_Q38_FFN_GRAPH_NATIVE_PREPARE_BEGIN
static bool PrepareQ38FfnGraphHeld(PioneerStoreContext& c)
{
	// SAME native source/model/zero and call scope,never legacy bootstrap.
	if (!c.io.StateZeroBeforeCohort() || c.Unsafe() || c.q38Dispatch || c.q38FfnGraph || c.q38FfnCode) return false;
	auto& dram=c.baseline.baseline.baseline.baseline.dram;auto& noc=c.baseline.baseline.baseline.noc;
	c.q38Dispatch=new(std::nothrow) PioneerQ38DispatchLease;
	if (!c.q38Dispatch) return false; // no new IO yet
	auto& lease=*c.q38Dispatch;lease.worker[0].master=&c.lease;lease.worker[0].noc=noc;
	c.io.fleetEnabled=true;
	bool okay=c.io.SelectRuntimeCore(0) && c.lease.ready && c.io.MapRuntime() && BlackholeDramAllCoresHeld(c.io,dram);
	for (unsigned w=1;w<120 && okay;w++) okay=PioneerWorkerNocProbe(c.io,dram,w,lease.worker[w].noc);
	c.q38DispatchRoutes=okay;
	for (unsigned w=0;w<120 && okay;w++) {
		auto& node=lease.worker[w];okay=c.io.SelectRuntimeCore(w) && c.io.MapQ38Panel();
		if (okay && w) okay=PioneerQ38CohortPeerLeaseBegin(c.io,dram,w,node);
		if (okay) okay=PioneerQ38DispatchPrepareRole(c.io,dram,c.banks,lease,w);
		if (!okay) lease.prepared=w+1;
	}
	if (okay) {
		// Publish BOTH owners before mapping/install. On any failure both
		// remain owned by this retained context,not a local destructor.
		c.q38FfnCode=new(std::nothrow) PioneerQ38FfnGraphCodeOwner;
		c.q38FfnGraph=new(std::nothrow) PioneerQ38FfnPersistentGraphOwner;
		okay=c.q38FfnCode && c.q38FfnGraph && c.io.SelectRuntimeCore(0) && c.io.MapFfnGraphCodePrefix();
	}
	if (!okay) { tt_resident_session_result retained={}; RetainQ38DispatchOwner(c,retained); }
	return okay;
}
// PIONEER_Q38_FFN_GRAPH_NATIVE_PREPARE_END
// PIONEER_Q38_FFN_GRAPH_NATIVE_CONTROL_BEGIN
static status_t Q38FfnGraphControl(PioneerProbeCookie* cookie,uint32 operation,void* data,size_t length)
{
	if (!cookie || cookie->magic!=0x50313030 || !data) return B_BAD_VALUE;
	thread_info thread={};status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK || thread.team!=cookie->team) return status!=B_OK?status:B_NOT_ALLOWED;
	if (operation==TT_PCI_PROBE_Q38_FFN_GRAPH_SUPPORT) {
		if (length!=sizeof(tt_q38_ffn_graph_support)) return B_BAD_VALUE;
		status=AcquireQ38GdnSessionLock(cookie);if (status!=B_OK) return status;
		const tt_q38_ffn_graph_support support={1,55,9,119,5120,5120,2,0};
		status=user_memcpy(data,&support,sizeof(support));release_sem(sLock);return status;
	}
	if (operation!=TT_PCI_PROBE_Q38_FFN_GRAPH_RUN || length!=sizeof(tt_q38_ffn_graph_run)) return B_BAD_VALUE;
	auto* request=(tt_q38_ffn_graph_run*)malloc(sizeof(tt_q38_ffn_graph_run));if (!request) return B_NO_MEMORY;
	status=user_memcpy(request,data,sizeof(*request));
	bool valid=status==B_OK && PioneerQ38FfnGraphRequest(*request);
	if (!valid) { free(request);return status!=B_OK?status:B_BAD_VALUE; }
	PioneerQ38GdnCallScope scope(cookie);
	auto* c=scope.Context();
	if (!c || !c->q38FfnModelPlan || !c->q38GraphMutable || !c->q38FfnZero
		|| c->q38FfnModelLayer!=request->layer || c->q38FfnGraph || c->q38FfnCode || c->q38Dispatch
		|| c->q38FfnGraphJobs[0] || c->q38FfnGraphJobs[1]) {
		free(request);return scope.Status()!=B_OK?scope.Status():B_NOT_ALLOWED;
	}
	bool okay=true;
	for (unsigned pass=0;pass<request->runs && okay;pass++) {
		c->q38FfnGraphJobs[pass]=new(std::nothrow) PioneerQ38FfnCanonicalJobs;
		okay=c->q38FfnGraphJobs[pass] && c->q38FfnGraphJobs[pass]->Build(*c->q38FfnModelPlan,request->layer,pass)
			&& c->q38GraphMutable->MatchInitializedFfn(c->q38FfnModelIdentity,*c->q38FfnGraphJobs[pass],request->layer,pass,
				c->initialized,PioneerResidentBanks::Pages);
	}
	PioneerQ38FfnPersistentGraphResult result;
	okay=okay && PrepareQ38FfnGraphHeld(*c);
	if (okay) okay=c->q38FfnGraph->Prepare(c->io,c->baseline.baseline.baseline.baseline.dram,c->banks,*c->q38Dispatch,
		*c->q38FfnCode,*c->q38GraphMutable,*c->q38FfnModelPlan,c->q38FfnModelIdentity,c->initialized,PioneerResidentBanks::Pages,
		*c->q38FfnGraphJobs[0],request->layer,0,result);
	for (unsigned pass=0;pass<request->runs && okay;pass++) {
		uint64_t began=c->io.Q38CohortNow();
		okay=c->q38FfnGraph->Run(c->io,*c->q38FfnGraphJobs[pass],request->layer,pass,request->input[pass],result)
			&& c->q38FfnGraph->CopyOutput(c->io,request->output[pass],result);
		// This clock is CPU monotonic,NOT a new card read after failure.
		uint64_t ended=c->io.Q38CohortNow();if (okay && ended>=began) request->elapsed_us[pass]=ended-began;else okay=false;
#ifdef PIONEER_Q38_FFN_HOST_PROFILE
		if (okay) {
			const auto& p=result.hostProfile;
			dprintf("P100A_FFN_HOST_PROFILE layer%u pass%u valid%u wall_us%llu sum_us%llu preflight%llu input_guard%llu input_stage%llu manifest%llu root_poll%llu root_proof%llu root_hold%llu output%llu polls%u root_reads%u root_writes%u;CPU_MONOTONIC_ROOT_POLL_INCLUDES_HOST_GUARDS_NOT_KERNEL_ONLY\n",
				request->layer,pass,unsigned(p.Complete()),(unsigned long long)request->elapsed_us[pass],(unsigned long long)p.Total(),
				(unsigned long long)p.us[Q38_FFN_PREFLIGHT],(unsigned long long)p.us[Q38_FFN_INPUT_GUARD],
				(unsigned long long)p.us[Q38_FFN_INPUT_STAGE],(unsigned long long)p.us[Q38_FFN_MANIFEST],
				(unsigned long long)p.us[Q38_FFN_ROOT_POLL],(unsigned long long)p.us[Q38_FFN_ROOT_PROOF],
				(unsigned long long)p.us[Q38_FFN_ROOT_HOLD],(unsigned long long)p.us[Q38_FFN_OUTPUT],
				p.polls,result.rootReads,result.rootWrites);
		}
#endif
	}
	if (okay) okay=c->q38FfnGraph->Close(c->io,result) && !c->Unsafe() && c->io.SelectRuntimeCore(0);
	request->state=okay?TT_RUNTIME_PASSED:TT_RUNTIME_COMPLETION_FAILED;request->closed=okay?1:0;request->recovery_required=okay?0:1;
	request->session=c->q38FfnModelIdentity.session;request->driver_generation=c->q38FfnModelIdentity.driverGeneration;
	request->installed_words=result.installedWords;request->restored_words=result.restoredWords;
	request->peer_boots=result.released;request->root_starts=result.rootStarts;
	if (!okay) {
		tt_resident_session_result retained={};RetainQ38DispatchOwner(*c,retained);
		for (auto& row:request->output) for (auto& value:row) value=0;
	}
	status=user_memcpy(data,request,sizeof(*request));
	if (status!=B_OK) { tt_resident_session_result retained={};RetainQ38DispatchOwner(*c,retained); }
	free(request);return status;
}
// PIONEER_Q38_FFN_GRAPH_NATIVE_CONTROL_END
#endif
#ifdef PIONEER_Q38_FFN_MODEL_INIT
// PIONEER_Q38_FFN_MODEL_NATIVE_BIND_BEGIN
// Exact native context,not a disconnected layout helper. Allocations are
// HEAP ONLY. Publish the whole graph before its atomic mutable claim;
// after publication every refusal retains ALL owners/clock/cookie.
static uint64_t sQ38FfnModelEpoch=0; // global lock;never wrap/reuse
static bool BindQ38FfnModel(PioneerStoreContext& c,unsigned layer)
{
	if (sColdRecoveryRequired || sReloadQuiesced || !sRuntimeOwner || sRuntimeOwner->magic!=0x50313030
		|| sRuntimeOwner->context!=&c || !sBankStorageOnly || !c.bankStorageOnly || c.bytes!=PioneerResidentBanks::Total
		|| c.io.q38MutableContext!=&c || c.q38GdnCallThread<0 || c.q38GdnCallThread!=find_thread(NULL)
		|| c.runtimeCoreMode!=-1 || c.io.runtimeCore || c.io.fleetEnabled || !c.io.q38SpanAllowed || c.Unsafe()
		|| !c.initialized || !c.hashes || layer>=64 || sQ38FfnModelEpoch==UINT64_MAX
		|| c.q38FfnModelPlan || c.q38GraphMutable || c.q38FfnZero || c.q38FfnModelLedger
		|| c.q38FfnModelIdentity.session || c.q38FfnModelIdentity.sequence || c.q38FfnModelIdentity.modelEpoch
		|| c.q38FfnModelIdentity.driverGeneration || c.q38FfnModelLayer!=64
		|| c.q38GdnBanks || c.q38StateZero || c.q38GdnIdentity.session || c.q38GdnIdentity.sequence
		|| c.q38GdnIdentity.modelEpoch || c.q38GdnIdentity.driverGeneration
		|| c.q38Dispatch || c.q38Cohort || c.fleet || sQ38LocalDeltaOwner
		|| c.q38Delta.saved || c.q38Delta.dirty || c.q38Delta.ready || c.q38Delta.running || c.q38Delta.awaiting
		|| c.panelProbe.saved || c.panelProbe.dirty || c.panelProbe.ready
		|| c.streamProbe.saved || c.streamProbe.dirty || c.streamProbe.ready
		|| !c.q38Span || !c.q38Span->closed || c.q38Span->dirty || c.q38Span->ready || c.q38Span->running || c.q38Span->unsafe
		|| !c.lease.saved || !c.lease.ready || c.lease.pending || c.lease.recovery_required
		|| !c.bulk.saved || !c.bulk.ready || c.bulk.dirty || c.bulk.unsafe
		|| !c.bulkHost.allocated || !c.bulkHost.ready || c.bulkHost.pending || c.bulkHost.quarantined || c.bulkHost.released
		|| !c.wideHost.allocated || !c.wideHost.ready || !c.wideHost.memory_type_set
		|| c.wideHost.pending || c.wideHost.quarantined || c.wideHost.released) return false;
	const auto& clock=c.q38Clock.Snapshot();
	if (!c.q38Clock.RequiresExplicitEnd() || clock.state!=TT_ARC_CLOCK_BUSY_VERIFIED
		|| !clock.busy_requested || !clock.busy_acknowledged || clock.restored) return false;
	auto* plan=new(std::nothrow) PioneerQ38CanonicalModelPlan;
	auto* graph=new(std::nothrow) PioneerQ38GraphMutableBanks;
	auto* zero=new(std::nothrow) PioneerQ38FfnZeroLease;
	if (!plan || !graph || !zero || !plan->Build() || !plan->FfnPagesPresent(layer,c.initialized,PioneerResidentBanks::Pages)) {
		delete zero; delete graph; delete plan; return false; // no claim/device IO
	}
	// The model epoch denotes this fixed layout binding,NOT a caller's SHA
	// or kernel authentication. Genuine host SAME-FILE provenance remains
	// mandatory on BOTH sides of this ioctl in the source-bound client.
	c.q38FfnModelIdentity={++sQ38FfnModelEpoch,1,1,sReloadGeneration};
	c.q38FfnModelPlan=plan; c.q38GraphMutable=graph; c.q38FfnZero=zero;
	c.q38FfnModelLedger=c.initialized; c.q38FfnModelLayer=layer;
	c.runtimeCoreMode=1; c.io.q38SpanAllowed=false;
	bool okay=graph->BindCanonical(c.banks,c.baseline.baseline.baseline.baseline.dram,c.q38FfnModelIdentity,
		c.initialized,PioneerResidentBanks::Pages,*plan,0);
	if (!okay) { tt_resident_session_result r={}; RetainQ38DispatchOwner(c,r); }
	return okay;
}
// PIONEER_Q38_FFN_MODEL_NATIVE_BIND_END
// PIONEER_Q38_FFN_MODEL_NATIVE_INITIALIZE_BEGIN
static bool InitializeQ38FfnModel(PioneerStoreContext& c,PioneerQ38FfnZeroResult& result)
{
	if (!c.io.StateZeroBeforeCohort()) return false;
	auto* ffn=c.q38GraphMutable->Ffn(c.q38FfnModelIdentity);
	if (!ffn) return false;
	auto& dram=c.baseline.baseline.baseline.baseline.dram;
	auto& noc=c.baseline.baseline.baseline.noc;
	auto& zero=*c.q38FfnZero;
	bool okay=PioneerQ38FfnZeroPrepare(c.io,dram,noc,*ffn,c.banks,c.q38FfnModelIdentity,c.host,
		c.lease,c.bulk,c.bulkHost,c.wideHost,zero)
		&& PioneerQ38FfnZeroRun(c.io,dram,noc,*ffn,c.banks,c.host,c.lease,c.bulk,c.bulkHost,c.wideHost,
			zero,c.q38FfnModelIdentity,0,0,PioneerQ38FfnBanks::Pages,result)
		&& result.state==TT_RUNTIME_PASSED && result.pages==74 && result.bytes==303104
		&& result.nonce==1 && result.started==1 && result.completed==1 && result.held==1 && !result.recovery_required
		&& PioneerQ38FfnZeroClose(c.io,dram,noc,*ffn,c.banks,c.host,c.lease,c.bulk,c.bulkHost,c.wideHost,zero,true);
	// Actual jobs/learned bitmap and this SAME joint owner's known-clean
	// zero publication are checked together,not synthesized from a count.
	auto* jobs=okay?new(std::nothrow) PioneerQ38FfnCanonicalJobs:NULL;
	okay=okay && jobs && jobs->Build(*c.q38FfnModelPlan,c.q38FfnModelLayer,0)
		&& c.q38GraphMutable->MatchInitializedFfn(c.q38FfnModelIdentity,*jobs,c.q38FfnModelLayer,0,
			c.initialized,PioneerResidentBanks::Pages) && !c.Unsafe();
	delete jobs;
	if (!okay) { tt_resident_session_result r={}; RetainQ38DispatchOwner(c,r); }
	return okay;
}
// PIONEER_Q38_FFN_MODEL_NATIVE_INITIALIZE_END
// PIONEER_Q38_FFN_MODEL_NATIVE_CONTROL_BEGIN
static status_t Q38FfnModelControl(PioneerProbeCookie* cookie,uint32 operation,void* data,size_t length)
{
	if (!cookie || cookie->magic!=0x50313030 || !data) return B_BAD_VALUE;
	thread_info info={}; status_t status=get_thread_info(find_thread(NULL),&info);
	if (status!=B_OK) return status;
	if (info.team!=cookie->team) return B_NOT_ALLOWED;
	if (operation==TT_PCI_PROBE_Q38_FFN_MODEL_SUPPORT) {
		if (length!=sizeof(tt_q38_ffn_model_support)) return B_BAD_VALUE;
		status=AcquireQ38GdnSessionLock(cookie); if (status!=B_OK) return status;
		const tt_q38_ffn_model_support support={1,55,
#ifdef PIONEER_Q38_FFN_NATIVE_GRAPH
			9,
#else
			8,
#endif
			64,74,1,0,0};
		status=user_memcpy(data,&support,sizeof(support)); release_sem(sLock); return status;
	}
	if (operation!=TT_PCI_PROBE_Q38_FFN_MODEL_INITIALIZE || length!=sizeof(tt_q38_ffn_model_initialize)) return B_BAD_VALUE;
	tt_q38_ffn_model_initialize request={}; status=user_memcpy(&request,data,sizeof(request));
	if (status!=B_OK || request.version!=1 || request.layer>=64 || request.reserved[0] || request.reserved[1]
		|| request.state || request.pages || request.closed || request.initialization_published || request.recovery_required
		|| request.reserved_reply || request.session || request.driver_generation) return status!=B_OK?status:B_BAD_VALUE;
	PioneerQ38GdnCallScope scope(cookie); if (scope.Status()!=B_OK) return scope.Status();
	auto* c=scope.Context(); if (!c || !BindQ38FfnModel(*c,request.layer)) return B_NOT_ALLOWED;
	PioneerQ38FfnZeroResult result;
	bool okay=InitializeQ38FfnModel(*c,result);
	request.state=okay?TT_RUNTIME_PASSED:TT_RUNTIME_COMPLETION_FAILED;
	request.pages=okay?result.pages:0;
	request.closed=okay?1:0; request.initialization_published=okay?1:0;
	request.recovery_required=okay?0:1;
	request.session=c->q38FfnModelIdentity.session; request.driver_generation=c->q38FfnModelIdentity.driverGeneration;
	status=user_memcpy(data,&request,sizeof(request));
	if (!okay || status!=B_OK || c->Unsafe()) { tt_resident_session_result r={}; RetainQ38DispatchOwner(*c,r); }
	return status;
}
// PIONEER_Q38_FFN_MODEL_NATIVE_CONTROL_END
#endif
// PIONEER_Q38_GDN_BIND_OWNER_BEGIN
// Private CPU-only transition,fixed zero diagnostic ONLY. Caller
// holds the call-scope seal/global lock;no user coordinates or addresses.
// Fresh placement is a proposal;real Bind independently validates/claims
// every page. No initialization/model/compute readiness is published here.
[[maybe_unused]] static bool BindQ38MutableOwner(PioneerStoreContext& c,const PioneerQ38GdnIdentity& identity)
{
	if (sColdRecoveryRequired || sReloadQuiesced || !sRuntimeOwner || sRuntimeOwner->magic!=0x50313030
		|| sRuntimeOwner->context!=&c || !sBankStorageOnly || !c.bankStorageOnly || c.bytes!=PioneerResidentBanks::Total
		|| c.io.q38MutableContext!=&c || c.q38GdnCallThread<0 || c.q38GdnCallThread!=find_thread(NULL)
		|| c.runtimeCoreMode!=1 || c.io.runtimeCore || c.io.fleetEnabled || !c.initialized || c.Unsafe()
		|| !identity.Valid() || identity.driverGeneration!=sReloadGeneration || c.q38GdnBanks || c.q38StateZero
		|| c.q38GdnIdentity.session || c.q38GdnIdentity.sequence || c.q38GdnIdentity.modelEpoch || c.q38GdnIdentity.driverGeneration
		|| c.q38Dispatch || c.q38Cohort || c.fleet || sQ38LocalDeltaOwner
		|| c.q38Delta.saved || c.q38Delta.dirty || c.q38Delta.ready || c.q38Delta.running || c.q38Delta.awaiting
		|| c.panelProbe.saved || c.panelProbe.dirty || c.panelProbe.ready || c.streamProbe.saved || c.streamProbe.dirty || c.streamProbe.ready) return false;
	if (c.q38Span && (!c.q38Span->closed || c.q38Span->dirty || c.q38Span->ready || c.q38Span->running || c.q38Span->unsafe)) return false;
	const auto& clock=c.q38Clock.Snapshot();
	if (!c.q38Clock.RequiresExplicitEnd() || clock.state!=TT_ARC_CLOCK_BUSY_VERIFIED
		|| !clock.busy_requested || !clock.busy_acknowledged || clock.restored) return false;
	auto* plan=new(std::nothrow) PioneerQ38GdnFreshPlan;
	if (!plan) return false;
	if (!plan->Build(c.initialized,PioneerResidentBanks::Pages)) { delete plan; return false; }
	auto* owner=new(std::nothrow) PioneerQ38GdnBanks;
	auto* zero=new(std::nothrow) PioneerQ38StateZeroLease;
	if (!owner || !zero) { delete owner; delete zero; delete plan; return false; } // no claim/IO yet
	// Publish ALL heap owners before a claim can succeed. A failed/partial
	// claim then retains the same context rather than freeing a referenced
	// owner. Only temporary CPU proposal storage is disposable afterward.
	c.q38GdnBanks=owner; c.q38StateZero=zero; c.q38GdnIdentity=identity; c.io.q38SpanAllowed=false;
	bool okay=owner->Bind(c.banks,c.baseline.baseline.baseline.baseline.dram,identity,c.initialized,
		PioneerResidentBanks::Pages,plan->Regions(),plan->Count());
	delete plan;
	if (!okay) { tt_resident_session_result r={}; RetainQ38DispatchOwner(c,r); }
	return okay;
}
// PIONEER_Q38_GDN_BIND_OWNER_END
[[maybe_unused]] static bool InitializeQ38MutableState(PioneerStoreContext& c,PioneerQ38InitializeOwnedResult& result)
{
	if (!c.io.StateZeroBeforeCohort()) {
		if (c.Unsafe()) { tt_resident_session_result r={}; RetainQ38DispatchOwner(c,r); }
		return false;
	}
	bool okay=PioneerQ38InitializeOwnedState(c.io,c.baseline.baseline.baseline.baseline.dram,
		c.baseline.baseline.baseline.noc,*c.q38GdnBanks,c.banks,c.q38GdnIdentity,c.host,
		c.lease,c.bulk,c.bulkHost,c.wideHost,*c.q38StateZero,result);
	if (!okay && c.Unsafe()) { tt_resident_session_result r={}; RetainQ38DispatchOwner(c,r); }
	return okay;
}
#ifdef PIONEER_Q38_GDN_ZERO_DIAGNOSTIC
static bool QualifyQ38MutableState(PioneerStoreContext& c,PioneerQ38InitializeOwnedResult& result)
{
	if (!c.io.StateZeroBeforeCohort()) {
		if (c.Unsafe()) { tt_resident_session_result r={}; RetainQ38DispatchOwner(c,r); }
		return false;
	}
	bool okay=PioneerQ38QualifyOwnedState(c.io,c.baseline.baseline.baseline.baseline.dram,
		c.baseline.baseline.baseline.noc,*c.q38GdnBanks,c.banks,c.q38GdnIdentity,c.host,
		c.lease,c.bulk,c.bulkHost,c.wideHost,*c.q38StateZero,result);
	if (!okay && c.Unsafe()) { tt_resident_session_result r={}; RetainQ38DispatchOwner(c,r); }
	return okay;
}
// PIONEER_Q38_GDN_ZERO_CONTROL_BEGIN
// Separately GATED fixed zero QUALIFICATION,not ordinary mutable inference.
// The exact closed-span handoff supplies the existing ready owners/views;
// no generic setup/cleanup is attempted if a prerequisite is absent.
static uint64_t sQ38GdnDiagnosticEpoch=0; // global lock;never wrap/reuse
static status_t Q38GdnZeroControl(PioneerProbeCookie* cookie,uint32 operation,void* data,size_t length)
{
	if (!cookie || cookie->magic!=0x50313030 || !data) return B_BAD_VALUE;
	thread_info info={}; status_t status=get_thread_info(find_thread(NULL),&info);
	if (status!=B_OK) return status;
	if (info.team!=cookie->team) return B_NOT_ALLOWED;
	if (operation==TT_PCI_PROBE_Q38_GDN_ZERO_SUPPORT) {
		if (length!=sizeof(tt_q38_gdn_zero_support)) return B_BAD_VALUE;
		status=AcquireQ38GdnSessionLock(cookie); if (status!=B_OK) return status;
		const tt_q38_gdn_zero_support support={1,55,1,73728,672,1,0,1};
		status=user_memcpy(data,&support,sizeof(support)); release_sem(sLock); return status;
	}
	if (operation!=TT_PCI_PROBE_Q38_GDN_ZERO_INITIALIZE || length!=sizeof(tt_q38_gdn_zero_request)) return B_BAD_VALUE;
	tt_q38_gdn_zero_request request={}; status=user_memcpy(&request,data,sizeof(request));
	if (status!=B_OK || request.version!=1 || (request.mode!=1 && request.mode!=2) || request.reserved[0] || request.reserved[1])
		return status!=B_OK?status:B_BAD_VALUE;
	PioneerQ38GdnCallScope scope(cookie); if (scope.Status()!=B_OK) return scope.Status();
	auto* c=scope.Context();
	if (!c || c->runtimeCoreMode!=-1 || c->q38GdnBanks || c->q38StateZero || !c->q38Span || !c->q38Span->closed
		|| c->q38Span->dirty || c->q38Span->ready || c->q38Span->running || c->q38Span->unsafe
		|| !c->io.q38SpanAllowed || !c->lease.saved || !c->lease.ready || c->lease.pending || c->lease.recovery_required
		|| !c->bulk.saved || !c->bulk.ready || c->bulk.dirty || c->bulk.unsafe
		|| !c->bulkHost.allocated || !c->bulkHost.ready || c->bulkHost.pending || c->bulkHost.quarantined || c->bulkHost.released
		|| !c->wideHost.ready || !c->wideHost.memory_type_set || c->wideHost.pending || c->wideHost.quarantined || c->wideHost.released
		|| sQ38GdnDiagnosticEpoch==UINT64_MAX) return B_NOT_ALLOWED; // CPU-only,NO partial setup
	// Private epochs are minted under the lock,not accepted from the user.
	// This fixed diagnostic epoch is NEVER a real-model binding/capability.
	PioneerQ38GdnIdentity identity={++sQ38GdnDiagnosticEpoch,1,UINT64_C(0x5147385a45524f31),sReloadGeneration};
	// SPAN_END leaves mode==-1. This is the ONLY one-way CPU handoff to
	// the private root-only mode1;do not assume SPAN_END selected it. After
	// this point even a CPU allocation refusal retains the full owner/clock.
	c->runtimeCoreMode=1; c->io.q38SpanAllowed=false;
	if (!BindQ38MutableOwner(*c,identity)) { tt_resident_session_result r={}; RetainQ38DispatchOwner(*c,r); return B_DEV_NOT_READY; }
	PioneerQ38InitializeOwnedResult result;
	unsigned mode=request.mode;
	bool okay=mode==1?QualifyQ38MutableState(*c,result):InitializeQ38MutableState(*c,result);
	okay=okay && result.regions==(mode==1?1U:672U) && result.jobs==(mode==1?1U:672U) && result.pages==(mode==1?1U:73728U)
		&& result.closed==1 && result.published==(mode==1?0U:1U) && !result.recovery_required && !c->Unsafe();
	request={}; request.version=1; request.mode=mode; request.state=okay?TT_RUNTIME_PASSED:TT_RUNTIME_COMPLETION_FAILED;
	request.regions=result.regions; request.jobs=result.jobs; request.pages=result.pages; request.closed=result.closed;
	request.initialization_published=okay && result.published; request.recovery_required=!okay || c->Unsafe();
	// Any failure AFTER binding,including failed reply delivery,retains ALL
	// owners. Never implicit CLOSE/END/STOP/ARC/restore/reset/retry/free here.
	status=user_memcpy(data,&request,sizeof(request));
	if (!okay || status!=B_OK || c->Unsafe()) { tt_resident_session_result r={}; RetainQ38DispatchOwner(*c,r); }
	return status;
}
// PIONEER_Q38_GDN_ZERO_CONTROL_END
#endif
[[maybe_unused]] static bool PrepareQ38MutableZero(PioneerStoreContext& c)
{
	if (!c.io.StateZeroBeforeCohort()) {
		if (c.Unsafe()) { tt_resident_session_result r={}; RetainQ38DispatchOwner(c,r); }
		return false;
	}
	bool okay=PioneerQ38StateZeroPrepare(c.io,c.baseline.baseline.baseline.baseline.dram,
		c.baseline.baseline.baseline.noc,*c.q38GdnBanks,c.banks,c.q38GdnIdentity,c.host,
		c.lease,c.bulk,c.bulkHost,c.wideHost,*c.q38StateZero);
	if (!okay && c.Unsafe()) { tt_resident_session_result r={}; RetainQ38DispatchOwner(c,r); }
	return okay;
}
[[maybe_unused]] static bool RunQ38MutableZero(PioneerStoreContext& c,unsigned region,unsigned first,unsigned pages,
	PioneerQ38StateZeroResult& result)
{
	if (!c.io.StateZeroBeforeCohort()) {
		if (c.Unsafe()) { tt_resident_session_result r={}; RetainQ38DispatchOwner(c,r); }
		return false;
	}
	bool okay=PioneerQ38StateZeroRun(c.io,c.baseline.baseline.baseline.baseline.dram,
		c.baseline.baseline.baseline.noc,*c.q38GdnBanks,c.banks,c.host,c.lease,c.bulk,
		c.bulkHost,c.wideHost,*c.q38StateZero,c.q38GdnIdentity,region,first,pages,result);
	if (!okay && c.Unsafe()) { tt_resident_session_result r={}; RetainQ38DispatchOwner(c,r); }
	return okay;
}
[[maybe_unused]] static bool CloseQ38MutableZero(PioneerStoreContext& c,bool knownClean)
{
	if (!c.io.StateZeroBeforeCohort()) {
		if (c.Unsafe()) { tt_resident_session_result r={}; RetainQ38DispatchOwner(c,r); }
		return false;
	}
	bool okay=PioneerQ38StateZeroClose(c.io,c.baseline.baseline.baseline.baseline.dram,
		c.baseline.baseline.baseline.noc,*c.q38GdnBanks,c.banks,c.host,c.lease,c.bulk,
		c.bulkHost,c.wideHost,*c.q38StateZero,knownClean);
	if (!okay && c.Unsafe()) { tt_resident_session_result r={}; RetainQ38DispatchOwner(c,r); }
	return okay;
}
// PIONEER_Q38_STATE_ZERO_NATIVE_ADAPTER_END
#endif

#ifdef PIONEER_Q38_RESIDENT_GDN_ROOT_DIAGNOSTIC
// PIONEER_Q38_RGDN_ROOT_CONTROL_BEGIN
// Separate ROOT0 QUALIFICATION control entry,not ordinary model/Work.
// No arbitrary placement/state inputs. Same fully initialized owner only.
static bool Q38RgdnRootParent(PioneerStoreContext& c)
{
	return c.q38GdnBanks && c.q38StateZero && c.q38StateZero->closed && !c.q38StateZero->unsafe
		&& !c.q38StateZero->running && !c.q38StateZero->dirty && !c.q38StateZero->ready
		&& c.q38GdnIdentity.Valid() && c.q38GdnIdentity.driverGeneration==sReloadGeneration
		&& c.q38StateZero->identity==c.q38GdnIdentity && c.runtimeCoreMode==1 && c.io.q38MutableContext==&c
		&& c.q38GdnBanks->MatchesInitializationOwner(c.banks,c.baseline.baseline.baseline.baseline.dram,c.q38GdnIdentity)
		&& c.q38GdnBanks->InitializationPublished(c.q38GdnIdentity);
}
static status_t Q38RgdnRootControl(PioneerProbeCookie* cookie,uint32 operation,void* data,size_t length)
{
	if (!cookie || cookie->magic!=0x50313030 || !data) return B_BAD_VALUE;
	thread_info info={}; status_t status=get_thread_info(find_thread(NULL),&info);
	if (status!=B_OK) return status;
	if (info.team!=cookie->team) return B_NOT_ALLOWED;
	if (operation==TT_PCI_PROBE_Q38_RGDN_ROOT_SUPPORT) {
		if (length!=sizeof(tt_q38_rgdn_root_support)) return B_BAD_VALUE;
		status=AcquireQ38GdnSessionLock(cookie); if (status!=B_OK) return status;
		const tt_q38_rgdn_root_support support={1,55,0,0,128,64,1,2,1,0};
		status=user_memcpy(data,&support,sizeof(support)); release_sem(sLock); return status;
	}
	if (operation==TT_PCI_PROBE_Q38_RGDN_ROOT_ACCEPT) {
		if (length!=sizeof(tt_q38_rgdn_root_accept)) return B_BAD_VALUE;
		tt_q38_rgdn_root_accept a={}; status=user_memcpy(&a,data,sizeof(a));
		if (status!=B_OK || a.version!=1 || !a.nonce || a.reserved[0] || a.reserved[1]) return status!=B_OK?status:B_BAD_VALUE;
		PioneerQ38GdnCallScope scope(cookie); if (scope.Status()!=B_OK) return scope.Status();
		auto* c=scope.Context();
		return c && Q38RgdnRootParent(*c) && c->q38ResidentGdnRoot && c->io.ResidentGdnBeforeRoot()
			&& PioneerQ38RgdnRootAccept(*c->q38ResidentGdnRoot,a)?B_OK:B_NOT_ALLOWED; // CPU-only ACK
	}
	if (operation==TT_PCI_PROBE_Q38_RGDN_ROOT_END) {
		if (length!=sizeof(tt_q38_rgdn_root_end)) return B_BAD_VALUE;
		tt_q38_rgdn_root_end r={}; status=user_memcpy(&r,data,sizeof(r));
		if (status!=B_OK || r.version!=1 || r.reserved) return status!=B_OK?status:B_BAD_VALUE;
		PioneerQ38GdnCallScope scope(cookie); if (scope.Status()!=B_OK) return scope.Status();
		auto* c=scope.Context(); if (!c || !Q38RgdnRootParent(*c)) return B_NOT_ALLOWED;
		bool okay=!c->q38ResidentGdnRoot || (c->io.ResidentGdnBeforeRoot() && PioneerQ38RgdnRootClose(c->io,c->baseline.baseline.baseline.baseline.dram,
			c->baseline.baseline.baseline.noc,*c->q38GdnBanks,c->banks,c->host,c->lease,c->bulk,c->bulkHost,c->wideHost,*c->q38ResidentGdnRoot,true));
		r={1,0,unsigned(okay),unsigned(!okay || c->Unsafe())};
		status=user_memcpy(data,&r,sizeof(r));
		if (!okay || status!=B_OK || c->Unsafe()) { tt_resident_session_result retained={}; RetainQ38DispatchOwner(*c,retained); }
		return status;
	}
	if (operation!=TT_PCI_PROBE_Q38_RGDN_ROOT_RUN || length!=sizeof(tt_q38_rgdn_root_request)) return B_BAD_VALUE;
	auto* job=(tt_q38_rgdn_root_request*)malloc(sizeof(tt_q38_rgdn_root_request)); if (!job) return B_NO_MEMORY;
	status=user_memcpy(job,data,sizeof(*job));
	if (status!=B_OK || !PioneerQ38RgdnRootValid(*job)) { free(job); return status!=B_OK?status:B_BAD_VALUE; }
	{
		PioneerQ38GdnCallScope scope(cookie); auto* c=scope.Context();
		if (scope.Status()!=B_OK) status=scope.Status();
		else if (!c || !Q38RgdnRootParent(*c) || (c->q38ResidentGdnRoot && (c->q38ResidentGdnRoot->closed
			|| c->q38ResidentGdnRoot->running || c->q38ResidentGdnRoot->unsafe || c->q38ResidentGdnRoot->awaiting))) status=B_NOT_ALLOWED;
		else {
			if (!c->q38ResidentGdnRoot) c->q38ResidentGdnRoot=new(std::nothrow) PioneerQ38ResidentGdnRootLease;
			if (!c->q38ResidentGdnRoot) status=B_NO_MEMORY; // no partial owner or physical work
			else {
				job->result={}; job->result.version=1; job->result.state=TT_RUNTIME_CONFIG_FAILED;
				for (auto& w:job->next) w=0;
				for (auto& w:job->output) w=0;
				auto& root=*c->q38ResidentGdnRoot;
				bool ready=root.ready;
				if (!ready && !root.saved) ready=PioneerQ38RgdnRootPrepare(c->io,c->baseline.baseline.baseline.baseline.dram,
					c->baseline.baseline.baseline.noc,*c->q38GdnBanks,c->banks,c->q38GdnIdentity,c->host,c->lease,c->bulk,c->bulkHost,c->wideHost,root);
				bool passed=ready && PioneerQ38RgdnRootRun(c->io,c->baseline.baseline.baseline.baseline.dram,
					c->baseline.baseline.baseline.noc,*c->q38GdnBanks,c->banks,c->host,c->lease,c->bulk,c->bulkHost,c->wideHost,root,*job);
				if (!passed || c->Unsafe()) job->result.recovery_required=1;
				status=user_memcpy(data,job,sizeof(*job));
				if (!passed || status!=B_OK || c->Unsafe()) { tt_resident_session_result retained={}; RetainQ38DispatchOwner(*c,retained); }
			}
		}
	} // clears ONLY thread seal/releases semaphore,NEVER END/cleanup
	free(job); return status;
}
// PIONEER_Q38_RGDN_ROOT_CONTROL_END
#endif

// sLock held. On uncertainty only the established PCI/TLB rollback is used;
// a wired system-team page is retained, never freed through a destructor.
static bool
CloseBulkScratch(PioneerStoreContext& context,bool safe)
{
	if (!context.bulk.saved) return safe;
	if (safe) safe=context.io.SelectRuntimeCore(0);
	if (!safe) context.bulk.unsafe=true;
	return PioneerBulkUploadClose(context.io,context.baseline.baseline.baseline.baseline.dram,
		context.baseline.baseline.baseline.noc,context.host,context.lease,context.bulkHost,context.bulk) && safe;
}

static bool
FleetHeld(PioneerStoreContext& context)
{
	if (!context.fleet) return true;
	auto& dram=context.baseline.baseline.baseline.baseline.dram;
	if (!PioneerFleetGlobalSources(context.io,dram)) return false;
	for (unsigned w=0;w<TT_PIONEER_WORKERS;w++) {
		auto& f=context.fleet[w]; if (!f.Store().ready) continue;
		if (!context.io.SelectRuntimeCore(w) || context.io.ResetRead()!=0x47800 || context.io.ResetRead()!=0x47800
			|| !BlackholeResidentIdle(context.io) || !PioneerFleetLocalSources(context.io,f.noc,f.Store())) return false;
		if (f.Compute().ready && f.preludeDirty) {
			uint32 entry=context.io.RuntimeCodeRead(0);
			if (entry!=0x0006b06f && (w || !context.attention.ready || entry!=TT_ATTENTION_TILE_BOOT)) return false;
		}
	}
	return context.io.SelectRuntimeCore(0) && PioneerFleetGlobalSources(context.io,dram);
}

static bool
SelectFleetEntry(PioneerStoreContext& context,bool attention,bool alreadyHeld=false)
{
	// alreadyHeld is private to one locked ioctl: between the audit and
	// this switch, only held-core code upload/readback has been performed.
	bool okay=(alreadyHeld || FleetHeld(context)) && (!attention || context.attention.ready);
	if (okay) {
		uint32 selected=attention?uint32(TT_ATTENTION_TILE_BOOT):0x0006b06f;
		context.io.RuntimeCodeWrite(0,selected); okay=context.io.RuntimeCodeRead(0)==selected;
	}
	if (!okay) {
		context.compute.unsafe=true; context.lease.recovery_required=true;
		context.attention.unsafe=true; context.io.QuarantineHostPage();
	}
	return okay;
}

static bool
SelectResidentFleetEntry(PioneerStoreContext& context,bool attention)
{
	bool okay=context.residentAttention.reserved && (!attention || context.attention.ready)
		&& (context.runtimeCoreMode==8
			?PioneerCoordinatedMasterBoundary(context.io,context.baseline.baseline.baseline.baseline.dram,context.fleet,context.attention.ready)
			:PioneerFleetMasterBoundary(context.io,context.baseline.baseline.baseline.baseline.dram,context.fleet,context.attention.ready));
	if (okay) {
		uint32 selected=attention?uint32(TT_ATTENTION_TILE_BOOT):0x0006b06f;
		context.io.RuntimeCodeWrite(0,selected); okay=context.io.RuntimeCodeRead(0)==selected;
	}
	if (!okay) {
		context.compute.unsafe=true; context.lease.recovery_required=true;
		context.attention.unsafe=true; context.io.QuarantineHostPage();
	}
	return okay;
}

static bool
CloseFleet(PioneerStoreContext& context,bool clean)
{
	if (!context.fleet) return clean;
	if (clean && (context.runtimeCoreMode==6 || context.runtimeCoreMode==7 || context.runtimeCoreMode==8 || context.runtimeCoreMode==9)) clean=PioneerPersistentStop(context.io,
		context.baseline.baseline.baseline.baseline.dram,context.fleet);
	if (clean) clean=FleetHeld(context);
	// Establish the global held/idle proof BEFORE restoring any one peer.
	if (!clean) for (unsigned w=0;w<TT_PIONEER_WORKERS;w++) {
		context.fleet[w].Compute().unsafe=true; context.fleet[w].Store().recovery_required=true;
	}
	for (unsigned end=TT_PIONEER_WORKERS;end;end--) {
		unsigned w=end-1; auto& f=context.fleet[w];
		if (!f.Compute().saved && !f.Store().controls_saved && !f.preludeSaved) continue;
		if (!context.io.SelectRuntimeCore(w)) clean=false;
		if (w && w<=8 && context.peerAttention[w-1].saved) {
			if (!clean) context.peerAttention[w-1].unsafe=true;
			clean=PioneerAttentionTileClose(context.io,context.baseline.baseline.baseline.baseline.dram,
				f.noc,f.Store(),context.peerAttention[w-1]) && clean;
		}
		clean=PioneerFleetPreludeClose(context.io,f,clean) && clean;
		if (w) {
			if (!clean) { f.Compute().unsafe=true; f.Store().recovery_required=true; }
			clean=PioneerRuntimeMatvecClose(context.io,context.baseline.baseline.baseline.baseline.dram,
				f.noc,f.Store(),f.Compute()) && clean;
			clean=PioneerResidentLeaseEnd(context.io,f.noc,f.Store()) && clean;
		}
	}
	return context.io.SelectRuntimeCore(0) && clean;
}

static void
EndResidentSession(PioneerProbeCookie* cookie,tt_resident_session_result& result,bool recheck)
{
	PioneerStoreContext* context=cookie->context;
	context->Snapshot(result);
	bool clean=!sColdRecoveryRequired && !context->Unsafe();
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH
	if (!context->dispatchOwnerRetained) context->q38EndPhase=10;
	if (!EndQ38DispatchOwnership(*context,result,clean)) return;
	if (context->bankStorageOnly) {
		// Dedicated54 clean path:single-attempt bulk/root restoration,never
		// fall through to legacy root-lease retry/rollback after a failure.
		context->q38EndPhase=20;
		if (!CloseBulkScratch(*context,true)) { RetainQ38DispatchOwner(*context,result); return; }
		context->q38EndPhase=30;
		if (context->lease.saved && !PioneerQ38DispatchPeerLeaseEnd(context->io,
			context->baseline.baseline.baseline.noc,context->lease)) { RetainQ38DispatchOwner(*context,result); return; }
		goto q38_dispatch_owned_restored;
	}
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_COHORT
	if (context->q38Cohort) clean=PioneerQ38CohortClose(context->io,context->baseline.baseline.baseline.baseline.dram,
		context->banks,*context->q38Cohort,clean) && clean;
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_STREAM
	clean=PioneerQ38PanelClose(context->io,context->baseline.baseline.baseline.baseline.dram,
		context->baseline.baseline.baseline.noc,context->lease,context->banks,context->streamProbe,clean) && clean;
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_PANEL
	clean=PioneerQ38PanelClose(context->io,context->baseline.baseline.baseline.baseline.dram,
		context->baseline.baseline.baseline.noc,context->lease,context->banks,context->panelProbe,clean) && clean;
#endif
	clean=CloseFleet(*context,clean);
	clean=PioneerDecoderClose(context->io,context->baseline.baseline.baseline.baseline.dram,
		context->baseline.baseline.baseline.noc,context->lease,context->decoder,clean) && clean;
	clean=PioneerLayerClose(context->io,context->baseline.baseline.baseline.baseline.dram,
		context->baseline.baseline.baseline.noc,context->lease,context->layer,clean) && clean;
	clean=PioneerCoordinatorClose(context->io,context->baseline.baseline.baseline.baseline.dram,
		context->baseline.baseline.baseline.noc,context->lease,context->coordinator,clean) && clean;
	if (!context->secondCompute.saved && !context->secondLease.saved) clean=CloseBulkScratch(*context,clean);
	if (context->attention.saved && !context->secondCompute.saved && !context->secondLease.saved) {
		if (!clean) context->attention.unsafe=true;
		clean=PioneerAttentionTileClose(context->io,context->baseline.baseline.baseline.baseline.dram,
			context->baseline.baseline.baseline.noc,context->lease,context->attention) && clean;
	}
	if (context->secondCompute.saved || context->secondLease.saved) {
		// Global uncertainty forbids restoring even a known-finished peer.
		if (clean && context->lease.ready && context->secondLease.ready) {
			clean=context->io.SelectRuntimeCore(0) && context->io.ResetRead()==0x47800
				&& BlackholeResidentIdle(context->io) && PioneerRuntimeSources(context->io,
					context->baseline.baseline.baseline.baseline.dram,context->baseline.baseline.baseline.noc,context->lease);
			clean=context->io.SelectRuntimeCore(1) && clean && context->io.ResetRead()==0x47800
				&& BlackholeResidentIdle(context->io) && PioneerRuntimeSources(context->io,
					context->baseline.baseline.baseline.baseline.dram,context->secondNoc,context->secondLease);
		}
		// All workers must be known held before restoring either program's
		// shared boot word. Attention saved the matrix entry; matrix close
		// later restores the original firmware entry in the opposite order.
		clean=CloseBulkScratch(*context,clean);
		if (context->attention.saved) {
			if (!context->io.SelectRuntimeCore(0)) clean=false;
			if (!clean) context->attention.unsafe=true;
			clean=PioneerAttentionTileClose(context->io,context->baseline.baseline.baseline.baseline.dram,
				context->baseline.baseline.baseline.noc,context->lease,context->attention) && clean;
		}
		if (!context->io.SelectRuntimeCore(1)) clean=false;
		if (!clean) { context->secondCompute.unsafe=true; context->secondLease.recovery_required=true; }
		clean=PioneerRuntimeMatvecClose(context->io,context->baseline.baseline.baseline.baseline.dram,
			context->secondNoc,context->secondLease,context->secondCompute) && clean;
		clean=PioneerResidentLeaseEnd(context->io,context->secondNoc,context->secondLease) && clean;
		if (!context->io.SelectRuntimeCore(0)) clean=false;
	}
	if (context->compute.saved) {
		if (!clean) context->compute.unsafe=true;
		clean=PioneerRuntimeMatvecClose(context->io,context->baseline.baseline.baseline.baseline.dram,
			context->baseline.baseline.baseline.noc,context->lease,context->compute) && clean;
	}
	if (context->lease.saved!=NULL) {
		if (!clean) context->lease.recovery_required=true;
		clean=PioneerResidentLeaseEnd(context->io,context->baseline.baseline.baseline.noc,context->lease) && clean;
	}
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH
q38_dispatch_owned_restored:
	context->q38EndPhase=40;
#endif
#ifdef PIONEER_Q38_NORMAL_CLOCK
	if (context->bankStorageOnly && !FinishQ38NormalClock(*context,clean)) {
		RetainQ38DispatchOwner(*context,result); return;
	}
#endif
	if (recheck && clean) {
		clean=context->io.PioneerHostDmaAuditIO::AfterNodeId(context->Pci().node_id[0])
			&& BlackholeHostReadPciQuiet(context->io,0x402)
			&& PioneerResidentSessionIdle(context->io,context->baseline.baseline.baseline.noc,context->idle);
	}
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH
	if (context->bankStorageOnly) {
		context->q38EndPhase=50;
		if (!clean || !PioneerQ38DispatchMmioEnd(context->io,context->Pci(),context->transaction)) { RetainQ38DispatchOwner(*context,result); return; }
	} else
#endif
	BlackholePioneerMmioEnd(context->io,context->Pci(),context->transaction);
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH
	context->q38EndPhase=60;
#endif
	clean &= !context->Unsafe() && context->Pci().restored;
	result.active=0; result.pci_restored=context->Pci().restored;
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH
	context->q38EndPhase=70;
#endif
	if (context->wideHost.allocated) {
		if (!clean) PioneerQuarantineWideOutput(context->io,context->wideHost);
		else clean=PioneerReleaseWideOutput(context->io,context->wideHost,true) && clean;
	}
	if (context->bulkHost.allocated) {
		if (!clean) PioneerQuarantineOutputBuffer(context->io,context->bulkHost);
		else clean=PioneerReleaseOutputBuffer(context->io,context->bulkHost,true) && clean;
	}
	if (context->host.allocated) {
		if (!clean) context->io.QuarantineHostPage();
		else {
			context->host.release_status=context->io.ReleaseHostPage();
			context->host.released=context->host.release_status==B_OK;
		}
	}
	result.host_released=(!context->host.allocated || context->host.released)
		&& (!context->bulkHost.allocated || context->bulkHost.released) && (!context->wideHost.allocated || context->wideHost.released);
	if (!clean || !result.host_released) sColdRecoveryRequired=true;
	result.cold=sColdRecoveryRequired;
	result.state=clean && result.host_released?TT_RESIDENT_SESSION_CLOSED:TT_RESIDENT_SESSION_CLEANUP_FAILED;
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH
	if (context->bankStorageOnly && (!clean || !result.host_released)) { RetainQ38DispatchOwner(*context,result); return; }
#endif
#ifdef PIONEER_Q38_HOST_PHASE_PROFILE
	// PIONEER_Q38_HOST_PHASE_PUBLICATION_BEGIN
	if (context->io.q38HostPhaseReportAllowed && context->bankStorageOnly && clean && result.host_released && !result.cold) context->io.Q38HostPhaseReport();
	// PIONEER_Q38_HOST_PHASE_PUBLICATION_END
#endif
	sPCI->unreserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
	cookie->context=NULL; sRuntimeOwner=NULL;
#ifdef PIONEER_Q38_LOCAL_DELTA_PROBE
	sQ38LocalDeltaOwner=NULL;
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_BANKS
	sBankStorageOnly=false;
#endif
	delete context;
}

// PIONEER_Q38_DISPATCH_COOKIE_RELEASE_BEGIN
static status_t
ReleaseResidentCookie(PioneerProbeCookie* cookie)
{
	if (cookie==NULL || cookie->magic!=0x50313030) return B_BAD_VALUE;
	if (cookie->context!=NULL) {
		// Closing must also work after the cold latch: do not use the
		// ordinary gate, which deliberately refuses all further card work.
		status_t status=acquire_sem(sLock);
		if (status!=B_OK) return status; // retain ownership, never silently free a possibly live page
		tt_resident_session_result result={};
#ifdef PIONEER_Q38_GDN_MUTABLE_STATE
		// Scope presence alone requires explicit END,even if a future
		// allocation bug misbound the context as a legacy/non-bank session.
		if (cookie->context->q38GdnBanks || cookie->context->q38StateZero
#ifdef PIONEER_Q38_RESIDENT_OUTPUT_OWNER
   || cookie->context->q38ResidentOutputNative
#ifdef PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT
   || cookie->context->q38ResidentLayerNative
#endif
#endif
#ifdef PIONEER_Q38_EMBEDDING_NATIVE
			|| cookie->context->q38EmbeddingModel
#endif
#ifdef PIONEER_Q38_FFN_MODEL_INIT
			|| cookie->context->q38FfnModelPlan || cookie->context->q38GraphMutable || cookie->context->q38FfnZero
#endif
			)
			RetainQ38DispatchOwner(*cookie->context,result);
		else
#endif
#ifdef TT_PIONEER_Q38_WORK_MODE
		// After a persistent RUN,ONLY explicit BANK_END authorizes normal
		// cleanup. A userspace CPU-reference/graph failure can be invisible
		// to the kernel:close WITHOUT END must retain ALL owners with NO
		// device IO/reset/STOP,even when the last RUN's metadata passed.
		if (cookie->context->bankStorageOnly && ((cookie->context->q38Dispatch && !cookie->context->q38Dispatch->closed)
#ifdef PIONEER_Q38_NORMAL_CLOCK
			|| cookie->context->q38Clock.RequiresExplicitEnd()
#endif
#ifdef PIONEER_Q38_LOCAL_DELTA_PROBE
			|| cookie->context->q38Delta.saved
#endif
#ifdef PIONEER_Q38_SPAN_PRELOAD
			|| cookie->context->q38Span
#endif
			))
			RetainQ38DispatchOwner(*cookie->context,result);
		else
#endif
		EndResidentSession(cookie,result,!sColdRecoveryRequired);
		release_sem(sLock);
	}
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH
	if (cookie->context && cookie->context->dispatchOwnerRetained) return B_OK; // retain cookie/owner too until cold recovery
#endif
	cookie->magic=0; delete cookie; return B_OK;
}
// PIONEER_Q38_DISPATCH_COOKIE_RELEASE_END

// sLock held; exact capacity/policy has already been selected by its ABI.
static status_t
BeginResidentSession(PioneerProbeCookie* cookie,uint64 capacity,tt_resident_session_result& result,bool bankOnly=false)
{
	if (cookie->context!=NULL) return B_BUSY;
	PioneerStoreContext* context=new(std::nothrow) PioneerStoreContext(capacity);
	if (context==NULL) return B_NO_MEMORY;
	if (context->initialized==NULL || context->hashes==NULL) { delete context; return B_NO_MEMORY; }
	status_t status=sPCI->reserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
	if (status!=B_OK) { delete context; return status; }
	if (sDeviceManager==NULL) status=get_module(B_DEVICE_MANAGER_MODULE_NAME,(module_info**)&sDeviceManager);
	if (status==B_OK) status=GetPioneerPciProfile(sDeviceManager,sCard,context->profile);
	context->profile.status=status;
	bool ready=status==B_OK && BlackholePioneerMmioBegin(context->io,context->Pci(),context->transaction)
		&& context->io.PioneerHostDmaAuditIO::AfterNodeId(context->Pci().node_id[0])
		&& PioneerResidentSessionIdle(context->io,context->baseline.baseline.baseline.noc,context->idle);
	unsigned failure=TT_RESIDENT_SESSION_PREFLIGHT_FAILED;
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_BANKS
	context->bankStorageOnly=bankOnly;
	sBankStorageOnly=bankOnly;
	if (ready && bankOnly) ready=context->banks.Bind(context->baseline.baseline.baseline.baseline.dram);
#ifdef PIONEER_Q38_NORMAL_CLOCK
	if (ready && bankOnly) ready=BeginQ38NormalClock(*context);
#endif
#else
	(void)bankOnly;
#endif
	if (ready) { failure=TT_RESIDENT_SESSION_HOST_FAILED; ready=PioneerPrepareHostPage(context->io,context->host); }
	cookie->context=context; sRuntimeOwner=cookie;
	context->Snapshot(result);
	if (!ready) {
		if (!context->transaction.config_attempted && context->Pci().state!=TT_PIONEER_MMIO_READ_OK)
			context->Pci().restored=1;
		EndResidentSession(cookie,result,false);
		if (!result.cold && result.host_released) result.state=failure;
	}
	return B_OK;
}

static status_t
ResidentSessionControl(PioneerProbeCookie* cookie,uint32 operation,void* data,size_t length)
{
	if (data==NULL || length!=sizeof(tt_resident_session_result)) return B_BAD_VALUE;
	thread_info thread={};
	status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	tt_resident_session_result result={};
	if (operation==TT_PCI_PROBE_PIONEER_SESSION_BEGIN) {
		status=user_memcpy(&result,data,sizeof(result)); if (status!=B_OK) return status;
		if (result.version!=1 || !PioneerResidentCapacity(result.bytes)) return B_BAD_VALUE;
	}
	status=AcquireProbeLock(cookie); if (status!=B_OK) return status;
	if (operation==TT_PCI_PROBE_PIONEER_SESSION_SUPPORT) {
		result={}; result.version=1; result.capabilities=TT_PIONEER_RESIDENT_CAPABILITIES;
	} else if (operation==TT_PCI_PROBE_PIONEER_SESSION_BEGIN) {
		status=BeginResidentSession(cookie,result.bytes,result);
		if (status!=B_OK) { release_sem(sLock); return status; }
	} else if (cookie->context==NULL || sRuntimeOwner!=cookie) {
		release_sem(sLock); return B_DEV_NOT_READY;
	} else if (operation==TT_PCI_PROBE_PIONEER_SESSION_END) {
		EndResidentSession(cookie,result,true);
	} else {
		// CPU metadata only. It does not revalidate hardware health or
		// authorize freeing a page; END performs its own live audit.
		cookie->context->Snapshot(result);
	}
	status=user_memcpy(data,&result,sizeof(result));
	if (status!=B_OK && operation==TT_PCI_PROBE_PIONEER_SESSION_BEGIN && cookie->context!=NULL) {
		tt_resident_session_result cleanup={}; EndResidentSession(cookie,cleanup,true);
	}
	release_sem(sLock); return status;
}

static status_t
ResidentStoreControl(PioneerProbeCookie* cookie,uint32 operation,void* data,size_t length)
{
	if (data==NULL || length!=sizeof(tt_pioneer_resident_page)) return B_BAD_VALUE;
	thread_info thread={};
	status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	tt_pioneer_resident_page* request=(tt_pioneer_resident_page*)malloc(sizeof(tt_pioneer_resident_page));
	if (request==NULL) return B_NO_MEMORY;
	status=user_memcpy(request,data,sizeof(*request));
	if (status!=B_OK || request->version!=1 || request->reserved!=0) {
		free(request); return status!=B_OK?status:B_BAD_VALUE;
	}
	status=AcquireProbeLock(cookie);
	if (status!=B_OK) { free(request); return status; }
	PioneerStoreContext* context=cookie->context;
	if (context==NULL || sRuntimeOwner!=cookie) status=B_DEV_NOT_READY;
	else if (context->runtimeCoreMode==4 || context->runtimeCoreMode==6 || context->runtimeCoreMode==7 || context->runtimeCoreMode==8 || context->runtimeCoreMode==9) status=B_NOT_ALLOWED;
	else if (!PioneerResidentOffset(context->bytes,request->offset)
		|| (context->residentAttention.reserved && request->offset>=context->bytes-TT_RESIDENT_KV_BYTES)) status=B_BAD_VALUE;
	else {
		bool upload=operation==TT_PCI_PROBE_PIONEER_STORE_UPLOAD;
		unsigned page=unsigned(request->offset/4096);
		if (!upload && !context->initialized[page]) status=B_ENTRY_NOT_FOUND;
		else {
			request->result={}; request->result.version=1; request->result.state=TT_STORE_CONFIG_FAILED;
			bool ready=context->lease.ready;
			if (!ready && !context->lease.saved) ready=PioneerResidentLeaseBegin(context->io,context->baseline.baseline.baseline.noc,context->lease);
			bool passed=ready && PioneerResidentStorePage(context->io,context->baseline.baseline.baseline.baseline.dram,
				context->baseline.baseline.baseline.noc,context->host,context->lease,context->bytes,request->offset,
				upload,request->data,request->result);
			if (upload && request->result.wrote_device && context->initialized[page]) {
				context->initialized[page]=0; context->uploadedPages--;
			}
			if (passed && upload) {
				context->hashes[page]=PioneerResidentPageHash(request->data);
				context->initialized[page]=1; context->uploadedPages++;
			} else if (passed && PioneerResidentPageHash(request->data)!=context->hashes[page]) {
				request->result.state=TT_STORE_MISMATCH; request->result.mismatches++;
			}
			if (context->lease.recovery_required || context->lease.pending || request->result.recovery_required) {
				context->lease.recovery_required=true; context->io.QuarantineHostPage();
				request->result.recovery_required=1; sColdRecoveryRequired=true;
			}
			status=user_memcpy(data,request,sizeof(*request));
		}
	}
	release_sem(sLock); free(request); return status;
}

#ifdef TT_PIONEER_EXPERIMENTAL_Q38_BANKS
// PIONEER_Q38_BANK_CONTROL_BEGIN
// Experimental45: storage-only, disjoint ABI, no code image/core release.
// An arena consumes the SAME global owner as legacy inference. There is no
// bank-local recovery/retry: uncertainty quarantines the entire card owner.
static status_t
Q38BankBulkUpload(PioneerProbeCookie* cookie,void* data,size_t length)
{
	if (length!=sizeof(tt_q38_bank_bulk)) return B_BAD_VALUE;
	auto* request=(tt_q38_bank_bulk*)malloc(sizeof(tt_q38_bank_bulk));
	if (!request) return B_NO_MEMORY;
	status_t status=user_memcpy(request,data,sizeof(*request));
	if (status!=B_OK || request->version!=TT_Q38_BANK_PROTOCOL || request->bank>=7 || request->reserved
		|| !PioneerQ38BankBulkRange(request->bank,request->offset,request->pages)) {
		free(request); return status!=B_OK?status:B_BAD_VALUE;
	}
	status=AcquireProbeLock(cookie,true); if (status!=B_OK) { free(request); return status; }
	auto* c=cookie->context;
	if (!c || sRuntimeOwner!=cookie || !c->bankStorageOnly) status=B_NOT_ALLOWED;
#ifdef PIONEER_Q38_SPAN_PRELOAD
	else if (c->q38Span && !c->q38Span->closed) status=B_NOT_ALLOWED;
#endif
	else {
		auto& dram=c->baseline.baseline.baseline.baseline.dram;
		auto& noc=c->baseline.baseline.baseline.noc;
		uint32 address=0,route=0,index=0;
		if (!c->banks.Range(dram,request->bank,request->offset,request->pages)
			|| !c->banks.Page(dram,request->bank,request->offset,address,route,index)) status=B_DEV_NOT_READY;
		else {
			request->result={}; request->result.transfer.version=1; request->result.transfer.state=TT_STORE_CONFIG_FAILED;
			bool ready=c->lease.ready;
			if (!ready && !c->lease.saved) ready=PioneerResidentLeaseBegin(c->io,noc,c->lease);
			if (ready) ready=c->io.SelectRuntimeCore(0) && c->io.MapBulk();
			if (ready && !c->bulkHost.allocated) ready=PioneerPrepareOutputBuffer(c->io,c->host,c->bulkHost);
			ready=ready && c->bulkHost.ready && PioneerBulkUploadPrepare(c->io,dram,noc,c->host,c->lease,c->bulkHost,c->bulk);
			bool passed=ready && PioneerBulkUpload(c->io,dram,noc,c->host,c->lease,c->bulkHost,c->bulk,
				PioneerResidentBanks::Capacity,request->offset,request->pages,request->data,request->result,&c->banks,request->bank);
			for (unsigned p=0;p<request->pages;p++) {
				if (request->result.transfer.wrote_device && c->initialized[index+p]) {
					c->initialized[index+p]=0; c->uploadedPages--;
				}
				if (passed) {
					c->hashes[index+p]=PioneerResidentPageHash(request->data+p*1024);
					c->initialized[index+p]=1; c->uploadedPages++;
				}
			}
			if (c->Unsafe() || request->result.transfer.recovery_required) {
				c->banks.Quarantine(); c->bulk.unsafe=true; c->lease.recovery_required=true;
				c->io.QuarantineHostPage(); PioneerQuarantineOutputBuffer(c->io,c->bulkHost);
				request->result.transfer.recovery_required=1; sColdRecoveryRequired=true;
			}
			status=user_memcpy(data,request,sizeof(*request));
		}
	}
	release_sem(sLock); free(request); return status;
}

static status_t
Q38BankControl(PioneerProbeCookie* cookie,uint32 operation,void* data,size_t length)
{
	if (!data) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	if (operation==TT_PCI_PROBE_Q38_BANK_BULK_UPLOAD) return Q38BankBulkUpload(cookie,data,length);
	if (operation==TT_PCI_PROBE_Q38_BANK_SUPPORT) {
		if (length!=sizeof(tt_q38_bank_support)) return B_BAD_VALUE;
		status=AcquireProbeLock(cookie,true); if (status!=B_OK) return status;
		const tt_q38_bank_support support={TT_Q38_BANK_DRIVER,TT_Q38_BANK_PROTOCOL,7,4096,4,1,
			PioneerResidentBanks::Capacity};
		status=user_memcpy(data,&support,sizeof(support)); release_sem(sLock); return status;
	}
	bool page=operation==TT_PCI_PROBE_Q38_BANK_UPLOAD || operation==TT_PCI_PROBE_Q38_BANK_READ;
	if (!page) {
		if (length!=sizeof(tt_q38_bank_session)) return B_BAD_VALUE;
		tt_q38_bank_session request={}; status=user_memcpy(&request,data,sizeof(request));
		if (status!=B_OK || request.version!=TT_Q38_BANK_PROTOCOL || request.reserved)
			return status!=B_OK?status:B_BAD_VALUE;
#ifdef PIONEER_Q38_GDN_MUTABLE_STATE
		status=(operation==TT_PCI_PROBE_Q38_BANK_END || operation==TT_PCI_PROBE_Q38_BANK_STATUS)?AcquireQ38GdnSessionLock(cookie):AcquireProbeLock(cookie,true);
#elif defined(PIONEER_Q38_LOCAL_DELTA_PROBE)
		status=(operation==TT_PCI_PROBE_Q38_BANK_END || operation==TT_PCI_PROBE_Q38_BANK_STATUS)?AcquireQ38DeltaLock(cookie):AcquireProbeLock(cookie,true);
#else
		status=AcquireProbeLock(cookie,true); if (status!=B_OK) return status;
#endif
		if (status!=B_OK) return status;
		bool began=false;
		if (operation==TT_PCI_PROBE_Q38_BANK_BEGIN) {
			bool empty=cookie->context==NULL;
			status=BeginResidentSession(cookie,PioneerResidentBanks::Total,request.result,true);
			began=empty && status==B_OK && cookie->context!=NULL;
		} else if (!cookie->context || sRuntimeOwner!=cookie || !cookie->context->bankStorageOnly) {
			status=B_NOT_ALLOWED;
		} else if (operation==TT_PCI_PROBE_Q38_BANK_END) {
#ifdef PIONEER_Q38_HOST_PHASE_PROFILE
			cookie->context->io.q38HostPhaseReportAllowed=true; // CPU diagnostic permission ONLY,not cleanup authority
#endif
#ifdef PIONEER_Q38_RESIDENT_LAYER_VALIDATION_NATIVE
#include "PioneerQ38ResidentLayerValidationEndCall.inc"
#elif defined(PIONEER_Q38_RECURRENT_BLOCK_FINAL_RELEASE)
#include "PioneerQ38RecurrentBlockFinalEndCall.inc"
#elif defined(PIONEER_Q38_MIXED_LIVE_FINAL_RELEASE)
#include "PioneerQ38MixedLiveFinalEndCall.inc"
#else
			EndResidentSession(cookie,request.result,true);
#endif
		} else if (operation==TT_PCI_PROBE_Q38_BANK_STATUS) {
			cookie->context->Snapshot(request.result);
		} else status=B_BAD_VALUE;
		if (status==B_OK) status=user_memcpy(data,&request,sizeof(request));
		if (status!=B_OK && began) {
			tt_resident_session_result cleanup={};
#ifdef PIONEER_Q38_NORMAL_CLOCK
			// A lost BEGIN reply must not remove a known busy request behind
			// the caller:it sees only transport uncertainty and closes ONLY.
			if (cookie->context && cookie->context->q38Clock.RequiresExplicitEnd())
				RetainQ38DispatchOwner(*cookie->context,cleanup);
			else
#endif
			EndResidentSession(cookie,cleanup,true);
		}
		release_sem(sLock); return status;
	}
	if (length!=sizeof(tt_q38_bank_page)) return B_BAD_VALUE;
	auto* request=(tt_q38_bank_page*)malloc(sizeof(tt_q38_bank_page));
	if (!request) return B_NO_MEMORY;
	status=user_memcpy(request,data,sizeof(*request));
	if (status!=B_OK || request->version!=TT_Q38_BANK_PROTOCOL || request->bank>=7
		|| (request->offset&4095) || request->offset>=PioneerResidentBanks::Capacity) {
		free(request); return status!=B_OK?status:B_BAD_VALUE;
	}
	status=AcquireProbeLock(cookie,true); if (status!=B_OK) { free(request); return status; }
	auto* c=cookie->context;
	if (!c || sRuntimeOwner!=cookie || !c->bankStorageOnly) status=B_NOT_ALLOWED;
#ifdef PIONEER_Q38_SPAN_PRELOAD
	else if (c->q38Span && !c->q38Span->closed) status=B_NOT_ALLOWED;
#endif
	else {
		auto& dram=c->baseline.baseline.baseline.baseline.dram;
		auto& noc=c->baseline.baseline.baseline.noc;
		uint32 address=0,route=0,index=0;
		bool upload=operation==TT_PCI_PROBE_Q38_BANK_UPLOAD;
		if (!c->banks.Page(dram,request->bank,request->offset,address,route,index)) status=B_DEV_NOT_READY;
		else if (!upload && !c->initialized[index]) status=B_ENTRY_NOT_FOUND;
		else {
			request->result={}; request->result.version=1; request->result.state=TT_STORE_CONFIG_FAILED;
			bool ready=c->lease.ready;
			if (!ready && !c->lease.saved) ready=PioneerResidentLeaseBegin(c->io,noc,c->lease);
			bool passed=ready && PioneerResidentStorePage(c->io,dram,noc,c->host,c->lease,
				PioneerResidentBanks::Capacity,request->offset,upload,request->data,request->result,
				&c->banks,request->bank);
			if (upload && request->result.wrote_device && c->initialized[index]) {
				c->initialized[index]=0; c->uploadedPages--;
			}
			if (passed && upload) {
				c->hashes[index]=PioneerResidentPageHash(request->data);
				c->initialized[index]=1; c->uploadedPages++;
			} else if (passed && PioneerResidentPageHash(request->data)!=c->hashes[index]) {
				request->result.state=TT_STORE_MISMATCH; request->result.mismatches++;
				c->initialized[index]=0; c->uploadedPages--;
			}
			if (c->Unsafe()) {
				c->banks.Quarantine(); c->lease.recovery_required=true;
				c->io.QuarantineHostPage(); request->result.recovery_required=1; sColdRecoveryRequired=true;
			}
			status=user_memcpy(data,request,sizeof(*request));
		}
	}
	release_sem(sLock); free(request); return status;
}
// PIONEER_Q38_BANK_CONTROL_END
#endif

#ifdef PIONEER_Q38_SPAN_PRELOAD
// PIONEER_Q38_SPAN_CONTROL_BEGIN
static status_t
Q38SpanControl(PioneerProbeCookie* cookie,uint32 operation,void* data,size_t length)
{
	if (!data) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	if (operation==TT_PCI_PROBE_Q38_SPAN_SUPPORT
#ifdef PIONEER_Q38_SPAN_FAST
		|| operation==TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT
#endif
		) {
		if (length!=sizeof(tt_q38_span_support)) return B_BAD_VALUE;
		status=AcquireProbeLock(cookie,true); if (status!=B_OK) return status;
		const tt_q38_span_support support={55,1,7,4096,TT_Q38_SPAN_PUBLIC_BYTES,1,1,0};
		status=user_memcpy(data,&support,sizeof(support)); release_sem(sLock); return status;
	}
	if (operation==TT_PCI_PROBE_Q38_SPAN_END) {
		if (length!=sizeof(tt_q38_span_end)) return B_BAD_VALUE;
		tt_q38_span_end request={}; status=user_memcpy(&request,data,sizeof(request));
		if (status!=B_OK || request.version!=1 || request.reserved) return status!=B_OK?status:B_BAD_VALUE;
		status=AcquireProbeLock(cookie,true); if (status!=B_OK) return status;
		auto* c=cookie->context;
		if (!c || sRuntimeOwner!=cookie || !PioneerQ38SpanOwner(*c) || !c->q38Span) status=B_NOT_ALLOWED;
		else {
			request.closed=unsigned(PioneerQ38SpanEndService(*c)); request.recovery_required=0;
			if (!request.closed) {
				c->q38Span->unsafe=true; tt_resident_session_result retained={}; RetainQ38DispatchOwner(*c,retained);
				request.recovery_required=1;
			}
			status=user_memcpy(data,&request,sizeof(request));
		}
		release_sem(sLock); return status;
	}
	if ((operation!=TT_PCI_PROBE_Q38_SPAN_UPLOAD
#ifdef PIONEER_Q38_SPAN_FAST
		&& operation!=TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD
#endif
		) || length!=sizeof(tt_q38_span_upload)) return B_BAD_VALUE;
	auto* request=(tt_q38_span_upload*)malloc(sizeof(tt_q38_span_upload)); if (!request) return B_NO_MEMORY;
	status=user_memcpy(request,data,sizeof(*request));
	if (status!=B_OK || !PioneerQ38SpanPublicShape(request->version,request->bank,request->offset,request->bytes,request->reserved)) {
		free(request); return status!=B_OK?status:B_BAD_VALUE;
	}
	status=AcquireProbeLock(cookie,true); if (status!=B_OK) { free(request); return status; }
	auto* c=cookie->context;
	if (!c || sRuntimeOwner!=cookie || !PioneerQ38SpanOwner(*c) || (c->q38Span && c->q38Span->closed)) status=B_NOT_ALLOWED;
	else {
		PioneerQ38SpanExtent extent;
		if (!PioneerQ38SpanAuthorize(c->banks,c->baseline.baseline.baseline.baseline.dram,c->initialized,
			uint32(c->bytes/4096),request->bank,request->offset,request->bytes,extent)) status=B_ENTRY_NOT_FOUND;
		else {
			if (!c->q38Span) c->q38Span=new(std::nothrow) PioneerQ38SpanLease;
			if (!c->q38Span) status=B_NO_MEMORY;
			else {
				bool passed;
#ifdef PIONEER_Q38_SPAN_FAST
				if (operation==TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD) passed=PioneerQ38SpanUploadService<PioneerStoreContext,true>(*c,*request);
				else
#endif
				passed=PioneerQ38SpanUploadService(*c,*request);
				if (!passed || c->Unsafe()) {
					c->q38Span->unsafe=true; tt_resident_session_result retained={}; RetainQ38DispatchOwner(*c,retained);
					request->result.state=TT_RUNTIME_COMPLETION_FAILED; request->result.pages=0;
					request->result.host_exact=0; request->result.recovery_required=1;
				}
				status=user_memcpy(data,request,sizeof(*request));
			}
		}
	}
	release_sem(sLock); free(request); return status;
}
// PIONEER_Q38_SPAN_CONTROL_END
#endif

#ifdef PIONEER_Q38_LOCAL_DELTA_PROBE
// PIONEER_Q38_DELTA_CONTROL_BEGIN
static status_t
Q38DeltaControl(PioneerProbeCookie* cookie,uint32 operation,void* data,size_t length)
{
	if (!data) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	if (operation==TT_PCI_PROBE_Q38_DELTA_SUPPORT) {
		if (length!=sizeof(tt_q38_delta_support)) return B_BAD_VALUE;
		status=AcquireQ38DeltaLock(cookie); if (status!=B_OK) return status;
		const tt_q38_delta_support support={TT_Q38_DELTA_PROBE_VERSION,55,1,128,64,8192,1};
		status=user_memcpy(data,&support,sizeof(support)); release_sem(sLock); return status;
	}
	if (operation==TT_PCI_PROBE_Q38_DELTA_ACCEPT) {
		if (length!=sizeof(tt_q38_delta_accept)) return B_BAD_VALUE;
		tt_q38_delta_accept a={}; status=user_memcpy(&a,data,sizeof(a)); if (status!=B_OK) return status;
		status=AcquireQ38DeltaLock(cookie); if (status!=B_OK) return status;
		auto* c=cookie->context;
		status=c && sRuntimeOwner==cookie && c->bankStorageOnly && PioneerQ38DeltaProbeAccept(c->q38Delta,a)?B_OK:B_NOT_ALLOWED;
		release_sem(sLock); return status;
	}
	if (operation!=TT_PCI_PROBE_Q38_DELTA_RUN || length!=sizeof(tt_q38_delta_probe)) return B_BAD_VALUE;
	auto* job=(tt_q38_delta_probe*)malloc(sizeof(tt_q38_delta_probe)); if (!job) return B_NO_MEMORY;
	status=user_memcpy(job,data,sizeof(*job));
	if (status!=B_OK || !PioneerQ38DeltaProbeValid(*job)) { free(job); return status!=B_OK?status:B_BAD_VALUE; }
	status=AcquireQ38DeltaLock(cookie); if (status!=B_OK) { free(job); return status; }
	auto* c=cookie->context;
	if (!c || sRuntimeOwner!=cookie || !c->bankStorageOnly || c->q38Dispatch || c->q38Cohort
		|| c->panelProbe.saved || c->streamProbe.saved || c->q38Delta.awaiting || c->q38Delta.unsafe || c->q38Delta.running
		|| c->q38Delta.nonce==UINT32_MAX) status=B_NOT_ALLOWED;
#ifdef PIONEER_Q38_SPAN_PRELOAD
	else if (c->q38Span) status=B_NOT_ALLOWED;
#endif
	else {
		sQ38LocalDeltaOwner=cookie; // exclusive intent BEFORE any prepare/mapping/reset
		auto& dram=c->baseline.baseline.baseline.baseline.dram; auto& noc=c->baseline.baseline.baseline.noc;
		job->result={}; job->result.version=TT_Q38_DELTA_PROBE_VERSION; job->result.state=TT_RUNTIME_CONFIG_FAILED;
		for (auto& v:job->next) v=0;
		for (auto& v:job->output) v=0;
		bool ready=c->lease.ready;
		if (!ready && !c->lease.saved) ready=PioneerResidentLeaseBegin(c->io,noc,c->lease);
		ready=ready && c->io.SelectRuntimeCore(0) && c->io.MapQ38Panel()
			&& PioneerQ38DeltaProbePrepare(c->io,dram,noc,c->lease,c->banks,c->q38Delta);
		bool passed=ready && PioneerQ38DeltaProbeRun(c->io,dram,noc,c->lease,c->banks,c->q38Delta,*job);
		if (!passed || c->Unsafe()) job->result.recovery_required=1;
		status=user_memcpy(data,job,sizeof(*job));
		if (!passed || status!=B_OK || c->Unsafe()) {
			// MEMORY ONLY. On UNKNOWN or lost reply,retain ALL owners.
			c->q38Delta.unsafe=true; c->banks.Quarantine(); c->lease.recovery_required=true;
			c->io.QuarantineHostPage(); sColdRecoveryRequired=true;
		}
	}
	release_sem(sLock); free(job); return status;
}
// PIONEER_Q38_DELTA_CONTROL_END
#endif

#ifdef TT_PIONEER_EXPERIMENTAL_Q38_PANEL
// PIONEER_Q38_PANEL_CONTROL_BEGIN
static status_t
Q38PanelControl(PioneerProbeCookie* cookie,uint32 operation,void* data,size_t length)
{
	if (!data) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	if (operation==TT_PCI_PROBE_Q38_PANEL_SUPPORT) {
		if (length!=sizeof(tt_q38_panel_support)) return B_BAD_VALUE;
		status=AcquireProbeLock(cookie,true); if (status!=B_OK) return status;
		const tt_q38_panel_support support={
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_COHORT
			TT_Q38_COHORT_DRIVER,
#else
			TT_Q38_PANEL_CURRENT_DRIVER,
#endif
			TT_Q38_PANEL_ABI_VERSION,1,16,TT_Q38_PANEL_COLUMNS,0x5100};
		status=user_memcpy(data,&support,sizeof(support)); release_sem(sLock); return status;
	}
	if (operation!=TT_PCI_PROBE_Q38_PANEL_RUN || length!=sizeof(tt_q38_panel_probe)) return B_BAD_VALUE;
	auto* request=(tt_q38_panel_probe*)malloc(sizeof(tt_q38_panel_probe));
	if (!request) return B_NO_MEMORY;
	status=user_memcpy(request,data,sizeof(*request));
	if (status!=B_OK || !PioneerQ38PanelProbeValid(*request)) { free(request); return status!=B_OK?status:B_BAD_VALUE; }
	status=AcquireProbeLock(cookie,true); if (status!=B_OK) { free(request); return status; }
	auto* c=cookie->context;
	if (!c || sRuntimeOwner!=cookie || !c->bankStorageOnly) status=B_NOT_ALLOWED;
#ifdef PIONEER_Q38_SPAN_PRELOAD
	else if (c->q38Span && !c->q38Span->closed) status=B_NOT_ALLOWED;
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_COHORT
	else if (c->q38Cohort) status=B_NOT_ALLOWED; // never overlap local and cohort L1 owners
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_STREAM
	else if (c->streamProbe.saved) status=B_NOT_ALLOWED; // overlapping L1 leases NEVER coexist
#endif
	else {
		auto& dram=c->baseline.baseline.baseline.baseline.dram;
		auto& noc=c->baseline.baseline.baseline.noc;
		request->result={}; request->result.version=1; request->result.state=TT_RUNTIME_CONFIG_FAILED;
		for (unsigned row=0;row<16;row++) request->dot[row]=request->coefficient[row]=request->minimum[row]=0;
		bool ready=c->lease.ready;
		if (!ready && !c->lease.saved) ready=PioneerResidentLeaseBegin(c->io,noc,c->lease);
		ready=ready && c->io.SelectRuntimeCore(0) && c->io.MapQ38Panel()
			&& PioneerQ38PanelPrepare(c->io,dram,noc,c->lease,c->banks,c->panelProbe);
		if (ready) PioneerQ38PanelRun(c->io,dram,noc,c->lease,c->banks,c->panelProbe,*request);
		if (c->Unsafe() || request->result.recovery_required) {
			c->banks.Quarantine(); c->panelProbe.unsafe=true; c->lease.recovery_required=true;
			c->io.QuarantineHostPage(); request->result.recovery_required=1; sColdRecoveryRequired=true;
		}
		status=user_memcpy(data,request,sizeof(*request));
	}
	release_sem(sLock); free(request); return status;
}
// PIONEER_Q38_PANEL_CONTROL_END
#endif

#ifdef TT_PIONEER_EXPERIMENTAL_Q38_STREAM
// PIONEER_Q38_STREAM_CONTROL_BEGIN
static status_t
Q38StreamControl(PioneerProbeCookie* cookie,uint32 operation,void* data,size_t length)
{
	if (!data) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	if (operation==TT_PCI_PROBE_Q38_STREAM_SUPPORT) {
		if (length!=sizeof(tt_q38_stream_support)) return B_BAD_VALUE;
		status=AcquireProbeLock(cookie,true); if (status!=B_OK) return status;
		const tt_q38_stream_support support={
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_COHORT
			TT_Q38_COHORT_DRIVER,
#else
			TT_Q38_STREAM_DRIVER,
#endif
			TT_Q38_STREAM_ABI,TT_Q38_STREAM_OUTPUT_ROWS,17408,7,0};
		status=user_memcpy(data,&support,sizeof(support)); release_sem(sLock); return status;
	}
	if (operation!=TT_PCI_PROBE_Q38_STREAM_RUN || length!=sizeof(tt_q38_stream_probe)) return B_BAD_VALUE;
	auto* request=(tt_q38_stream_probe*)malloc(sizeof(tt_q38_stream_probe));
	if (!request) return B_NO_MEMORY;
	status=user_memcpy(request,data,sizeof(*request));
	if (status!=B_OK || !PioneerQ38StreamValid(*request)) { free(request); return status!=B_OK?status:B_BAD_VALUE; }
	status=AcquireProbeLock(cookie,true); if (status!=B_OK) { free(request); return status; }
	auto* c=cookie->context;
	if (!c || sRuntimeOwner!=cookie || !c->bankStorageOnly || c->panelProbe.saved) status=B_NOT_ALLOWED;
#ifdef PIONEER_Q38_SPAN_PRELOAD
	else if (c->q38Span && !c->q38Span->closed) status=B_NOT_ALLOWED;
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_COHORT
	else if (c->q38Cohort) status=B_NOT_ALLOWED;
#endif
	else {
		auto& dram=c->baseline.baseline.baseline.baseline.dram; auto& noc=c->baseline.baseline.baseline.noc;
		PioneerQ38StreamExtent extent;
		if (c->bytes!=PioneerResidentBanks::Total || !PioneerQ38StreamAuthorize(c->banks,dram,c->initialized,
			uint32(c->bytes/4096),*request,extent)) status=B_ENTRY_NOT_FOUND;
		else {
			request->result={}; request->result.version=1; request->result.state=TT_RUNTIME_CONFIG_FAILED;
			for (unsigned row=0;row<TT_Q38_STREAM_OUTPUT_ROWS;row++) request->output[row]=0;
			bool ready=c->lease.ready;
			if (!ready && !c->lease.saved) ready=PioneerResidentLeaseBegin(c->io,noc,c->lease);
			ready=ready && c->io.SelectRuntimeCore(0) && c->io.MapQ38Panel()
				&& PioneerQ38StreamPrepare(c->io,dram,noc,c->lease,c->banks,c->streamProbe);
			if (ready) PioneerQ38StreamRun(c->io,dram,noc,c->lease,c->banks,c->streamProbe,c->initialized,uint32(c->bytes/4096),*request);
			if (c->Unsafe() || request->result.recovery_required) {
				c->banks.Quarantine(); c->streamProbe.unsafe=true; c->lease.recovery_required=true;
				c->io.QuarantineHostPage(); request->result.recovery_required=1; sColdRecoveryRequired=true;
			}
			status=user_memcpy(data,request,sizeof(*request));
		}
	}
	release_sem(sLock); free(request); return status;
}
// PIONEER_Q38_STREAM_CONTROL_END
#endif

#if defined(TT_PIONEER_EXPERIMENTAL_Q38_COHORT) && !defined(TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH)
// PIONEER_Q38_COHORT_PREPARE_BEGIN
static bool
PrepareQ38Cohort(PioneerStoreContext& c,unsigned workers)
{
	auto& dram=c.baseline.baseline.baseline.baseline.dram; auto& noc=c.baseline.baseline.baseline.noc;
	if (!workers || workers>120 || !c.io.SelectRuntimeCore(0)) return false;
	bool ready=c.lease.ready;
	if (!ready && !c.lease.saved) ready=PioneerResidentLeaseBegin(c.io,noc,c.lease);
	if (!ready || !c.io.MapRuntime()) return false;
	if (!c.q38Cohort) {
		c.q38Cohort=new(std::nothrow) PioneerQ38CohortLease;
		if (!c.q38Cohort) return false;
		c.io.fleetEnabled=true; c.q38Cohort->worker[0].master=&c.lease; c.q38Cohort->worker[0].noc=noc;
	}
	if (!c.q38CohortRoutes) {
		ready=BlackholeDramAllCoresHeld(c.io,dram);
		for (unsigned w=1;w<120 && ready;w++) ready=PioneerWorkerNocProbe(c.io,dram,w,c.q38Cohort->worker[w].noc);
		c.q38CohortRoutes=ready;
	}
	for (unsigned w=0;w<workers && ready;w++) {
		auto& node=c.q38Cohort->worker[w];
		ready=c.io.SelectRuntimeCore(w) && c.io.MapQ38Panel();
		if (!ready) break;
		if (c.q38Cohort->prepared<w+1) c.q38Cohort->prepared=w+1;
		if (w && !node.Store().ready) ready=!node.Store().saved && PioneerQ38CohortPeerLeaseBegin(c.io,dram,w,node);
		if (ready) ready=PioneerQ38CohortPrepareWorker(c.io,dram,c.banks,node,w);
	}
	return ready;
}
// PIONEER_Q38_COHORT_PREPARE_END
// PIONEER_Q38_COHORT_CONTROL_BEGIN
static status_t
Q38CohortControl(PioneerProbeCookie* cookie,uint32 operation,void* data,size_t length)
{
	if (!data) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	if (operation==TT_PCI_PROBE_Q38_COHORT_SUPPORT) {
		if (length!=sizeof(tt_q38_cohort_support)) return B_BAD_VALUE;
		status=AcquireProbeLock(cookie,true); if (status!=B_OK) return status;
		const tt_q38_cohort_support support={TT_Q38_COHORT_DRIVER,TT_Q38_COHORT_ABI,120,256,17408,7,64,0};
		status=user_memcpy(data,&support,sizeof(support)); release_sem(sLock); return status;
	}
	if (operation!=TT_PCI_PROBE_Q38_COHORT_RUN || length!=sizeof(tt_q38_cohort_probe)) return B_BAD_VALUE;
	auto* request=(tt_q38_cohort_probe*)malloc(sizeof(tt_q38_cohort_probe)); if (!request) return B_NO_MEMORY;
	status=user_memcpy(request,data,sizeof(*request));
	if (status!=B_OK || !PioneerQ38CohortValid(*request)) { free(request); return status!=B_OK?status:B_BAD_VALUE; }
	status=AcquireProbeLock(cookie,true); if (status!=B_OK) { free(request); return status; }
	auto* c=cookie->context;
	if (!c || sRuntimeOwner!=cookie || !c->bankStorageOnly || c->panelProbe.saved || c->streamProbe.saved) status=B_NOT_ALLOWED;
#ifdef PIONEER_Q38_SPAN_PRELOAD
	else if (c->q38Span && !c->q38Span->closed) status=B_NOT_ALLOWED;
#endif
	else {
		auto& dram=c->baseline.baseline.baseline.baseline.dram;
		if (c->bytes!=PioneerResidentBanks::Total || !PioneerQ38CohortAuthorize(c->banks,dram,c->initialized,
			uint32(c->bytes/4096),*request,nullptr)) status=B_ENTRY_NOT_FOUND;
		else {
			request->result={}; request->result.version=1; request->result.state=TT_RUNTIME_CONFIG_FAILED;
			for (auto& row:request->output) for (auto& value:row) value=0;
			if (PrepareQ38Cohort(*c,request->workers)) PioneerQ38CohortRun(c->io,dram,c->banks,*c->q38Cohort,
				c->initialized,uint32(c->bytes/4096),*request);
			bool unsafe=c->Unsafe() || request->result.recovery_required;
			if (!unsafe && !c->io.SelectRuntimeCore(0)) unsafe=true;
			if (unsafe) {
				if (c->q38Cohort) PioneerQ38CohortQuarantine(*c->q38Cohort,c->banks);
				c->banks.Quarantine(); c->lease.recovery_required=true; c->io.QuarantineHostPage();
				request->result.recovery_required=1; sColdRecoveryRequired=true;
				for (auto& row:request->output) for (auto& value:row) value=0;
			}
			status=user_memcpy(data,request,sizeof(*request));
		}
	}
	release_sem(sLock); free(request); return status;
}
// PIONEER_Q38_COHORT_CONTROL_END
#endif

#ifdef TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH
// PIONEER_Q38_DISPATCH_PREPARE_BEGIN
static bool
PrepareQ38Dispatch(PioneerStoreContext& c)
{
#ifdef PIONEER_Q38_SPAN_PRELOAD
	if (c.q38Span && !c.q38Span->closed) return false;
	c.io.q38SpanAllowed=false; // one-way ownership handoff, even if prepare fails
#endif
	auto& dram=c.baseline.baseline.baseline.baseline.dram; auto& noc=c.baseline.baseline.baseline.noc;
	if (c.q38Dispatch && c.q38Dispatch->booted) return !PioneerQ38DispatchUnsafe(*c.q38Dispatch) && !c.q38Dispatch->closed;
	if (c.q38Dispatch) return false; // no second prepare/retry after any attempt
	if (!c.io.SelectRuntimeCore(0)) return false;
	bool ready=c.lease.ready;
	if (!ready && !c.lease.saved) ready=PioneerResidentLeaseBegin(c.io,noc,c.lease);
	if (!ready || !c.io.MapRuntime()) return false;
	c.q38Dispatch=new(std::nothrow) PioneerQ38DispatchLease;
	if (!c.q38Dispatch) return false;
#ifdef PIONEER_Q38_WORK_OUTPUT_DMA
	if (!c.wideHost.ready && (c.wideHost.allocated || !PioneerPrepareWideOutput(c.io,c.host,c.bulkHost,c.wideHost))) return false;
	if (!PioneerQ38OutputDmaOwner(c.io,c.host,c.bulkHost,c.wideHost,false)) return false;
#endif
	auto& lease=*c.q38Dispatch; c.io.fleetEnabled=true; lease.worker[0].master=&c.lease; lease.worker[0].noc=noc;
	ready=BlackholeDramAllCoresHeld(c.io,dram);
	for (unsigned w=1;w<120 && ready;w++) ready=PioneerWorkerNocProbe(c.io,dram,w,lease.worker[w].noc);
	c.q38DispatchRoutes=ready;
	for (unsigned w=0;w<120 && ready;w++) {
		auto& node=lease.worker[w];
		ready=c.io.SelectRuntimeCore(w) && c.io.MapQ38Panel();
		if (ready && w) ready=PioneerQ38CohortPeerLeaseBegin(c.io,dram,w,node);
		if (ready) ready=PioneerQ38DispatchPrepareRole(c.io,dram,c.banks,lease,w);
		if (!ready) lease.prepared=w+1; // include any just-touched peer NIU
	}
	if (!ready) return PioneerQ38DispatchQuarantine(lease,c.banks);
#ifdef PIONEER_Q38_FFN_CODE_DIAGNOSTIC
#ifdef PIONEER_Q38_FFN_ROOT_LIFETIME_DIAGNOSTIC
	// PIONEER_Q38_FFN_ROOT_LIFETIME_NATIVE_OWNER_BEGIN
	// Actual retained native cookie/global-call-lock handoff. NOT model or
	// initial-input permission. ALL120 are prepared/held;the parent owns the
	// complete L1 snapshot,normal busy clock and ALL host/maps until END.
	auto retainRoot=[&](){PioneerQ38DispatchQuarantine(lease,c.banks);tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);return false;};
	const auto& rootClock=c.q38Clock.Snapshot(); // CPU snapshot ONLY
	if (sColdRecoveryRequired || sReloadQuiesced || !sReloadGeneration || !sRuntimeOwner
		|| sRuntimeOwner->magic!=0x50313030 || sRuntimeOwner->context!=&c || !sBankStorageOnly
		|| !c.bankStorageOnly || c.bytes!=PioneerResidentBanks::Total || c.q38Dispatch!=&lease || !c.initialized
		|| c.Unsafe() || c.host.state!=TT_HOST_PAGE_READY || c.host.released
		|| !c.bulkHost.allocated || !c.bulkHost.ready || c.bulkHost.pending || c.bulkHost.quarantined || c.bulkHost.released
		|| !c.wideHost.allocated || !c.wideHost.ready || c.wideHost.pending || c.wideHost.quarantined || c.wideHost.released
		|| !c.q38Clock.RequiresExplicitEnd() || rootClock.state!=TT_ARC_CLOCK_BUSY_VERIFIED
		|| !rootClock.busy_requested || !rootClock.busy_acknowledged || rootClock.restored) return retainRoot();
	if (c.q38FfnCode || c.q38FfnRootLifetime) return retainRoot();
	// Publish BOTH heaps on the context BEFORE any adapter/device access.
	c.q38FfnCode=new(std::nothrow) PioneerQ38FfnGraphCodeOwner;
	c.q38FfnRootLifetime=new(std::nothrow) PioneerQ38FfnRootLifetimeOwner;
	PioneerQ38FfnRootLifetimeResult rootResult;
	bool rootReady=c.q38FfnCode && c.q38FfnRootLifetime && c.io.SelectRuntimeCore(0) && c.io.MapFfnGraphCodePrefix()
		&& c.q38FfnRootLifetime->Qualify(c.io,dram,c.banks,lease,*c.q38FfnCode,rootResult);
	if (!rootReady) {
		retainRoot(); // MEMORY ONLY:clock+ALL hosts+cold latch NOW,NO rollback
		dprintf("P100A_FFN_ROOT_LIFETIME_FAIL phase%u worker%u word%u saved%u installed%u restored%u frames%u/%u meta%u/%u stack%u/%u ready%u root%u/%u stopped%u held%u;CLOSE_ONLY_NO_RETRY_RESET_RESTORE\n",rootResult.phase,rootResult.worker,rootResult.word,rootResult.savedWords,rootResult.installedWords,rootResult.restoredWords,rootResult.savedFrameWords,rootResult.restoredFrameWords,rootResult.savedRootWords,rootResult.restoredRootWords,rootResult.savedStackWords,rootResult.restoredStackWords,rootResult.ready,rootResult.rootStarted,rootResult.rootCompleted,rootResult.stopped,rootResult.held);
		return false;
	}
	dprintf("P100A_FFN_ROOT_LIFETIME_PASS root1 workers119 saved%u installed%u restored%u frames%u/%u meta%u/%u stack%u/%u ready%u root%u/%u stopped%u held%u rootHeld%u read%u write%u;NEW_ROOT_AND_WORKERS_EXECUTED_STOP_ONLY_NOT_MODEL_OR_SPEED\n",rootResult.savedWords,rootResult.installedWords,rootResult.restoredWords,rootResult.savedFrameWords,rootResult.restoredFrameWords,rootResult.savedRootWords,rootResult.restoredRootWords,rootResult.savedStackWords,rootResult.restoredStackWords,rootResult.ready,rootResult.rootStarted,rootResult.rootCompleted,rootResult.stopped,rootResult.held,rootResult.rootHeld,rootResult.rootReads,rootResult.rootWrites);
	// PIONEER_Q38_FFN_ROOT_LIFETIME_NATIVE_OWNER_END
#elif defined(PIONEER_Q38_FFN_PEER_LIFETIME_DIAGNOSTIC)
	// PIONEER_Q38_FFN_PEER_LIFETIME_NATIVE_OWNER_BEGIN
	// Actual existing native cookie/global-call-lock handoff,not an IO
	// boolean. ALL120 prepared/held,normal firmware busy clock owned,ALL
	// host/maps on this context. Publish BOTH heap owners before ANY IO.
	auto retainPeers=[&](){PioneerQ38DispatchQuarantine(lease,c.banks);tt_resident_session_result retained={};RetainQ38DispatchOwner(c,retained);return false;};
	const auto& peerClock=c.q38Clock.Snapshot(); // existing retained clock,CPU ONLY
	if (sColdRecoveryRequired || sReloadQuiesced || !sReloadGeneration || !sRuntimeOwner
		|| sRuntimeOwner->magic!=0x50313030 || sRuntimeOwner->context!=&c || !sBankStorageOnly
		|| !c.bankStorageOnly || c.bytes!=PioneerResidentBanks::Total || c.q38Dispatch!=&lease || !c.initialized
		|| c.Unsafe() || c.host.state!=TT_HOST_PAGE_READY || c.host.released
		|| !c.bulkHost.allocated || !c.bulkHost.ready || c.bulkHost.pending || c.bulkHost.quarantined || c.bulkHost.released
		|| !c.wideHost.allocated || !c.wideHost.ready || c.wideHost.pending || c.wideHost.quarantined || c.wideHost.released
		|| !c.q38Clock.RequiresExplicitEnd() || peerClock.state!=TT_ARC_CLOCK_BUSY_VERIFIED
		|| !peerClock.busy_requested || !peerClock.busy_acknowledged || peerClock.restored) return retainPeers();
	if (c.q38FfnCode || c.q38FfnPeerLifetime) {
		return retainPeers();
	}
	c.q38FfnCode=new(std::nothrow) PioneerQ38FfnGraphCodeOwner;
	c.q38FfnPeerLifetime=new(std::nothrow) PioneerQ38FfnPeerLifetimeOwner;
	PioneerQ38FfnPeerLifetimeResult peerResult;
	bool peersReady=c.q38FfnCode && c.q38FfnPeerLifetime && c.io.SelectRuntimeCore(0) && c.io.MapFfnGraphCodePrefix()
		&& c.q38FfnPeerLifetime->Qualify(c.io,dram,c.banks,lease,*c.q38FfnCode,peerResult);
	if (!peersReady) {
		retainPeers(); // clock+ALL host owners+cold latch in MEMORY ONLY,now
		dprintf("P100A_FFN_PEER_LIFETIME_FAIL phase%u worker%u word%u saved%u installed%u restored%u frames%u/%u armed%u released%u ready%u stop%u stopped%u held%u;CLOSE_ONLY_NO_RETRY_RESET_RESTORE\n",peerResult.phase,peerResult.worker,peerResult.word,peerResult.savedWords,peerResult.installedWords,peerResult.restoredWords,peerResult.savedFrameWords,peerResult.restoredFrameWords,peerResult.armed,peerResult.released,peerResult.ready,peerResult.stopIssued,peerResult.stopped,peerResult.held);
		return false;
	}
	dprintf("P100A_FFN_PEER_LIFETIME_PASS workers119 saved%u installed%u restored%u frames%u/%u armed%u released%u ready%u stop%u stopped%u held%u;NEW_WORKERS_EXECUTED_ROOT_NEVER_LAUNCHED_NOT_MODEL_OR_SPEED\n",peerResult.savedWords,peerResult.installedWords,peerResult.restoredWords,peerResult.savedFrameWords,peerResult.restoredFrameWords,peerResult.armed,peerResult.released,peerResult.ready,peerResult.stopIssued,peerResult.stopped,peerResult.held);
	// PIONEER_Q38_FFN_PEER_LIFETIME_NATIVE_OWNER_END
#else
	// PIONEER_Q38_FFN_CODE_NATIVE_OWNER_BEGIN
	// Same native cookie/global-lock/exclusive context as the old prepare.
	// Every participant is held;the new code is NEVER bootstrapped/started.
	// Prefix mapping+complete saved owner live on this retained context,not
	// a local destructor that could free resources after an unknown result.
	if (c.q38FfnCode) {
		PioneerQ38DispatchQuarantine(lease,c.banks);c.io.QuarantineHostPage();return false;
	}
	c.q38FfnCode=new(std::nothrow) PioneerQ38FfnGraphCodeOwner;
	PioneerQ38FfnGraphCodeResult codeResult;
	bool codeReady=c.q38FfnCode && c.io.SelectRuntimeCore(0) && c.io.MapFfnGraphCodePrefix()
		&& PioneerQ38FfnGraphCodeRoundTrip(c.io,dram,c.banks,lease,*c.q38FfnCode,codeResult);
	if (!codeReady) {
		PioneerQ38DispatchQuarantine(lease,c.banks);c.io.QuarantineHostPage();
		dprintf("P100A_FFN_CODE_FAIL phase%u worker%u word%u saved%u installed%u restored%u;CLOSE_ONLY_NO_START_RETRY\n",codeResult.phase,codeResult.worker,codeResult.word,codeResult.savedWords,codeResult.installedWords,codeResult.restoredWords);
		return false;
	}
	dprintf("P100A_FFN_CODE_PASS workers120 saved%u installed%u restored%u;NEVER_LAUNCHED_NEW_FFN_NOT_MODEL_OR_SPEED\n",codeResult.savedWords,codeResult.installedWords,codeResult.restoredWords);
	// PIONEER_Q38_FFN_CODE_NATIVE_OWNER_END
#endif
#endif
#ifdef PIONEER_Q38_GRAPH_CSR_DIAGNOSTIC
	// Native caller is the EXISTING cookie/global-lock/exclusive cohort
	// handoff. No new FFN image is installed or accepted through this ABI.
	// Before bootstrap every saved participant is held;after bootstrap every
	// peer is at its exact current idle WAIT. The helper never resets or
	// releases anything. Failure returns into the existing CLOSE-ONLY/cold
	// latch,retaining the entire context and ALL host/clock/mapping owners.
	PioneerQ38GraphCsrResult heldCsr;
	if (!PioneerQ38GraphCsrProbe(c.io,dram,c.banks,lease,heldCsr)) {
		dprintf("P100A_GRAPH_CSR_FAIL live0 worker%u group%u packets%u words%u;CLOSE_ONLY_NO_RETRY\n",heldCsr.lastWorker,heldCsr.lastGroup,heldCsr.packets,heldCsr.words);
		return false;
	}
	if (!PioneerQ38DispatchBootstrap(c.io,dram,c.banks,lease)) return false;
	PioneerQ38GraphCsrResult liveCsr;
	if (!PioneerQ38GraphCsrProbe(c.io,dram,c.banks,lease,liveCsr)) {
		dprintf("P100A_GRAPH_CSR_FAIL live1 worker%u group%u packets%u words%u;CLOSE_ONLY_NO_RETRY\n",liveCsr.lastWorker,liveCsr.lastGroup,liveCsr.packets,liveCsr.words);
		return false;
	}
	dprintf("P100A_GRAPH_CSR_PASS held%u live%u workers119 packets%u words%u;EXISTING_LEGACY_COHORT_FIXED_REGISTERS_ONLY_NOT_NEW_FFN_IMAGE_OR_SPEED\n",heldCsr.passed,liveCsr.passed,heldCsr.packets+liveCsr.packets,heldCsr.words+liveCsr.words);
	return true;
#else
	return PioneerQ38DispatchBootstrap(c.io,dram,c.banks,lease);
#endif
}
// PIONEER_Q38_DISPATCH_PREPARE_END
// PIONEER_Q38_DISPATCH_CONTROL_BEGIN
static status_t
Q38DispatchControl(PioneerProbeCookie* cookie,uint32 operation,void* data,size_t length)
{
	if (!data) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	// PIONEER_Q38_WORK_SUPPORT_BEGIN
#ifdef TT_PIONEER_Q38_WORK_MODE
	if (operation==TT_PCI_PROBE_Q38_WORK_SUPPORT) {
		if (length!=sizeof(tt_q38_dispatch_support)) return B_BAD_VALUE;
		status=AcquireProbeLock(cookie,true); if (status!=B_OK) return status;
		const tt_q38_dispatch_support support={TT_Q38_DISPATCH_DRIVER,TT_Q38_WORK_SUPPORT_VERSION,119,256,17408,7,64,1,0};
		status=user_memcpy(data,&support,sizeof(support)); release_sem(sLock); return status;
	}
#endif
	// PIONEER_Q38_WORK_SUPPORT_END
	if (operation==TT_PCI_PROBE_Q38_DISPATCH_SUPPORT) {
		if (length!=sizeof(tt_q38_dispatch_support)) return B_BAD_VALUE;
		status=AcquireProbeLock(cookie,true); if (status!=B_OK) return status;
		const tt_q38_dispatch_support support={TT_Q38_DISPATCH_DRIVER,1,119,256,17408,7,64,1,0};
		status=user_memcpy(data,&support,sizeof(support)); release_sem(sLock); return status;
	}
	if ((operation!=TT_PCI_PROBE_Q38_DISPATCH_RUN
#ifdef TT_PIONEER_Q38_WORK_MODE
		&& operation!=TT_PCI_PROBE_Q38_WORK_RUN
#endif
		) || length!=sizeof(tt_q38_dispatch_probe)) return B_BAD_VALUE;
	auto* request=(tt_q38_dispatch_probe*)malloc(sizeof(tt_q38_dispatch_probe)); if (!request) return B_NO_MEMORY;
	status=user_memcpy(request,data,sizeof(*request));
	if (status!=B_OK || !PioneerQ38DispatchValid(*request)) { free(request); return status!=B_OK?status:B_BAD_VALUE; }
	status=AcquireProbeLock(cookie,true
#ifdef PIONEER_Q38_REUSE_ACTIVATIONS
		,true
#endif
	); if (status!=B_OK) { free(request); return status; }
	auto* c=cookie->context;
	if (!c || sRuntimeOwner!=cookie || !c->bankStorageOnly || c->bytes!=PioneerResidentBanks::Total
		|| c->q38Cohort || c->panelProbe.saved || c->streamProbe.saved) status=B_NOT_ALLOWED;
#ifdef PIONEER_Q38_SPAN_PRELOAD
	else if (c->q38Span && !c->q38Span->closed) status=B_NOT_ALLOWED;
#endif
	else {
		auto& dram=c->baseline.baseline.baseline.baseline.dram; bool authorized=true;
		// Complete ALL logical initialized extents BEFORE prepare/boot/IO.
		for (unsigned w=0;w<request->peers;w++) {
			PioneerQ38CohortExtent extent;
			authorized &= PioneerQ38CohortAuthorizePart(c->banks,dram,c->initialized,uint32(c->bytes/4096),
				request->type,request->inputs,request->part[w],w+1,extent);
		}
		if (!authorized) status=B_ENTRY_NOT_FOUND;
		else {
			request->result={}; request->result.version=1; request->result.state=TT_RUNTIME_CONFIG_FAILED;
			for (auto& row:request->output) for (auto& value:row) value=0;
			bool prepared=PrepareQ38Dispatch(*c);
#ifdef PIONEER_Q38_REUSE_ACTIVATIONS
			if (prepared) c->q38Dispatch->inputEpoch=sQ38InputEpoch;
#endif
			if (prepared) {
#ifdef TT_PIONEER_Q38_WORK_MODE
				if (operation==TT_PCI_PROBE_Q38_WORK_RUN) PioneerQ38DispatchRun<decltype(c->io),true>(c->io,dram,c->banks,*c->q38Dispatch,c->initialized,uint32(c->bytes/4096),*request
#ifdef PIONEER_Q38_WORK_OUTPUT_DMA
					,&c->host,&c->bulkHost,&c->wideHost
#endif
					);
				else
#endif
				PioneerQ38DispatchRun(c->io,dram,c->banks,*c->q38Dispatch,c->initialized,uint32(c->bytes/4096),*request);
			}
			bool unsafe=!prepared || c->Unsafe() || request->result.recovery_required;
			if (!unsafe && !c->io.SelectRuntimeCore(0)) unsafe=true;
			if (unsafe) {
				if (c->q38Dispatch) PioneerQ38DispatchQuarantine(*c->q38Dispatch,c->banks);
				c->banks.Quarantine(); c->lease.recovery_required=true; c->io.QuarantineHostPage(); sColdRecoveryRequired=true;
				request->result.state=TT_RUNTIME_COMPLETION_FAILED; request->result.recovery_required=1;
				for (auto& row:request->output) for (auto& value:row) value=0;
			}
			status=user_memcpy(data,request,sizeof(*request));
		}
	}
	release_sem(sLock); free(request); return status;
}
// PIONEER_Q38_DISPATCH_CONTROL_END
#endif

static status_t
ResidentStreamControl(PioneerProbeCookie* cookie,void* data,size_t length)
{
	if (data==NULL || length!=sizeof(tt_pioneer_resident_stream)) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	tt_pioneer_resident_stream* request=(tt_pioneer_resident_stream*)malloc(sizeof(tt_pioneer_resident_stream));
	if (!request) return B_NO_MEMORY;
	status=user_memcpy(request,data,sizeof(*request));
	if (status!=B_OK || request->version!=1 || request->reserved) { free(request); return status!=B_OK?status:B_BAD_VALUE; }
	status=AcquireProbeLock(cookie); if (status!=B_OK) { free(request); return status; }
	PioneerStoreContext* context=cookie->context;
	if (!context || sRuntimeOwner!=cookie) status=B_DEV_NOT_READY;
	else if (context->runtimeCoreMode==4 || context->runtimeCoreMode==6 || context->runtimeCoreMode==7 || context->runtimeCoreMode==8 || context->runtimeCoreMode==9) status=B_NOT_ALLOWED;
	else if (!PioneerResidentOffset(context->bytes,request->offset)
		|| (context->residentAttention.reserved && request->offset>=context->bytes-TT_RESIDENT_KV_BYTES)) status=B_BAD_VALUE;
	else {
		request->result={}; request->result.version=1; request->result.state=TT_STORE_CONFIG_FAILED;
		bool ready=context->lease.ready;
		if (!ready && !context->lease.saved) ready=PioneerResidentLeaseBegin(context->io,context->baseline.baseline.baseline.noc,context->lease);
		bool passed=ready && PioneerResidentStreamPage(context->io,context->baseline.baseline.baseline.baseline.dram,
			context->baseline.baseline.baseline.noc,context->host,context->lease,context->bytes,request->offset,request->data,request->result);
		unsigned page=unsigned(request->offset/4096);
		if (request->result.wrote_device && context->initialized[page]) { context->initialized[page]=0; context->uploadedPages--; }
		if (passed) { context->hashes[page]=PioneerResidentPageHash(request->data); context->initialized[page]=1; context->uploadedPages++; }
		if (context->lease.recovery_required || context->lease.pending || request->result.recovery_required) {
			context->lease.recovery_required=true; context->io.QuarantineHostPage(); request->result.recovery_required=1; sColdRecoveryRequired=true;
		}
		status=user_memcpy(data,request,sizeof(*request));
	}
	release_sem(sLock); free(request); return status;
}

static status_t
BulkUploadControl(PioneerProbeCookie* cookie,void* data,size_t length)
{
	if (!data || length!=sizeof(tt_pioneer_bulk_upload)) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	auto* request=(tt_pioneer_bulk_upload*)malloc(sizeof(tt_pioneer_bulk_upload));
	if (!request) return B_NO_MEMORY;
	status=user_memcpy(request,data,sizeof(*request));
	if (status!=B_OK || request->version!=1 || request->reserved[0] || request->reserved[1]
		|| request->pages<1 || request->pages>4) { free(request); return status!=B_OK?status:B_BAD_VALUE; }
	status=AcquireProbeLock(cookie); if (status!=B_OK) { free(request); return status; }
	PioneerStoreContext* context=cookie->context;
	if (!context || sRuntimeOwner!=cookie) status=B_DEV_NOT_READY;
	else if (context->runtimeCoreMode!=-1 && context->runtimeCoreMode!=3 && context->runtimeCoreMode!=5 && context->runtimeCoreMode!=8) status=B_NOT_ALLOWED;
	else if (!PioneerBulkUploadRange(context->bytes,request->offset,request->pages)
		|| (context->residentAttention.reserved && request->offset+request->pages*4096>context->bytes-TT_RESIDENT_KV_BYTES)) status=B_BAD_VALUE;
	else {
		request->result={}; request->result.transfer.version=1; request->result.transfer.state=TT_STORE_CONFIG_FAILED;
		bool ready=context->lease.ready;
		if (!ready && !context->lease.saved) ready=PioneerResidentLeaseBegin(context->io,context->baseline.baseline.baseline.noc,context->lease);
		if (ready && context->runtimeCoreMode==3) ready=PioneerRuntimeAttentionPeersHeld(context->io,
			context->baseline.baseline.baseline.baseline.dram,context->baseline.baseline.baseline.noc,context->secondNoc,
			context->lease,context->secondLease,context->compute,context->secondCompute,context->attention);
		if (ready && context->runtimeCoreMode==5) ready=context->residentAttention.reserved
			?PioneerFleetMasterBoundary(context->io,context->baseline.baseline.baseline.baseline.dram,context->fleet,context->attention.ready)
			:FleetHeld(*context);
		if (ready && context->runtimeCoreMode==8) {
			ready=PioneerCoordinatedMasterBoundary(context->io,context->baseline.baseline.baseline.baseline.dram,context->fleet,context->attention.ready);
			for (unsigned p=0;p<request->pages;p++) ready &= !context->initialized[request->offset/4096+p];
			ready &= request->offset+request->pages*4096<=context->bytes-TT_RESIDENT_KV_BYTES;
		}
		if (ready) ready=context->io.SelectRuntimeCore(0) && context->io.MapBulk();
		if (ready && !context->bulkHost.allocated) ready=PioneerPrepareOutputBuffer(context->io,context->host,context->bulkHost);
		ready=ready && context->bulkHost.ready && PioneerBulkUploadPrepare(context->io,
			context->baseline.baseline.baseline.baseline.dram,context->baseline.baseline.baseline.noc,
			context->host,context->lease,context->bulkHost,context->bulk);
		bool passed=ready && PioneerBulkUpload(context->io,context->baseline.baseline.baseline.baseline.dram,
			context->baseline.baseline.baseline.noc,context->host,context->lease,context->bulkHost,context->bulk,
			context->bytes,request->offset,request->pages,request->data,request->result);
		unsigned first=unsigned(request->offset/4096);
		for (unsigned p=0;p<request->pages;p++) {
			if (request->result.transfer.wrote_device && context->initialized[first+p]) {
				context->initialized[first+p]=0; context->uploadedPages--;
			}
			if (passed) {
				context->hashes[first+p]=PioneerResidentPageHash(request->data+p*1024);
				context->initialized[first+p]=1; context->uploadedPages++;
			}
		}
		if (context->Unsafe() || request->result.transfer.recovery_required) {
			context->bulk.unsafe=true; context->lease.recovery_required=true;
			context->io.QuarantineHostPage(); PioneerQuarantineOutputBuffer(context->io,context->bulkHost);
			request->result.transfer.recovery_required=1; sColdRecoveryRequired=true;
		}
		status=user_memcpy(data,request,sizeof(*request));
	}
	release_sem(sLock); free(request); return status;
}

static status_t
RuntimeMatvecControl(PioneerProbeCookie* cookie,void* data,size_t length,bool dmaOutput,bool fp16,bool int8,bool q8=false,bool compact=false,bool native=false,bool f32Scales=false,bool bulk=false,bool second=false,bool dual=false,bool reuse=false)
{
	size_t bytes=dual?sizeof(tt_pioneer_runtime_dual_matvec):q8?sizeof(tt_pioneer_runtime_q8_matvec):sizeof(tt_pioneer_runtime_matvec);
	if (data==NULL || length!=bytes) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	tt_pioneer_runtime_matvec* request=(tt_pioneer_runtime_matvec*)malloc(bytes);
	if (!request) return B_NO_MEMORY;
	status=user_memcpy(request,data,bytes);
	const uint32_t* scales=q8?((tt_pioneer_runtime_q8_matvec*)request)->scale:NULL;
	if (status!=B_OK || request->version!=1 || request->reserved || !(dual?PioneerRuntimeDualShape(request->inputs,request->outputs):compact?PioneerQ8CompactShape(request->inputs,request->outputs):PioneerMatvecShape(request->inputs,request->outputs))) {
		free(request); return status!=B_OK?status:B_BAD_VALUE;
	}
	status=AcquireProbeLock(cookie); if (status!=B_OK) { free(request); return status; }
	PioneerStoreContext* context=cookie->context;
	if (!context || sRuntimeOwner!=cookie) status=B_DEV_NOT_READY;
	else if (!(compact?PioneerQ8CompactWeightRange(context->bytes,request->weight_offset,request->inputs,request->outputs):PioneerMatvecWeightRange(context->bytes,request->weight_offset,request->inputs,request->outputs))) status=B_BAD_VALUE;
	else {
		uint64 weightBytes=compact?PioneerQ8CompactWeightBytes(request->inputs,request->outputs):PioneerMatvecWeightBytes(request->inputs,request->outputs);
		bool initialized=true;
		for (uint64 page=request->weight_offset/4096;page<(request->weight_offset+weightBytes+4095)/4096;page++)
			initialized &= context->initialized[page]!=0;
		if (!initialized) status=B_ENTRY_NOT_FOUND;
		else {
			request->result={}; request->result.version=1; request->result.state=TT_RUNTIME_CONFIG_FAILED;
			if (dual) memset(((tt_pioneer_runtime_dual_matvec*)request)->core,0,sizeof(((tt_pioneer_runtime_dual_matvec*)request)->core));
			memset(request->output,0,sizeof(request->output));
			context->io.runtimeProfile.Begin(uint64(system_time()),reuse?11:dual?10:second?9:bulk?8:f32Scales?7:native?6:compact?5:q8?4:int8?3:fp16?2:dmaOutput?1:0);
			int coreMode=reuse?3:dual?2:int(second);
			bool ready=context->runtimeCoreMode<0 || context->runtimeCoreMode==coreMode;
			ready=ready && (context->lease.ready || (!context->lease.saved
				&& PioneerResidentLeaseBegin(context->io,context->baseline.baseline.baseline.noc,context->lease)));
			if (ready) ready=context->io.MapRuntime(bulk);
			if (ready && second && !context->secondNoc.version)
				ready=BlackholeComputeNocProbe(context->io,context->baseline.baseline.baseline.baseline.dram,context->secondNoc,3);
			if (ready && second) ready=context->secondNoc.state==TT_COMPUTE_NOC_PASSED
				&& !context->secondNoc.recovery_required && context->secondNoc.tlb_restored
				&& context->io.AllocateSecondRuntime() && context->io.SelectRuntimeCore(1);
			auto& compute=second?context->secondCompute:context->compute;
			auto& lease=second?context->secondLease:context->lease;
			auto& noc=second?context->secondNoc:context->baseline.baseline.baseline.noc;
			if (ready && second && !lease.ready) ready=!lease.saved && PioneerResidentLeaseBegin(context->io,noc,lease,3);
			if (ready) context->runtimeCoreMode=coreMode;
			if (ready) ready=PioneerRuntimeMatvecPrepare(context->io,context->baseline.baseline.baseline.baseline.dram,
				noc,lease,compute,fp16,int8,q8,compact,native,f32Scales,bulk);
			if (ready && dual) ready=context->io.SelectRuntimeCore(0)
				&& PioneerRuntimeMatvecPrepare(context->io,context->baseline.baseline.baseline.baseline.dram,
					context->baseline.baseline.baseline.noc,context->lease,context->compute,false,true,true,true,true,true,true);
			if (ready && context->attention.saved) ready=reuse
				&& PioneerRuntimeAttentionSelect(context->io,context->baseline.baseline.baseline.baseline.dram,
					context->baseline.baseline.baseline.noc,context->secondNoc,context->lease,context->secondLease,
					context->compute,context->secondCompute,context->attention,false);
			if (ready) context->io.RuntimeProfileMark(TT_PROFILE_PREPARED);
			if (ready && dual) {
				auto& parts=((tt_pioneer_runtime_dual_matvec*)request)->core;
				PioneerRuntimeDualMatvec(context->io,context->baseline.baseline.baseline.baseline.dram,
					context->baseline.baseline.baseline.noc,context->secondNoc,context->host,context->lease,context->secondLease,
					context->compute,context->secondCompute,context->bytes,request->weight_offset,request->inputs,request->outputs,
					request->input,scales,request->output,parts,reuse?context->activation:NULL);
				auto& total=request->result; total=parts[0];
				total.state=parts[0].state==TT_RUNTIME_PASSED?parts[1].state:parts[0].state;
				total.prepared=parts[0].prepared && parts[1].prepared;
				total.started=parts[0].started && parts[1].started; total.completed=parts[0].completed && parts[1].completed;
				total.stopped=parts[0].stopped && parts[1].stopped; total.recovery_required=parts[0].recovery_required || parts[1].recovery_required;
				total.staged_pages+=parts[1].staged_pages; total.tiles+=parts[1].tiles; total.polls+=parts[1].polls; total.nonfinite+=parts[1].nonfinite;
				for (unsigned w=0;w<4;w++) total.after[w]+=parts[1].after[w]-parts[1].before[w];
			} else if (ready) PioneerRuntimeMatvec(context->io,context->baseline.baseline.baseline.baseline.dram,
				noc,context->host,lease,compute,context->bytes,
				request->weight_offset,request->inputs,request->outputs,request->input,request->output,request->result,dmaOutput,scales,second?3:2);
			if (second && !context->io.SelectRuntimeCore(0)) context->secondLease.recovery_required=true;
			if (context->Unsafe()) {
				context->lease.recovery_required=true; context->io.QuarantineHostPage();
				request->result.recovery_required=1; sColdRecoveryRequired=true;
			}
			const auto& r=request->result;
			context->io.runtimeProfile.End(uint64(system_time()),r.state,r.recovery_required,r.started,r.completed,r.stopped);
			status=user_memcpy(data,request,dual?bytes:sizeof(*request));
		}
	}
	release_sem(sLock); free(request); return status;
}

static status_t
CoordinatedSupportControl(PioneerProbeCookie* cookie,void* data,size_t length)
{
	if (!data || length!=sizeof(tt_coordinated_support)) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	status=AcquireProbeLock(cookie); if (status!=B_OK) return status;
	// CPU metadata only. No map/read/write/probe/start/reset/DMA operation.
	tt_coordinated_support support={43,119,3,1,8192}; // v43 exact unpack source Wait Gate;SAME math bytes except ONE scheduling word/owners/proofs
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_BANKS
	support.version=TT_Q38_BANK_DRIVER; // separate experimental build; NEVER masquerade as qualified43
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_PANEL
	support.version=46; // local panel arithmetic probe,NOT full model support
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_AFFINE
	support.version=47; // separate scaled-local-panel probe,NOT full model support
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_BLOCK
	support.version=48; // complete LOCAL quantization block,NOT fullK/model support
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_STREAM
	support.version=TT_Q38_STREAM_DRIVER; // guarded fullK primitive,NOT model/Assistant support
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_COHORT
	support.version=TT_Q38_COHORT_DRIVER; // separate shared-input cohort qualification,NOT inference
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH
	support.version=TT_Q38_DISPATCH_DRIVER; // separate persistent dispatch,NOT model qualification
#endif
	status=user_memcpy(data,&support,sizeof(support)); release_sem(sLock); return status;
}

static bool
PrepareCoordinatedFleet(PioneerStoreContext& c,unsigned workers,bool debug,bool parallel=false,bool glu=false,bool inlineMath=false,bool partialMath=false,bool cachedQuery=false,bool inputPull=false,bool phasePipeline=false,bool descriptorTable=false,bool accumPipeline=false,bool smallRows=false,bool parallelRope=false,const PioneerDecoderJob* model=NULL,bool fusedGather=false,bool vectorGlu=false,bool roundedProduct=false,bool inlineAttention=false,bool optimizedAttention=false,bool finiteHalf=false,bool unpackWaitGate=false)
{
	if (unpackWaitGate && (!finiteHalf || roundedProduct)) return false;
	if (finiteHalf && !optimizedAttention) return false;
	if (optimizedAttention && !inlineAttention) return false;
	if (inlineAttention && (!vectorGlu || roundedProduct)) return false;
	if (roundedProduct && (!vectorGlu || !accumPipeline || partialMath)) return false;
	if (vectorGlu && (!parallelRope || !glu)) return false;
	if (fusedGather && (!parallelRope || smallRows)) return false;
	if (parallelRope && (!accumPipeline || smallRows || !c.io.peerRopeEnabled)) return false;
	if ((smallRows && !accumPipeline) || (accumPipeline && !descriptorTable)) return false;
	if (descriptorTable && (!model || !phasePipeline || !inputPull || !cachedQuery || !inlineMath || partialMath || !glu || !parallel)) return false;
	bool ready=c.lease.ready && c.io.MapRuntime(true);
	if (ready && !c.fleet) {
		c.fleet=new(std::nothrow) PioneerFleetWorker[TT_PIONEER_WORKERS]; ready=c.fleet!=NULL;
		if (ready) {
			c.runtimeCoreMode=8; c.io.fleetEnabled=true; c.io.persistentHostStages=true; c.io.payloadBlocks=true;
			c.fleet[0].masterStore=&c.lease; c.fleet[0].masterCompute=&c.compute;
			c.fleet[0].noc=c.baseline.baseline.baseline.noc;
		}
	}
	auto& dram=c.baseline.baseline.baseline.baseline.dram;
	if (ready && !c.fleetRoutes) {
		ready=BlackholeDramAllCoresHeld(c.io,dram);
		for (unsigned w=1;w<TT_PIONEER_WORKERS && ready;w++) ready=PioneerWorkerNocProbe(c.io,dram,w,c.fleet[w].noc);
		c.fleetRoutes=ready;
	}
	if (ready) {
		auto& f=c.fleet[0];
		ready=c.io.AllocateFleetWorker(0) && c.io.SelectRuntimeCore(0)
			&& PioneerRuntimeMatvecPrepare(c.io,dram,f.noc,f.Store(),f.Compute(),false,true,true,true,true,true,true)
			&& PioneerFleetPreludePrepare(c.io,f) && c.io.MapCoordinator()
			&& PioneerCoordinatorPrepare(c.io,dram,f.noc,f.Store(),c.coordinator,true,debug,!debug,!debug);
	}
	if (ready) ready=PioneerCoordinatedMasterBoundary(c.io,dram,c.fleet,c.attention.ready);
	for (unsigned w=1;w<=workers && ready;w++) {
		auto& f=c.fleet[w];
		if (f.persistentActive) { ready=f.coordinated && f.persistentIdle && !f.Compute().running && f.attentionDispatch==(parallel && w<=8) && f.gluDispatch==(glu && w<=8) && f.inlineMath==inlineMath && f.coordinatedInline==inlineMath && f.partialMath==partialMath && f.phasePipeline==phasePipeline && f.descriptorTable==descriptorTable && f.accumPipeline==accumPipeline && f.roundedProduct==roundedProduct && f.unpackWaitGate==unpackWaitGate && f.smallRows==smallRows && f.fusedGather==fusedGather && (!descriptorTable || f.descriptorTablePrepared) && (w>8 || (c.peerAttention[w-1].cachedQuery==cachedQuery && c.peerAttention[w-1].inputPull==inputPull && c.peerAttention[w-1].parallelRope==parallelRope && c.peerAttention[w-1].vectorGlu==vectorGlu && c.peerAttention[w-1].inlineAttention==inlineAttention && c.peerAttention[w-1].optimizedAttention==optimizedAttention && c.peerAttention[w-1].finiteHalf==finiteHalf)); continue; }
		ready=c.io.AllocateFleetWorker(w) && c.io.SelectRuntimeCore(w);
		if (ready && !f.Store().ready) ready=!f.Store().saved && PioneerFleetLeaseBegin(c.io,dram,w,f);
		if (ready) ready=PioneerRuntimeMatvecPrepare(c.io,dram,f.noc,f.Store(),f.Compute(),false,true,true,true,true,true,true)
			&& PioneerFleetPreludePrepare(c.io,f);
		if (ready && parallel && w<=8) {
			ready=c.io.MapAttention(parallelRope) && PioneerAttentionTilePrepare(c.io,dram,f.noc,f.Store(),c.peerAttention[w-1],false,true,glu,cachedQuery,inputPull,parallelRope,vectorGlu,inlineAttention,optimizedAttention,finiteHalf);
			// The group entry returns to the persistent prelude;boot still
			// starts at that prelude,never directly into an attention job.
			if (!PioneerAttentionTileHeld(c.io,dram,f.noc,f.Store())) { c.peerAttention[w-1].unsafe=true; ready=false; }
			else { c.io.RuntimeCodeWrite(0,0x0006b06f); ready &= c.io.RuntimeCodeRead(0)==0x0006b06f; }
		}
		if (ready) ready=PioneerPersistentPrepare(c.io,f,inlineMath,true,parallel && w<=8,glu && w<=8,inlineMath,partialMath,phasePipeline,descriptorTable,accumPipeline,fusedGather,roundedProduct,unpackWaitGate)
			&& (!descriptorTable || PioneerPersistentTablePrepare(c.io,dram,f,*model,w,smallRows)) && PioneerPersistentBoot(c.io,f);
	}
	return ready && c.io.SelectRuntimeCore(0);
}

static status_t
DecoderModelControl(PioneerProbeCookie* cookie,void* data,size_t length)
{
	if (!data || length!=sizeof(tt_pioneer_decoder_model)) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	auto* request=(tt_pioneer_decoder_model*)malloc(length); if (!request) return B_NO_MEMORY;
	status=user_memcpy(request,data,length);
	if (status!=B_OK || request->version<1 || request->version>27 || (request->version<17 && (request->reserved[0] || request->reserved[1] || request->reserved[2]))) { free(request); return status!=B_OK?status:B_BAD_VALUE; }
	status=AcquireProbeLock(cookie); if (status!=B_OK) { free(request); return status; }
	auto* c=cookie->context;
	if (!c || sRuntimeOwner!=cookie) status=B_DEV_NOT_READY;
	else if (c->runtimeCoreMode!=8 || c->decoder.saved || c->decoderReady || c->bytes!=UINT64_C(0x80000000) || !c->fleet
		|| !c->bulk.ready || !c->bulkHost.ready || !c->wideHost.ready || c->residentAttention.reserved) status=B_NOT_ALLOWED;
	else if (request->version>=17?!PioneerDecoderBatchModelRegister(request->offsets,request->reserved[0],request->reserved[1],request->reserved[2],c->initialized,c->bytes,c->decoderModel)
		:!PioneerDecoderModelRegister(request->offsets,c->initialized,c->bytes,c->decoderModel)) status=B_ENTRY_NOT_FOUND;
	else {
		request->state=TT_RUNTIME_CONFIG_FAILED; request->ready=request->extents=request->recovery_required=0;
		auto& dram=c->baseline.baseline.baseline.baseline.dram;
		bool parallel=request->version>=2,glu=request->version>=3,greedy=request->version>=4,paired=request->version>=5,inlineMath=request->version>=6,partialMath=request->version==7,cachedQuery=request->version>=8,inputPull=request->version==9 || request->version==11 || request->version>=12,phasePipeline=request->version==10 || request->version>=12,diagnostic=request->version==11,descriptorTable=request->version>=13,accumPipeline=request->version>=14,smallRows=request->version==15,parallelRope=request->version>=16,batch=request->version>=17,fusedGather=request->version>=18,directQuant=request->version>=19,vectorGlu=request->version>=20,roundedProduct=request->version==21,inlineAttention=request->version>=22,optimizedMaster=request->version>=23,optimizedAttention=request->version>=24,finiteHalf=request->version>=25,fixedRoutes=request->version>=26,unpackWaitGate=request->version==27,ready=true;
		// The parallel registry must precede ANY peer preparation. Weight
		// spans prepare only master0. Never replace an executing/idle peer's
		// immutable code to upgrade an existing matrix session.
		if (parallel) for (unsigned w=1;w<120;w++) ready &= !c->fleet[w].Compute().saved && !c->fleet[w].persistentPrepared && !c->fleet[w].persistentActive;
		if (ready) { c->io.peerAttentionEnabled=parallel; c->io.peerRopeEnabled=parallelRope; ready=PrepareCoordinatedFleet(*c,119,false,parallel,glu,inlineMath,partialMath,cachedQuery,inputPull,phasePipeline,descriptorTable,accumPipeline,smallRows,parallelRope,descriptorTable?&c->decoderModel:NULL,fusedGather,vectorGlu,roundedProduct,inlineAttention,optimizedAttention,finiteHalf,unpackWaitGate); }
		ready=ready && PioneerCoordinatedMasterBoundary(c->io,dram,c->fleet,c->attention.ready);
		// One-way seal:all legacy mutable/runtime operations reject mode9,
		// including both legacy page/stream paths. END still live-stops ALL
		// persistent workers before restoring either separate code owner.
		c->runtimeCoreMode=9;
		ready=ready && PioneerLayerClose(c->io,dram,c->fleet[0].noc,c->lease,c->layer,true)
			&& c->io.MapAttention() && PioneerAttentionTilePrepare(c->io,dram,c->fleet[0].noc,c->lease,c->attention,true)
			&& c->io.MapDecoder() && PioneerDecoderPrepare(c->io,dram,c->fleet,c->coordinator,c->layer,c->attention,c->decoder,parallel,parallel?c->peerAttention:NULL,parallel,glu,greedy,paired,inlineMath,partialMath,cachedQuery,inputPull,phasePipeline,diagnostic,descriptorTable,accumPipeline,smallRows,parallelRope,batch,fusedGather,directQuant,vectorGlu,roundedProduct,inlineAttention,optimizedMaster,optimizedAttention,finiteHalf,fixedRoutes,unpackWaitGate);
		if (ready && (!PioneerCoordinatedMasterBoundary(c->io,dram,c->fleet,true)
			|| !PioneerWideOutputMapStable(c->io,c->wideHost,c->host,c->bulkHost))) { c->decoder.unsafe=true; ready=false; }
		if (ready) {
			c->residentAttention.reserved=c->residentAttention.payloadBlocks=c->residentAttention.coordinated=true;
			c->decoderReady=true; request->state=TT_RUNTIME_PASSED; request->ready=1; request->extents=batch?311:310;
		}
		if (c->Unsafe()) { c->lease.recovery_required=true; c->io.QuarantineHostPage(); sColdRecoveryRequired=true; request->recovery_required=1; }
		status=user_memcpy(data,request,length);
	}
	release_sem(sLock); free(request); return status;
}

template<class Run> static status_t
DecoderRunControlImpl(PioneerProbeCookie* cookie,void* data,size_t length,bool greedy,bool batch=false)
{
	if (!data || length!=sizeof(Run)) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	auto* request=(Run*)malloc(length); if (!request) return B_NO_MEMORY;
	status=user_memcpy(request,data,offsetof(Run,result));
	if (status!=B_OK || request->version!=1 || !request->tokens || request->tokens>256 || (batch?(!request->reserved[0] || request->reserved[0]>2 || request->reserved[0]>257-request->tokens):request->reserved[0]!=0) || request->reserved[1]) { free(request); return status!=B_OK?status:B_BAD_VALUE; }
	if (batch) for (unsigned i=0;i<128;i++) if (request->rope[i]) { free(request); return B_BAD_VALUE; }
	status=AcquireProbeLock(cookie); if (status!=B_OK) { free(request); return status; }
	auto* c=cookie->context;
	if (!c || sRuntimeOwner!=cookie) status=B_DEV_NOT_READY;
	else if (c->runtimeCoreMode!=9 || !c->decoderReady || !c->decoder.ready || !c->fleet || (greedy && !c->decoder.greedy) || (batch && !c->decoder.batch)) status=B_NOT_ALLOWED;
	else {
		request->result={}; memset(request->output,0,sizeof(request->output)); c->io.runtimeProfile.Begin(uint64(system_time()),21);
		c->io.RuntimeProfileMark(TT_PROFILE_PREPARED);
		PioneerDecoderRun(c->io,c->baseline.baseline.baseline.baseline.dram,c->host,c->bulkHost,c->wideHost,c->fleet,c->bulk,
			c->coordinator,c->decoder,c->attention,c->residentAttention,c->decoderModel,request->tokens,request->embedding,request->rope,request->output,request->result,greedy,batch?request->reserved[0]:0);
		if (c->Unsafe()) { c->lease.recovery_required=true; c->io.QuarantineHostPage(); sColdRecoveryRequired=true; request->result.master.recovery_required=1; }
		auto& r=request->result.master; c->io.runtimeProfile.End(uint64(system_time()),r.state,r.recovery_required,r.started,r.completed,r.stopped);
		if (batch?!PioneerDecoderBatchProof(request->result,request->tokens,request->output[3],c->coordinator.nonce,c->decoder)
			:!PioneerDecoderProof(request->result,request->tokens,c->coordinator.nonce,c->decoder,greedy)) memset(request->output,0,sizeof(request->output));
		status=user_memcpy(data,request,length);
	}
	release_sem(sLock); free(request); return status;
}
static status_t DecoderRunControl(PioneerProbeCookie* cookie,void* data,size_t length)
{ return DecoderRunControlImpl<tt_pioneer_decoder_run>(cookie,data,length,false); }
static status_t DecoderGreedyControl(PioneerProbeCookie* cookie,void* data,size_t length)
{ return DecoderRunControlImpl<tt_pioneer_decoder_greedy>(cookie,data,length,true); }
static status_t DecoderBatchControl(PioneerProbeCookie* cookie,void* data,size_t length)
{ return DecoderRunControlImpl<tt_pioneer_decoder_batch>(cookie,data,length,false,true); }

static status_t
DecoderStageProfileControl(PioneerProbeCookie* cookie,void* data,size_t length)
{
	if (!data || length!=sizeof(tt_decoder_stage_profile)) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	status=AcquireProbeLock(cookie); if (status!=B_OK) return status;
	auto* c=cookie->context;
	// PRIVATE CPU snapshot only. No map,MMIO,DMA,probe,launch or reset.
	if (!c || sRuntimeOwner!=cookie || c->runtimeCoreMode!=9 || !c->decoderReady || !c->decoder.profiling
		|| c->decoder.running || c->decoder.unsafe || c->decoder.stages.magic!=0x31525044) status=B_DEV_NOT_READY;
	else status=user_memcpy(data,&c->decoder.stages,sizeof(c->decoder.stages));
	release_sem(sLock); return status;
}

static status_t
LayerSpanControl(PioneerProbeCookie* cookie,void* data,size_t length)
{
	const size_t prefix=offsetof(tt_pioneer_layer_span,data);
	if (!data || length<prefix || length>sizeof(tt_pioneer_layer_span)) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	auto* request=(tt_pioneer_layer_span*)malloc(length); if (!request) return B_NO_MEMORY;
	// Freeze the complete request in ONE user copy; no double-fetch size or
	// later caller mutation can change the kernel-private upload/comparison.
	status=user_memcpy(request,data,length);
	if (status!=B_OK || request->version!=1 || request->reserved[0] || request->reserved[1]
		|| request->payload_reserved || length!=PioneerSpanRequestBytes(request->bytes)) { free(request); return status!=B_OK?status:B_BAD_VALUE; }
	status=AcquireProbeLock(cookie); if (status!=B_OK) { free(request); return status; }
	auto* c=cookie->context;
	if (!c || sRuntimeOwner!=cookie) status=B_DEV_NOT_READY;
	else if ((c->runtimeCoreMode!=-1 && c->runtimeCoreMode!=8) || c->bytes!=UINT64_C(0x80000000)
		|| (c->coordinator.ready && (!c->coordinator.fused || !c->coordinator.compact))) status=B_NOT_ALLOWED;
	else if (!PioneerResidentOffset(c->bytes,request->offset) || request->offset>=c->bytes-TT_RESIDENT_KV_BYTES
		|| request->bytes>c->bytes-TT_RESIDENT_KV_BYTES-request->offset) status=B_BAD_VALUE;
	else {
		unsigned first=unsigned(request->offset/4096),pages=request->bytes/4096; bool fresh=true;
		for (unsigned p=0;p<pages;p++) fresh &= !c->initialized[first+p];
		if (!fresh) status=B_NOT_ALLOWED;
		else {
			request->result={}; request->result.master.version=1; request->result.master.state=TT_RUNTIME_CONFIG_FAILED;
			c->io.runtimeProfile.Begin(uint64(system_time()),19); auto& dram=c->baseline.baseline.baseline.baseline.dram;
			bool ready=c->lease.ready || (!c->lease.saved && PioneerResidentLeaseBegin(c->io,c->baseline.baseline.baseline.noc,c->lease));
			if (ready && c->runtimeCoreMode==8) ready=PioneerCoordinatedMasterBoundary(c->io,dram,c->fleet,c->attention.ready);
			if (ready) ready=c->io.SelectRuntimeCore(0) && c->io.MapBulk();
			if (ready && !c->bulkHost.allocated) ready=PioneerPrepareOutputBuffer(c->io,c->host,c->bulkHost);
			ready=ready && c->bulkHost.ready && PioneerBulkUploadPrepare(c->io,dram,c->baseline.baseline.baseline.noc,
				c->host,c->lease,c->bulkHost,c->bulk);
			ready=ready && PrepareCoordinatedFleet(*c,0,false) && PioneerCoordinatedMasterBoundary(c->io,dram,c->fleet,c->attention.ready)
				&& c->io.MapLayer() && PioneerLayerPrepare(c->io,dram,c->fleet[0].noc,c->lease,c->coordinator,c->layer);
			if (ready && !c->wideHost.ready) ready=!c->wideHost.allocated && PioneerPrepareWideOutput(c->io,c->host,c->bulkHost,c->wideHost);
			if (ready) {
				c->io.RuntimeProfileMark(TT_PROFILE_PREPARED);
				PioneerSpanUpload(c->io,dram,c->host,c->bulkHost,c->wideHost,c->fleet,c->bulk,c->coordinator,c->layer,c->attention.ready,
					c->bytes,request->offset,request->bytes,request->data,request->result);
			}
			if (c->Unsafe()) {
				c->layer.unsafe=c->coordinator.unsafe=c->bulk.unsafe=true; c->lease.recovery_required=true; c->io.QuarantineHostPage();
				PioneerQuarantineOutputBuffer(c->io,c->bulkHost); PioneerQuarantineWideOutput(c->io,c->wideHost);
				sColdRecoveryRequired=true; request->result.master.recovery_required=1;
			}
			auto& r=request->result.master;
			c->io.runtimeProfile.End(uint64(system_time()),r.state,r.recovery_required,r.started,r.completed,r.stopped);
			// Only the COMPLETE transport proof and independent frozen CPU
			// echo permit ALL pages to become initialized. Never partial bits.
			if (PioneerSpanProof(request->result,request->bytes,c->coordinator.nonce)) for (unsigned p=0;p<pages;p++) {
				c->hashes[first+p]=PioneerResidentPageHash(request->data+p*1024); c->initialized[first+p]=1; c->uploadedPages++;
			}
			// Return metadata only,not a redundant1MiB payload copy.
			status=user_memcpy(data,request,prefix);
		}
	}
	release_sem(sLock); free(request); return status;
}

static status_t
LayerWideControl(PioneerProbeCookie* cookie,void* data,size_t length,bool narrow=false)
{
	const size_t prefix=offsetof(tt_pioneer_coordinated_fleet,output);
	if (!data || length<prefix || length>sizeof(tt_pioneer_coordinated_fleet)) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	auto* request=(tt_pioneer_coordinated_fleet*)malloc(length); if (!request) return B_NO_MEMORY;
	status=user_memcpy(request,data,prefix); PioneerFleetPlan plan;
	if (status!=B_OK || request->version!=1 || !request->workers || request->workers>119
		|| length!=PioneerCoordinatedRequestBytes(request->outputs)
		|| (narrow && request->outputs>4096) || !PioneerFleetPartition(request->inputs,request->outputs,request->workers,plan)) {
		free(request); return status!=B_OK?status:B_BAD_VALUE;
	}
	status=AcquireProbeLock(cookie); if (status!=B_OK) { free(request); return status; }
	auto* c=cookie->context; uint64 bytes=PioneerQ8CompactWeightBytes(request->inputs,request->outputs);
	if (!c || sRuntimeOwner!=cookie) status=B_DEV_NOT_READY;
	else if ((c->runtimeCoreMode!=-1 && c->runtimeCoreMode!=8) || c->bytes!=UINT64_C(0x80000000)
		|| !c->bulk.ready || !c->bulkHost.ready || (c->coordinator.ready && (!c->coordinator.fused || !c->coordinator.compact))) status=B_NOT_ALLOWED;
	else if (!PioneerResidentOffset(c->bytes,request->weight_offset) || !bytes || request->weight_offset>=c->bytes-TT_RESIDENT_KV_BYTES
		|| bytes>c->bytes-TT_RESIDENT_KV_BYTES-request->weight_offset) status=B_BAD_VALUE;
	else {
		bool initialized=true;
		for (uint64 page=request->weight_offset/4096;page<(request->weight_offset+bytes+4095)/4096;page++) initialized &= c->initialized[page]!=0;
		if (!initialized) status=B_ENTRY_NOT_FOUND;
		else {
			request->result={}; request->result.master.version=1; request->result.master.state=TT_RUNTIME_CONFIG_FAILED;
			memset(request->output,0,request->outputs*4); c->io.runtimeProfile.Begin(uint64(system_time()),narrow?20:18);
			auto& dram=c->baseline.baseline.baseline.baseline.dram;
			bool ready=PrepareCoordinatedFleet(*c,plan.workers,false)
				&& PioneerCoordinatedMasterBoundary(c->io,dram,c->fleet,c->attention.ready)
				&& c->io.MapLayer() && PioneerLayerPrepare(c->io,dram,c->fleet[0].noc,c->lease,c->coordinator,c->layer);
			if (ready && !narrow && !c->wideHost.ready) ready=!c->wideHost.allocated && PioneerPrepareWideOutput(c->io,c->host,c->bulkHost,c->wideHost);
			if (ready) {
				c->io.RuntimeProfileMark(TT_PROFILE_PREPARED);
				PioneerLayerJob job(&c->layer,0,0,narrow?3:6,narrow?NULL:&c->wideHost);
				PioneerCoordinatedMatvec(c->io,dram,c->host,c->bulkHost,c->fleet,c->bulk,c->coordinator,c->attention.ready,c->bytes,
					request->weight_offset,request->inputs,request->outputs,request->workers,request->input,request->scale,request->output,request->result,&job);
			}
			if (c->Unsafe()) {
				c->layer.unsafe=true; c->coordinator.unsafe=true; c->lease.recovery_required=true; c->io.QuarantineHostPage();
				PioneerQuarantineOutputBuffer(c->io,c->bulkHost); PioneerQuarantineWideOutput(c->io,c->wideHost);
				sColdRecoveryRequired=true; request->result.master.recovery_required=1;
			}
			auto& r=request->result.master;
			c->io.runtimeProfile.End(uint64(system_time()),r.state,r.recovery_required,r.started,r.completed,r.stopped);
			if (!(narrow?PioneerLayerNarrowProof(request->result,request->inputs,request->outputs,request->workers,c->coordinator.nonce)
				:PioneerLayerWideProof(request->result,request->inputs,request->outputs,request->workers,c->coordinator.nonce))) memset(request->output,0,request->outputs*4);
			status=user_memcpy(data,request,length);
		}
	}
	release_sem(sLock); free(request); return status;
}

static status_t
LayerGluControl(PioneerProbeCookie* cookie,void* data,size_t length)
{
	if (!data || length!=sizeof(tt_pioneer_layer_glu)) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	auto* request=(tt_pioneer_layer_glu*)malloc(length); if (!request) return B_NO_MEMORY;
	status=user_memcpy(request,data,offsetof(tt_pioneer_layer_glu,result));
	if (status!=B_OK || request->version!=1 || !request->outputs || request->outputs>3072 || (request->outputs&31)
		|| request->reserved[0] || request->reserved[1]) { free(request); return status!=B_OK?status:B_BAD_VALUE; }
	status=AcquireProbeLock(cookie); if (status!=B_OK) { free(request); return status; }
	auto* c=cookie->context;
	if (!c || sRuntimeOwner!=cookie) status=B_DEV_NOT_READY;
	else if ((c->runtimeCoreMode!=-1 && c->runtimeCoreMode!=8) || c->bytes!=UINT64_C(0x80000000)
		|| !c->bulk.ready || !c->bulkHost.ready || (c->coordinator.ready && (!c->coordinator.fused || !c->coordinator.compact))) status=B_NOT_ALLOWED;
	else {
		request->result={}; request->result.master.version=1; request->result.master.state=TT_RUNTIME_CONFIG_FAILED;
		memset(request->output,0,sizeof(request->output)); c->io.runtimeProfile.Begin(uint64(system_time()),17);
		auto& dram=c->baseline.baseline.baseline.baseline.dram;
		bool ready=PrepareCoordinatedFleet(*c,0,false) && PioneerCoordinatedMasterBoundary(c->io,dram,c->fleet,c->attention.ready)
			&& c->io.MapLayer() && PioneerLayerPrepare(c->io,dram,c->fleet[0].noc,c->lease,c->coordinator,c->layer);
		if (ready) PioneerLayerGlu(c->io,dram,c->host,c->bulkHost,c->fleet,c->bulk,c->coordinator,c->layer,c->attention.ready,
			request->outputs,request->gate,request->up,request->output,request->result);
		if (c->Unsafe()) {
			c->layer.unsafe=true; c->coordinator.unsafe=true; c->lease.recovery_required=true; c->io.QuarantineHostPage();
			PioneerQuarantineOutputBuffer(c->io,c->bulkHost); sColdRecoveryRequired=true; request->result.master.recovery_required=1;
		}
		auto& r=request->result.master;
		c->io.runtimeProfile.End(uint64(system_time()),r.state,r.recovery_required,r.started,r.completed,r.stopped);
		if (!PioneerLayerGluProof(request->result,request->outputs,c->coordinator.nonce)) memset(request->output,0,sizeof(request->output));
		status=user_memcpy(data,request,length);
	}
	release_sem(sLock); free(request); return status;
}

static status_t
LayerFfnControl(PioneerProbeCookie* cookie,void* data,size_t length)
{
	if (!data || length!=sizeof(tt_pioneer_layer_ffn)) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	auto* request=(tt_pioneer_layer_ffn*)malloc(length); if (!request) return B_NO_MEMORY;
	status=user_memcpy(request,data,offsetof(tt_pioneer_layer_ffn,result));
	if (status!=B_OK || request->version!=1 || !request->workers || request->workers>119 || request->reserved[0] || request->reserved[1]) {
		free(request); return status!=B_OK?status:B_BAD_VALUE;
	}
	status=AcquireProbeLock(cookie); if (status!=B_OK) { free(request); return status; }
	auto* c=cookie->context;
	if (!c || sRuntimeOwner!=cookie) status=B_DEV_NOT_READY;
	else if ((c->runtimeCoreMode!=-1 && c->runtimeCoreMode!=8) || c->bytes!=UINT64_C(0x80000000)
		|| !c->bulk.ready || !c->bulkHost.ready || (c->coordinator.ready && (!c->coordinator.fused || !c->coordinator.compact))) status=B_NOT_ALLOWED;
	else {
		const uint64 offsets[]={request->gate_offset,request->up_offset,request->down_offset};
		const uint64 bytes=PioneerQ8CompactWeightBytes(1024,3072);
		bool bounded=true,initialized=true;
		for (unsigned i=0;i<3 && bounded;i++) {
			uint64 at=offsets[i];
			bounded=PioneerResidentOffset(c->bytes,at) && at<c->bytes-TT_RESIDENT_KV_BYTES && bytes<=c->bytes-TT_RESIDENT_KV_BYTES-at;
			if (bounded) for (uint64 page=at/4096;page<(at+bytes+4095)/4096;page++) initialized &= c->initialized[page]!=0;
		}
		if (!bounded) status=B_BAD_VALUE;
		else if (!initialized) status=B_ENTRY_NOT_FOUND;
		else {
			request->result={}; request->result.master.version=1; request->result.master.state=TT_RUNTIME_CONFIG_FAILED;
			memset(request->output,0,sizeof(request->output));
			c->io.runtimeProfile.Begin(uint64(system_time()),16);
			auto& dram=c->baseline.baseline.baseline.baseline.dram;
			unsigned workers=request->workers<96?request->workers:96;
			bool ready=PrepareCoordinatedFleet(*c,workers,false)
				&& PioneerCoordinatedMasterBoundary(c->io,dram,c->fleet,c->attention.ready)
				&& c->io.MapLayer() && PioneerLayerPrepare(c->io,dram,c->fleet[0].noc,c->lease,c->coordinator,c->layer);
			if (ready) {
				c->io.RuntimeProfileMark(TT_PROFILE_PREPARED);
				PioneerLayerJob job={&c->layer,request->up_offset,request->down_offset};
				PioneerCoordinatedMatvec(c->io,dram,c->host,c->bulkHost,c->fleet,c->bulk,c->coordinator,c->attention.ready,
					c->bytes,request->gate_offset,1024,1024,request->workers,request->input,request->scale,request->output,request->result,&job);
			}
			if (c->Unsafe()) {
				c->layer.unsafe=true; c->coordinator.unsafe=true; c->lease.recovery_required=true; c->io.QuarantineHostPage();
				PioneerQuarantineOutputBuffer(c->io,c->bulkHost); sColdRecoveryRequired=true; request->result.master.recovery_required=1;
			}
			auto& r=request->result.master;
			c->io.runtimeProfile.End(uint64(system_time()),r.state,r.recovery_required,r.started,r.completed,r.stopped);
			if (!PioneerCoordinatedLayerProof(request->result,request->workers,c->coordinator.nonce)) memset(request->output,0,sizeof(request->output));
			status=user_memcpy(data,request,length);
		}
	}
	release_sem(sLock); free(request); return status;
}

static status_t
CoordinatedFleetControl(PioneerProbeCookie* cookie,void* data,size_t length)
{
	const size_t prefix=offsetof(tt_pioneer_coordinated_fleet,output);
	if (!data || length<prefix || length>sizeof(tt_pioneer_coordinated_fleet)) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	auto* request=(tt_pioneer_coordinated_fleet*)malloc(length); if (!request) return B_NO_MEMORY;
	status=user_memcpy(request,data,prefix);
	PioneerFleetPlan plan;
	if (status!=B_OK || (request->version!=1 && request->version!=2) || !request->workers || request->workers>119
		|| length!=PioneerCoordinatedRequestBytes(request->outputs)
		|| !PioneerFleetPartition(request->inputs,request->outputs,request->workers,plan)) {
		free(request); return status!=B_OK?status:B_BAD_VALUE;
	}
	status=AcquireProbeLock(cookie); if (status!=B_OK) { free(request); return status; }
	auto* context=cookie->context;
	uint64 bytes=PioneerQ8CompactWeightBytes(request->inputs,request->outputs);
	if (!context || sRuntimeOwner!=cookie) status=B_DEV_NOT_READY;
	else if ((context->runtimeCoreMode!=-1 && context->runtimeCoreMode!=8) || context->bytes!=UINT64_C(0x80000000)
		|| !context->bulk.ready || !context->bulkHost.ready) status=B_NOT_ALLOWED;
	else if (context->coordinator.ready && context->coordinator.fused!=(request->version==1)) status=B_NOT_ALLOWED;
	else if (!PioneerResidentOffset(context->bytes,request->weight_offset) || !bytes
		|| request->weight_offset>=context->bytes-TT_RESIDENT_KV_BYTES
		|| bytes>context->bytes-TT_RESIDENT_KV_BYTES-request->weight_offset) status=B_BAD_VALUE;
	else {
		bool initialized=true;
		for (uint64 page=request->weight_offset/4096;page<(request->weight_offset+bytes+4095)/4096;page++) initialized &= context->initialized[page]!=0;
		if (!initialized) status=B_ENTRY_NOT_FOUND;
		else {
			request->result={}; request->result.master.version=1; request->result.master.state=TT_RUNTIME_CONFIG_FAILED;
			memset(request->output,0,request->outputs*4);
			context->io.runtimeProfile.Begin(uint64(system_time()),15);
			bool ready=context->lease.ready && context->io.MapRuntime(true);
			if (ready && !context->fleet) {
				context->fleet=new(std::nothrow) PioneerFleetWorker[TT_PIONEER_WORKERS]; ready=context->fleet!=NULL;
				if (ready) {
					context->runtimeCoreMode=8; context->io.fleetEnabled=true;
					context->io.persistentHostStages=true; context->io.payloadBlocks=true;
					context->fleet[0].masterStore=&context->lease; context->fleet[0].masterCompute=&context->compute;
					context->fleet[0].noc=context->baseline.baseline.baseline.noc;
				}
			}
			auto& dram=context->baseline.baseline.baseline.baseline.dram;
			if (ready && !context->fleetRoutes) {
				ready=BlackholeDramAllCoresHeld(context->io,dram);
				for (unsigned w=1;w<TT_PIONEER_WORKERS && ready;w++) ready=PioneerWorkerNocProbe(context->io,dram,w,context->fleet[w].noc);
				context->fleetRoutes=ready;
			}
			if (ready) {
				auto& f=context->fleet[0];
				ready=context->io.AllocateFleetWorker(0) && context->io.SelectRuntimeCore(0)
					&& PioneerRuntimeMatvecPrepare(context->io,dram,f.noc,f.Store(),f.Compute(),false,true,true,true,true,true,true)
					&& PioneerFleetPreludePrepare(context->io,f) && context->io.MapCoordinator()
					&& PioneerCoordinatorPrepare(context->io,dram,f.noc,f.Store(),context->coordinator,true,request->version==2,request->version==1,request->version==1);
			}
			if (ready) ready=PioneerCoordinatedMasterBoundary(context->io,dram,context->fleet,context->attention.ready);
			for (unsigned w=1;w<=plan.workers && ready;w++) {
				auto& f=context->fleet[w];
				if (f.persistentActive) { ready=f.coordinated && f.persistentIdle && !f.Compute().running; continue; }
				ready=context->io.AllocateFleetWorker(w) && context->io.SelectRuntimeCore(w);
				if (ready && !f.Store().ready) ready=!f.Store().saved && PioneerFleetLeaseBegin(context->io,dram,w,f);
				if (ready) ready=PioneerRuntimeMatvecPrepare(context->io,dram,f.noc,f.Store(),f.Compute(),false,true,true,true,true,true,true)
					&& PioneerFleetPreludePrepare(context->io,f) && PioneerPersistentPrepare(context->io,f,false,true)
					&& PioneerPersistentBoot(context->io,f);
			}
			if (ready) {
				context->io.RuntimeProfileMark(TT_PROFILE_PREPARED);
				PioneerCoordinatedMatvec(context->io,dram,context->host,context->bulkHost,context->fleet,context->bulk,context->coordinator,
					context->attention.ready,context->bytes,request->weight_offset,request->inputs,request->outputs,request->workers,
					request->input,request->scale,request->output,request->result);
			}
			if (context->Unsafe()) {
				context->coordinator.unsafe=true; context->lease.recovery_required=true; context->io.QuarantineHostPage();
				PioneerQuarantineOutputBuffer(context->io,context->bulkHost); sColdRecoveryRequired=true;
				request->result.master.recovery_required=1;
			}
			auto& r=request->result.master;
			context->io.runtimeProfile.End(uint64(system_time()),r.state,r.recovery_required,r.started,r.completed,r.stopped);
			if (!PioneerCoordinatedProof(request->result,request->inputs,request->outputs,request->workers,context->coordinator.nonce,request->version==2))
				memset(request->output,0,request->outputs*4);
			status=user_memcpy(data,request,length);
		}
	}
	release_sem(sLock); free(request); return status;
}

static status_t
RuntimeFleetControl(PioneerProbeCookie* cookie,void* data,size_t length)
{
	const size_t prefix=sizeof(tt_pioneer_runtime_fleet)-TT_FLEET_OUTPUTS_MAX*sizeof(uint32_t);
	if (!data || length<prefix || length>sizeof(tt_pioneer_runtime_fleet)) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	// Allocate the checked inline message, not a fresh1MiB maximum-sized
	// heap block for every tiny projection. All later accesses are bounded
	// by the immutable copied header and the exact length equality below.
	auto* request=(tt_pioneer_runtime_fleet*)malloc(length);
	if (!request) return B_NO_MEMORY;
	status=user_memcpy(request,data,prefix);
	PioneerFleetPlan plan;
	if (status!=B_OK || request->version!=2 || length!=PioneerFleetRequestBytes(request->outputs)
		|| !PioneerFleetPartition(request->inputs,request->outputs,request->workers,plan)) {
		free(request); return status!=B_OK?status:B_BAD_VALUE;
	}
	status=AcquireProbeLock(cookie); if (status!=B_OK) { free(request); return status; }
	PioneerStoreContext* context=cookie->context;
	uint64 bytes=PioneerQ8CompactWeightBytes(request->inputs,request->outputs);
	if (!context || sRuntimeOwner!=cookie) status=B_DEV_NOT_READY;
	else if (!PioneerResidentOffset(context->bytes,request->weight_offset) || bytes>context->bytes-request->weight_offset) status=B_BAD_VALUE;
	else {
		bool initialized=true;
		for (uint64 page=request->weight_offset/4096;page<(request->weight_offset+bytes+4095)/4096;page++) initialized &= context->initialized[page]!=0;
		if (!initialized) status=B_ENTRY_NOT_FOUND;
		else {
			memset(request->output,0,request->outputs*4); request->result={}; request->result.version=1;
			request->result.state=TT_RUNTIME_CONFIG_FAILED;
			context->io.runtimeProfile.Begin(uint64(system_time()),12);
			bool ready=context->runtimeCoreMode==-1 || context->runtimeCoreMode==5;
			ready=ready && (context->lease.ready || (!context->lease.saved
				&& PioneerResidentLeaseBegin(context->io,context->baseline.baseline.baseline.noc,context->lease)));
			if (ready) ready=context->io.MapRuntime(true);
			if (ready && !context->fleet) {
				context->fleet=new(std::nothrow) PioneerFleetWorker[TT_PIONEER_WORKERS];
				ready=context->fleet!=NULL;
				if (ready) {
					context->runtimeCoreMode=5; context->io.fleetEnabled=true;
					context->fleet[0].masterStore=&context->lease; context->fleet[0].masterCompute=&context->compute;
					context->fleet[0].noc=context->baseline.baseline.baseline.noc;
				}
			}
			auto& dram=context->baseline.baseline.baseline.baseline.dram;
			if (ready && !context->fleetRoutes) {
				ready=BlackholeDramAllCoresHeld(context->io,dram);
				for (unsigned w=1;w<TT_PIONEER_WORKERS && ready;w++)
					ready=PioneerWorkerNocProbe(context->io,dram,w,context->fleet[w].noc);
				context->fleetRoutes=ready;
			}
			for (unsigned w=0;w<plan.workers && ready;w++) {
				auto& f=context->fleet[w];
				ready=context->io.AllocateFleetWorker(w) && context->io.SelectRuntimeCore(w);
				if (ready && !f.Store().ready) ready=!f.Store().saved && PioneerFleetLeaseBegin(context->io,dram,w,f);
				if (ready) ready=PioneerRuntimeMatvecPrepare(context->io,dram,f.noc,f.Store(),f.Compute(),false,true,true,true,true,true,true);
				if (ready) ready=PioneerFleetPreludePrepare(context->io,f);
			}
			if (ready && context->attention.saved) {
				ready=context->io.SelectRuntimeCore(0);
				if (ready && context->io.RuntimeCodeRead(0)!=0x0006b06f) ready=context->residentAttention.reserved
					?SelectResidentFleetEntry(*context,false):SelectFleetEntry(*context,false);
			}
			if (ready) {
				context->io.RuntimeProfileMark(TT_PROFILE_PREPARED);
				PioneerFleetMatvec(context->io,dram,context->host,context->fleet,context->bytes,request->weight_offset,
					request->inputs,request->outputs,request->workers,request->input,request->scale,request->output,request->result);
			}
			if (!context->io.SelectRuntimeCore(0)) { context->lease.recovery_required=true; request->result.recovery_required=1; }
			if (context->Unsafe()) { context->io.QuarantineHostPage(); request->result.recovery_required=1; sColdRecoveryRequired=true; }
			auto& r=request->result;
			context->io.runtimeProfile.End(uint64(system_time()),r.state,r.recovery_required,r.started,r.completed,r.stopped);
			// Never expose even a partial output vector after failed coverage.
			if (!PioneerFleetProof(r,request->inputs,request->outputs,request->workers)) memset(request->output,0,request->outputs*4);
			status=user_memcpy(data,request,length);
		}
	}
	release_sem(sLock); free(request); return status;
}

static status_t
RuntimePersistentFleetControl(PioneerProbeCookie* cookie,void* data,size_t length)
{
	const size_t prefix=offsetof(tt_pioneer_runtime_persistent_fleet,output);
	if (!data || length<prefix || length>sizeof(tt_pioneer_runtime_persistent_fleet)) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	auto* request=(tt_pioneer_runtime_persistent_fleet*)malloc(length);
	if (!request) return B_NO_MEMORY;
	status=user_memcpy(request,data,prefix); PioneerFleetPlan plan;
	if (status!=B_OK || (request->version!=1 && request->version!=2) || length!=PioneerPersistentRequestBytes(request->outputs)
		|| !PioneerFleetPartition(request->inputs,request->outputs,request->workers,plan)) {
		free(request); return status!=B_OK?status:B_BAD_VALUE;
	}
	status=AcquireProbeLock(cookie); if (status!=B_OK) { free(request); return status; }
	PioneerStoreContext* context=cookie->context;
	uint64 bytes=PioneerQ8CompactWeightBytes(request->inputs,request->outputs);
	bool inlineMath=request->version==2; int mode=inlineMath?7:6;
	if (!context || sRuntimeOwner!=cookie) status=B_DEV_NOT_READY;
	else if (context->runtimeCoreMode!=-1 && context->runtimeCoreMode!=mode) status=B_NOT_ALLOWED;
	else if (!PioneerResidentOffset(context->bytes,request->weight_offset) || bytes>context->bytes-request->weight_offset) status=B_BAD_VALUE;
	else {
		bool initialized=true;
		for (uint64 page=request->weight_offset/4096;page<(request->weight_offset+bytes+4095)/4096;page++) initialized &= context->initialized[page]!=0;
		if (!initialized) status=B_ENTRY_NOT_FOUND;
		else {
			memset(request->output,0,request->outputs*4); request->result={}; request->result.version=1;
			request->result.state=TT_RUNTIME_CONFIG_FAILED; context->io.runtimeProfile.Begin(uint64(system_time()),13);
			bool ready=context->lease.ready || (!context->lease.saved
				&& PioneerResidentLeaseBegin(context->io,context->baseline.baseline.baseline.noc,context->lease));
			if (ready) ready=context->io.MapRuntime(true);
			if (ready && !context->fleet) {
				context->fleet=new(std::nothrow) PioneerFleetWorker[TT_PIONEER_WORKERS]; ready=context->fleet!=NULL;
				if (ready) {
					context->runtimeCoreMode=mode; context->io.fleetEnabled=true; context->io.persistentHostStages=true;
					context->fleet[0].masterStore=&context->lease; context->fleet[0].masterCompute=&context->compute;
					context->fleet[0].noc=context->baseline.baseline.baseline.noc;
				}
			}
			auto& dram=context->baseline.baseline.baseline.baseline.dram;
			if (ready && !context->fleetRoutes) {
				ready=BlackholeDramAllCoresHeld(context->io,dram);
				for (unsigned w=1;w<TT_PIONEER_WORKERS && ready;w++) ready=PioneerWorkerNocProbe(context->io,dram,w,context->fleet[w].noc);
				context->fleetRoutes=ready;
			}
			for (unsigned w=0;w<plan.workers && ready;w++) {
				auto& f=context->fleet[w];
				ready=context->io.AllocateFleetWorker(w) && context->io.SelectRuntimeCore(w);
				if (ready && !f.Store().ready) ready=!f.Store().saved && PioneerFleetLeaseBegin(context->io,dram,w,f);
				if (ready) ready=PioneerRuntimeMatvecPrepare(context->io,dram,f.noc,f.Store(),f.Compute(),false,true,true,true,true,true,true);
				if (ready && !f.preludeSaved) ready=PioneerFleetPreludePrepare(context->io,f);
				if (ready) ready=PioneerPersistentPrepare(context->io,f,inlineMath);
			}
			if (ready) {
				context->io.RuntimeProfileMark(TT_PROFILE_PREPARED);
				PioneerPersistentMatvec(context->io,dram,context->host,context->fleet,context->bytes,request->weight_offset,
					request->inputs,request->outputs,request->workers,request->input,request->scale,request->output,request->result);
			}
			if (!context->io.SelectRuntimeCore(0)) { context->lease.recovery_required=true; request->result.recovery_required=1; }
			if (context->Unsafe()) { context->io.QuarantineHostPage(); request->result.recovery_required=1; sColdRecoveryRequired=true; }
			auto& r=request->result;
			context->io.runtimeProfile.End(uint64(system_time()),r.state,r.recovery_required,r.started,r.completed,0);
			if (!PioneerPersistentProof(r,request->inputs,request->outputs,request->workers)) memset(request->output,0,request->outputs*4);
			status=user_memcpy(data,request,length);
		}
	}
	release_sem(sLock); free(request); return status;
}

static status_t
RuntimeProfileControl(PioneerProbeCookie* cookie,void* data,size_t length)
{
	if (data==NULL || length!=sizeof(tt_runtime_profile_result)) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	status=AcquireProbeLock(cookie); if (status!=B_OK) return status;
	if (!cookie->context || sRuntimeOwner!=cookie) status=B_DEV_NOT_READY;
	else status=user_memcpy(data,&cookie->context->io.runtimeProfile.Snapshot(),sizeof(tt_runtime_profile_result));
	release_sem(sLock); return status;
}

static status_t
ResidentAttentionControl(PioneerProbeCookie* cookie,void* data,size_t length)
{
	if (!data || length!=sizeof(tt_pioneer_resident_attention)) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	auto* request=(tt_pioneer_resident_attention*)malloc(length); if (!request) return B_NO_MEMORY;
	status=user_memcpy(request,data,length);
	if (status!=B_OK || (request->version!=1 && request->version!=2 && request->version!=3) || request->reserved
		|| !PioneerResidentAttentionInput(request->tokens,request->layer,request->input)) {
		free(request); return status!=B_OK?status:B_BAD_VALUE;
	}
	status=AcquireProbeLock(cookie); if (status!=B_OK) { free(request); return status; }
	auto* context=cookie->context;
	if (!context || sRuntimeOwner!=cookie) status=B_DEV_NOT_READY;
	else if ((context->runtimeCoreMode!=5 && context->runtimeCoreMode!=8) || !context->fleet || !context->compute.ready
		|| !context->uploadedPages || !PioneerResidentAttentionCapacity(context->bytes)) status=B_NOT_ALLOWED;
	else {
		memset(&request->result,0,sizeof(request->result)); memset(request->output,0,sizeof(request->output));
		auto& kv=context->residentAttention;
		bool ready=(!context->attention.saved || context->attention.resident)
			&& request->tokens<=kv.tokens[request->layer]+1
			&& PioneerResidentAttentionVersion(request->version,kv,context->runtimeCoreMode==8);
		if (ready && !kv.reserved) {
			for (uint64 page=(context->bytes-TT_RESIDENT_KV_BYTES)/4096;page<context->bytes/4096;page++)
				ready &= !context->initialized[page];
			if (ready) {
				kv.reserved=true; kv.payloadBlocks=request->version>=2; kv.coordinated=request->version==3;
				context->io.payloadBlocks=kv.payloadBlocks;
				context->io.persistentHostStages=kv.payloadBlocks;
			}
		}
		ready=ready && (context->runtimeCoreMode==8
			?PioneerCoordinatedMasterBoundary(context->io,context->baseline.baseline.baseline.baseline.dram,context->fleet,context->attention.ready)
			:PioneerFleetMasterBoundary(context->io,context->baseline.baseline.baseline.baseline.dram,context->fleet,context->attention.ready))
			&& context->io.MapAttention() && context->io.BulkMappingStable()
			&& PioneerAttentionTilePrepare(context->io,context->baseline.baseline.baseline.baseline.dram,
				context->baseline.baseline.baseline.noc,context->lease,context->attention,true)
			&& SelectResidentFleetEntry(*context,true);
		if (ready) ready=PioneerResidentAttentionRun(context->io,context->baseline.baseline.baseline.baseline.dram,
			context->baseline.baseline.baseline.noc,context->lease,context->attention,context->bulk,kv,
			context->bytes,request->tokens,request->layer,request->input,request->output,request->result);
		if (context->Unsafe()) {
			context->lease.recovery_required=true; context->io.QuarantineHostPage();
			PioneerQuarantineOutputBuffer(context->io,context->bulkHost); sColdRecoveryRequired=true;
			request->result.math.recovery_required=1;
		}
		if (!ready) memset(request->output,0,sizeof(request->output));
		status=user_memcpy(data,request,length);
	}
	release_sem(sLock); free(request); return status;
}

static status_t
CoordinatorProbeControl(PioneerProbeCookie* cookie,void* data,size_t length)
{
	if (!data || length!=sizeof(tt_pioneer_coordinator_probe)) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	tt_pioneer_coordinator_probe request={};
	status=user_memcpy(&request,data,sizeof(request));
	if (status!=B_OK || request.version!=1 || !request.workers || request.workers>119
		|| request.reserved[0] || request.reserved[1]) return status!=B_OK?status:B_BAD_VALUE;
	status=AcquireProbeLock(cookie); if (status!=B_OK) return status;
	auto* context=cookie->context;
	if (!context || sRuntimeOwner!=cookie) status=B_DEV_NOT_READY;
	else if (context->runtimeCoreMode!=5 || !context->fleet || !context->fleetRoutes
		|| !context->compute.ready || !context->bulk.ready || !context->uploadedPages) status=B_NOT_ALLOWED;
	else {
		memset(&request.result,0,sizeof(request.result));
		auto& dram=context->baseline.baseline.baseline.baseline.dram;
		bool ready=PioneerPeerPcMasterHeld(context->io,dram,context->fleet,context->attention.ready)
			&& context->io.MapCoordinator() && PioneerCoordinatorPrepare(context->io,dram,
				context->baseline.baseline.baseline.noc,context->lease,context->coordinator);
		if (ready) PioneerCoordinatorProbe(context->io,dram,context->fleet,context->bulk,
			context->coordinator,context->attention.ready,request.workers,request.result);
		if (context->Unsafe()) {
			context->lease.recovery_required=true; context->io.QuarantineHostPage();
			PioneerQuarantineOutputBuffer(context->io,context->bulkHost); sColdRecoveryRequired=true;
			request.result.recovery_required=1;
		}
		status=user_memcpy(data,&request,sizeof(request));
	}
	release_sem(sLock); return status;
}

static status_t
PeerPcReadControl(PioneerProbeCookie* cookie,void* data,size_t length)
{
	if (!data || length!=sizeof(tt_pioneer_peer_pc_read)) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	tt_pioneer_peer_pc_read request={};
	status=user_memcpy(&request,data,sizeof(request));
	if (status!=B_OK || request.version!=1 || !request.worker || request.worker>=TT_PIONEER_WORKERS
		|| request.reserved[0] || request.reserved[1]) return status!=B_OK?status:B_BAD_VALUE;
	status=AcquireProbeLock(cookie); if (status!=B_OK) return status;
	auto* context=cookie->context;
	if (!context || sRuntimeOwner!=cookie) status=B_DEV_NOT_READY;
	else if (context->runtimeCoreMode!=5 || !context->fleet || !context->fleetRoutes
		|| !context->compute.ready || !context->bulk.ready || !context->uploadedPages) status=B_NOT_ALLOWED;
	else {
		PioneerPeerPcRead(context->io,context->baseline.baseline.baseline.baseline.dram,
			context->fleet,context->bulk,context->attention.ready,request.worker,request.result);
		if (context->Unsafe()) {
			context->lease.recovery_required=true; context->io.QuarantineHostPage();
			PioneerQuarantineOutputBuffer(context->io,context->bulkHost); sColdRecoveryRequired=true;
			request.result.recovery_required=1;
		}
		status=user_memcpy(data,&request,sizeof(request));
	}
	release_sem(sLock); return status;
}

static status_t
FleetAttentionControl(PioneerProbeCookie* cookie,void* data,size_t length)
{
	const size_t fixed=sizeof(tt_pioneer_fleet_attention)-256*1024*sizeof(uint32_t);
	if (!data || length<fixed || length>sizeof(tt_pioneer_fleet_attention)) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	auto* request=(tt_pioneer_fleet_attention*)malloc(length); if (!request) return B_NO_MEMORY;
	status=user_memcpy(request,data,fixed);
	if (status!=B_OK || request->version!=1 || request->reserved[0] || request->reserved[1]
		|| length!=PioneerFleetAttentionBytes(request->tokens)) { free(request); return status!=B_OK?status:B_BAD_VALUE; }
	status=user_memcpy(request->prefix,(const uint8*)data+fixed,length-fixed);
	if (status!=B_OK) { free(request); return status; }
	for (unsigned group=0;group<8;group++) if (!PioneerAttentionTileInputValid(request->tokens,request->query+group*256,
		request->prefix+group*request->tokens*128,request->prefix+group*request->tokens*128+request->tokens*64)) { free(request); return B_BAD_VALUE; }
	status=AcquireProbeLock(cookie); if (status!=B_OK) { free(request); return status; }
	auto* context=cookie->context;
	if (!context || sRuntimeOwner!=cookie) status=B_DEV_NOT_READY;
	else if (context->runtimeCoreMode!=5 || context->residentAttention.reserved || !context->fleet || !context->compute.ready || !context->uploadedPages) status=B_NOT_ALLOWED;
	else {
		memset(request->result,0,sizeof(request->result)); memset(request->output,0,sizeof(request->output));
		bool ready=FleetHeld(*context) && context->io.MapAttention()
			&& PioneerAttentionTilePrepare(context->io,context->baseline.baseline.baseline.baseline.dram,
				context->baseline.baseline.baseline.noc,context->lease,context->attention)
			&& SelectFleetEntry(*context,true,true);
		for (unsigned group=0;group<8 && ready;group++) {
			ready=PioneerAttentionTileRun(context->io,context->baseline.baseline.baseline.baseline.dram,
				context->baseline.baseline.baseline.noc,context->lease,context->attention,request->tokens,
				request->query+group*256,request->prefix+group*request->tokens*128,
				request->prefix+group*request->tokens*128+request->tokens*64,request->output+group*256,request->result[group]);
		}
		if (context->Unsafe()) {
			context->lease.recovery_required=true; context->io.QuarantineHostPage(); sColdRecoveryRequired=true;
			for (auto& r:request->result) r.recovery_required=1;
		}
		if (!ready) memset(request->output,0,sizeof(request->output));
		status=user_memcpy(data,request,length);
	}
	release_sem(sLock); free(request); return status;
}

static status_t
AttentionTileTestControl(PioneerProbeCookie* cookie,void* data,size_t length,bool shared)
{
	if (data==NULL || length!=sizeof(tt_pioneer_attention_tile_test)) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	auto* request=(tt_pioneer_attention_tile_test*)malloc(sizeof(tt_pioneer_attention_tile_test));
	if (!request) return B_NO_MEMORY;
	status=user_memcpy(request,data,sizeof(*request));
	if (status!=B_OK || request->version!=1 || request->reserved[0] || request->reserved[1]
		|| !PioneerAttentionTileInputValid(request->tokens,request->query,request->key,request->value)) {
		free(request); return status!=B_OK?status:B_BAD_VALUE;
	}
	status=AcquireProbeLock(cookie); if (status!=B_OK) { free(request); return status; }
	PioneerStoreContext* context=cookie->context;
	if (!context || sRuntimeOwner!=cookie) status=B_DEV_NOT_READY;
	else if (shared?(context->runtimeCoreMode!=3 || !context->uploadedPages || !context->compute.ready || !context->secondCompute.ready)
		:(context->uploadedPages || (context->runtimeCoreMode!=-1 && context->runtimeCoreMode!=4)
			|| context->compute.saved || context->secondCompute.saved || context->secondLease.saved)) status=B_NOT_ALLOWED;
	else {
		if (!shared) context->runtimeCoreMode=4;
		request->result={}; request->result.version=1; request->result.state=TT_ATTN_TILE_CONFIG_FAILED;
		memset(request->output,0,sizeof(request->output));
		bool ready=context->lease.ready;
		if (!ready && !context->lease.saved)
			ready=PioneerResidentLeaseBegin(context->io,context->baseline.baseline.baseline.noc,context->lease);
		if (ready && shared) ready=PioneerRuntimeAttentionPeersHeld(context->io,context->baseline.baseline.baseline.baseline.dram,
			context->baseline.baseline.baseline.noc,context->secondNoc,context->lease,context->secondLease,
			context->compute,context->secondCompute,context->attention);
		ready=ready && context->io.MapAttention()
			&& PioneerAttentionTilePrepare(context->io,context->baseline.baseline.baseline.baseline.dram,
				context->baseline.baseline.baseline.noc,context->lease,context->attention);
		if (ready && shared) ready=PioneerRuntimeAttentionSelect(context->io,context->baseline.baseline.baseline.baseline.dram,
			context->baseline.baseline.baseline.noc,context->secondNoc,context->lease,context->secondLease,
			context->compute,context->secondCompute,context->attention,true);
		if (ready) PioneerAttentionTileRun(context->io,context->baseline.baseline.baseline.baseline.dram,
			context->baseline.baseline.baseline.noc,context->lease,context->attention,
			request->tokens,request->query,request->key,request->value,request->output,request->result);
		if (context->Unsafe()) {
			context->lease.recovery_required=true; context->io.QuarantineHostPage();
			request->result.recovery_required=1; sColdRecoveryRequired=true;
		}
		status=user_memcpy(data,request,sizeof(*request));
	}
	release_sem(sLock); free(request); return status;
}

#ifdef TT_PIONEER_DRIVER_RELOAD
// PIONEER_DRIVER_RELOAD_CONTROL_BEGIN
static status_t
DriverReloadControl(PioneerProbeCookie* cookie,uint32 operation,void* data,size_t length)
{
	if (!data || length!=sizeof(tt_pioneer_driver_reload)
		|| (operation!=TT_PCI_PROBE_RELOAD_STATUS && operation!=TT_PCI_PROBE_RELOAD_QUIESCE)) return B_BAD_VALUE;
	thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	tt_pioneer_driver_reload r={}; status=user_memcpy(&r,data,sizeof(r));
	if (status!=B_OK || !PioneerDriverReloadRequest(r)) return status!=B_OK?status:B_BAD_VALUE;
	status=acquire_sem(sLock); if (status!=B_OK) return status;
	if (!sReloadGuard || sReloadGuard->version!=1 || !sReloadGeneration) status=B_DEV_NOT_READY;
	else if ((r.expected_tag && r.expected_tag!=PIONEER_DRIVER_BUILD_TAG)
		|| (r.expected_generation && r.expected_generation!=sReloadGeneration)) status=B_BAD_VALUE;
	else if (operation==TT_PCI_PROBE_RELOAD_QUIESCE &&
		(!r.expected_tag || !r.expected_generation || sColdRecoveryRequired || sRuntimeOwner
			|| cookie->context || sOpenCookies!=1)) status=B_BUSY;
	else {
		if (operation==TT_PCI_PROBE_RELOAD_QUIESCE) sReloadQuiesced=true;
		r={}; r.version=1; r.driver=TT_Q38_DISPATCH_DRIVER; r.build_tag=PIONEER_DRIVER_BUILD_TAG;
		r.generation=sReloadGeneration; r.guard_version=sReloadGuard->version;
		r.quiesced=sReloadQuiesced; r.cold=sColdRecoveryRequired; r.active_owner=sRuntimeOwner!=NULL;
		r.open_count=sOpenCookies;
		// Failed reply delivery leaves the hardware gate CLOSED. It never
		// resumes computation or clears the resident cold latch.
		status=user_memcpy(data,&r,sizeof(r));
	}
	release_sem(sLock); return status;
}
// PIONEER_DRIVER_RELOAD_CONTROL_END
#endif

static status_t
ProbeControl(void* cookie, uint32 operation, void* data, size_t length)
{
	PioneerProbeCookie* instance=(PioneerProbeCookie*)cookie;
	if (geteuid() != 0 || instance==NULL || instance->magic!=0x50313030)
#include "PioneerQ38MixedLiveNativeRouting.inc"
#ifdef PIONEER_Q38_RECURRENT_PREFIX_NATIVE_CONTEXT
 // Only retain the real bank/source constructors in this private renamed
 // compile body. No old compute dispatcher or prefix launch opcode.
 if(operation==TT_PCI_PROBE_RELOAD_STATUS || operation==TT_PCI_PROBE_RELOAD_QUIESCE)
  return DriverReloadControl(instance,operation,data,length);
 if(operation==TT_PCI_PROBE_Q38_BANK_SUPPORT || operation==TT_PCI_PROBE_Q38_BANK_BEGIN || operation==TT_PCI_PROBE_Q38_BANK_END)
  return Q38BankControl(instance,operation,data,length);
 if(operation==TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT || operation==TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD || operation==TT_PCI_PROBE_Q38_SPAN_END)
  return Q38SpanControl(instance,operation,data,length);
 return B_NOT_ALLOWED;
#endif
#ifdef PIONEER_Q38_RECURRENT_BLOCK_NATIVE_CONTEXT
 // Retain genuine constructors in the PRIVATE renamed compile body.
 // Only the separately qualified physical role gets its fixed diagnostic
 // selector;the QRBNODRV branch retains NO public compute or Work route.
 if(operation==TT_PCI_PROBE_RELOAD_STATUS || operation==TT_PCI_PROBE_RELOAD_QUIESCE)
  return DriverReloadControl(instance,operation,data,length);
#ifdef PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_NATIVE
 if(operation==TT_PCI_PROBE_Q38_RECURRENT_BLOCK_VALIDATION_SUPPORT || operation==TT_PCI_PROBE_Q38_RECURRENT_BLOCK_VALIDATE)
  return Q38RecurrentBlockValidationControl(instance,operation,data,length);
#endif
 if(operation==TT_PCI_PROBE_Q38_BANK_SUPPORT || operation==TT_PCI_PROBE_Q38_BANK_BEGIN || operation==TT_PCI_PROBE_Q38_BANK_END)
  return Q38BankControl(instance,operation,data,length);
 if(operation==TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT || operation==TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD || operation==TT_PCI_PROBE_Q38_SPAN_END)
  return Q38SpanControl(instance,operation,data,length);
 return B_NOT_ALLOWED;
#endif
#ifdef TT_PIONEER_DRIVER_RELOAD
	if (operation==TT_PCI_PROBE_RELOAD_STATUS || operation==TT_PCI_PROBE_RELOAD_QUIESCE)
		return DriverReloadControl(instance,operation,data,length);
#endif
#ifdef PIONEER_Q38_RESIDENT_OUTPUT_VALIDATION_NATIVE
 // PIONEER_Q38_RESIDENT_OUTPUT_VALIDATION_NATIVE_ROUTING_BEGIN
 if(operation==TT_PCI_PROBE_Q38_RESIDENT_OUTPUT_VALIDATION_SUPPORT || operation==TT_PCI_PROBE_Q38_RESIDENT_OUTPUT_VALIDATE)
  return Q38ResidentOutputValidationControl(instance,operation,data,length);
 if(operation==TT_PCI_PROBE_Q38_BANK_SUPPORT || operation==TT_PCI_PROBE_Q38_BANK_BEGIN || operation==TT_PCI_PROBE_Q38_BANK_END)
  return Q38BankControl(instance,operation,data,length);
 if(operation==TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT || operation==TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD || operation==TT_PCI_PROBE_Q38_SPAN_END)
  return Q38SpanControl(instance,operation,data,length);
 return B_NOT_ALLOWED; // DEBUG68 closed dispatcher;legacy body below is unreachable,not an extra surface or stack grant
 // PIONEER_Q38_RESIDENT_OUTPUT_VALIDATION_NATIVE_ROUTING_END
#endif
#ifdef PIONEER_Q38_RESIDENT_LAYER_VALIDATION_NATIVE
 // PIONEER_Q38_RESIDENT_LAYER_VALIDATION_NATIVE_ROUTING_BEGIN
 if(operation==TT_PCI_PROBE_Q38_RESIDENT_LAYER_VALIDATION_SUPPORT || operation==TT_PCI_PROBE_Q38_RESIDENT_LAYER_VALIDATE)
  return Q38ResidentLayerValidationControl(instance,operation,data,length);
 if(operation==TT_PCI_PROBE_Q38_BANK_SUPPORT || operation==TT_PCI_PROBE_Q38_BANK_BEGIN || operation==TT_PCI_PROBE_Q38_BANK_END)
  return Q38BankControl(instance,operation,data,length);
 if(operation==TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT || operation==TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD || operation==TT_PCI_PROBE_Q38_SPAN_END)
  return Q38SpanControl(instance,operation,data,length);
 return B_NOT_ALLOWED; // DEBUG69 closed dispatcher,never an old68/Work/arbitrary-address route
 // PIONEER_Q38_RESIDENT_LAYER_VALIDATION_NATIVE_ROUTING_END
#endif
#ifdef PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE
 // PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE_ROUTING_BEGIN
 if(operation==TT_PCI_PROBE_Q38_GDN_OUTPUT_VALIDATION_SUPPORT || operation==TT_PCI_PROBE_Q38_GDN_OUTPUT_VALIDATE)
  return Q38GdnOutputValidationControl(instance,operation,data,length);
 if(operation!=TT_PCI_PROBE_Q38_BANK_SUPPORT && operation!=TT_PCI_PROBE_Q38_BANK_BEGIN && operation!=TT_PCI_PROBE_Q38_BANK_END
  && operation!=TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT && operation!=TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD && operation!=TT_PCI_PROBE_Q38_SPAN_END)
  return B_NOT_ALLOWED; // DEBUG55 closed allowlist;no old DEBUG/Work/address/numeric-input route
 // PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE_ROUTING_END
#endif
#ifdef PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE
 // PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE_ROUTING_BEGIN
 if(operation==TT_PCI_PROBE_Q38_GDN_RENDEZVOUS_VALIDATION_SUPPORT || operation==TT_PCI_PROBE_Q38_GDN_RENDEZVOUS_VALIDATE)
  return Q38GdnRendezvousValidationControl(instance,operation,data,length);
 if(operation!=TT_PCI_PROBE_Q38_BANK_SUPPORT && operation!=TT_PCI_PROBE_Q38_BANK_BEGIN && operation!=TT_PCI_PROBE_Q38_BANK_END
  && operation!=TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT && operation!=TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD && operation!=TT_PCI_PROBE_Q38_SPAN_END)
  return B_NOT_ALLOWED; // DEBUG52 closed allowlist,never old DEBUG/Work/address or numeric-input route
 // PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE_ROUTING_END
#endif
#ifdef PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE
 // PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE_ROUTING_BEGIN
 if(operation==TT_PCI_PROBE_Q38_GDN_RECURRENT_VALIDATION_SUPPORT || operation==TT_PCI_PROBE_Q38_GDN_RECURRENT_VALIDATE)
  return Q38GdnRecurrentValidationControl(instance,operation,data,length);
 if(operation!=TT_PCI_PROBE_Q38_BANK_SUPPORT && operation!=TT_PCI_PROBE_Q38_BANK_BEGIN && operation!=TT_PCI_PROBE_Q38_BANK_END
  && operation!=TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT && operation!=TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD && operation!=TT_PCI_PROBE_Q38_SPAN_END)
  return B_NOT_ALLOWED; // DEBUG49 allowlist;no old DEBUG/Work/compute or caller-controlled read route
 // PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE_ROUTING_END
#endif
#if defined(PIONEER_Q38_GDN_RECURRENT_NATIVE) && !defined(PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE) && !defined(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE) && !defined(PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE)
 // PIONEER_Q38_GDN_RECURRENT_NATIVE_INERT_ROUTING_BEGIN
 return B_NOT_ALLOWED; // COMPILE-ONLY tag48 has NO public compute/upload/old Work selector
 // PIONEER_Q38_GDN_RECURRENT_NATIVE_INERT_ROUTING_END
#endif
#if defined(PIONEER_Q38_GDN_STATE_INIT_NATIVE) && !defined(PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE) && !defined(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE) && !defined(PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE)
 // PIONEER_Q38_GDN_STATE_INIT_NATIVE_INERT_ROUTING_BEGIN
 return B_NOT_ALLOWED; // COMPILE-ONLY tag47 has NO public zero/compute/upload/old Work selector
 // PIONEER_Q38_GDN_STATE_INIT_NATIVE_INERT_ROUTING_END
#endif
#if defined(PIONEER_Q38_GDN_INPUT_OWNER) && !defined(PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE) && !defined(PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE) && !defined(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE) && !defined(PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE)
 // PIONEER_Q38_GDN_INPUT_NATIVE_INERT_ROUTING_BEGIN
 return B_NOT_ALLOWED; // COMPILE-ONLY tag45 has NO public execution/upload/old Work selector
 // PIONEER_Q38_GDN_INPUT_NATIVE_INERT_ROUTING_END
#endif
#ifdef PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE
 // PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE_ROUTING_BEGIN
 if(operation==TT_PCI_PROBE_Q38_GDN_INPUT_VALIDATION_SUPPORT || operation==TT_PCI_PROBE_Q38_GDN_INPUT_VALIDATE)
  return Q38GdnInputValidationControl(instance,operation,data,length);
 if(operation==TT_PCI_PROBE_Q38_DECODER_VALIDATION_SUPPORT || operation==TT_PCI_PROBE_Q38_DECODER_VALIDATE
  || operation==TT_PCI_PROBE_Q38_PROJECTION_VALIDATION_SUPPORT || operation==TT_PCI_PROBE_Q38_PROJECTION_VALIDATE
  || operation==TT_PCI_PROBE_Q38_EMBEDDING_SUPPORT || operation==TT_PCI_PROBE_Q38_EMBEDDING_PRODUCE
  || operation==TT_PCI_PROBE_Q38_EMBEDDING_VALIDATION_SUPPORT || operation==TT_PCI_PROBE_Q38_EMBEDDING_VALIDATE
  || operation==TT_PCI_PROBE_Q38_RMS_VALIDATION_SUPPORT || operation==TT_PCI_PROBE_Q38_RMS_VALIDATE
  || operation==TT_PCI_PROBE_Q38_FFN_MODEL_SUPPORT || operation==TT_PCI_PROBE_Q38_FFN_MODEL_INITIALIZE
  || operation==TT_PCI_PROBE_Q38_FFN_GRAPH_SUPPORT || operation==TT_PCI_PROBE_Q38_FFN_GRAPH_RUN
  || operation==TT_PCI_PROBE_Q38_WORK_SUPPORT || operation==TT_PCI_PROBE_Q38_WORK_RUN
  || operation==TT_PCI_PROBE_Q38_LEGACY_WORK_SUPPORT || operation==TT_PCI_PROBE_Q38_LEGACY_WORK_RUN
  || operation==TT_PCI_PROBE_Q38_DISPATCH_SUPPORT || operation==TT_PCI_PROBE_Q38_DISPATCH_RUN)
  return B_NOT_ALLOWED; // namespace15 fixed qualifier is NOT old DEBUG/Work authority
 // PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE_ROUTING_END
#endif
#ifdef PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE
 // PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE_ROUTING_BEGIN
 if(operation==TT_PCI_PROBE_Q38_DECODER_VALIDATION_SUPPORT || operation==TT_PCI_PROBE_Q38_DECODER_VALIDATE)
  return Q38ResidentDecoderValidationControl(instance,operation,data,length);
 if(operation==TT_PCI_PROBE_Q38_PROJECTION_VALIDATION_SUPPORT || operation==TT_PCI_PROBE_Q38_PROJECTION_VALIDATE
  || operation==TT_PCI_PROBE_Q38_EMBEDDING_SUPPORT || operation==TT_PCI_PROBE_Q38_EMBEDDING_PRODUCE
  || operation==TT_PCI_PROBE_Q38_EMBEDDING_VALIDATION_SUPPORT || operation==TT_PCI_PROBE_Q38_EMBEDDING_VALIDATE
  || operation==TT_PCI_PROBE_Q38_RMS_VALIDATION_SUPPORT || operation==TT_PCI_PROBE_Q38_RMS_VALIDATE
  || operation==TT_PCI_PROBE_Q38_FFN_MODEL_SUPPORT || operation==TT_PCI_PROBE_Q38_FFN_MODEL_INITIALIZE
  || operation==TT_PCI_PROBE_Q38_FFN_GRAPH_SUPPORT || operation==TT_PCI_PROBE_Q38_FFN_GRAPH_RUN
  || operation==TT_PCI_PROBE_Q38_WORK_SUPPORT || operation==TT_PCI_PROBE_Q38_WORK_RUN
  || operation==TT_PCI_PROBE_Q38_LEGACY_WORK_SUPPORT || operation==TT_PCI_PROBE_Q38_LEGACY_WORK_RUN
  || operation==TT_PCI_PROBE_Q38_DISPATCH_SUPPORT || operation==TT_PCI_PROBE_Q38_DISPATCH_RUN)
  return B_NOT_ALLOWED; // fixed tag44 DEBUG is NOT old qualification/Work/full-inference authority
 // PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE_ROUTING_END
#endif
#ifdef PIONEER_Q38_PROJECTION_VALIDATION_NATIVE
	// PIONEER_Q38_PROJECTION_VALIDATION_NATIVE_ROUTING_BEGIN
	if (operation==TT_PCI_PROBE_Q38_PROJECTION_VALIDATION_SUPPORT || operation==TT_PCI_PROBE_Q38_PROJECTION_VALIDATE)
		return Q38ProjectionValidationControl(instance,operation,data,length);
	if (operation==TT_PCI_PROBE_Q38_EMBEDDING_SUPPORT || operation==TT_PCI_PROBE_Q38_EMBEDDING_PRODUCE
		|| operation==TT_PCI_PROBE_Q38_EMBEDDING_VALIDATION_SUPPORT || operation==TT_PCI_PROBE_Q38_EMBEDDING_VALIDATE
		|| operation==TT_PCI_PROBE_Q38_RMS_VALIDATION_SUPPORT || operation==TT_PCI_PROBE_Q38_RMS_VALIDATE
		|| operation==TT_PCI_PROBE_Q38_FFN_MODEL_SUPPORT || operation==TT_PCI_PROBE_Q38_FFN_MODEL_INITIALIZE
		|| operation==TT_PCI_PROBE_Q38_FFN_GRAPH_SUPPORT || operation==TT_PCI_PROBE_Q38_FFN_GRAPH_RUN
		|| operation==TT_PCI_PROBE_Q38_WORK_SUPPORT || operation==TT_PCI_PROBE_Q38_WORK_RUN
		|| operation==TT_PCI_PROBE_Q38_LEGACY_WORK_SUPPORT || operation==TT_PCI_PROBE_Q38_LEGACY_WORK_RUN
		|| operation==TT_PCI_PROBE_Q38_DISPATCH_SUPPORT || operation==TT_PCI_PROBE_Q38_DISPATCH_RUN)
		return B_NOT_ALLOWED; // tag42/namespace13 is NO old public/Work execution grant
	// PIONEER_Q38_PROJECTION_VALIDATION_NATIVE_ROUTING_END
#endif
#ifdef PIONEER_Q38_EMBEDDING_QUALIFICATION
	// PIONEER_Q38_EMBEDDING_NATIVE_ROUTING_BEGIN
#ifdef PIONEER_Q38_LEARNED_RMS_VALIDATION_NATIVE
	if (operation==TT_PCI_PROBE_Q38_RMS_VALIDATION_SUPPORT || operation==TT_PCI_PROBE_Q38_RMS_VALIDATE)
		return Q38EmbeddingControl(instance,operation,data,length);
	if (operation==TT_PCI_PROBE_Q38_EMBEDDING_SUPPORT || operation==TT_PCI_PROBE_Q38_EMBEDDING_PRODUCE
		|| operation==TT_PCI_PROBE_Q38_EMBEDDING_VALIDATION_SUPPORT || operation==TT_PCI_PROBE_Q38_EMBEDDING_VALIDATE)
		return B_NOT_ALLOWED;
#elif defined(PIONEER_Q38_EMBEDDING_VALIDATION_NATIVE)
	if (operation==TT_PCI_PROBE_Q38_EMBEDDING_VALIDATION_SUPPORT || operation==TT_PCI_PROBE_Q38_EMBEDDING_VALIDATE)
		return Q38EmbeddingControl(instance,operation,data,length);
	if (operation==TT_PCI_PROBE_Q38_EMBEDDING_SUPPORT || operation==TT_PCI_PROBE_Q38_EMBEDDING_PRODUCE)
		return B_NOT_ALLOWED;
#else
	if (operation==TT_PCI_PROBE_Q38_EMBEDDING_SUPPORT || operation==TT_PCI_PROBE_Q38_EMBEDDING_PRODUCE)
		return Q38EmbeddingControl(instance,operation,data,length);
	if (operation==TT_PCI_PROBE_Q38_EMBEDDING_VALIDATION_SUPPORT || operation==TT_PCI_PROBE_Q38_EMBEDDING_VALIDATE)
		return B_NOT_ALLOWED;
#endif
#ifndef PIONEER_Q38_LEARNED_RMS_VALIDATION_NATIVE
	if (operation==TT_PCI_PROBE_Q38_RMS_VALIDATION_SUPPORT || operation==TT_PCI_PROBE_Q38_RMS_VALIDATE)
		return B_NOT_ALLOWED;
#endif
	if (operation==TT_PCI_PROBE_Q38_FFN_MODEL_SUPPORT || operation==TT_PCI_PROBE_Q38_FFN_MODEL_INITIALIZE
		|| operation==TT_PCI_PROBE_Q38_FFN_GRAPH_SUPPORT || operation==TT_PCI_PROBE_Q38_FFN_GRAPH_RUN
		|| operation==TT_PCI_PROBE_Q38_WORK_SUPPORT || operation==TT_PCI_PROBE_Q38_WORK_RUN
		|| operation==TT_PCI_PROBE_Q38_DISPATCH_SUPPORT || operation==TT_PCI_PROBE_Q38_DISPATCH_RUN)
		return B_NOT_ALLOWED; // tag39 is NOT old FFN/Work code ownership or execution promotion
	// PIONEER_Q38_EMBEDDING_NATIVE_ROUTING_END
#endif
#ifdef PIONEER_Q38_FFN_MODEL_INIT
	if (operation==TT_PCI_PROBE_Q38_FFN_MODEL_SUPPORT || operation==TT_PCI_PROBE_Q38_FFN_MODEL_INITIALIZE)
		return Q38FfnModelControl(instance,operation,data,length);
#endif
#ifdef PIONEER_Q38_FFN_NATIVE_GRAPH
	if (operation==TT_PCI_PROBE_Q38_FFN_GRAPH_SUPPORT || operation==TT_PCI_PROBE_Q38_FFN_GRAPH_RUN)
		return Q38FfnGraphControl(instance,operation,data,length);
#endif
#ifdef PIONEER_Q38_GDN_ZERO_DIAGNOSTIC
	if (operation==TT_PCI_PROBE_Q38_GDN_ZERO_SUPPORT || operation==TT_PCI_PROBE_Q38_GDN_ZERO_INITIALIZE)
		return Q38GdnZeroControl(instance,operation,data,length);
#endif
#ifdef PIONEER_Q38_RESIDENT_GDN_ROOT_DIAGNOSTIC
	if (operation>=TT_PCI_PROBE_Q38_RGDN_ROOT_SUPPORT && operation<=TT_PCI_PROBE_Q38_RGDN_ROOT_END)
		return Q38RgdnRootControl(instance,operation,data,length);
#endif
#ifdef PIONEER_Q38_LOCAL_DELTA_PROBE
	if (operation>=TT_PCI_PROBE_Q38_DELTA_SUPPORT && operation<=TT_PCI_PROBE_Q38_DELTA_ACCEPT)
		return Q38DeltaControl(instance,operation,data,length);
#endif
#ifdef PIONEER_Q38_SPAN_PRELOAD
	if (operation>=TT_PCI_PROBE_Q38_SPAN_SUPPORT && operation<=
#ifdef PIONEER_Q38_SPAN_FAST
		TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD
#else
		TT_PCI_PROBE_Q38_SPAN_END
#endif
		)
		return Q38SpanControl(instance,operation,data,length);
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH
	// PIONEER_Q38_FFN_CODE_NATIVE_ROUTING_BEGIN
#ifdef TT_PIONEER_Q38_WORK_MODE
	if (operation==TT_PCI_PROBE_Q38_WORK_SUPPORT || operation==TT_PCI_PROBE_Q38_WORK_RUN)
		return Q38DispatchControl(instance,operation,data,length);
#endif
#ifdef PIONEER_Q38_FFN_CODE_DIAGNOSTIC
	if (operation==TT_PCI_PROBE_Q38_DISPATCH_SUPPORT || operation==TT_PCI_PROBE_Q38_DISPATCH_RUN
		|| operation==TT_PCI_PROBE_Q38_LEGACY_WORK_SUPPORT || operation==TT_PCI_PROBE_Q38_LEGACY_WORK_RUN)
		return B_NOT_ALLOWED;
#else
	if (operation==TT_PCI_PROBE_Q38_DISPATCH_SUPPORT || operation==TT_PCI_PROBE_Q38_DISPATCH_RUN)
		return Q38DispatchControl(instance,operation,data,length);
#endif
	// PIONEER_Q38_FFN_CODE_NATIVE_ROUTING_END
	if (operation==TT_PCI_PROBE_Q38_COHORT_SUPPORT || operation==TT_PCI_PROBE_Q38_COHORT_RUN
		|| operation==TT_PCI_PROBE_Q38_PANEL_RUN || operation==TT_PCI_PROBE_Q38_STREAM_RUN) return B_NOT_ALLOWED;
#else
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_COHORT
	if (operation==TT_PCI_PROBE_Q38_COHORT_SUPPORT || operation==TT_PCI_PROBE_Q38_COHORT_RUN)
		return Q38CohortControl(instance,operation,data,length);
#endif
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_STREAM
	if (operation==TT_PCI_PROBE_Q38_STREAM_SUPPORT || operation==TT_PCI_PROBE_Q38_STREAM_RUN)
		return Q38StreamControl(instance,operation,data,length);
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_BANKS
	if (operation>=TT_PCI_PROBE_Q38_BANK_SUPPORT && operation<=TT_PCI_PROBE_Q38_BANK_BULK_UPLOAD)
		return Q38BankControl(instance,operation,data,length);
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_PANEL
	if (operation==TT_PCI_PROBE_Q38_PANEL_SUPPORT || operation==TT_PCI_PROBE_Q38_PANEL_RUN)
		return Q38PanelControl(instance,operation,data,length);
#endif
	if (operation==TT_PCI_PROBE_PIONEER_SESSION_BEGIN || operation==TT_PCI_PROBE_PIONEER_SESSION_END
		|| operation==TT_PCI_PROBE_PIONEER_SESSION_STATUS || operation==TT_PCI_PROBE_PIONEER_SESSION_SUPPORT)
		return ResidentSessionControl(instance,operation,data,length);
	if (operation==TT_PCI_PROBE_PIONEER_STORE_UPLOAD || operation==TT_PCI_PROBE_PIONEER_STORE_READ)
		return ResidentStoreControl(instance,operation,data,length);
	if (operation==TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC || operation==TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_DMA
		|| operation==TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_FP16 || operation==TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_INT8
		|| operation==TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_Q8 || operation==TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_Q8_COMPACT
		|| operation==TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_Q8_NATIVE || operation==TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_Q8_NATIVE_F32_SCALES || operation==TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_Q8_BULK
		|| operation==TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_Q8_SECOND || operation==TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_Q8_DUAL || operation==TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_Q8_DUAL_REUSE) {
		bool reuse=operation==TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_Q8_DUAL_REUSE;
		bool dual=reuse || operation==TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_Q8_DUAL;
		bool second=dual || operation==TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_Q8_SECOND;
		bool bulk=second || operation==TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_Q8_BULK;
		bool f32Scales=bulk || operation==TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_Q8_NATIVE_F32_SCALES;
		bool native=f32Scales || operation==TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_Q8_NATIVE;
		bool compact=native || operation==TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_Q8_COMPACT;
		bool q8=compact || operation==TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_Q8;
		return RuntimeMatvecControl(instance,data,length,operation==TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_DMA,
			operation==TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_FP16,q8 || operation==TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_INT8,q8,compact,native,f32Scales,bulk,second,dual,reuse);
	}
	if (operation==TT_PCI_PROBE_PIONEER_RUNTIME_PROFILE)
		return RuntimeProfileControl(instance,data,length);
	if (operation==TT_PCI_PROBE_PIONEER_RUNTIME_Q8_FLEET)
		return RuntimeFleetControl(instance,data,length);
	if (operation==TT_PCI_PROBE_PIONEER_COORDINATED_FLEET)
		return CoordinatedFleetControl(instance,data,length);
	if (operation==TT_PCI_PROBE_PIONEER_LAYER_FFN)
		return LayerFfnControl(instance,data,length);
	if (operation==TT_PCI_PROBE_PIONEER_LAYER_GLU)
		return LayerGluControl(instance,data,length);
	if (operation==TT_PCI_PROBE_PIONEER_LAYER_WIDE)
		return LayerWideControl(instance,data,length);
	if (operation==TT_PCI_PROBE_PIONEER_LAYER_NARROW)
		return LayerWideControl(instance,data,length,true);
	if (operation==TT_PCI_PROBE_PIONEER_DECODER_MODEL)
		return DecoderModelControl(instance,data,length);
	if (operation==TT_PCI_PROBE_PIONEER_DECODER_RUN)
		return DecoderRunControl(instance,data,length);
	if (operation==TT_PCI_PROBE_PIONEER_DECODER_GREEDY)
		return DecoderGreedyControl(instance,data,length);
	if (operation==TT_PCI_PROBE_PIONEER_DECODER_BATCH)
		return DecoderBatchControl(instance,data,length);
	if (operation==TT_PCI_PROBE_PIONEER_DECODER_STAGES)
		return DecoderStageProfileControl(instance,data,length);
	if (operation==TT_PCI_PROBE_PIONEER_LAYER_SPAN)
		return LayerSpanControl(instance,data,length);
	if (operation==TT_PCI_PROBE_PIONEER_COORDINATED_SUPPORT)
		return CoordinatedSupportControl(instance,data,length);
	if (operation==TT_PCI_PROBE_PIONEER_RUNTIME_PERSISTENT_FLEET)
		return RuntimePersistentFleetControl(instance,data,length);
	if (operation==TT_PCI_PROBE_PIONEER_FLEET_ATTENTION)
		return FleetAttentionControl(instance,data,length);
	if (operation==TT_PCI_PROBE_PIONEER_RESIDENT_ATTENTION)
		return ResidentAttentionControl(instance,data,length);
	if (operation==TT_PCI_PROBE_PIONEER_PEER_PC_READ)
		return PeerPcReadControl(instance,data,length);
	if (operation==TT_PCI_PROBE_PIONEER_COORDINATOR_PROBE)
		return CoordinatorProbeControl(instance,data,length);
	if (operation==TT_PCI_PROBE_PIONEER_ATTENTION_TILE_TEST || operation==TT_PCI_PROBE_PIONEER_ATTENTION_TILE_SHARED)
		return AttentionTileTestControl(instance,data,length,operation==TT_PCI_PROBE_PIONEER_ATTENTION_TILE_SHARED);
	if (operation==TT_PCI_PROBE_PIONEER_STREAM_UPLOAD)
		return ResidentStreamControl(instance,data,length);
	if (operation==TT_PCI_PROBE_PIONEER_BULK_UPLOAD)
		return BulkUploadControl(instance,data,length);
	if (operation==TT_PCI_PROBE_PIONEER_HOST_WRITE_SUPPORT || operation==TT_PCI_PROBE_PIONEER_HOST_WRITE_PAGE_SUPPORT) {
		if (data==NULL || length!=sizeof(uint32)) return B_BAD_VALUE;
		status_t status=AcquireProbeLock(); if (status!=B_OK) return status;
		release_sem(sLock); const uint32 version=1;
		return user_memcpy(data,&version,sizeof(version));
	}
	if (operation==TT_PCI_PROBE_PIONEER_HOST_WRITE || operation==TT_PCI_PROBE_PIONEER_HOST_WRITE_PAGE) {
		if (data==NULL || length!=sizeof(tt_pioneer_host_write_result)) return B_BAD_VALUE;
		status_t status=AcquireProbeLock(); if (status!=B_OK) return status;
		status=sPCI->reserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		if (status!=B_OK) { release_sem(sLock); return status; }
		if (sDeviceManager==NULL) status=get_module(B_DEVICE_MANAGER_MODULE_NAME,(module_info**)&sDeviceManager);
		tt_pioneer_profile profile={};
		if (status==B_OK) status=GetPioneerPciProfile(sDeviceManager,sCard,profile);
		profile.status=status;
		tt_pioneer_host_write_result result={};
		tt_pioneer_dram_samples_result& baseline=result.baseline.baseline.baseline.baseline;
		tt_pioneer_arc_result& control=baseline.control.control.control;
		tt_arc_telemetry_result& telemetry=baseline.control.control.telemetry;
		tt_tensix_reset_result& tensix=baseline.control.tensix;
		control.pci.version=control.arc.version=telemetry.version=tensix.version=baseline.dram.version=1;
		control.arc.tlb_restored=telemetry.tlb_restored=tensix.tlb_restored=1;
		baseline.dram.data_restored=baseline.dram.tlb_restored=result.baseline.baseline.baseline.noc.tlb_restored=1;
		result.baseline.baseline.path.tlb_restored=result.baseline.audit.tlb_restored=1;
		tt_host_read_result& transfer=result.write.transfer;
		transfer.bme_restored=transfer.l1_restored=transfer.niu_restored=transfer.tlb_restored=1;
		if (status==B_OK) {
			PioneerHostWriteIO io(profile,control.arc,telemetry,tensix,baseline.dram,
				result.baseline.baseline.baseline.noc,result.baseline.baseline.path,result.baseline.audit,result.write,
				operation==TT_PCI_PROBE_PIONEER_HOST_WRITE_PAGE?4096:64);
			BlackholePioneerMmioProbe(io,control.pci);
		}
		if (transfer.recovery_required || result.baseline.audit.recovery_required
			|| result.baseline.baseline.path.recovery_required || result.baseline.baseline.baseline.noc.recovery_required
			|| baseline.dram.recovery_required || control.pci.state==TT_PIONEER_MMIO_RESTORE_FAILED) {
			transfer.recovery_required=1; sColdRecoveryRequired=true;
		}
		sPCI->unreserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		release_sem(sLock); return user_memcpy(data,&result,sizeof(result));
	}
	if (operation==TT_PCI_PROBE_PIONEER_HOST_READ_SUPPORT || operation==TT_PCI_PROBE_PIONEER_HOST_READ_PAGE_SUPPORT) {
		if (data==NULL || length!=sizeof(uint32)) return B_BAD_VALUE;
		status_t status=AcquireProbeLock(); if (status!=B_OK) return status;
		release_sem(sLock); const uint32 version=1;
		return user_memcpy(data,&version,sizeof(version));
	}
	if (operation==TT_PCI_PROBE_PIONEER_HOST_READ || operation==TT_PCI_PROBE_PIONEER_HOST_READ_PAGE) {
		if (data==NULL || length!=sizeof(tt_pioneer_host_read_result)) return B_BAD_VALUE;
		status_t status=AcquireProbeLock(); if (status!=B_OK) return status;
		status=sPCI->reserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		if (status!=B_OK) { release_sem(sLock); return status; }
		if (sDeviceManager==NULL) status=get_module(B_DEVICE_MANAGER_MODULE_NAME,(module_info**)&sDeviceManager);
		tt_pioneer_profile profile={};
		if (status==B_OK) status=GetPioneerPciProfile(sDeviceManager,sCard,profile);
		profile.status=status;
		tt_pioneer_host_read_result result={};
		tt_pioneer_dram_samples_result& baseline=result.baseline.baseline.baseline.baseline;
		tt_pioneer_arc_result& control=baseline.control.control.control;
		tt_arc_telemetry_result& telemetry=baseline.control.control.telemetry;
		tt_tensix_reset_result& tensix=baseline.control.tensix;
		control.pci.version=control.arc.version=telemetry.version=tensix.version=baseline.dram.version=1;
		control.arc.tlb_restored=telemetry.tlb_restored=tensix.tlb_restored=1;
		baseline.dram.data_restored=baseline.dram.tlb_restored=result.baseline.baseline.baseline.noc.tlb_restored=1;
		result.baseline.baseline.path.tlb_restored=result.baseline.audit.tlb_restored=1;
		result.read.bme_restored=result.read.l1_restored=result.read.niu_restored=result.read.tlb_restored=1;
		if (status==B_OK) {
			PioneerHostReadIO io(profile,control.arc,telemetry,tensix,baseline.dram,
				result.baseline.baseline.baseline.noc,result.baseline.baseline.path,result.baseline.audit,result.read,
				operation==TT_PCI_PROBE_PIONEER_HOST_READ_PAGE?4096:64);
			BlackholePioneerMmioProbe(io,control.pci);
		}
		if (result.read.recovery_required || result.baseline.audit.recovery_required
			|| result.baseline.baseline.path.recovery_required || result.baseline.baseline.baseline.noc.recovery_required
			|| baseline.dram.recovery_required || control.pci.state==TT_PIONEER_MMIO_RESTORE_FAILED) {
			result.read.recovery_required=1; sColdRecoveryRequired=true;
		}
		sPCI->unreserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		release_sem(sLock); return user_memcpy(data,&result,sizeof(result));
	}
	if (operation==TT_PCI_PROBE_PIONEER_HOST_DMA_AUDIT_SUPPORT) {
		if (data==NULL || length!=sizeof(uint32)) return B_BAD_VALUE;
		status_t status=AcquireProbeLock(); if (status!=B_OK) return status;
		release_sem(sLock); const uint32 version=1;
		return user_memcpy(data,&version,sizeof(version));
	}
	if (operation==TT_PCI_PROBE_PIONEER_HOST_DMA_AUDIT) {
		if (data==NULL || length!=sizeof(tt_pioneer_host_dma_audit_result)) return B_BAD_VALUE;
		status_t status=AcquireProbeLock(); if (status!=B_OK) return status;
		status=sPCI->reserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		if (status!=B_OK) { release_sem(sLock); return status; }
		if (sDeviceManager==NULL) status=get_module(B_DEVICE_MANAGER_MODULE_NAME,(module_info**)&sDeviceManager);
		tt_pioneer_profile profile={};
		if (status==B_OK) status=GetPioneerPciProfile(sDeviceManager,sCard,profile);
		profile.status=status;
		tt_pioneer_host_dma_audit_result result={};
		tt_pioneer_dram_samples_result& baseline=result.baseline.baseline.baseline;
		tt_pioneer_arc_result& control=baseline.control.control.control;
		tt_arc_telemetry_result& telemetry=baseline.control.control.telemetry;
		tt_tensix_reset_result& tensix=baseline.control.tensix;
		control.pci.version=control.arc.version=telemetry.version=tensix.version=baseline.dram.version=1;
		control.arc.tlb_restored=telemetry.tlb_restored=tensix.tlb_restored=1;
		baseline.dram.data_restored=baseline.dram.tlb_restored=result.baseline.baseline.noc.tlb_restored=1;
		result.baseline.path.tlb_restored=result.audit.tlb_restored=1;
		if (status==B_OK) {
			PioneerHostDmaAuditIO io(profile,control.arc,telemetry,tensix,baseline.dram,
				result.baseline.baseline.noc,result.baseline.path,result.audit);
			BlackholePioneerMmioProbe(io,control.pci);
		}
		if (result.audit.recovery_required || result.baseline.path.recovery_required
			|| result.baseline.baseline.noc.recovery_required || baseline.dram.recovery_required
			|| control.pci.state==TT_PIONEER_MMIO_RESTORE_FAILED) {
			result.audit.recovery_required=1; sColdRecoveryRequired=true;
		}
		sPCI->unreserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		release_sem(sLock); return user_memcpy(data,&result,sizeof(result));
	}
	if (operation==TT_PCI_PROBE_PIONEER_OUTPUT_BUFFER_SUPPORT) {
		if (data==NULL || length!=sizeof(uint32)) return B_BAD_VALUE;
		status_t status=AcquireProbeLock(); if (status!=B_OK) return status;
		release_sem(sLock); const uint32 version=1;
		return user_memcpy(data,&version,sizeof(version));
	}
	if (operation==TT_PCI_PROBE_PIONEER_OUTPUT_BUFFER_PROBE) {
		if (data==NULL || length!=sizeof(tt_output_buffer_probe_result)) return B_BAD_VALUE;
		status_t status=AcquireProbeLock(); if (status!=B_OK) return status;
		tt_output_buffer_probe_result result={};
		PioneerOutputBufferProbeIO io;
		PioneerOutputBufferProbe(io,result); // CPU-only, no physical address escapes to user/device
		release_sem(sLock); return user_memcpy(data,&result,sizeof(result));
	}
	if (operation==TT_PCI_PROBE_PIONEER_HOST_PAGE_SUPPORT) {
		if (data==NULL || length!=sizeof(uint32)) return B_BAD_VALUE;
		status_t status=AcquireProbeLock(); if (status!=B_OK) return status;
		release_sem(sLock); const uint32 version=1;
		return user_memcpy(data,&version,sizeof(version));
	}
	if (operation==TT_PCI_PROBE_PIONEER_HOST_PAGE) {
		if (data==NULL || length!=sizeof(tt_host_page_result)) return B_BAD_VALUE;
		status_t status=AcquireProbeLock(); if (status!=B_OK) return status;
		tt_host_page_result result={};
		PioneerHostPageIO io;
		PioneerHostPageProbe(io,result); // CPU-only: never calls PCI/MMIO
		release_sem(sLock); return user_memcpy(data,&result,sizeof(result));
	}
	if (operation==TT_PCI_PROBE_PIONEER_HOST_PATH_SUPPORT) {
		if (data==NULL || length!=sizeof(uint32)) return B_BAD_VALUE;
		status_t status=AcquireProbeLock(); if (status!=B_OK) return status;
		release_sem(sLock); const uint32 version=1;
		return user_memcpy(data,&version,sizeof(version));
	}
	if (operation==TT_PCI_PROBE_PIONEER_HOST_PATH) {
		if (data==NULL || length!=sizeof(tt_pioneer_host_path_result)) return B_BAD_VALUE;
		status_t status=AcquireProbeLock(); if (status!=B_OK) return status;
		status=sPCI->reserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		if (status!=B_OK) { release_sem(sLock); return status; }
		if (sDeviceManager==NULL) status=get_module(B_DEVICE_MANAGER_MODULE_NAME,(module_info**)&sDeviceManager);
		tt_pioneer_profile profile={};
		if (status==B_OK) status=GetPioneerPciProfile(sDeviceManager,sCard,profile);
		profile.status=status;
		tt_pioneer_host_path_result result={};
		tt_pioneer_dram_samples_result& baseline=result.baseline.baseline;
		tt_pioneer_arc_result& control=baseline.control.control.control;
		tt_arc_telemetry_result& telemetry=baseline.control.control.telemetry;
		tt_tensix_reset_result& tensix=baseline.control.tensix;
		control.pci.version=control.arc.version=telemetry.version=tensix.version=baseline.dram.version=1;
		control.arc.tlb_restored=telemetry.tlb_restored=tensix.tlb_restored=1;
		baseline.dram.data_restored=baseline.dram.tlb_restored=result.baseline.noc.tlb_restored=result.path.tlb_restored=1;
		if (status==B_OK) {
			PioneerHostPathIO io(profile,control.arc,telemetry,tensix,baseline.dram,result.baseline.noc,result.path);
			BlackholePioneerMmioProbe(io,control.pci);
		}
		if (result.path.recovery_required || result.baseline.noc.recovery_required || baseline.dram.recovery_required
			|| control.pci.state==TT_PIONEER_MMIO_RESTORE_FAILED) {
			result.path.recovery_required=1; sColdRecoveryRequired=true;
		}
		sPCI->unreserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		release_sem(sLock); return user_memcpy(data,&result,sizeof(result));
	}
	if (operation==TT_PCI_PROBE_PIONEER_RESIDENT_GEMV_SUPPORT) {
		if (data==NULL || length!=sizeof(uint32)) return B_BAD_VALUE;
		status_t status=AcquireProbeLock(); if (status!=B_OK) return status;
		release_sem(sLock); const uint32 version=1;
		return user_memcpy(data,&version,sizeof(version));
	}
	if (operation==TT_PCI_PROBE_PIONEER_RESIDENT_GEMV) {
		if (data==NULL || length!=sizeof(tt_pioneer_resident_gemv_result)) return B_BAD_VALUE;
		struct { uint32 version,inputs; } request={};
		status_t status=user_memcpy(&request,data,sizeof(request));
		if (status!=B_OK) return status;
		if (request.version!=1 || !BlackholeResidentGemvShape(request.inputs)) return B_BAD_VALUE;
		status=AcquireProbeLock(); if (status!=B_OK) return status;
		status=sPCI->reserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		if (status!=B_OK) { release_sem(sLock); return status; }
		if (sDeviceManager==NULL) status=get_module(B_DEVICE_MANAGER_MODULE_NAME,(module_info**)&sDeviceManager);
		tt_pioneer_profile profile={};
		if (status==B_OK) status=GetPioneerPciProfile(sDeviceManager,sCard,profile);
		profile.status=status;
		tt_pioneer_resident_gemv_result reply={}; reply.version=1; reply.inputs=request.inputs;
		tt_pioneer_resident_math_result& result=reply.result;
		tt_pioneer_dram_samples_result& baseline=result.baseline.baseline;
		tt_pioneer_arc_result& control=baseline.control.control.control;
		tt_arc_telemetry_result& telemetry=baseline.control.control.telemetry;
		tt_tensix_reset_result& tensix=baseline.control.tensix;
		control.pci.version=control.arc.version=telemetry.version=tensix.version=baseline.dram.version=1;
		control.arc.tlb_restored=telemetry.tlb_restored=tensix.tlb_restored=1;
		baseline.dram.data_restored=baseline.dram.tlb_restored=result.baseline.noc.tlb_restored=1;
		result.compute.gddr_restored=result.compute.niu_restored=1;
		result.compute.math.l1_restored=result.compute.math.tlb_restored=1;
		if (status==B_OK) {
			PioneerResidentMathIO io(profile,control.arc,telemetry,tensix,baseline.dram,result.baseline.noc,result.compute,request.inputs);
			BlackholePioneerMmioProbe(io,control.pci);
		}
		if (result.compute.recovery_required || result.compute.math.recovery_required || result.baseline.noc.recovery_required
			|| baseline.dram.recovery_required || control.pci.state==TT_PIONEER_MMIO_RESTORE_FAILED) {
			result.compute.recovery_required=1; sColdRecoveryRequired=true;
		}
		sPCI->unreserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		release_sem(sLock); return user_memcpy(data,&reply,sizeof(reply));
	}
	if (operation==TT_PCI_PROBE_PIONEER_RESIDENT_MATH_SUPPORT) {
		if (data==NULL || length!=sizeof(uint32)) return B_BAD_VALUE;
		status_t status=AcquireProbeLock(); if (status!=B_OK) return status;
		release_sem(sLock); const uint32 version=1;
		return user_memcpy(data,&version,sizeof(version));
	}
	if (operation==TT_PCI_PROBE_PIONEER_RESIDENT_MATH) {
		if (data==NULL || length!=sizeof(tt_pioneer_resident_math_result)) return B_BAD_VALUE;
		status_t status=AcquireProbeLock(); if (status!=B_OK) return status;
		status=sPCI->reserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		if (status!=B_OK) { release_sem(sLock); return status; }
		if (sDeviceManager==NULL) status=get_module(B_DEVICE_MANAGER_MODULE_NAME,(module_info**)&sDeviceManager);
		tt_pioneer_profile profile={};
		if (status==B_OK) status=GetPioneerPciProfile(sDeviceManager,sCard,profile);
		profile.status=status;
		tt_pioneer_resident_math_result result={};
		tt_pioneer_dram_samples_result& baseline=result.baseline.baseline;
		tt_pioneer_arc_result& control=baseline.control.control.control;
		tt_arc_telemetry_result& telemetry=baseline.control.control.telemetry;
		tt_tensix_reset_result& tensix=baseline.control.tensix;
		control.pci.version=control.arc.version=telemetry.version=tensix.version=baseline.dram.version=1;
		control.arc.tlb_restored=telemetry.tlb_restored=tensix.tlb_restored=1;
		baseline.dram.data_restored=baseline.dram.tlb_restored=result.baseline.noc.tlb_restored=1;
		result.compute.gddr_restored=result.compute.niu_restored=1;
		result.compute.math.l1_restored=result.compute.math.tlb_restored=1;
		if (status==B_OK) {
			PioneerResidentMathIO io(profile,control.arc,telemetry,tensix,baseline.dram,result.baseline.noc,result.compute);
			BlackholePioneerMmioProbe(io,control.pci);
		}
		if (result.compute.recovery_required || result.compute.math.recovery_required || result.baseline.noc.recovery_required
			|| baseline.dram.recovery_required || control.pci.state==TT_PIONEER_MMIO_RESTORE_FAILED) {
			result.compute.recovery_required=1; sColdRecoveryRequired=true;
		}
		sPCI->unreserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		release_sem(sLock); return user_memcpy(data,&result,sizeof(result));
	}
	if (operation==TT_PCI_PROBE_PIONEER_RESIDENT_READ_SUPPORT) {
		if (data==NULL || length!=sizeof(uint32)) return B_BAD_VALUE;
		status_t status=AcquireProbeLock(); if (status!=B_OK) return status;
		release_sem(sLock); const uint32 version=1;
		return user_memcpy(data,&version,sizeof(version));
	}
	if (operation==TT_PCI_PROBE_PIONEER_RESIDENT_READ) {
		if (data==NULL || length!=sizeof(tt_pioneer_resident_read_result)) return B_BAD_VALUE;
		status_t status=AcquireProbeLock(); if (status!=B_OK) return status;
		status=sPCI->reserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		if (status!=B_OK) { release_sem(sLock); return status; }
		if (sDeviceManager==NULL) status=get_module(B_DEVICE_MANAGER_MODULE_NAME,(module_info**)&sDeviceManager);
		tt_pioneer_profile profile={};
		if (status==B_OK) status=GetPioneerPciProfile(sDeviceManager,sCard,profile);
		profile.status=status;
		tt_pioneer_resident_read_result result={};
		tt_pioneer_dram_samples_result& baseline=result.baseline.baseline;
		tt_pioneer_arc_result& control=baseline.control.control.control;
		tt_arc_telemetry_result& telemetry=baseline.control.control.telemetry;
		tt_tensix_reset_result& tensix=baseline.control.tensix;
		control.pci.version=control.arc.version=telemetry.version=tensix.version=baseline.dram.version=1;
		control.arc.tlb_restored=telemetry.tlb_restored=tensix.tlb_restored=1;
		baseline.dram.data_restored=baseline.dram.tlb_restored=result.baseline.noc.tlb_restored=1;
		result.read.l1_restored=result.read.niu_restored=result.read.tlb_restored=1;
		if (status==B_OK) {
			PioneerResidentReadIO io(profile,control.arc,telemetry,tensix,baseline.dram,result.baseline.noc,result.read);
			BlackholePioneerMmioProbe(io,control.pci);
		}
		if (result.read.recovery_required || result.baseline.noc.recovery_required
			|| baseline.dram.recovery_required || control.pci.state==TT_PIONEER_MMIO_RESTORE_FAILED) {
			result.read.recovery_required=1; sColdRecoveryRequired=true;
		}
		sPCI->unreserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		release_sem(sLock);
		return user_memcpy(data,&result,sizeof(result));
	}
	if (operation==TT_PCI_PROBE_PIONEER_FLEET_NOC_SUPPORT) {
		if (!data || length!=sizeof(uint32)) return B_BAD_VALUE;
		status_t status=AcquireProbeLock(); if (status!=B_OK) return status;
		release_sem(sLock); const uint32 workers=TT_PIONEER_WORKERS;
		return user_memcpy(data,&workers,sizeof(workers));
	}
	if (operation==TT_PCI_PROBE_PIONEER_FLEET_NOC) {
		if (!data || length!=sizeof(tt_pioneer_fleet_noc_result)) return B_BAD_VALUE;
		thread_info thread={}; status_t status=get_thread_info(find_thread(NULL),&thread);
		if (status!=B_OK) return status;
		if (thread.team!=instance->team) return B_NOT_ALLOWED;
		auto* result=(tt_pioneer_fleet_noc_result*)calloc(1,sizeof(tt_pioneer_fleet_noc_result));
		if (!result) return B_NO_MEMORY;
		status=AcquireProbeLock(); if (status!=B_OK) { free(result); return status; }
		status=sPCI->reserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		if (status!=B_OK) { release_sem(sLock); free(result); return status; }
		if (!sDeviceManager) status=get_module(B_DEVICE_MANAGER_MODULE_NAME,(module_info**)&sDeviceManager);
		tt_pioneer_profile profile={};
		if (status==B_OK) status=GetPioneerPciProfile(sDeviceManager,sCard,profile);
		profile.status=status;
		auto& control=result->baseline.control.control.control;
		control.pci.version=control.arc.version=result->baseline.control.control.telemetry.version=1;
		control.arc.tlb_restored=result->baseline.control.control.telemetry.tlb_restored=1;
		result->baseline.control.tensix.version=result->baseline.control.tensix.tlb_restored=1;
		result->baseline.dram.version=result->baseline.dram.data_restored=result->baseline.dram.tlb_restored=1;
		if (status==B_OK) { PioneerFleetNocIO io(profile,*result); BlackholePioneerMmioProbe(io,control.pci); }
		if (result->baseline.dram.recovery_required || control.pci.state==TT_PIONEER_MMIO_RESTORE_FAILED)
			result->recovery_required=1;
		if (result->recovery_required) sColdRecoveryRequired=true;
		sPCI->unreserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		status=user_memcpy(data,result,sizeof(*result));
		release_sem(sLock); free(result); return status;
	}
	if (operation==TT_PCI_PROBE_PIONEER_COMPUTE_NOC_SUPPORT || operation==TT_PCI_PROBE_PIONEER_SECOND_COMPUTE_NOC_SUPPORT) {
		if (data==NULL || length!=sizeof(uint32)) return B_BAD_VALUE;
		status_t status=AcquireProbeLock(); if (status!=B_OK) return status;
		release_sem(sLock); const uint32 version=operation==TT_PCI_PROBE_PIONEER_SECOND_COMPUTE_NOC_SUPPORT?2:1;
		return user_memcpy(data,&version,sizeof(version));
	}
	if (operation==TT_PCI_PROBE_PIONEER_COMPUTE_NOC || operation==TT_PCI_PROBE_PIONEER_SECOND_COMPUTE_NOC) {
		if (data==NULL || length!=sizeof(tt_pioneer_compute_noc_result)) return B_BAD_VALUE;
		bool second=operation==TT_PCI_PROBE_PIONEER_SECOND_COMPUTE_NOC;
		if (second) {
			thread_info thread={}; status_t caller=get_thread_info(find_thread(NULL),&thread);
			if (caller!=B_OK) return caller;
			if (thread.team!=instance->team) return B_NOT_ALLOWED;
		}
		status_t status=AcquireProbeLock(); if (status!=B_OK) return status;
		status=sPCI->reserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		if (status!=B_OK) { release_sem(sLock); return status; }
		if (sDeviceManager==NULL) status=get_module(B_DEVICE_MANAGER_MODULE_NAME,(module_info**)&sDeviceManager);
		tt_pioneer_profile profile={};
		if (status==B_OK) status=GetPioneerPciProfile(sDeviceManager,sCard,profile);
		profile.status=status;
		tt_pioneer_compute_noc_result result={};
		tt_pioneer_arc_result& control=result.baseline.control.control.control;
		tt_arc_telemetry_result& telemetry=result.baseline.control.control.telemetry;
		tt_tensix_reset_result& tensix=result.baseline.control.tensix;
		control.pci.version=control.arc.version=telemetry.version=tensix.version=result.baseline.dram.version=1;
		control.arc.tlb_restored=telemetry.tlb_restored=tensix.tlb_restored=1;
		result.baseline.dram.data_restored=result.baseline.dram.tlb_restored=result.noc.tlb_restored=1;
		if (status==B_OK) {
			PioneerComputeNocIO io(profile,control.arc,telemetry,tensix,result.baseline.dram,result.noc,second?3:2);
			BlackholePioneerMmioProbe(io,control.pci);
		}
		if (result.noc.recovery_required || result.baseline.dram.recovery_required
			|| control.pci.state==TT_PIONEER_MMIO_RESTORE_FAILED) {
			result.noc.recovery_required=1; sColdRecoveryRequired=true;
		}
		sPCI->unreserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		release_sem(sLock);
		return user_memcpy(data,&result,sizeof(result));
	}
	if (operation == TT_PCI_PROBE_PIONEER_CLOCKED_MATMUL_SUPPORT) {
		if (data == NULL || length != sizeof(tt_matmul_support)) return B_BAD_VALUE;
		status_t status = AcquireProbeLock(); if (status != B_OK) return status;
		const tt_matmul_support support = {7,TT_TENSIX_CODE_WORDS,TT_TENSIX_IMAGE_WORDS,0x6000,0x70000,TT_TENSIX_PARK_PC,0x17f800,0x87ffff,0x46800};
		release_sem(sLock); return user_memcpy(data,&support,sizeof(support));
	}
	if (operation == TT_PCI_PROBE_PIONEER_CLOCKED_MATMUL) {
		if (data == NULL || length != sizeof(tt_pioneer_clocked_matmul_result)) return B_BAD_VALUE;
		tt_pioneer_clocked_matmul_result result = {};
		status_t status = user_memcpy(data,&result,sizeof(result)); if (status != B_OK) return status;
		status = AcquireProbeLock(); if (status != B_OK) return status;
		status = sPCI->reserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		if (status != B_OK) { release_sem(sLock); return status; }
		if (sDeviceManager == NULL) status = get_module(B_DEVICE_MANAGER_MODULE_NAME,(module_info**)&sDeviceManager);
		tt_pioneer_profile profile = {};
		if (status == B_OK) status = GetPioneerPciProfile(sDeviceManager,sCard,profile);
		profile.status = status;
		tt_pioneer_arc_result& control = result.matmul.baseline.control.control.control;
		tt_arc_telemetry_result& health = result.matmul.baseline.control.control.telemetry;
		tt_tensix_reset_result& tensix = result.matmul.baseline.control.tensix;
		tt_tensix_matmul_result& math = result.matmul.math;
		control.pci.version = control.arc.version = health.version = tensix.version = result.matmul.baseline.dram.version = math.version = result.clock.version = 1;
		control.pci.profile_status = status; control.pci.profile_stage = profile.stage;
		control.arc.tlb_restored = health.tlb_restored = tensix.tlb_restored = result.matmul.baseline.dram.tlb_restored = result.matmul.baseline.dram.data_restored = 1;
		math.l1_restored = math.tlb_restored = result.clock.restored = result.clock.last.request_restored = result.clock.last.tlb_restored = 1;
		if (status == B_OK) {
			PioneerMatmulIO io(profile,control.arc,health,tensix,result.matmul.baseline.dram,math,false,&result.clock);
			BlackholePioneerMmioProbe(io,control.pci);
		}
		if (math.recovery_required || result.clock.recovery_required || result.clock.last.recovery_required
			|| result.matmul.baseline.dram.recovery_required || control.pci.state == TT_PIONEER_MMIO_RESTORE_FAILED) {
			math.recovery_required = result.clock.recovery_required = 1; sColdRecoveryRequired = true;
		}
		sPCI->unreserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		release_sem(sLock);
		dprintf("PIONEER-P100A: clocked matrix state %" B_PRIu32 " complete/stop %" B_PRIu32 "/%" B_PRIu32
			" clock restored %" B_PRIu32 " PCI restored %" B_PRIu32 " recovery %" B_PRIu32 "\n",
			math.state,math.completed,math.stopped,result.clock.restored,control.pci.restored,math.recovery_required);
		return user_memcpy(data,&result,sizeof(result));
	}
	if (operation == TT_PCI_PROBE_PIONEER_ARC_CLOCK_CYCLE_SUPPORT) {
		if (data == NULL || length != sizeof(tt_arc_clock_support)) return B_BAD_VALUE;
		status_t status = AcquireProbeLock(); if (status != B_OK) return status;
		const tt_arc_clock_support support = {1,0x34,0x52,0x54,800,1400};
		release_sem(sLock); return user_memcpy(data,&support,sizeof(support));
	}
	if (operation == TT_PCI_PROBE_PIONEER_ARC_CLOCK_CYCLE) {
		if (data == NULL || length != sizeof(tt_pioneer_arc_clock_result)) return B_BAD_VALUE;
		tt_pioneer_arc_clock_result result = {};
		status_t status = user_memcpy(data,&result,sizeof(result)); if (status != B_OK) return status;
		status = AcquireProbeLock(); if (status != B_OK) return status;
		status = sPCI->reserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		if (status != B_OK) { release_sem(sLock); return status; }
		if (sDeviceManager == NULL) status = get_module(B_DEVICE_MANAGER_MODULE_NAME,(module_info**)&sDeviceManager);
		tt_pioneer_profile profile = {};
		if (status == B_OK) status = GetPioneerPciProfile(sDeviceManager,sCard,profile);
		profile.status = status;
		tt_pioneer_mmio_result& pci = result.control.control.pci;
		tt_arc_scratch_result& arc = result.control.control.arc;
		tt_arc_telemetry_result& health = result.control.telemetry;
		pci.version = arc.version = health.version = result.clock.version = 1;
		pci.profile_status = status; pci.profile_stage = profile.stage;
		arc.tlb_restored = health.tlb_restored = result.clock.restored = 1;
		result.clock.last.tlb_restored = result.clock.last.request_restored = 1;
		if (status == B_OK) {
			PioneerArcQueryIO io(profile,arc,health,result.clock.last,&result.clock);
			BlackholePioneerMmioProbe(io,pci);
		}
		if (result.clock.recovery_required || result.clock.last.recovery_required || pci.state == TT_PIONEER_MMIO_RESTORE_FAILED) {
			result.clock.recovery_required = 1; sColdRecoveryRequired = true;
		}
		sPCI->unreserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		release_sem(sLock);
		dprintf("PIONEER-P100A: clock cycle state %" B_PRIu32 " MHz %" B_PRIu32 "/%" B_PRIu32 "/%" B_PRIu32
			" restored %" B_PRIu32 " PCI restored %" B_PRIu32 " recovery %" B_PRIu32 "\n",
			result.clock.state,result.clock.before_mhz,result.clock.busy_mhz,result.clock.after_mhz,
			result.clock.restored,pci.restored,result.clock.recovery_required);
		return user_memcpy(data,&result,sizeof(result));
	}
	if (operation == TT_PCI_PROBE_PIONEER_ARC_QUERY_SUPPORT) {
		if (data == NULL || length != sizeof(tt_arc_query_support)) return B_BAD_VALUE;
		status_t status = AcquireProbeLock(); if (status != B_OK) return status;
		const tt_arc_query_support support = {2,0x34,3,4,TT_ARC_QUEUE_CSM_BYTES};
		release_sem(sLock);
		return user_memcpy(data,&support,sizeof(support));
	}
	if (operation == TT_PCI_PROBE_PIONEER_ARC_CLOCK_QUERY) {
		if (data == NULL || length != sizeof(tt_pioneer_arc_query_result)) return B_BAD_VALUE;
		tt_pioneer_arc_query_result result = {};
		status_t status = user_memcpy(data,&result,sizeof(result)); if (status != B_OK) return status;
		status = AcquireProbeLock(); if (status != B_OK) return status;
		status = sPCI->reserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		if (status != B_OK) { release_sem(sLock); return status; }
		if (sDeviceManager == NULL) status = get_module(B_DEVICE_MANAGER_MODULE_NAME,(module_info**)&sDeviceManager);
		tt_pioneer_profile profile = {};
		if (status == B_OK) status = GetPioneerPciProfile(sDeviceManager,sCard,profile);
		profile.status = status;
		tt_pioneer_mmio_result& pci = result.control.control.pci;
		tt_arc_scratch_result& arc = result.control.control.arc;
		tt_arc_telemetry_result& health = result.control.telemetry;
		pci.version = arc.version = health.version = result.query.version = 1;
		pci.profile_status = status; pci.profile_stage = profile.stage;
		arc.tlb_restored = health.tlb_restored = result.query.tlb_restored = result.query.request_restored = 1;
		if (status == B_OK) {
			PioneerArcQueryIO io(profile,arc,health,result.query);
			BlackholePioneerMmioProbe(io,pci);
		}
		if (result.query.recovery_required || pci.state == TT_PIONEER_MMIO_RESTORE_FAILED) {
			result.query.recovery_required = 1; sColdRecoveryRequired = true;
		}
		sPCI->unreserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		release_sem(sLock);
		dprintf("PIONEER-P100A: ARC query state %" B_PRIu32 " submit/reply/ack %" B_PRIu32 "/%" B_PRIu32 "/%" B_PRIu32
			" request/TLB/PCI restored %" B_PRIu32 "/%" B_PRIu32 "/%" B_PRIu32 " recovery %" B_PRIu32 "\n",
			result.query.state,result.query.submitted,result.query.responded,result.query.acknowledged,
			result.query.request_restored,result.query.tlb_restored,pci.restored,result.query.recovery_required);
		return user_memcpy(data,&result,sizeof(result));
	}
	if (operation == TT_PCI_PROBE_PIONEER_MATMUL_SUPPORT || operation == TT_PCI_PROBE_PIONEER_TENSIX_CONFIG_SUPPORT || operation == TT_PCI_PROBE_PIONEER_SFPU_LOAD_SUPPORT || operation == TT_PCI_PROBE_PIONEER_SFPU_STORE_SUPPORT || operation == TT_PCI_PROBE_PIONEER_OPERAND_SUPPORT || operation == TT_PCI_PROBE_PIONEER_MATRIX_EXEC_SUPPORT || operation == TT_PCI_PROBE_PIONEER_ZEROACC_SUPPORT || operation == TT_PCI_PROBE_PIONEER_MOVD_SUPPORT || operation == TT_PCI_PROBE_PIONEER_KERNEL_OPERAND_SUPPORT || operation == TT_PCI_PROBE_PIONEER_KERNEL_MATRIX_EXEC_SUPPORT || operation == TT_PCI_PROBE_PIONEER_KERNEL_MATMUL_SUPPORT || operation == TT_PCI_PROBE_PIONEER_DST_READ_SUPPORT
		|| operation == TT_PCI_PROBE_PIONEER_SRCA_ROUNDTRIP_SUPPORT || operation == TT_PCI_PROBE_PIONEER_SRCB_ROUNDTRIP_SUPPORT
		|| operation == TT_PCI_PROBE_PIONEER_PURE_SRCA_SUPPORT || operation == TT_PCI_PROBE_PIONEER_PURE_SRCB_SUPPORT || operation == TT_PCI_PROBE_PIONEER_PURE_MATMUL_SUPPORT
		|| operation == TT_PCI_PROBE_PIONEER_UNPACK_MATMUL_SUPPORT || operation == TT_PCI_PROBE_PIONEER_UNPACK_SRCA_SUPPORT || operation == TT_PCI_PROBE_PIONEER_UNPACK_SRCB_SUPPORT || operation == TT_PCI_PROBE_PIONEER_UNPACK_CONFIG_SUPPORT
		|| operation == TT_PCI_PROBE_PIONEER_IDENTITY_MATMUL_SUPPORT || operation == TT_PCI_PROBE_PIONEER_DENSE_MATMUL_SUPPORT || operation == TT_PCI_PROBE_PIONEER_DENSE_SRCA_SUPPORT || operation == TT_PCI_PROBE_PIONEER_PATTERN_SRCB_SUPPORT || operation == TT_PCI_PROBE_PIONEER_DATA_TILE_SUPPORT || operation == TT_PCI_PROBE_PIONEER_DATA_BATCH_SUPPORT) {
		if (data == NULL || length != sizeof(tt_matmul_support)) return B_BAD_VALUE;
		status_t status = AcquireProbeLock(); if (status != B_OK) return status;
		uint32 version = 5;
		if (operation == TT_PCI_PROBE_PIONEER_TENSIX_CONFIG_SUPPORT) version = 6;
		if (operation == TT_PCI_PROBE_PIONEER_SFPU_LOAD_SUPPORT) version = 8;
		if (operation == TT_PCI_PROBE_PIONEER_SFPU_STORE_SUPPORT) version = 9;
		if (operation == TT_PCI_PROBE_PIONEER_OPERAND_SUPPORT) version = 10;
		if (operation == TT_PCI_PROBE_PIONEER_MATRIX_EXEC_SUPPORT) version = 11;
		if (operation == TT_PCI_PROBE_PIONEER_ZEROACC_SUPPORT) version = 12;
		if (operation == TT_PCI_PROBE_PIONEER_MOVD_SUPPORT) version = 13;
		if (operation == TT_PCI_PROBE_PIONEER_KERNEL_OPERAND_SUPPORT) version = 14;
		if (operation == TT_PCI_PROBE_PIONEER_KERNEL_MATRIX_EXEC_SUPPORT) version = 15;
		if (operation == TT_PCI_PROBE_PIONEER_KERNEL_MATMUL_SUPPORT) version = 16;
		if (operation == TT_PCI_PROBE_PIONEER_DST_READ_SUPPORT) version = 17;
		if (operation == TT_PCI_PROBE_PIONEER_SRCA_ROUNDTRIP_SUPPORT) version = 18;
		if (operation == TT_PCI_PROBE_PIONEER_SRCB_ROUNDTRIP_SUPPORT) version = 19;
		if (operation == TT_PCI_PROBE_PIONEER_PURE_SRCA_SUPPORT) version = 20;
		if (operation == TT_PCI_PROBE_PIONEER_PURE_SRCB_SUPPORT) version = 21;
		if (operation == TT_PCI_PROBE_PIONEER_PURE_MATMUL_SUPPORT) version = 22;
		if (operation == TT_PCI_PROBE_PIONEER_UNPACK_MATMUL_SUPPORT) version = 31;
		if (operation == TT_PCI_PROBE_PIONEER_UNPACK_SRCA_SUPPORT) version = 32;
		if (operation == TT_PCI_PROBE_PIONEER_UNPACK_SRCB_SUPPORT) version = 33;
		if (operation == TT_PCI_PROBE_PIONEER_UNPACK_CONFIG_SUPPORT) version = 34;
		if (operation == TT_PCI_PROBE_PIONEER_IDENTITY_MATMUL_SUPPORT) version = 35;
		if (operation == TT_PCI_PROBE_PIONEER_DENSE_MATMUL_SUPPORT) version = 36;
		if (operation == TT_PCI_PROBE_PIONEER_DENSE_SRCA_SUPPORT) version = 37;
		if (operation == TT_PCI_PROBE_PIONEER_PATTERN_SRCB_SUPPORT) version = 38;
		if (operation == TT_PCI_PROBE_PIONEER_DATA_TILE_SUPPORT) version = 39;
		if (operation == TT_PCI_PROBE_PIONEER_DATA_BATCH_SUPPORT) version = 40;
		unsigned stage = version >= 31 ? version-15 : 0;
		const tt_matmul_support support = {version,BlackholeTensixCodeWords(stage),BlackholeTensixImageWords(stage),0x6000,0x70000,BlackholeTensixParkPc(stage),0x17f800,0x87ffff,0x46800};
		release_sem(sLock);
		return user_memcpy(data,&support,sizeof(support));
	}
	if (operation == TT_PCI_PROBE_PIONEER_DATA_BATCH) {
		if (data == NULL || length != sizeof(tt_pioneer_data_batch)) return B_BAD_VALUE;
		struct Transaction {
			tt_pioneer_data_batch* p;
			Transaction() : p((tt_pioneer_data_batch*)calloc(1,sizeof(tt_pioneer_data_batch))) {}
			~Transaction() { free(p); }
		} transaction;
		if (transaction.p == NULL) return B_NO_MEMORY;
		status_t status = user_memcpy(transaction.p,data,offsetof(tt_pioneer_data_batch,output));
		if (status != B_OK) return status;
		if (transaction.p->version != 1 || transaction.p->count == 0
			|| transaction.p->count > TT_DATA_TILE_BATCH_MAX || transaction.p->completed != 0
			|| transaction.p->reserved != 0) return B_BAD_VALUE;
		for (unsigned i = 0; i < transaction.p->count; i++)
			if (!BlackholeTileDataValid(transaction.p->input[i])) return B_BAD_VALUE;
		status = AcquireProbeLock(); if (status != B_OK) return status;
		status = sPCI->reserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		if (status != B_OK) { release_sem(sLock); return status; }
		if (sDeviceManager == NULL) status = get_module(B_DEVICE_MANAGER_MODULE_NAME,(module_info**)&sDeviceManager);
		tt_pioneer_profile profile = {};
		if (status == B_OK) status = GetPioneerPciProfile(sDeviceManager,sCard,profile);
		profile.status = status;
		tt_pioneer_matmul_result& result = transaction.p->result;
		tt_pioneer_arc_result& control = result.baseline.control.control.control;
		tt_arc_telemetry_result& telemetry = result.baseline.control.control.telemetry;
		tt_tensix_reset_result& tensix = result.baseline.control.tensix;
		control.pci.version = control.arc.version = telemetry.version = tensix.version = result.baseline.dram.version = 1;
		control.pci.profile_status = status; control.pci.profile_stage = profile.stage;
		control.arc.tlb_restored = telemetry.tlb_restored = tensix.tlb_restored = result.baseline.dram.tlb_restored = result.baseline.dram.data_restored = 1;
		result.math.l1_restored = result.math.tlb_restored = 1;
		if (status == B_OK) {
			PioneerMatmulIO io(profile,control.arc,telemetry,tensix,result.baseline.dram,result.math,
				false,NULL,24,transaction.p->input,&transaction.p->output[0][0],
				transaction.p->count,&transaction.p->completed);
			BlackholePioneerMmioProbe(io,control.pci);
		}
		if (result.math.recovery_required || result.baseline.dram.recovery_required || control.pci.state == TT_PIONEER_MMIO_RESTORE_FAILED) {
			result.math.recovery_required = 1; sColdRecoveryRequired = true;
		}
		sPCI->unreserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		release_sem(sLock);
		if (transaction.p->completed != transaction.p->count || result.math.state != TT_MATMUL_DATA_EXECUTED
			|| !result.math.completed || !result.math.stopped || !result.math.l1_restored
			|| !result.math.tlb_restored || !control.pci.restored || result.math.recovery_required)
			memset(transaction.p->output,0,sizeof(transaction.p->output));
		return user_memcpy(data,transaction.p,sizeof(tt_pioneer_data_batch));
	}
	if (operation == TT_PCI_PROBE_PIONEER_DATA_TILE) {
		if (data == NULL || length != sizeof(tt_pioneer_data_tile)) return B_BAD_VALUE;
		// Heap-owned snapshot: never consult userspace during device execution.
		// Result buffers are zeroed, not copied from the caller.
		struct Transaction {
			tt_pioneer_data_tile* p;
			Transaction() : p((tt_pioneer_data_tile*)calloc(1,sizeof(tt_pioneer_data_tile))) {}
			~Transaction() { free(p); }
		} transaction;
		if (transaction.p == NULL) return B_NO_MEMORY;
		status_t status = user_memcpy(&transaction.p->input,data,sizeof(tt_tensix_tile_data));
		if (status != B_OK) return status;
		if (!BlackholeTileDataValid(transaction.p->input)) return B_BAD_VALUE;
		status = AcquireProbeLock(); if (status != B_OK) return status;
		status = sPCI->reserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		if (status != B_OK) { release_sem(sLock); return status; }
		if (sDeviceManager == NULL) status = get_module(B_DEVICE_MANAGER_MODULE_NAME,(module_info**)&sDeviceManager);
		tt_pioneer_profile profile = {};
		if (status == B_OK) status = GetPioneerPciProfile(sDeviceManager,sCard,profile);
		profile.status = status;
		tt_pioneer_matmul_result& result = transaction.p->result;
		tt_pioneer_arc_result& control = result.baseline.control.control.control;
		tt_arc_telemetry_result& telemetry = result.baseline.control.control.telemetry;
		tt_tensix_reset_result& tensix = result.baseline.control.tensix;
		control.pci.version = control.arc.version = telemetry.version = tensix.version = result.baseline.dram.version = 1;
		control.pci.profile_status = status; control.pci.profile_stage = profile.stage;
		control.arc.tlb_restored = telemetry.tlb_restored = tensix.tlb_restored = result.baseline.dram.tlb_restored = result.baseline.dram.data_restored = 1;
		result.math.l1_restored = result.math.tlb_restored = 1;
		if (status == B_OK) {
			PioneerMatmulIO io(profile,control.arc,telemetry,tensix,result.baseline.dram,result.math,
				false,NULL,24,&transaction.p->input,transaction.p->output);
			BlackholePioneerMmioProbe(io,control.pci);
		}
		if (result.math.recovery_required || result.baseline.dram.recovery_required || control.pci.state == TT_PIONEER_MMIO_RESTORE_FAILED) {
			result.math.recovery_required = 1; sColdRecoveryRequired = true;
		}
		sPCI->unreserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		release_sem(sLock);
		// Execution is not a numerical pass: the caller validates the result.
		if (result.math.state != TT_MATMUL_DATA_EXECUTED || !result.math.completed
			|| !result.math.stopped || !result.math.l1_restored || !result.math.tlb_restored
			|| !control.pci.restored || result.math.recovery_required)
			memset(transaction.p->output,0,sizeof(transaction.p->output));
		return user_memcpy(data,transaction.p,sizeof(tt_pioneer_data_tile));
	}
	if (operation == TT_PCI_PROBE_PIONEER_TENSIX_MATMUL || operation == TT_PCI_PROBE_PIONEER_TENSIX_CONFIG || operation == TT_PCI_PROBE_PIONEER_SFPU_LOAD || operation == TT_PCI_PROBE_PIONEER_SFPU_STORE || operation == TT_PCI_PROBE_PIONEER_OPERAND_SMOKE || operation == TT_PCI_PROBE_PIONEER_MATRIX_EXEC_SMOKE || operation == TT_PCI_PROBE_PIONEER_ZEROACC_SMOKE || operation == TT_PCI_PROBE_PIONEER_MOVD_SMOKE || operation == TT_PCI_PROBE_PIONEER_KERNEL_OPERAND || operation == TT_PCI_PROBE_PIONEER_KERNEL_MATRIX_EXEC || operation == TT_PCI_PROBE_PIONEER_KERNEL_MATMUL || operation == TT_PCI_PROBE_PIONEER_DST_READ
		|| operation == TT_PCI_PROBE_PIONEER_SRCA_ROUNDTRIP || operation == TT_PCI_PROBE_PIONEER_SRCB_ROUNDTRIP
		|| operation == TT_PCI_PROBE_PIONEER_PURE_SRCA || operation == TT_PCI_PROBE_PIONEER_PURE_SRCB || operation == TT_PCI_PROBE_PIONEER_PURE_MATMUL
		|| operation == TT_PCI_PROBE_PIONEER_UNPACK_MATMUL || operation == TT_PCI_PROBE_PIONEER_UNPACK_SRCA || operation == TT_PCI_PROBE_PIONEER_UNPACK_SRCB || operation == TT_PCI_PROBE_PIONEER_UNPACK_CONFIG
		|| operation == TT_PCI_PROBE_PIONEER_IDENTITY_MATMUL || operation == TT_PCI_PROBE_PIONEER_DENSE_MATMUL || operation == TT_PCI_PROBE_PIONEER_DENSE_SRCA || operation == TT_PCI_PROBE_PIONEER_PATTERN_SRCB) {
		if (data == NULL || length != sizeof(tt_pioneer_matmul_result)) return B_BAD_VALUE;
		tt_pioneer_matmul_result result = {};
		status_t status = user_memcpy(data,&result,sizeof(result)); if (status != B_OK) return status;
		status = AcquireProbeLock(); if (status != B_OK) return status;
		status = sPCI->reserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		if (status != B_OK) { release_sem(sLock); return status; }
		if (sDeviceManager == NULL) status = get_module(B_DEVICE_MANAGER_MODULE_NAME,(module_info**)&sDeviceManager);
		tt_pioneer_profile profile = {};
		if (status == B_OK) status = GetPioneerPciProfile(sDeviceManager,sCard,profile);
		profile.status = status;
		tt_pioneer_arc_result& control = result.baseline.control.control.control;
		tt_arc_telemetry_result& telemetry = result.baseline.control.control.telemetry;
		tt_tensix_reset_result& tensix = result.baseline.control.tensix;
		control.pci.version = control.arc.version = telemetry.version = tensix.version = result.baseline.dram.version = result.math.version = 1;
		control.pci.profile_status = status; control.pci.profile_stage = profile.stage;
		control.arc.tlb_restored = telemetry.tlb_restored = tensix.tlb_restored = result.baseline.dram.tlb_restored = result.baseline.dram.data_restored = 1;
		result.math.l1_restored = result.math.tlb_restored = 1;
		if (status == B_OK) {
			unsigned sfpuOnly = operation == TT_PCI_PROBE_PIONEER_SFPU_STORE ? 2U : operation == TT_PCI_PROBE_PIONEER_SFPU_LOAD ? 1U : 0U;
			if (operation == TT_PCI_PROBE_PIONEER_OPERAND_SMOKE) sfpuOnly = 3;
			if (operation == TT_PCI_PROBE_PIONEER_MATRIX_EXEC_SMOKE) sfpuOnly = 4;
			if (operation == TT_PCI_PROBE_PIONEER_ZEROACC_SMOKE) sfpuOnly = 5;
			if (operation == TT_PCI_PROBE_PIONEER_MOVD_SMOKE) sfpuOnly = 6;
			if (operation == TT_PCI_PROBE_PIONEER_KERNEL_MATMUL) sfpuOnly = 7;
			if (operation == TT_PCI_PROBE_PIONEER_KERNEL_OPERAND) sfpuOnly = 8;
			if (operation == TT_PCI_PROBE_PIONEER_KERNEL_MATRIX_EXEC) sfpuOnly = 9;
			if (operation == TT_PCI_PROBE_PIONEER_DST_READ) sfpuOnly = 10;
			if (operation == TT_PCI_PROBE_PIONEER_SRCA_ROUNDTRIP) sfpuOnly = 11;
			if (operation == TT_PCI_PROBE_PIONEER_SRCB_ROUNDTRIP) sfpuOnly = 12;
			if (operation == TT_PCI_PROBE_PIONEER_PURE_SRCA) sfpuOnly = 13;
			if (operation == TT_PCI_PROBE_PIONEER_PURE_SRCB) sfpuOnly = 14;
			if (operation == TT_PCI_PROBE_PIONEER_PURE_MATMUL) sfpuOnly = 15;
			if (operation == TT_PCI_PROBE_PIONEER_UNPACK_MATMUL) sfpuOnly = 16;
			if (operation == TT_PCI_PROBE_PIONEER_UNPACK_SRCA) sfpuOnly = 17;
			if (operation == TT_PCI_PROBE_PIONEER_UNPACK_SRCB) sfpuOnly = 18;
			if (operation == TT_PCI_PROBE_PIONEER_UNPACK_CONFIG) sfpuOnly = 19;
			if (operation == TT_PCI_PROBE_PIONEER_IDENTITY_MATMUL) sfpuOnly = 20;
			if (operation == TT_PCI_PROBE_PIONEER_DENSE_MATMUL) sfpuOnly = 21;
			if (operation == TT_PCI_PROBE_PIONEER_DENSE_SRCA) sfpuOnly = 22;
			if (operation == TT_PCI_PROBE_PIONEER_PATTERN_SRCB) sfpuOnly = 23;
			PioneerMatmulIO io(profile,control.arc,telemetry,tensix,result.baseline.dram,result.math,operation == TT_PCI_PROBE_PIONEER_TENSIX_CONFIG,NULL,sfpuOnly);
			BlackholePioneerMmioProbe(io,control.pci);
		}
		if (result.math.recovery_required || result.baseline.dram.recovery_required || control.pci.state == TT_PIONEER_MMIO_RESTORE_FAILED) {
			result.math.recovery_required = 1; sColdRecoveryRequired = true;
		}
		sPCI->unreserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
		release_sem(sLock);
		dprintf("PIONEER-P100A: matrix state %" B_PRIu32 " started/completed/stopped %" B_PRIu32 "/%" B_PRIu32 "/%" B_PRIu32
			" mismatch %" B_PRIu32 " L1/TLB/PCI restored %" B_PRIu32 "/%" B_PRIu32 "/%" B_PRIu32 " recovery %" B_PRIu32 "\n",
			result.math.state,result.math.started,result.math.completed,result.math.stopped,result.math.mismatches,
			result.math.l1_restored,result.math.tlb_restored,control.pci.restored,result.math.recovery_required);
		return user_memcpy(data,&result,sizeof(result));
	}
	if (operation == TT_PCI_PROBE_PIONEER_DRAM_SAMPLES_SUPPORT) {
		if (data == NULL || length != sizeof(tt_dram_samples_support)) return B_BAD_VALUE;
		status_t status = AcquireProbeLock();
		if (status != B_OK) return status;
		const tt_dram_samples_support support = {2, TT_DRAM_BANKS, TT_DRAM_REGIONS, TT_DRAM_WORDS, 0xfff, 0xf7};
		release_sem(sLock);
		return user_memcpy(data, &support, sizeof(support));
	}
	if (operation == TT_PCI_PROBE_PIONEER_DRAM_ALL_ROUTES || operation == TT_PCI_PROBE_PIONEER_DRAM_SAMPLES) {
		if (data == NULL || length != sizeof(tt_pioneer_dram_samples_result)) return B_BAD_VALUE;
		tt_pioneer_dram_samples_result result = {};
		status_t status = user_memcpy(data, &result, sizeof(result));
		if (status != B_OK) return status;
		status = AcquireProbeLock();
		if (status != B_OK) return status;
		status = sPCI->reserve_device(sCard.bus, sCard.device, sCard.function, TT_PCI_PROBE_DEVICE, &sCookie);
		if (status != B_OK) { release_sem(sLock); return status; }
		if (sDeviceManager == NULL) status = get_module(B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&sDeviceManager);
		tt_pioneer_profile profile = {};
		if (status == B_OK) status = GetPioneerPciProfile(sDeviceManager, sCard, profile);
		profile.status = status;
		tt_pioneer_arc_result& control = result.control.control.control;
		tt_arc_telemetry_result& telemetry = result.control.control.telemetry;
		tt_tensix_reset_result& tensix = result.control.tensix;
		control.pci.version = control.arc.version = telemetry.version = tensix.version = result.dram.version = 1;
		control.pci.profile_status = status; control.pci.profile_stage = profile.stage;
		control.arc.tlb_restored = telemetry.tlb_restored = tensix.tlb_restored = result.dram.tlb_restored = result.dram.data_restored = 1;
		if (status == B_OK) {
			PioneerDramSamplesIO io(profile, control.arc, telemetry, tensix, result.dram, operation == TT_PCI_PROBE_PIONEER_DRAM_SAMPLES);
			BlackholePioneerMmioProbe(io, control.pci);
		}
		if (result.dram.recovery_required || control.pci.state == TT_PIONEER_MMIO_RESTORE_FAILED) {
			result.dram.recovery_required = 1; sColdRecoveryRequired = true;
		}
		sPCI->unreserve_device(sCard.bus, sCard.device, sCard.function, TT_PCI_PROBE_DEVICE, &sCookie);
		release_sem(sLock);
		dprintf("PIONEER-P100A: DRAM samples state %" B_PRIu32 " routes/cores %" B_PRIu32 "/%" B_PRIu32
			" data/TLB/PCI restored %" B_PRIu32 "/%" B_PRIu32 "/%" B_PRIu32 " recovery %" B_PRIu32 "\n",
			result.dram.state, result.dram.routes_verified, result.dram.cores_checked,
			result.dram.data_restored, result.dram.tlb_restored, control.pci.restored, result.dram.recovery_required);
		return user_memcpy(data, &result, sizeof(result));
	}
	if (operation == TT_PCI_PROBE_PIONEER_DRAM_SUPPORT) {
		if (data == NULL || length != sizeof(tt_dram_route_support)) return B_BAD_VALUE;
		status_t status = AcquireProbeLock();
		if (status != B_OK) return status;
		const tt_dram_route_support support = {2, 0, TT_DRAM_SOURCE_WORDS, TT_DRAM_ID_WORDS, 18, 12};
		release_sem(sLock);
		return user_memcpy(data, &support, sizeof(support));
	}
	if (operation == TT_PCI_PROBE_PIONEER_DRAM_ROUTE) {
		if (data == NULL || length != sizeof(tt_pioneer_dram_route_result)) return B_BAD_VALUE;
		tt_pioneer_dram_route_result result = {};
		status_t status = user_memcpy(data, &result, sizeof(result));
		if (status != B_OK) return status;
		status = AcquireProbeLock();
		if (status != B_OK) return status;
		status = sPCI->reserve_device(sCard.bus, sCard.device, sCard.function, TT_PCI_PROBE_DEVICE, &sCookie);
		if (status != B_OK) { release_sem(sLock); return status; }
		if (sDeviceManager == NULL) status = get_module(B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&sDeviceManager);
		tt_pioneer_profile profile = {};
		if (status == B_OK) status = GetPioneerPciProfile(sDeviceManager, sCard, profile);
		profile.status = status;
		tt_pioneer_arc_result& control = result.control.control.control;
		tt_arc_telemetry_result& telemetry = result.control.control.telemetry;
		tt_tensix_reset_result& tensix = result.control.tensix;
		control.pci.version = control.arc.version = telemetry.version = tensix.version = result.dram.version = 1;
		control.pci.profile_status = status; control.pci.profile_stage = profile.stage;
		control.arc.tlb_restored = telemetry.tlb_restored = tensix.tlb_restored = result.dram.tlb_restored = 1;
		if (status == B_OK) {
			PioneerDramRouteIO io(profile, control.arc, telemetry, tensix, result.dram);
			BlackholePioneerMmioProbe(io, control.pci);
		}
		if (result.dram.recovery_required || control.pci.state == TT_PIONEER_MMIO_RESTORE_FAILED) {
			result.dram.recovery_required = 1;
			sColdRecoveryRequired = true;
		}
		sPCI->unreserve_device(sCard.bus, sCard.device, sCard.function, TT_PCI_PROBE_DEVICE, &sCookie);
		release_sem(sLock);
		dprintf("PIONEER-P100A: DRAM route state %" B_PRIu32 " TLB/PCI restored %" B_PRIu32 "/%" B_PRIu32
			" cold recovery %" B_PRIu32 "\n", result.dram.state,
			result.dram.tlb_restored, control.pci.restored, result.dram.recovery_required);
		return user_memcpy(data, &result, sizeof(result));
	}
	if (operation == TT_PCI_PROBE_PIONEER_BRISC_SUPPORT || operation == TT_PCI_PROBE_PIONEER_TRISC_SUPPORT || operation == TT_PCI_PROBE_PIONEER_TRISC_RESET_SUPPORT) {
		if (data == NULL || length != sizeof(tt_brisc_support)) return B_BAD_VALUE;
		status_t status = AcquireProbeLock();
		if (status != B_OK) return status;
		const tt_brisc_support support = {operation == TT_PCI_PROBE_PIONEER_TRISC_RESET_SUPPORT ? 3U : operation == TT_PCI_PROBE_PIONEER_TRISC_SUPPORT ? 2U : 1U, TT_BRISC_CODE_WORDS, TT_BRISC_IMAGE_WORDS, 0x70000, 0x17ffc0, 0x70048};
		release_sem(sLock);
		// Loaded-module ABI/image metadata only: no PCI or MMIO access.
		return user_memcpy(data, &support, sizeof(support));
	}
	if (operation == TT_PCI_PROBE_PIONEER_BRISC_SMOKE || operation == TT_PCI_PROBE_PIONEER_TRISC_SMOKE || operation == TT_PCI_PROBE_PIONEER_TRISC_RESET_SMOKE) {
		if (data == NULL || length != sizeof(tt_pioneer_brisc_result)) return B_BAD_VALUE;
		tt_pioneer_brisc_result result = {};
		status_t status = user_memcpy(data, &result, sizeof(result));
		if (status != B_OK) return status;
		status = AcquireProbeLock();
		if (status != B_OK) return status;
		status = sPCI->reserve_device(sCard.bus, sCard.device, sCard.function, TT_PCI_PROBE_DEVICE, &sCookie);
		if (status != B_OK) { release_sem(sLock); return status; }
		if (sDeviceManager == NULL) status = get_module(B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&sDeviceManager);
		tt_pioneer_profile profile = {};
		if (status == B_OK) status = GetPioneerPciProfile(sDeviceManager, sCard, profile);
		profile.status = status;
		tt_pioneer_arc_result& control = result.control.control.control;
		tt_arc_telemetry_result& telemetry = result.control.control.telemetry;
		tt_tensix_reset_result& tensix = result.control.tensix;
		control.pci.version = control.arc.version = telemetry.version = tensix.version = result.brisc.version = 1;
		control.pci.profile_status = status; control.pci.profile_stage = profile.stage;
		control.arc.tlb_restored = telemetry.tlb_restored = tensix.tlb_restored = 1;
		result.brisc.l1_restored = result.brisc.tlb_restored = 1;
		if (status == B_OK) {
			PioneerBriscIO io(profile, control.arc, telemetry, tensix, result.brisc, operation != TT_PCI_PROBE_PIONEER_BRISC_SMOKE, operation == TT_PCI_PROBE_PIONEER_TRISC_RESET_SMOKE);
			BlackholePioneerMmioProbe(io, control.pci);
		}
		if (result.brisc.recovery_required || control.pci.state == TT_PIONEER_MMIO_RESTORE_FAILED) {
			result.brisc.recovery_required = 1;
			sColdRecoveryRequired = true;
		}
		sPCI->unreserve_device(sCard.bus, sCard.device, sCard.function, TT_PCI_PROBE_DEVICE, &sCookie);
		release_sem(sLock);
		dprintf("PIONEER-P100A: BRISC state %" B_PRIu32 " started/completed/stopped %" B_PRIu32 "/%" B_PRIu32 "/%" B_PRIu32
			" L1/TLB/PCI restored %" B_PRIu32 "/%" B_PRIu32 "/%" B_PRIu32 " cold recovery %" B_PRIu32 "\n",
			result.brisc.state, result.brisc.started, result.brisc.completed, result.brisc.stopped,
			result.brisc.l1_restored, result.brisc.tlb_restored, control.pci.restored, result.brisc.recovery_required);
		return user_memcpy(data, &result, sizeof(result));
	}
	if (operation == TT_PCI_PROBE_PIONEER_L1_SCRATCH) {
		if (data == NULL || length != sizeof(tt_pioneer_l1_result)) return B_BAD_VALUE;
		tt_pioneer_l1_result result = {};
		status_t status = user_memcpy(data, &result, sizeof(result));
		if (status != B_OK) return status;
		status = AcquireProbeLock();
		if (status != B_OK) return status;
		status = sPCI->reserve_device(sCard.bus, sCard.device, sCard.function,
			TT_PCI_PROBE_DEVICE, &sCookie);
		if (status != B_OK) { release_sem(sLock); return status; }
		if (sDeviceManager == NULL)
			status = get_module(B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&sDeviceManager);
		tt_pioneer_profile profile = {};
		if (status == B_OK) status = GetPioneerPciProfile(sDeviceManager, sCard, profile);
		profile.status = status;
		tt_pioneer_arc_result& control = result.control.control.control;
		tt_arc_telemetry_result& telemetry = result.control.control.telemetry;
		tt_tensix_reset_result& tensix = result.control.tensix;
		control.pci.version = control.arc.version = telemetry.version = tensix.version = result.l1.version = 1;
		control.pci.profile_status = status;
		control.pci.profile_stage = profile.stage;
		control.arc.tlb_restored = telemetry.tlb_restored = tensix.tlb_restored = 1;
		result.l1.tlb_restored = result.l1.data_restored = 1;
		if (status == B_OK) {
			PioneerL1IO io(profile, control.arc, telemetry, tensix, result.l1);
			BlackholePioneerMmioProbe(io, control.pci);
		}
		sPCI->unreserve_device(sCard.bus, sCard.device, sCard.function, TT_PCI_PROBE_DEVICE, &sCookie);
		release_sem(sLock);
		dprintf("PIONEER-P100A: L1 state %" B_PRIu32 " passes %" B_PRIu32 " mismatches %#" B_PRIx32
			" data/TLB/PCI restored %" B_PRIu32 "/%" B_PRIu32 "/%" B_PRIu32 " (no core start/DMA/flash)\n",
			result.l1.state, result.l1.passes, result.l1.mismatch_mask,
			result.l1.data_restored, result.l1.tlb_restored, control.pci.restored);
		return user_memcpy(data, &result, sizeof(result));
	}
	if (operation == TT_PCI_PROBE_PIONEER_TENSIX_RESET) {
		if (data == NULL || length != sizeof(tt_pioneer_tensix_result)) return B_BAD_VALUE;
		tt_pioneer_tensix_result result = {};
		status_t status = user_memcpy(data, &result, sizeof(result));
		if (status != B_OK) return status;
		status = AcquireProbeLock();
		if (status != B_OK) return status;
		status = sPCI->reserve_device(sCard.bus, sCard.device, sCard.function,
			TT_PCI_PROBE_DEVICE, &sCookie);
		if (status != B_OK) { release_sem(sLock); return status; }
		if (sDeviceManager == NULL)
			status = get_module(B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&sDeviceManager);
		tt_pioneer_profile profile = {};
		if (status == B_OK) status = GetPioneerPciProfile(sDeviceManager, sCard, profile);
		profile.status = status;
		tt_pioneer_arc_result& control = result.control.control;
		control.pci.version = control.arc.version = result.control.telemetry.version = result.tensix.version = 1;
		control.pci.profile_status = status;
		control.pci.profile_stage = profile.stage;
		control.arc.tlb_restored = result.control.telemetry.tlb_restored = result.tensix.tlb_restored = 1;
		if (status == B_OK) {
			PioneerTensixIO io(profile, control.arc, result.control.telemetry, result.tensix);
			BlackholePioneerMmioProbe(io, control.pci);
		}
		sPCI->unreserve_device(sCard.bus, sCard.device, sCard.function, TT_PCI_PROBE_DEVICE, &sCookie);
		release_sem(sLock);
		dprintf("PIONEER-P100A: Tensix reset state %" B_PRIu32 " raw %08" B_PRIx32
			" NIU %#" B_PRIx32 " TLB restored %" B_PRIu32 " PCI restored %" B_PRIu32 " (read-only core)\n",
			result.tensix.state, result.tensix.reset[0], result.tensix.niu_config,
			result.tensix.tlb_restored, control.pci.restored);
		return user_memcpy(data, &result, sizeof(result));
	}
	if (operation == TT_PCI_PROBE_PIONEER_TELEMETRY || operation == TT_PCI_PROBE_PIONEER_OPERATING_TELEMETRY) {
		if (data == NULL || length != sizeof(tt_pioneer_telemetry_result)) return B_BAD_VALUE;
		tt_pioneer_telemetry_result result = {};
		status_t status = user_memcpy(data, &result, sizeof(result));
		if (status != B_OK) return status;
		status = AcquireProbeLock();
		if (status != B_OK) return status;
		status = sPCI->reserve_device(sCard.bus, sCard.device, sCard.function,
			TT_PCI_PROBE_DEVICE, &sCookie);
		if (status != B_OK) { release_sem(sLock); return status; }
		if (sDeviceManager == NULL)
			status = get_module(B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&sDeviceManager);
		tt_pioneer_profile profile = {};
		if (status == B_OK) status = GetPioneerPciProfile(sDeviceManager, sCard, profile);
		profile.status = status;
		result.control.pci.version = result.control.arc.version = result.telemetry.version = 1;
		result.control.pci.profile_status = status;
		result.control.pci.profile_stage = profile.stage;
		result.control.arc.tlb_restored = result.telemetry.tlb_restored = 1;
		if (status == B_OK) {
			PioneerTelemetryIO io(profile, result.control.arc, result.telemetry, operation == TT_PCI_PROBE_PIONEER_OPERATING_TELEMETRY);
			BlackholePioneerMmioProbe(io, result.control.pci);
		}
		sPCI->unreserve_device(sCard.bus, sCard.device, sCard.function, TT_PCI_PROBE_DEVICE, &sCookie);
		release_sem(sLock);
		dprintf("PIONEER-P100A: telemetry state %" B_PRIu32 " version %#" B_PRIx32
			" count %" B_PRIu32 " mask %#" B_PRIx32 " TLB restored %" B_PRIu32
			" PCI restored %" B_PRIu32 " (no firmware/DMA writes)\n",
			result.telemetry.state, result.telemetry.table_version, result.telemetry.entry_count,
			result.telemetry.present_mask, result.telemetry.tlb_restored, result.control.pci.restored);
		return user_memcpy(data, &result, sizeof(result));
	}
	if (operation == TT_PCI_PROBE_PIONEER_ARC_SCRATCH) {
		if (data == NULL || length != sizeof(tt_pioneer_arc_result))
			return B_BAD_VALUE;
		tt_pioneer_arc_result result = {};
		status_t status = user_memcpy(data, &result, sizeof(result));
		if (status != B_OK) return status;
		status = AcquireProbeLock();
		if (status != B_OK) return status;
		status = sPCI->reserve_device(sCard.bus, sCard.device, sCard.function,
			TT_PCI_PROBE_DEVICE, &sCookie);
		if (status != B_OK) { release_sem(sLock); return status; }
		if (sDeviceManager == NULL)
			status = get_module(B_DEVICE_MANAGER_MODULE_NAME,
				(module_info**)&sDeviceManager);
		tt_pioneer_profile profile = {};
		if (status == B_OK)
			status = GetPioneerPciProfile(sDeviceManager, sCard, profile);
		profile.status = status;
		result.pci.version = result.arc.version = 1;
		result.pci.profile_status = status;
		result.pci.profile_stage = profile.stage;
		result.arc.tlb_restored = 1;
		if (status == B_OK) {
			PioneerArcIO io(profile, result.arc);
			BlackholePioneerMmioProbe(io, result.pci);
		}
		sPCI->unreserve_device(sCard.bus, sCard.device, sCard.function,
			TT_PCI_PROBE_DEVICE, &sCookie);
		release_sem(sLock);
		dprintf("PIONEER-P100A: ARC scratch state %" B_PRIu32
			" TLB restored %" B_PRIu32 " PCI state %" B_PRIu32
			" restored %" B_PRIu32 " boot %08" B_PRIx32 "/%08" B_PRIx32
			" error %08" B_PRIx32 " (no firmware/DMA writes)\n",
			result.arc.state, result.arc.tlb_restored, result.pci.state,
			result.pci.restored, result.arc.boot_before, result.arc.boot_after,
			result.arc.error_status0);
		return user_memcpy(data, &result, sizeof(result));
	}
	if (operation == TT_PCI_PROBE_PIONEER_NOC_ID) {
		if (data == NULL || length != sizeof(tt_pioneer_mmio_result))
			return B_BAD_VALUE;
		tt_pioneer_mmio_result result = {};
		status_t status = user_memcpy(data, &result, sizeof(result));
		if (status != B_OK)
			return status;
		status = AcquireProbeLock();
		if (status != B_OK)
			return status;
		status = sPCI->reserve_device(sCard.bus, sCard.device, sCard.function,
			TT_PCI_PROBE_DEVICE, &sCookie);
		if (status != B_OK) {
			release_sem(sLock);
			return status;
		}
		if (sDeviceManager == NULL)
			status = get_module(B_DEVICE_MANAGER_MODULE_NAME,
				(module_info**)&sDeviceManager);
		tt_pioneer_profile profile = {};
		if (status == B_OK)
			status = GetPioneerPciProfile(sDeviceManager, sCard, profile);
		profile.status = status;
		result.version = 1;
		result.profile_status = status;
		result.profile_stage = profile.stage;
		if (status == B_OK) {
			PioneerMmioIO io(profile);
			BlackholePioneerMmioProbe(io, result);
		}
		sPCI->unreserve_device(sCard.bus, sCard.device, sCard.function,
			TT_PCI_PROBE_DEVICE, &sCookie);
		release_sem(sLock);
		dprintf("PIONEER-P100A: NoC ID probe state %" B_PRIu32
			" stage %" B_PRIu32 " restored %" B_PRIu32 " values %08" B_PRIx32
			"/%08" B_PRIx32 "/%08" B_PRIx32 " (DMA never enabled)\n",
			result.state, result.stage, result.restored,
			result.node_id[0], result.node_id[1], result.node_id[2]);
		return user_memcpy(data, &result, sizeof(result));
	}
	if (operation == TT_PCI_PROBE_PIONEER_PROFILE) {
		if (data == NULL || length != sizeof(tt_pioneer_profile))
			return B_BAD_VALUE;
		dprintf("PIONEER-P100A: profile semaphore begin\n");
		status_t status = AcquireProbeLock();
		dprintf("PIONEER-P100A: profile semaphore status %" B_PRId32 "\n", status);
		if (status != B_OK)
			return status;
		tt_pioneer_profile profile = {};
		if (sDeviceManager == NULL) {
			dprintf("PIONEER-P100A: profile device manager module begin\n");
			status = get_module(B_DEVICE_MANAGER_MODULE_NAME,
				(module_info**)&sDeviceManager);
			dprintf("PIONEER-P100A: profile device manager module status %" B_PRId32 "\n", status);
		}
		if (status == B_OK)
			status = GetPioneerPciProfile(sDeviceManager, sCard, profile);
		dprintf("PIONEER-P100A: profile returned status %" B_PRId32 " stage %u\n", status, profile.stage);
		profile.status = status;
		release_sem(sLock);
		return user_memcpy(data, &profile, sizeof(profile));
	}
	if (operation != TT_PCI_PROBE_BAR4_ADDRESS || data == NULL
		|| length != sizeof(tt_bar4_probe_result))
		return B_BAD_VALUE;
	tt_bar4_probe_result result = {};
	status_t status = user_memcpy(data, &result, sizeof(result));
	if (status != B_OK)
		return status;
	status = AcquireProbeLock();
	if (status != B_OK)
		return status;
	status = sPCI->reserve_device(sCard.bus, sCard.device, sCard.function,
		TT_PCI_PROBE_DEVICE, &sCookie);
	if (status != B_OK) {
		release_sem(sLock);
		return status;
	}
	ConfigIO io;
	BlackholeBar4AddressProbe(io, result);
	sPCI->unreserve_device(sCard.bus, sCard.device, sCard.function,
		TT_PCI_PROBE_DEVICE, &sCookie);
	release_sem(sLock);
	dprintf("PIONEER-P100A: BAR4 address probe state %" B_PRIu32
		" trial %08" B_PRIx32 ":%08" B_PRIx32 " command %04" B_PRIx32
		" -> %04" B_PRIx32 " (no MMIO/DMA)\n", result.state,
		result.trial[1], result.trial[0], result.command_before,
		result.command_after);
	return user_memcpy(data, &result, sizeof(result));
}

static status_t
ProbeData(void*, off_t, void*, size_t* length)
{
	*length = 0;
	return B_NOT_ALLOWED;
}

static status_t
ProbeWrite(void*, off_t, const void*, size_t* length)
{
	*length = 0;
	return B_NOT_ALLOWED;
}

device_hooks*
find_device(const char* name)
{
#if defined(PIONEER_Q38_RESIDENT_OUTPUT_CONTEXT_COMPILE_ONLY) || defined(PIONEER_Q38_RESIDENT_LAYER_CONTEXT_COMPILE_ONLY) || (defined(PIONEER_Q38_MIXED_LIVE_CONTEXT_COMPILE_ONLY) && !defined(PIONEER_Q38_MIXED_LIVE_EMIT_CONTEXT_BODY))
	return NULL; // no hooks,including no old Work/CPU/DEBUG controls
#endif
	static device_hooks hooks = {
		ProbeOpen, ProbeClose, ProbeFree, ProbeControl, ProbeData, ProbeWrite,
		NULL, NULL, NULL, NULL
	};
	return strcmp(name, TT_PCI_PROBE_DEVICE) == 0 ? &hooks : NULL;
}
