/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_NATIVE_EXACT_Q4_PROTOCOL_H
#define PIONEER_NATIVE_EXACT_Q4_PROTOCOL_H
#include "PioneerNativeBfpProtocol.h"
// NEW fixed synthetic qualification family. Pointer-free identical carrier
// layout is deliberate;new exact tag/opcodes are mandatory. No old BFP or
// model/Work request is admitted. No supplied weights,code,address or route.
constexpr uint64_t PioneerNativeExactQ4Tag=UINT64_C(0x2026110500010095);
enum {PIONEER_NATIVE_EXACT_Q4_SUPPORT=B_DEVICE_OP_CODES_END+0x8c00,
 PIONEER_NATIVE_EXACT_Q4_RUN=B_DEVICE_OP_CODES_END+0x8c01,
 PIONEER_NATIVE_EXACT_Q4_END=B_DEVICE_OP_CODES_END+0x8c02};
using PioneerNativeExactQ4Request=PioneerNativeBfpRequest;
static inline bool PioneerNativeExactQ4RequestValid(const PioneerNativeExactQ4Request& r) {
 return r.version==1 && r.expected_tag==PioneerNativeExactQ4Tag && r.expected_generation
  && !r.reserved && PioneerNativeBfpEmptyResult(r.result);
}
#endif
