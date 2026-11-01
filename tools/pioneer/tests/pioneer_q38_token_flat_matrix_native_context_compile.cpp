/* SPDX-License-Identifier: MIT */
// Private renamed-ABI context ONLY,not installable or Work.
#define PIONEER_Q38_TOKEN_FLAT_MATRIX_PROGRAMS 1
#define PIONEER_Q38_TOKEN_FLAT_MATRIX_NATIVE_CONTEXT 1
#include "pioneer_q38_token_empty_receipt_poll_native_context_compile.cpp"
static_assert(PioneerQ38TokenFlatMatrixSelection==2
 && PioneerQ38TokenPrograms::FlatMatrixGrammar==1 && PioneerQ38TokenPrograms::EmptyReceiptPoll==1,
 "actual distinct private flat QGW3 code binding");
