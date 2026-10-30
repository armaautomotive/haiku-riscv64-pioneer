/* SPDX-License-Identifier: MIT */
#include "PioneerQ38GlobalChainIsolation.h"
#ifdef PIONEER_Q38_GLOBAL_CHAIN_SINGLE_CLOSED_AUDIT
#include "PioneerQ38SingleClosedAuditRole.h"
#endif
#ifndef PIONEER_Q38_SINGLE_CLOSED_AUDIT_BINDING_H
#define PIONEER_Q38_SINGLE_CLOSED_AUDIT_BINDING_H
#ifdef PIONEER_Q38_GLOBAL_CHAIN_SINGLE_CLOSED_AUDIT
enum {PioneerQ38SingleClosedAuditProfile=1};
#else
enum {PioneerQ38SingleClosedAuditProfile=0};
#endif
#endif
#ifdef PIONEER_Q38_GLOBAL_CHAIN_SINGLE_CLOSED_AUDIT
static_assert(PioneerQ38SingleClosedAuditProfile==1,"late single-closed-audit policy change");
#else
static_assert(PioneerQ38SingleClosedAuditProfile==0,"late repeated-closed-audit policy change");
#endif
#ifdef PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION
#include "PioneerQ38PreciseActivationRootRole.h"
#endif
#if !defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE) || PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE != 1 \
 || defined(PIONEER_Q38_GLOBAL_CHAIN_ROOT_IMAGE) || !defined(PIONEER_Q38_SIGMOID_PRECISION_IMAGE) || PIONEER_Q38_SIGMOID_PRECISION_IMAGE != 1
#error Exact DISTINCT returning QGC1 arithmetic component required
#endif
#if defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) || defined(PIONEER_Q38_GLOBAL_CHAIN_FEEDBACK) || defined(PIONEER_Q38_GLOBAL_CHAIN_FEEDBACK_CAPSULE_IMAGE) \
 || defined(PIONEER_Q38_LAYER_COMPONENT_DENSE_STOP) || defined(PIONEER_Q38_LAYER_COMPONENT_CHAIN_LAYERS) \
 || defined(PIONEER_Q38_MIXED_RECURRENT_COHORT_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_PROJECTION_ROOT_IMAGE) \
 || defined(PIONEER_Q38_MIXED_RECURRENT_PROJECTION_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_DENSE_ROOT_IMAGE) \
 || defined(PIONEER_Q38_MIXED_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_JOINED_ROOT_IMAGE)
#error Global components never include the old terminal STOP or another parent
#endif
#ifdef PIONEER_Q38_GLOBAL_CHAIN_CROSS_TOKEN_COMPONENT
#if PIONEER_Q38_GLOBAL_CHAIN_CROSS_TOKEN_COMPONENT != 1 || !defined(PIONEER_Q38_LAYER_COMPONENT_RECURRENT) \
 || defined(PIONEER_Q38_GLOBAL_CHAIN_CONV_CAPSULE_IMAGE)
#error Cross-token output admission belongs ONLY to the exact returning recurrent component
#endif
#endif
#ifdef PIONEER_Q38_GLOBAL_CHAIN_TOKEN_KV_APPEND
#if PIONEER_Q38_GLOBAL_CHAIN_TOKEN_KV_APPEND != 1 || !defined(PIONEER_Q38_LAYER_COMPONENT_DENSE) \
 || defined(PIONEER_Q38_GLOBAL_CHAIN_CONV_CAPSULE_IMAGE) || defined(PIONEER_Q38_GLOBAL_CHAIN_CROSS_TOKEN_COMPONENT)
#error Token KV append belongs ONLY to the exact returning dense component
#endif
#endif
#ifdef PIONEER_Q38_GLOBAL_CHAIN_SEAL_UNROLL
#if PIONEER_Q38_GLOBAL_CHAIN_SEAL_UNROLL != 1 || defined(PIONEER_Q38_GLOBAL_CHAIN_CONV_CAPSULE_IMAGE)
#error Exact optional check-loop code generation for returning components only
#endif
#endif
#ifdef PIONEER_Q38_GLOBAL_CHAIN_F32_HIDDEN
#if PIONEER_Q38_GLOBAL_CHAIN_F32_HIDDEN != 1 || defined(PIONEER_Q38_GLOBAL_CHAIN_CONV_CAPSULE_IMAGE)
#error Exact separate returning FP32-hidden arithmetic only,not a capsule/old family
#endif
#endif
#ifdef PIONEER_Q38_GLOBAL_CHAIN_LOCAL_SOURCE_GUARDS
#if PIONEER_Q38_GLOBAL_CHAIN_LOCAL_SOURCE_GUARDS != 1 || !defined(PIONEER_Q38_GLOBAL_CHAIN_QUIET_UNROLL) || PIONEER_Q38_GLOBAL_CHAIN_QUIET_UNROLL != 1 \
 || defined(PIONEER_Q38_GLOBAL_CHAIN_CONV_CAPSULE_IMAGE) \
 || (!defined(PIONEER_Q38_GLOBAL_CHAIN_CROSS_TOKEN_COMPONENT) && !defined(PIONEER_Q38_GLOBAL_CHAIN_TOKEN_KV_APPEND))
#error Exact QGW1 cross-token FP32/boundary/queue-unrolled candidate required for worker-local source guards
#endif
#endif
#ifdef PIONEER_Q38_GLOBAL_CHAIN_QUIET_UNROLL
#if PIONEER_Q38_GLOBAL_CHAIN_QUIET_UNROLL != 1 || !defined(PIONEER_Q38_GLOBAL_CHAIN_BOUNDARY_SEALS) || PIONEER_Q38_GLOBAL_CHAIN_BOUNDARY_SEALS != 1 \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_F32_HIDDEN) || PIONEER_Q38_GLOBAL_CHAIN_F32_HIDDEN != 1 \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_SEAL_UNROLL) || PIONEER_Q38_GLOBAL_CHAIN_SEAL_UNROLL != 1 \
 || defined(PIONEER_Q38_GLOBAL_CHAIN_CONV_CAPSULE_IMAGE)
#error Exact separate FP32/boundary/unrolled queue-check candidate required
#endif
#endif
#ifdef PIONEER_Q38_GLOBAL_CHAIN_CONV_CAPSULE_IMAGE
#ifdef PIONEER_Q38_GLOBAL_CHAIN_BOUNDARY_SEALS
#error Boundary seals do not belong to the convolution arithmetic capsule
#endif
#if PIONEER_Q38_GLOBAL_CHAIN_CONV_CAPSULE_IMAGE != 1 || !defined(PIONEER_Q38_MIXED_ROOT_CAPSULE_IMAGE) || PIONEER_Q38_MIXED_ROOT_CAPSULE_IMAGE != 1 \
 || !defined(PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE) || PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE != 1 \
 || defined(PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_LAYER_RETURN_COMPONENT_IMAGE) \
 || defined(PIONEER_Q38_LAYER_COMPONENT_RECURRENT) || defined(PIONEER_Q38_LAYER_COMPONENT_DENSE)
#error Exact QGC1 ROOT convolution capsule only
#endif
#else
#ifdef PIONEER_Q38_GLOBAL_CHAIN_BOUNDARY_SEALS
#if PIONEER_Q38_GLOBAL_CHAIN_BOUNDARY_SEALS != 1 || !defined(PIONEER_Q38_GLOBAL_CHAIN_F32_HIDDEN) || PIONEER_Q38_GLOBAL_CHAIN_F32_HIDDEN != 1 \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_SEAL_UNROLL) || PIONEER_Q38_GLOBAL_CHAIN_SEAL_UNROLL != 1
#error Exact separate FP32/unrolled returning boundary-seal candidate required
#endif
#endif
#if !defined(PIONEER_Q38_LAYER_RETURN_COMPONENT_IMAGE) || PIONEER_Q38_LAYER_RETURN_COMPONENT_IMAGE != 1 \
 || defined(PIONEER_Q38_MIXED_ROOT_CAPSULE_IMAGE) || defined(PIONEER_Q38_LAYER_COMPONENT_RECURRENT) == defined(PIONEER_Q38_LAYER_COMPONENT_DENSE)
#error Exactly one returning arithmetic family required, no capsule promotion
#endif
#ifdef PIONEER_Q38_LAYER_COMPONENT_RECURRENT
#if PIONEER_Q38_LAYER_COMPONENT_RECURRENT != 1 || !defined(PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE) || PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE != 1 || defined(PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE)
#error Exact recurrent arithmetic implementation for the NEW G worker ABI
#endif
#else
#if PIONEER_Q38_LAYER_COMPONENT_DENSE != 1 || !defined(PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE) || PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE != 1 || defined(PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE)
#error Exact dense arithmetic implementation for the NEW G worker ABI
#endif
#endif
#endif
