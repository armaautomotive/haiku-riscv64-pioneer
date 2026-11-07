/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_QGW8_UPLOAD_PROTOCOL_H
#define PIONEER_Q38_QGW8_UPLOAD_PROTOCOL_H
#if !defined(PIONEER_Q38_QGW8_CONTROL_INSPECTION) || PIONEER_Q38_QGW8_CONTROL_INSPECTION != 1 \
 || !defined(PIONEER_Q38_QGW8_SPAN_INSPECTION) || PIONEER_Q38_QGW8_SPAN_INSPECTION != 1 \
 || !defined(PIONEER_Q38_QGW8_LAYOUT_INSPECTION) || PIONEER_Q38_QGW8_LAYOUT_INSPECTION != 1 \
 || !defined(PIONEER_Q38_HYBRID_TRACKED_UPLOAD) || PIONEER_Q38_HYBRID_TRACKED_UPLOAD != 1 \
 || (defined(_KERNEL_MODE) && (!defined(PIONEER_Q38_HYBRID_KERNEL_COMPILE_ONLY) || PIONEER_Q38_HYBRID_KERNEL_COMPILE_ONLY != 1)) \
 || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) \
 || defined(PIONEER_Q38_BF16_FULL_NATIVE) || defined(PIONEER_Q38_BF16_FULL_CLIENT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error QGW8 upload control is non-loadable inspection only
#endif
#include <stdint.h>
// No ioctl number is allocated by this inspection packet. The magic is
// intentionally distinct from the legacy hybrid declaration; this does not
// authenticate the source or authorize a physical driver.
namespace PioneerQ38Qgw8Upload {
enum {Version=1,Begin=1,Seal=2,Magic=0x38554851};
struct Packet {
 uint32_t magic=Magic,version=Version,operation=0,reserved=0;
 uint32_t accepted=0,sealed=0,spans=0,recovery_required=0;
};
static_assert(sizeof(Packet)==32,"bounded pointer-free QGW8 inspection packet");
static inline bool Request(const Packet& p){
 return p.magic==Magic && p.version==Version && (p.operation==Begin || p.operation==Seal)
  && !p.reserved && !p.accepted && !p.sealed && !p.spans && !p.recovery_required;
}
}
#endif
