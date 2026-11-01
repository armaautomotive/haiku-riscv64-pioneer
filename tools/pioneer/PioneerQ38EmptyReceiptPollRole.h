/* SPDX-License-Identifier: MIT */
// Returning BUILD only. An empty L1 receipt poll has no NoC side effect;
// source faults defer ONLY until real receipt consumption or timeout.
#include "PioneerQ38ReadSourceBoundaryRole.h"
#ifdef PIONEER_Q38_EMPTY_RECEIPT_POLL_IMAGE
#if PIONEER_Q38_EMPTY_RECEIPT_POLL_IMAGE != 1 \
 || !defined(PIONEER_Q38_READ_SOURCE_BOUNDARY_IMAGE) || PIONEER_Q38_READ_SOURCE_BOUNDARY_IMAGE != 1 \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION) || PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION != 1
#error Empty-receipt polling requires the exact returning read-source/QGW3 BUILD
#endif
#if defined(PIONEER_Q38_TOKEN_EMPTY_RECEIPT_POLL_PROGRAMS) \
 || defined(PIONEER_Q38_TOKEN_EMPTY_RECEIPT_POLL_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_TOKEN_EMPTY_RECEIPT_POLL_BENCHMARK_NATIVE)
#error Empty-receipt BUILD cannot alias DATA or native ownership
#endif
#endif
#ifndef PIONEER_Q38_EMPTY_RECEIPT_POLL_BINDING_H
#define PIONEER_Q38_EMPTY_RECEIPT_POLL_BINDING_H
#ifdef PIONEER_Q38_EMPTY_RECEIPT_POLL_IMAGE
enum {PioneerQ38EmptyReceiptPollProfile=1};
#else
enum {PioneerQ38EmptyReceiptPollProfile=0};
#endif
#endif
#ifdef PIONEER_Q38_EMPTY_RECEIPT_POLL_IMAGE
static_assert(PioneerQ38EmptyReceiptPollProfile==1,"late empty-receipt selection");
#else
static_assert(PioneerQ38EmptyReceiptPollProfile==0,"late empty-receipt removal");
#endif
