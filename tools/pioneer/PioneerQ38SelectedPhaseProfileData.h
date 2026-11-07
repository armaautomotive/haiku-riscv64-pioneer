/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_FULL_OFFLOAD_FAST_DECODE_PROFILE
#if PIONEER_Q38_FULL_OFFLOAD_FAST_DECODE_PROFILE != 1 || defined(PIONEER_Q38_BF16_FULL_PROFILE) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_PHASE_PROFILE) || defined(PIONEER_Q38_TOKEN_PHASE_PROFILE_PROGRAMS)
#error Exact separate fast decode timing DATA selector required
#endif
#include "PioneerQ38FastDecodeProfileData.h"
#elif defined(PIONEER_Q38_BF16_FULL_PROFILE)
#if PIONEER_Q38_BF16_FULL_PROFILE != 1
#error Exact BF16 timing DATA selector required
#endif
#include "PioneerQ38Bf16ProfileData.h"
#else
#include "PioneerQ38TokenPhaseProfileData.h"
#endif
#ifndef PIONEER_Q38_SELECTED_PHASE_PROFILE_DATA_H
#define PIONEER_Q38_SELECTED_PHASE_PROFILE_DATA_H
#ifdef PIONEER_Q38_FULL_OFFLOAD_FAST_DECODE_PROFILE
namespace PioneerQ38SelectedPhaseProfileData=PioneerQ38FastDecodeProfileData;
#elif defined(PIONEER_Q38_BF16_FULL_PROFILE)
namespace PioneerQ38SelectedPhaseProfileData=PioneerQ38Bf16ProfileData;
#else
namespace PioneerQ38SelectedPhaseProfileData=PioneerQ38TokenPhaseProfileData;
#endif
#endif
#ifdef PIONEER_Q38_FULL_OFFLOAD_FAST_DECODE_PROFILE
static_assert(PioneerQ38SelectedPhaseProfileData::Magic==0x33485051,"late fast decode timing DATA selection");
#elif defined(PIONEER_Q38_BF16_FULL_PROFILE)
static_assert(PioneerQ38SelectedPhaseProfileData::Magic==0x32485051,"late BF16 timing DATA selection");
#else
static_assert(PioneerQ38SelectedPhaseProfileData::Magic==0x31485051,"late old timing DATA selection");
#endif
