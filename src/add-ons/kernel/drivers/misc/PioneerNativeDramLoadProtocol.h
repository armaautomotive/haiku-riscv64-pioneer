/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_NATIVE_DRAM_LOAD_PROTOCOL_H
#define PIONEER_NATIVE_DRAM_LOAD_PROTOCOL_H
#include "PioneerNativeBfpProtocol.h"
#include "PioneerNativeDramLoadVariant.h"
// Distinct LOAD-ONLY82/default or explicit C64 placement83 compile role.
// Tag/opcodes/marker/count differ;the fixed pointer-free ABI size is shared.
// No runtime mode switch or FPU/model/Work results. Exact native
// payload/header verification happens in explicit END BEFORE restoration.
constexpr uint64_t PioneerNativeDramLoadTag=PioneerNativeDramLoadVariant::Tag;
constexpr uint32_t PioneerNativeDramLoadNone=UINT32_MAX;
constexpr uint32_t PioneerNativeDramLoadMarker=PioneerNativeDramLoadVariant::Marker;
constexpr uint32_t PioneerNativeDramLoadCount=PioneerNativeDramLoadVariant::Count;
enum {PIONEER_NATIVE_DRAM_LOAD_SUPPORT=B_DEVICE_OP_CODES_END+PioneerNativeDramLoadVariant::Opcode,
	PIONEER_NATIVE_DRAM_LOAD_RUN=PIONEER_NATIVE_DRAM_LOAD_SUPPORT+1,
	PIONEER_NATIVE_DRAM_LOAD_END=PIONEER_NATIVE_DRAM_LOAD_SUPPORT+2};
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
