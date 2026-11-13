/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_NATIVE_MODEL_BAND_PROTOCOL_H
#define PIONEER_NATIVE_MODEL_BAND_PROTOCOL_H
#include "PioneerNativeBfpProtocol.h"
// Distinct fixed learned-weight diagnostic80. Three COMPILED samples only;
// no runtime weights/code/addresses/route,model inference or Work endpoint.
#if (defined(PIONEER_NATIVE_RECURRENT_BAND_DIAGNOSTIC) + defined(PIONEER_NATIVE_QGWD_DENSE_BAND_DIAGNOSTIC) + defined(PIONEER_NATIVE_RECURRENT_OUT_BAND_DIAGNOSTIC)) > 1
#error Recurrent and dense band diagnostics require distinct identities
#endif
#ifdef PIONEER_NATIVE_RECURRENT_OUT_BAND_DIAGNOSTIC
#if PIONEER_NATIVE_RECURRENT_OUT_BAND_DIAGNOSTIC != 1 || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Recurrent output band protocol is a separate fixed diagnostic only
#endif
constexpr uint64_t PioneerNativeModelBandTag=UINT64_C(0x2026111200010148);
#elif defined(PIONEER_NATIVE_RECURRENT_BAND_DIAGNOSTIC)
#if PIONEER_NATIVE_RECURRENT_BAND_DIAGNOSTIC != 1 || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Recurrent band protocol is a separate fixed diagnostic only
#endif
constexpr uint64_t PioneerNativeModelBandTag=UINT64_C(0x2026111200010147);
#elif defined(PIONEER_NATIVE_QGWD_DENSE_BAND_DIAGNOSTIC)
#if PIONEER_NATIVE_QGWD_DENSE_BAND_DIAGNOSTIC != 1 || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error QGWD dense-band protocol is a separate fixed diagnostic only
#endif
constexpr uint64_t PioneerNativeModelBandTag=UINT64_C(0x2026111000010140);
#else
constexpr uint64_t PioneerNativeModelBandTag=UINT64_C(0x2026110400010080);
#endif
constexpr uint32_t PioneerNativeModelBandNone=UINT32_MAX;
enum { PIONEER_NATIVE_MODEL_BAND_SUPPORT=B_DEVICE_OP_CODES_END+0x7c00,
	PIONEER_NATIVE_MODEL_BAND_RUN=B_DEVICE_OP_CODES_END+0x7c01,
	PIONEER_NATIVE_MODEL_BAND_END=B_DEVICE_OP_CODES_END+0x7c02 };
struct PioneerNativeModelBandRequest {
	uint32_t version,sample;
	uint64_t expected_tag,expected_generation,nonce;
	uint32_t accept,reserved;
	PioneerNativeBfpResult result;
};
static_assert(sizeof(PioneerNativeModelBandRequest)==360,"fixed pointer-free learned-band protocol");
static inline bool PioneerNativeModelBandRequestValid(const PioneerNativeModelBandRequest& r) {
	return r.version==1 && r.expected_tag==PioneerNativeModelBandTag && r.expected_generation
		&& !r.reserved && PioneerNativeBfpEmptyResult(r.result);
}
#endif
