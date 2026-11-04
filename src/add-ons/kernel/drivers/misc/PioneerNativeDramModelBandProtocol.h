/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_NATIVE_DRAM_MODEL_BAND_PROTOCOL_H
#define PIONEER_NATIVE_DRAM_MODEL_BAND_PROTOCOL_H
#include "PioneerNativeBfpProtocol.h"
// Separate learned-weight GDDR->L1/FPU diagnostic81. Fixed compiled samples,
// never an arbitrary payload/address/route/Work or reset capability.
constexpr uint64_t PioneerNativeDramModelBandTag=UINT64_C(0x2026110400010081);
constexpr uint32_t PioneerNativeDramModelBandNone=UINT32_MAX;
enum {PIONEER_NATIVE_DRAM_MODEL_BAND_SUPPORT=B_DEVICE_OP_CODES_END+0x7d00,
	PIONEER_NATIVE_DRAM_MODEL_BAND_RUN=B_DEVICE_OP_CODES_END+0x7d01,
	PIONEER_NATIVE_DRAM_MODEL_BAND_END=B_DEVICE_OP_CODES_END+0x7d02};
struct PioneerNativeDramModelBandResult {
	PioneerNativeBfpResult compute;
	uint32_t phase,gddr_restored,niu_restored,clock_restored,requests,flits,saved_l1,saved_gddr;
};
struct PioneerNativeDramModelBandRequest {
	uint32_t version,sample;
	uint64_t expected_tag,expected_generation,nonce;
	uint32_t accept,reserved;
	PioneerNativeDramModelBandResult result;
};
static_assert(sizeof(PioneerNativeDramModelBandResult)==352,"fixed GDDR native diagnostic result");
static_assert(sizeof(PioneerNativeDramModelBandRequest)==392,"fixed pointer-free diagnostic81 protocol");
static inline bool PioneerNativeDramModelBandRequestValid(const PioneerNativeDramModelBandRequest& r) {
	const auto* bytes=reinterpret_cast<const uint8_t*>(&r.result);
	for(unsigned i=0;i<sizeof(r.result);i++)if(bytes[i])return false;
	return r.version==1 && r.expected_tag==PioneerNativeDramModelBandTag && r.expected_generation && !r.reserved;
}
#endif
