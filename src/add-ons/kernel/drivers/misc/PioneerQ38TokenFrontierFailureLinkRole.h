/* SPDX-License-Identifier: MIT */
// Separate QFL1 pre-error saved-LINK observation. No old installer or Work
// promotion. Recheck every include, including all original DATA/ABI parents.
#include "PioneerQ38TokenFrontierFailureDetailRole.h"
#if defined(PIONEER_Q38_TOKEN_FRONTIER_FAILURE_LINK_BENCHMARK_NATIVE) && !defined(PIONEER_Q38_TOKEN_FRONTIER_FAILURE_LINK_NATIVE)
#error Public failure-link binding requires its actual observer
#endif
#ifndef PIONEER_Q38_TOKEN_FRONTIER_FAILURE_LINK_SELECTION_H
#define PIONEER_Q38_TOKEN_FRONTIER_FAILURE_LINK_SELECTION_H
#ifdef PIONEER_Q38_TOKEN_FRONTIER_FAILURE_LINK_NATIVE
#ifdef PIONEER_Q38_TOKEN_FRONTIER_FAILURE_LINK_BENCHMARK_NATIVE
enum {PioneerQ38TokenFrontierFailureLinkSelection=2};
#else
enum {PioneerQ38TokenFrontierFailureLinkSelection=1};
#endif
#else
enum {PioneerQ38TokenFrontierFailureLinkSelection=0};
#endif
#endif
#ifdef PIONEER_Q38_TOKEN_FRONTIER_FAILURE_LINK_NATIVE
#if defined(PIONEER_Q38_NATIVE_ROOT_PROFILE_FIXTURE) || defined(PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTROL_FIXTURE)
#error QFL1 diagnostic cannot borrow profile,full-flow or control TEST aliases
#endif
#if PIONEER_Q38_TOKEN_FRONTIER_FAILURE_LINK_NATIVE != 1 || !defined(PIONEER_Q38_TOKEN_FRONTIER_FAILURE_DETAIL_NATIVE) || PIONEER_Q38_TOKEN_FRONTIER_FAILURE_DETAIL_NATIVE != 1
#error Exact failure-link observer and complete failure-detail parents required
#endif
#ifdef PIONEER_Q38_TOKEN_FRONTIER_FAILURE_LINK_BENCHMARK_NATIVE
#if PIONEER_Q38_TOKEN_FRONTIER_FAILURE_LINK_BENCHMARK_NATIVE != 1 || !defined(PIONEER_Q38_TOKEN_FRONTIER_FAILURE_DETAIL_BENCHMARK_NATIVE) || PIONEER_Q38_TOKEN_FRONTIER_FAILURE_DETAIL_BENCHMARK_NATIVE != 1 || defined(PIONEER_Q38_TOKEN_NATIVE_FRONTIER_FAILURE_LINK_FIXTURE)
#error Exact separately selected public failure-link diagnostic required
#endif
static_assert(PioneerQ38TokenFrontierFailureLinkSelection==2
 && PioneerQ38TokenFrontierFailureDetailSelection==2,"late/public failure-link binding");
#else
#ifdef PIONEER_Q38_TOKEN_FRONTIER_FAILURE_DETAIL_BENCHMARK_NATIVE
#error Private failure-link cannot borrow a public parent
#endif
static_assert(PioneerQ38TokenFrontierFailureLinkSelection==1
 && PioneerQ38TokenFrontierFailureDetailSelection==1,"late/private failure-link binding");
#endif
#else
static_assert(PioneerQ38TokenFrontierFailureLinkSelection==0,"late failure-link removal");
#endif
