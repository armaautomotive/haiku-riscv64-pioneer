/* SPDX-License-Identifier: MIT */
#define B_DEVICE_OP_CODES_END 10000
#include "../../../src/add-ons/kernel/drivers/misc/PioneerNativeBfp4DramProtocol.h"
#include <cassert>
#include <cstdio>
int main(){
#if defined(PIONEER_Q38_BFP4_MATCHED_DIAGNOSTIC)
 constexpr uint64_t other=UINT64_C(0x2026110600010120);
 assert(PioneerNativeBfp4DramVariant::Tag==UINT64_C(0x2026110700010121));
 assert(PioneerNativeBfp4DramVariant::Opcode==0x8923);
#else
 constexpr uint64_t other=UINT64_C(0x2026110700010121);
 assert(PioneerNativeBfp4DramVariant::Tag==UINT64_C(0x2026110600010120));
 assert(PioneerNativeBfp4DramVariant::Opcode==0x8920);
#endif
 PioneerNativeBfp4DramRequest request={};request.version=1;
 request.expected_generation=7;request.expected_tag=PioneerNativeBfp4DramVariant::Tag;
 assert(PioneerNativeBfp4DramRequestValid(request));
 request.expected_tag=other;assert(!PioneerNativeBfp4DramRequestValid(request));
 request.expected_tag=PioneerNativeBfp4DramVariant::Tag;request.result.compute.state=1;
 assert(!PioneerNativeBfp4DramRequestValid(request));
 puts("PASS distinct BFP4 diagnostic tags/opcodes and cross-variant request refusal; NO DEVICE");
}
