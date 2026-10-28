/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_STATE_MATH_H
#define PIONEER_Q38_STATE_MATH_H
#include "PioneerQ38ResidualRms.h"
// Exact opt-in new returning components ONLY. The default preprocessed
// calls remain the old helpers;never widen learned/activation/FP16 domains.
#ifdef PIONEER_Q38_GLOBAL_CHAIN_F32_HIDDEN
#if PIONEER_Q38_GLOBAL_CHAIN_F32_HIDDEN != 1 || !defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE) \
 || PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE != 1 || defined(PIONEER_Q38_GLOBAL_CHAIN_CONV_CAPSULE_IMAGE)
#error Exact separate returning FP32-hidden component required
#endif
#include "PioneerQ38F32HiddenRms.h"
static inline void PioneerQ38StateMathUnused(){(void)&PioneerQ38LearnedRms;(void)&PioneerQ38ResidualRms;}
#define PioneerQ38StateDomain PioneerQ38F32HiddenDomain
#define PioneerQ38StateRms PioneerQ38F32HiddenRms
#define PioneerQ38StateResidualRms PioneerQ38F32HiddenResidualRms
#else
#define PioneerQ38StateDomain PioneerQ38RmsDomain
#define PioneerQ38StateRms PioneerQ38LearnedRms
#define PioneerQ38StateResidualRms PioneerQ38ResidualRms
#endif
#endif
