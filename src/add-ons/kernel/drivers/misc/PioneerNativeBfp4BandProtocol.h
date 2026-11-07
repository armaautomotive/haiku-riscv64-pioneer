/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_NATIVE_BFP4_BAND_PROTOCOL_H
#define PIONEER_NATIVE_BFP4_BAND_PROTOCOL_H
#include "PioneerNativeBfpProtocol.h"
// Fixed synthetic BFP4 diagnostic only: separate tag/opcodes from BFP8,
// old NFB1, model inference and Work. No arbitrary addresses or payloads.
constexpr uint64_t PioneerNativeBfp4BandTag=UINT64_C(0x2026110600010118);
enum { PIONEER_NATIVE_BFP4_BAND_SUPPORT=B_DEVICE_OP_CODES_END+0x8900,
 PIONEER_NATIVE_BFP4_BAND_RUN=B_DEVICE_OP_CODES_END+0x8901,
 PIONEER_NATIVE_BFP4_BAND_END=B_DEVICE_OP_CODES_END+0x8902 };
struct PioneerNativeBfp4BandRequest {
 uint32_t version,width;
 uint64_t expected_tag,expected_generation,nonce;
 uint32_t accept,reserved;
 PioneerNativeBfpResult result;
};
static_assert(sizeof(PioneerNativeBfp4BandRequest)==360,"fixed pointer-free BFP4 protocol");
static inline bool PioneerNativeBfp4BandRequestValid(const PioneerNativeBfp4BandRequest& r){
 return r.version==1 && r.expected_tag==PioneerNativeBfp4BandTag && r.expected_generation
  && !r.reserved && PioneerNativeBfpEmptyResult(r.result);
}
#endif
