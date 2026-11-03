/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_NATIVE_BFP_PROTOCOL_H
#define PIONEER_NATIVE_BFP_PROTOCOL_H
#include <stdint.h>
#include <stddef.h>
// Isolated fixed synthetic tile qualification, NOT model/Work inference.
// No user code, weights, addresses, routes, resets or pointer capabilities.
constexpr uint64_t PioneerNativeBfpTag=UINT64_C(0x2026110300010078);
enum { PIONEER_NATIVE_BFP_SUPPORT=B_DEVICE_OP_CODES_END+0x7a00,
	PIONEER_NATIVE_BFP_RUN=B_DEVICE_OP_CODES_END+0x7a01,
	PIONEER_NATIVE_BFP_END=B_DEVICE_OP_CODES_END+0x7a02 };
enum { PIONEER_NATIVE_BFP_EMPTY=0,PIONEER_NATIVE_BFP_RESULT=1,
	PIONEER_NATIVE_BFP_CLOSED=2,PIONEER_NATIVE_BFP_REFUSED=3,PIONEER_NATIVE_BFP_UNKNOWN=4 };
struct PioneerNativeBfpResult {
	uint32_t state,started,completed,held,l1_restored,tlb_restored,cold,polls;
	uint32_t controls[3],pc[2],phase,count,marker;
	uint32_t output[64];
};
struct PioneerNativeBfpRequest {
	uint32_t version,bits;
	uint64_t expected_tag,expected_generation,nonce;
	uint32_t accept,reserved;
	PioneerNativeBfpResult result;
};
static_assert(sizeof(PioneerNativeBfpResult)==320,"fixed result layout");
static_assert(sizeof(PioneerNativeBfpRequest)==360,"fixed pointer-free private protocol");
static inline bool PioneerNativeBfpEmptyResult(const PioneerNativeBfpResult& r) {
	const uint8_t* p=reinterpret_cast<const uint8_t*>(&r);
	for(unsigned i=0;i<sizeof(r);i++)if(p[i])return false;
	return true;
}
static inline bool PioneerNativeBfpRequestValid(const PioneerNativeBfpRequest& r) {
	return r.version==1 && r.expected_tag==PioneerNativeBfpTag && r.expected_generation
		&& !r.reserved && PioneerNativeBfpEmptyResult(r.result);
}
#endif
