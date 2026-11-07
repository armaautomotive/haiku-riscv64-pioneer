/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_NATIVE_BFP4_DRAM_PROTOCOL_H
#define PIONEER_NATIVE_BFP4_DRAM_PROTOCOL_H
#include "PioneerNativeBfpProtocol.h"
#if defined(PIONEER_Q38_BFP4_MATCHED_DIAGNOSTIC) && PIONEER_Q38_BFP4_MATCHED_DIAGNOSTIC != 1
#error The matched-input BFP4 diagnostic role must be explicitly 1
#endif
// New fixed-sample BFP4 GDDR diagnostic. Not arbitrary payloads, model Work,
// the earlier BFP8 GDDR diagnostic, or a capability to reset/retry the card.
struct PioneerNativeBfp4DramVariant {
 static constexpr unsigned Role=120,Marker=0x34534442,Fault=0x4542444e,Count=0x02001400;
#if defined(PIONEER_Q38_BFP4_MATCHED_DIAGNOSTIC)
 static constexpr unsigned Opcode=0x8923;
 static constexpr unsigned long long Tag=0x2026110700010121ULL;
#else
 static constexpr unsigned Opcode=0x8920;
 static constexpr unsigned long long Tag=0x2026110600010120ULL;
#endif
};
constexpr uint32_t PioneerNativeBfp4DramNone=UINT32_MAX;
enum {PIONEER_NATIVE_BFP4_DRAM_SUPPORT=B_DEVICE_OP_CODES_END+PioneerNativeBfp4DramVariant::Opcode,
 PIONEER_NATIVE_BFP4_DRAM_RUN=PIONEER_NATIVE_BFP4_DRAM_SUPPORT+1,
 PIONEER_NATIVE_BFP4_DRAM_END=PIONEER_NATIVE_BFP4_DRAM_SUPPORT+2};
struct PioneerNativeBfp4DramResult {
 PioneerNativeBfpResult compute;
 uint32_t phase,gddr_restored,niu_restored,clock_restored,requests,flits,saved_l1,saved_gddr;
 uint32_t timing[16];
};
struct PioneerNativeBfp4DramRequest {
 uint32_t version,sample;
 uint64_t expected_tag,expected_generation,nonce;
 uint32_t accept,reserved;
 PioneerNativeBfp4DramResult result;
};
static_assert(sizeof(PioneerNativeBfp4DramResult)==416,"fixed timed GDDR result");
static_assert(sizeof(PioneerNativeBfp4DramRequest)==456,"fixed pointer-free BFP4 GDDR request");
static inline bool PioneerNativeBfp4DramRequestValid(const PioneerNativeBfp4DramRequest& r){
 const auto* bytes=reinterpret_cast<const uint8_t*>(&r.result);
 for(unsigned i=0;i<sizeof(r.result);i++)if(bytes[i])return false;
 return r.version==1 && r.expected_tag==PioneerNativeBfp4DramVariant::Tag && r.expected_generation && !r.reserved;
}
#endif
