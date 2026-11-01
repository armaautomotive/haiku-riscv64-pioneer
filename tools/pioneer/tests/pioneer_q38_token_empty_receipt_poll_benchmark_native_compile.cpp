/* SPDX-License-Identifier: MIT */
// Bounded diagnostic only,NO runtime CPU oracle or Assistant Work.
#define PIONEER_Q38_TOKEN_EMPTY_RECEIPT_POLL_PROGRAMS 1
#define PIONEER_Q38_TOKEN_EMPTY_RECEIPT_POLL_BENCHMARK_NATIVE 1
#include "pioneer_q38_token_counter_gather_benchmark_native_compile.cpp"
static_assert(PioneerQ38TokenEmptyReceiptPollSelection==3
 && PioneerQ38TokenEmptyReceiptPollPhysicalSelection==1 && PioneerQ38TokenPrograms::EmptyReceiptPoll==1
 && PioneerQ38TokenPrograms::ReadSourceBoundary==1,"actual separate bounded diagnostic firmware binding");
