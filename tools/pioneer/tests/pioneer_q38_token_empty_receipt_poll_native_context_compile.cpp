/* SPDX-License-Identifier: MIT */
// Private renamed-ABI context ONLY,not installable or Work.
#define PIONEER_Q38_TOKEN_EMPTY_RECEIPT_POLL_PROGRAMS 1
#define PIONEER_Q38_TOKEN_EMPTY_RECEIPT_POLL_NATIVE_CONTEXT 1
#include "pioneer_q38_token_counter_gather_native_context_compile.cpp"
static_assert(PioneerQ38TokenEmptyReceiptPollSelection==2
 && PioneerQ38TokenPrograms::EmptyReceiptPoll==1 && PioneerQ38TokenPrograms::ReadSourceBoundary==1
 && PioneerQ38TokenPrograms::CounterGather==1,"actual separate private firmware binding");
