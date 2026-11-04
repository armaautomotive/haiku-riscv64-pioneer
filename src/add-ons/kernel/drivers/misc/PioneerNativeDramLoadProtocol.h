/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_NATIVE_DRAM_LOAD_PROTOCOL_H
#define PIONEER_NATIVE_DRAM_LOAD_PROTOCOL_H
#include "PioneerNativeBfpProtocol.h"
// Distinct LOAD-ONLY diagnostic82. No FPU/model/Work results. Exact native
// payload/header verification happens in explicit END BEFORE restoration.
constexpr uint64_t PioneerNativeDramLoadTag=UINT64_C(0x2026110400010082);
constexpr uint32_t PioneerNativeDramLoadNone=UINT32_MAX;
constexpr uint32_t PioneerNativeDramLoadMarker=0x314c444e; // NDL1,not NDB1
constexpr uint32_t PioneerNativeDramLoadCount=0x80001400; // LOAD ONLY,K5120
enum {PIONEER_NATIVE_DRAM_LOAD_SUPPORT=B_DEVICE_OP_CODES_END+0x7e00,
	PIONEER_NATIVE_DRAM_LOAD_RUN=B_DEVICE_OP_CODES_END+0x7e01,
	PIONEER_NATIVE_DRAM_LOAD_END=B_DEVICE_OP_CODES_END+0x7e02};
struct PioneerNativeDramLoadResult {
	// Lifecycle fields only. output[] must remain staged sentinel7fc00001;
	// it is NEVER learned math or a model reply in this protocol.
	PioneerNativeBfpResult compute;
	uint32_t phase,gddr_restored,niu_restored,clock_restored,requests,flits,saved_l1,saved_gddr;
	uint32_t payload_checked,headers_checked,mismatch_kind,mismatch_offset;
};
struct PioneerNativeDramLoadRequest {
	uint32_t version,sample;
	uint64_t expected_tag,expected_generation,nonce;
	uint32_t accept,reserved;
	PioneerNativeDramLoadResult result;
};
static_assert(sizeof(PioneerNativeDramLoadResult)==368,"distinct load-only byte-check result");
static_assert(sizeof(PioneerNativeDramLoadRequest)==408,"distinct pointer-free load-only82 ABI");
static inline bool PioneerNativeDramLoadRequestValid(const PioneerNativeDramLoadRequest& r) {
	const auto* bytes=reinterpret_cast<const uint8_t*>(&r.result);
	for(unsigned i=0;i<sizeof(r.result);i++)if(bytes[i])return false;
	return r.version==1 && r.expected_tag==PioneerNativeDramLoadTag && r.expected_generation && !r.reserved;
}
#endif
