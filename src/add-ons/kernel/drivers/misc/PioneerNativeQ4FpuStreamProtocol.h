/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_NATIVE_Q4_FPU_STREAM_PROTOCOL_H
#define PIONEER_NATIVE_Q4_FPU_STREAM_PROTOCOL_H
#include "PioneerNativeBfpProtocol.h"
// NEW86 fixed9 full-K bands;no caller code/payload/route/address/reset capability.
struct PioneerNativeQ4FpuStreamVariant {
 static constexpr unsigned Role=86,Marker=0x31534651,Fault=0x45534651,Opcode=0x8200,Samples=9;
 static constexpr uint64_t Tag=UINT64_C(0x2026110400010086);
 static constexpr unsigned Width(unsigned s){return s<6?5120:s<9?17408:0;}
 static constexpr unsigned Tensor(unsigned s){return s<3?7:s<6?8:s<9?6:UINT32_MAX;}
 static constexpr unsigned First(unsigned s){return s>=9?UINT32_MAX:s%3==0?0:s%3==1?(s<6?8704:2560):(s<6?17376:5088);}
 static constexpr unsigned Count(unsigned s){return s<9?0x04000000|Width(s):0;}
};
constexpr uint32_t PioneerNativeQ4FpuStreamNone=UINT32_MAX;
enum {PIONEER_NATIVE_Q4_FPU_STREAM_SUPPORT=B_DEVICE_OP_CODES_END+PioneerNativeQ4FpuStreamVariant::Opcode,
 PIONEER_NATIVE_Q4_FPU_STREAM_RUN=PIONEER_NATIVE_Q4_FPU_STREAM_SUPPORT+1,
 PIONEER_NATIVE_Q4_FPU_STREAM_END=PIONEER_NATIVE_Q4_FPU_STREAM_SUPPORT+2};
struct PioneerNativeQ4FpuStreamResult {
 PioneerNativeBfpResult compute;
 uint32_t phase,gddr_restored,niu_restored,clock_restored,requests,flits,saved_l1,saved_gddr;
};
struct PioneerNativeQ4FpuStreamRequest {
 uint32_t version,sample;
 uint64_t expected_tag,expected_generation,nonce;
 uint32_t accept,reserved;
 PioneerNativeQ4FpuStreamResult result;
};
static_assert(sizeof(PioneerNativeQ4FpuStreamResult)==352 && sizeof(PioneerNativeQ4FpuStreamRequest)==392,"fixed pointer-free diagnostic86 ABI");
static inline bool PioneerNativeQ4FpuStreamRequestValid(const PioneerNativeQ4FpuStreamRequest& r) {
 const auto* bytes=reinterpret_cast<const uint8_t*>(&r.result);
 for(unsigned i=0;i<sizeof(r.result);i++)if(bytes[i])return false;
 return r.version==1 && r.expected_tag==PioneerNativeQ4FpuStreamVariant::Tag && r.expected_generation && !r.reserved;
}
#endif
