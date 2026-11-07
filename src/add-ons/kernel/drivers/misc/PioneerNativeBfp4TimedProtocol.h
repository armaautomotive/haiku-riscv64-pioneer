/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_NATIVE_BFP4_TIMED_PROTOCOL_H
#define PIONEER_NATIVE_BFP4_TIMED_PROTOCOL_H
#include "PioneerNativeBfpProtocol.h"
// Fixed synthetic timed diagnostic only. Never accepted by untimed or Work.
constexpr uint64_t PioneerNativeBfp4TimedTag=UINT64_C(0x2026110600010119);
enum { PIONEER_NATIVE_BFP4_TIMED_SUPPORT=B_DEVICE_OP_CODES_END+0x8910,
 PIONEER_NATIVE_BFP4_TIMED_RUN=B_DEVICE_OP_CODES_END+0x8911,
 PIONEER_NATIVE_BFP4_TIMED_END=B_DEVICE_OP_CODES_END+0x8912 };
struct PioneerNativeBfp4TimedRequest {
 uint32_t version,width;
 uint64_t expected_tag,expected_generation,nonce;
 uint32_t accept,reserved;
 PioneerNativeBfpResult result;
 uint32_t timing[16];
};
static_assert(sizeof(PioneerNativeBfp4TimedRequest)==424,"fixed pointer-free timed BFP4 protocol");
static inline bool PioneerNativeBfp4TimedRequestValid(const PioneerNativeBfp4TimedRequest& r){
 if(r.version!=1 || r.expected_tag!=PioneerNativeBfp4TimedTag || !r.expected_generation
  || r.reserved || !PioneerNativeBfpEmptyResult(r.result))return false;
 for(auto word:r.timing)if(word)return false;
 return true;
}
#endif
