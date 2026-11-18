/* SPDX-License-Identifier: MIT */
#define PIONEER_ATTENTION_FREESTANDING
#include "PioneerQwen35DeviceIO.h"

#if !defined(__riscv_xlen) || __riscv_xlen!=32 || defined(PIONEER_Q35_OPERATOR_TEST)
#error Build the token core for the RV32 ROOT controller
#endif

extern "C" bool q35_token_core(PioneerQwen35::DeviceIO* transport,
	PioneerQwen35::RootScratch* scratch,const PioneerQwen35::Request* request)
{
	return transport && scratch && request && PioneerQwen35::Generate(*transport,*scratch,*request);
}
