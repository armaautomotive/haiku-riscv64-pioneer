/* SPDX-License-Identifier: MIT */
#include "PioneerQ38Bf16PairIssueFullBinding.h"
#ifndef PIONEER_Q38_BF16_FULL_PROFILE_BINDING_H
#define PIONEER_Q38_BF16_FULL_PROFILE_BINDING_H
#ifdef PIONEER_Q38_BF16_FULL_PROFILE
enum {PioneerQ38Bf16FullProfileSelection=1};
#else
enum {PioneerQ38Bf16FullProfileSelection=0};
#endif
#endif
#ifdef PIONEER_Q38_BF16_FULL_PROFILE
static_assert(PioneerQ38Bf16FullProfileSelection==1,"late BF16 timing identity selection");
#else
static_assert(PioneerQ38Bf16FullProfileSelection==0,"late BF16 timing identity removal");
#endif
