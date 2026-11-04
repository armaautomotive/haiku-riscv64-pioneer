/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_NATIVE_BFP_BAND_PROTOCOL_H
#define PIONEER_NATIVE_BFP_BAND_PROTOCOL_H
#include "PioneerNativeBfpProtocol.h"
// Distinct fixed full-K synthetic diagnostic,NOT old NFB1/Work/model IO.
constexpr uint64_t PioneerNativeBfpBandTag=UINT64_C(0x2026110400010079);
enum { PIONEER_NATIVE_BFP_BAND_SUPPORT=B_DEVICE_OP_CODES_END+0x7b00,
	PIONEER_NATIVE_BFP_BAND_RUN=B_DEVICE_OP_CODES_END+0x7b01,
	PIONEER_NATIVE_BFP_BAND_END=B_DEVICE_OP_CODES_END+0x7b02 };
struct PioneerNativeBfpBandRequest {
	uint32_t version,width;
	uint64_t expected_tag,expected_generation,nonce;
	uint32_t accept,reserved;
	PioneerNativeBfpResult result;
};
static_assert(sizeof(PioneerNativeBfpBandRequest)==360,"fixed pointer-free full-K protocol");
static inline bool PioneerNativeBfpBandRequestValid(const PioneerNativeBfpBandRequest& r) {
	return r.version==1 && r.expected_tag==PioneerNativeBfpBandTag && r.expected_generation
		&& !r.reserved && PioneerNativeBfpEmptyResult(r.result);
}
#endif
