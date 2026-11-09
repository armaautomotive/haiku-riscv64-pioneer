/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_NATIVE_SRCA_PROBE_PROTOCOL_H
#define PIONEER_NATIVE_SRCA_PROBE_PROTOCOL_H
#include "PioneerNativeBfpProtocol.h"
// Fixed synthetic two-base diagnostic only. No user-provided program,
// addresses, weights, or model/Work execution. No driver currently exposes it.
constexpr uint64_t PioneerNativeSrcaProbeTag=UINT64_C(0x2026110900010135);
enum { PIONEER_NATIVE_SRCA_PROBE_SUPPORT=B_DEVICE_OP_CODES_END+0x8e00,
 PIONEER_NATIVE_SRCA_PROBE_RUN=B_DEVICE_OP_CODES_END+0x8e01,
 PIONEER_NATIVE_SRCA_PROBE_END=B_DEVICE_OP_CODES_END+0x8e02 };
using PioneerNativeSrcaProbeRequest=PioneerNativeBfpRequest;
static inline bool PioneerNativeSrcaProbeRequestValid(const PioneerNativeSrcaProbeRequest& r){
 return r.version==1 && r.expected_tag==PioneerNativeSrcaProbeTag && r.expected_generation
  && !r.reserved && PioneerNativeBfpEmptyResult(r.result);
}
#endif
