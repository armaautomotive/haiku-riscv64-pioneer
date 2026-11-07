/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_STATE_MATH_H
#define PIONEER_Q38_STATE_MATH_H
#include "PioneerQ38ResidualRms.h"
#ifdef PIONEER_Q38_FAST_RMS_COMPONENT_DIAGNOSTIC
enum {PioneerQ38StateFastRmsSelection=1};
#if PIONEER_Q38_FAST_RMS_COMPONENT_DIAGNOSTIC != 1 || !defined(PIONEER_Q38_FAST_RMS_DIAGNOSTIC) \
 || PIONEER_Q38_FAST_RMS_DIAGNOSTIC != 1 || !defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE) \
 || PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE != 1 || !defined(PIONEER_Q38_GLOBAL_CHAIN_F32_HIDDEN) \
 || PIONEER_Q38_GLOBAL_CHAIN_F32_HIDDEN != 1 || !defined(PIONEER_Q38_GLOBAL_CHAIN_HYBRID_BFP) \
 || PIONEER_Q38_GLOBAL_CHAIN_HYBRID_BFP != 1
#error Approximate RMS component requires the exact separate hybrid inspection image
#endif
#else
enum {PioneerQ38StateFastRmsSelection=0};
#endif
// Exact opt-in new returning components ONLY. The default preprocessed
// calls remain the old helpers;never widen learned/activation/FP16 domains.
#ifdef PIONEER_Q38_GLOBAL_CHAIN_F32_HIDDEN
#if PIONEER_Q38_GLOBAL_CHAIN_F32_HIDDEN != 1 || !defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE) \
 || PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE != 1 || defined(PIONEER_Q38_GLOBAL_CHAIN_CONV_CAPSULE_IMAGE)
#error Exact separate returning FP32-hidden component required
#endif
#include "PioneerQ38F32HiddenRms.h"
#ifdef PIONEER_Q38_FAST_RMS_COMPONENT_DIAGNOSTIC
#include "PioneerQ38FastF32HiddenRms.h"
static inline void PioneerQ38StateMathUnused(){(void)&PioneerQ38LearnedRms;(void)&PioneerQ38ResidualRms;
 (void)&PioneerQ38F32HiddenRms;(void)&PioneerQ38F32HiddenResidualRms;}
#define PioneerQ38StateDomain PioneerQ38F32HiddenDomain
#define PioneerQ38StateRms PioneerQ38FastF32HiddenRms
#define PioneerQ38StateResidualRms PioneerQ38FastF32HiddenResidualRms
#else
static inline void PioneerQ38StateMathUnused(){(void)&PioneerQ38LearnedRms;(void)&PioneerQ38ResidualRms;}
#define PioneerQ38StateDomain PioneerQ38F32HiddenDomain
#define PioneerQ38StateRms PioneerQ38F32HiddenRms
#define PioneerQ38StateResidualRms PioneerQ38F32HiddenResidualRms
#endif
#else
#define PioneerQ38StateDomain PioneerQ38RmsDomain
#define PioneerQ38StateRms PioneerQ38LearnedRms
#define PioneerQ38StateResidualRms PioneerQ38ResidualRms
#endif
#endif
#ifdef PIONEER_Q38_FAST_RMS_COMPONENT_DIAGNOSTIC
static_assert(PioneerQ38StateFastRmsSelection==1,"late approximate RMS component selection");
#else
static_assert(PioneerQ38StateFastRmsSelection==0,"late approximate RMS component removal");
#endif
