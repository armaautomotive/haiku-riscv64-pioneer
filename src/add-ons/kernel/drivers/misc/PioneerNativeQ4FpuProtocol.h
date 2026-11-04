/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_NATIVE_Q4_FPU_PROTOCOL_H
#define PIONEER_NATIVE_Q4_FPU_PROTOCOL_H
#include "PioneerNativeBfpProtocol.h"
// Distinct diagnostic85:THREE compiled learned K256 samples only. No runtime
// operands/code/addresses/route/GDDR/reset/model/Work capability is accepted.
struct PioneerNativeQ4FpuVariant {
	static constexpr unsigned Role=85,Marker=0x31504651,Fault=0x45504651,Count=0x04000100,Opcode=0x8100;
	static constexpr uint64_t Tag=UINT64_C(0x2026110400010085);
};
constexpr uint32_t PioneerNativeQ4FpuNone=UINT32_MAX;
enum {PIONEER_NATIVE_Q4_FPU_SUPPORT=B_DEVICE_OP_CODES_END+PioneerNativeQ4FpuVariant::Opcode,
	PIONEER_NATIVE_Q4_FPU_RUN=PIONEER_NATIVE_Q4_FPU_SUPPORT+1,
	PIONEER_NATIVE_Q4_FPU_END=PIONEER_NATIVE_Q4_FPU_SUPPORT+2};
struct PioneerNativeQ4FpuResult {
	PioneerNativeBfpResult compute;
	uint32_t phase,niu_restored,clock_restored,saved_l1;
};
struct PioneerNativeQ4FpuRequest {
	uint32_t version,sample;
	uint64_t expected_tag,expected_generation,nonce;
	uint32_t accept,reserved;
	PioneerNativeQ4FpuResult result;
};
static_assert(sizeof(PioneerNativeQ4FpuResult)==336 && sizeof(PioneerNativeQ4FpuRequest)==376,"fixed pointer-free diagnostic85 ABI");
static inline bool PioneerNativeQ4FpuRequestValid(const PioneerNativeQ4FpuRequest& r) {
	const auto* bytes=reinterpret_cast<const uint8_t*>(&r.result);
	for(unsigned i=0;i<sizeof(r.result);i++)if(bytes[i])return false;
	return r.version==1 && r.expected_tag==PioneerNativeQ4FpuVariant::Tag && r.expected_generation && !r.reserved;
}
#endif
