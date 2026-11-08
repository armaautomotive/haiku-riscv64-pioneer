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
// Distinct selector and packet for the isolated inspection route only. The
// compile-role checks above still forbid use in a loadable driver. Neither
// the packet nor the selector authenticates the uploaded model bytes.
#define TT_PCI_PROBE_Q38_QGW8_UPLOAD_CONTROL (B_DEVICE_OP_CODES_END+0x8831)
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
