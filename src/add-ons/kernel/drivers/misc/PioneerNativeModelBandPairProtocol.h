/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_NATIVE_MODEL_BAND_PAIR_PROTOCOL_H
#define PIONEER_NATIVE_MODEL_BAND_PAIR_PROTOCOL_H
#include "PioneerNativeBfpProtocol.h"
#if !defined(PIONEER_NATIVE_MODEL_BAND_PAIR_DIAGNOSTIC) || PIONEER_NATIVE_MODEL_BAND_PAIR_DIAGNOSTIC != 1 \
 || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Two-core QKV protocol is a distinct fixed diagnostic, never Work or validation
#endif
// Fixed pinned QKV bands 0 and 1; no caller-selected source, address, core,
// executable or weights. The separate tag prevents old clients or drivers
// from interpreting this larger request as a single-core operation.
constexpr uint64_t PioneerNativeModelBandPairTag=UINT64_C(0x2026111300010153);
enum { PIONEER_NATIVE_MODEL_BAND_PAIR_SUPPORT=B_DEVICE_OP_CODES_END+0x7d00,
 PIONEER_NATIVE_MODEL_BAND_PAIR_RUN=B_DEVICE_OP_CODES_END+0x7d01,
 PIONEER_NATIVE_MODEL_BAND_PAIR_END=B_DEVICE_OP_CODES_END+0x7d02 };
struct PioneerNativeModelBandPairRequest {
 uint32_t version,sample;
 uint64_t expected_tag,expected_generation,nonce;
 uint32_t accept,reserved2;
 PioneerNativeBfpResult result[2];
};
static_assert(sizeof(PioneerNativeModelBandPairRequest)==680,"fixed pointer-free pair protocol");
static inline bool PioneerNativeModelBandPairRequestValid(const PioneerNativeModelBandPairRequest& r){
 return r.version==1 && !r.reserved2 && r.expected_tag==PioneerNativeModelBandPairTag
  && r.expected_generation && PioneerNativeBfpEmptyResult(r.result[0]) && PioneerNativeBfpEmptyResult(r.result[1]);
}
#endif
