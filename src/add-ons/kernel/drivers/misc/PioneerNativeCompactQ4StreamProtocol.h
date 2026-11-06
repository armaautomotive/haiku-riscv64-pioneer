/* SPDX-License-Identifier: MIT */
#include "PioneerNativeCompactQ4TimingRole.h"
#ifndef PIONEER_NATIVE_COMPACT_Q4_STREAM_PROTOCOL_H
#define PIONEER_NATIVE_COMPACT_Q4_STREAM_PROTOCOL_H
#include "PioneerNativeBfpProtocol.h"
// NEW104 fixed9 full-K bands;no caller code/payload/route/address/reset capability.
struct PioneerNativeCompactQ4StreamVariant {
#ifdef PIONEER_NATIVE_FACTORED_Q4_TRACE
 static constexpr unsigned Role=112,Marker=0x31525151,Fault=0x45525151,Opcode=0x8800,Samples=1;
 static constexpr uint64_t Tag=UINT64_C(0x2026110600010112);
 static constexpr unsigned ImageBytes=12104,ReplyExtraBytes=64;
#elif defined(PIONEER_NATIVE_FACTORED_Q4_COMPARE)
 static constexpr unsigned Role=111,Marker=0x31435151,Fault=0x45435151,Opcode=0x8700,Samples=9;
 static constexpr uint64_t Tag=UINT64_C(0x2026110600010111);
 static constexpr unsigned ImageBytes=11792,ReplyExtraBytes=64;
#elif defined(PIONEER_NATIVE_COMPACT_Q4_PAIR_ISSUE_TIMING)
 static constexpr unsigned Role=108,Marker=0x31504351,Fault=0x45504351,Opcode=0x8600,Samples=9;
 static constexpr uint64_t Tag=UINT64_C(0x2026110600010108);
 static constexpr unsigned ImageBytes=7820,ReplyExtraBytes=64;
#elif defined(PIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING)
 static constexpr unsigned Role=107,Marker=0x31544351,Fault=0x45544351,Opcode=0x8500,Samples=9;
 static constexpr uint64_t Tag=UINT64_C(0x2026110600010107);
 static constexpr unsigned ImageBytes=7972,ReplyExtraBytes=64;
#else
 static constexpr unsigned Role=104,Marker=0x31534351,Fault=0x45534351,Opcode=0x8400,Samples=9;
 static constexpr uint64_t Tag=UINT64_C(0x2026110500010104);
 static constexpr unsigned ImageBytes=7644,ReplyExtraBytes=0;
#endif
 static constexpr unsigned Width(unsigned s){return s>=Samples?0:s<6?5120:17408;}
 static constexpr unsigned Tensor(unsigned s){return s>=Samples?UINT32_MAX:s<3?7:s<6?8:6;}
 static constexpr unsigned First(unsigned s){return s>=Samples?UINT32_MAX:s%3==0?0:s%3==1?(s<6?8704:2560):(s<6?17376:5088);}
 static constexpr unsigned Count(unsigned s){return s<Samples?0x04000000|Width(s):0;}
};
constexpr uint32_t PioneerNativeCompactQ4StreamNone=UINT32_MAX;
enum {PIONEER_NATIVE_COMPACT_Q4_STREAM_SUPPORT=B_DEVICE_OP_CODES_END+PioneerNativeCompactQ4StreamVariant::Opcode,
 PIONEER_NATIVE_COMPACT_Q4_STREAM_RUN=PIONEER_NATIVE_COMPACT_Q4_STREAM_SUPPORT+1,
 PIONEER_NATIVE_COMPACT_Q4_STREAM_END=PIONEER_NATIVE_COMPACT_Q4_STREAM_SUPPORT+2};
struct PioneerNativeCompactQ4StreamResult {
 PioneerNativeBfpResult compute;
 uint32_t phase,gddr_restored,niu_restored,clock_restored,requests,flits,saved_l1,saved_gddr;
#ifdef PIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING
 uint32_t timing[16]; // Raw preserved telemetry,NEVER completion/cleanup authority.
#endif
};
struct PioneerNativeCompactQ4StreamRequest {
 uint32_t version,sample;
 uint64_t expected_tag,expected_generation,nonce;
 uint32_t accept,reserved;
 PioneerNativeCompactQ4StreamResult result;
};
static_assert(sizeof(PioneerNativeCompactQ4StreamResult)==352+PioneerNativeCompactQ4StreamVariant::ReplyExtraBytes
 && sizeof(PioneerNativeCompactQ4StreamRequest)==392+PioneerNativeCompactQ4StreamVariant::ReplyExtraBytes,"fixed pointer-free compact Q4 variant ABI");
static inline bool PioneerNativeCompactQ4StreamRequestValid(const PioneerNativeCompactQ4StreamRequest& r) {
 const auto* bytes=reinterpret_cast<const uint8_t*>(&r.result);
 for(unsigned i=0;i<sizeof(r.result);i++)if(bytes[i])return false;
 return r.version==1 && r.expected_tag==PioneerNativeCompactQ4StreamVariant::Tag && r.expected_generation && !r.reserved;
}
#endif
