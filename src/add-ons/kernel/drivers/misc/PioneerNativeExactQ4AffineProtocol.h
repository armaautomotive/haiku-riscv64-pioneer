/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_NATIVE_EXACT_Q4_AFFINE_PROTOCOL_H
#define PIONEER_NATIVE_EXACT_Q4_AFFINE_PROTOCOL_H
#include "PioneerNativeBfpProtocol.h"
// Distinct FIXED synthetic K17408+original-affine qualification. No supplied
// source/data/address/code,model,Work,fallback or old-BFP operation admitted.
constexpr uint64_t PioneerNativeExactQ4AffineTag=UINT64_C(0x2026110500010096);
enum {PIONEER_NATIVE_EXACT_Q4_AFFINE_SUPPORT=B_DEVICE_OP_CODES_END+0x8d00,
 PIONEER_NATIVE_EXACT_Q4_AFFINE_RUN=B_DEVICE_OP_CODES_END+0x8d01,
 PIONEER_NATIVE_EXACT_Q4_AFFINE_END=B_DEVICE_OP_CODES_END+0x8d02};
using PioneerNativeExactQ4AffineRequest=PioneerNativeBfpRequest;
static inline bool PioneerNativeExactQ4AffineRequestValid(const PioneerNativeExactQ4AffineRequest& r){
 return r.version==1 && r.expected_tag==PioneerNativeExactQ4AffineTag && r.expected_generation
  && !r.reserved && PioneerNativeBfpEmptyResult(r.result);
}
#endif
