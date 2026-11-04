/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_NATIVE_MIXED_FPU_STREAM_PROTOCOL_H
#define PIONEER_NATIVE_MIXED_FPU_STREAM_PROTOCOL_H
#include "PioneerNativeBfpProtocol.h"
#include "PioneerNativeMixedFpuStreamVariant.h"
constexpr uint32_t PioneerNativeMixedFpuStreamNone=UINT32_MAX;
enum {PIONEER_NATIVE_MIXED_FPU_STREAM_SUPPORT=B_DEVICE_OP_CODES_END+PioneerNativeMixedFpuStreamVariant::Opcode,
	PIONEER_NATIVE_MIXED_FPU_STREAM_RUN=PIONEER_NATIVE_MIXED_FPU_STREAM_SUPPORT+1,
	PIONEER_NATIVE_MIXED_FPU_STREAM_END=PIONEER_NATIVE_MIXED_FPU_STREAM_SUPPORT+2};
struct PioneerNativeMixedFpuStreamResult {
	PioneerNativeBfpResult compute;
	uint32_t phase,gddr_restored,niu_restored,clock_restored,requests,flits,saved_l1,saved_gddr;
};
struct PioneerNativeMixedFpuStreamRequest {
	uint32_t version,sample;
	uint64_t expected_tag,expected_generation,nonce;
	uint32_t accept,reserved;
	PioneerNativeMixedFpuStreamResult result;
};
static_assert(sizeof(PioneerNativeMixedFpuStreamResult)==352 && sizeof(PioneerNativeMixedFpuStreamRequest)==392,"fixed pointer-free diagnostic87 ABI");
static inline bool PioneerNativeMixedFpuStreamRequestValid(const PioneerNativeMixedFpuStreamRequest& r) {
	const auto* bytes=reinterpret_cast<const uint8_t*>(&r.result);
	for(unsigned i=0;i<sizeof(r.result);i++)if(bytes[i])return false;
	return r.version==1 && r.expected_tag==PioneerNativeMixedFpuStreamVariant::Tag && r.expected_generation && !r.reserved;
}
#endif
