/* SPDX-License-Identifier: MIT */
#include "../../../../../tools/pioneer/PioneerQ38QgwdNativeInspectionRole.h"
#ifndef PIONEER_Q38_QGWD_UPLOAD_PROTOCOL_H
#define PIONEER_Q38_QGWD_UPLOAD_PROTOCOL_H
#if !defined(PIONEER_Q38_QGWD_UPLOAD_SESSION_INSPECTION) || PIONEER_Q38_QGWD_UPLOAD_SESSION_INSPECTION != 1 \
 || (defined(_KERNEL_MODE) && !defined(PIONEER_Q38_QGWD_NATIVE_INSPECTION) && (!defined(PIONEER_Q38_HYBRID_KERNEL_COMPILE_ONLY) || PIONEER_Q38_HYBRID_KERNEL_COMPILE_ONLY != 1)) \
 || ((defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT)) && !defined(PIONEER_Q38_QGWD_NATIVE_INSPECTION)) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) \
 || defined(PIONEER_Q38_BF16_FULL_NATIVE) || defined(PIONEER_Q38_BF16_FULL_CLIENT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error QGWD upload control is synthetic inspection only
#endif
#include <stdint.h>
// QGWD is a distinct physical format: its converted stripes leave holes in
// the QGW8 reservations. This selector is NOT implemented by a live driver.
#define TT_PCI_PROBE_Q38_QGWD_UPLOAD_CONTROL (B_DEVICE_OP_CODES_END+0x8834)
namespace PioneerQ38QgwdUpload {
enum {Version=1,Begin=1,Seal=2,Magic=0x44554851};
struct Packet {
 uint32_t magic=Magic,version=Version,operation=0,reserved=0;
 uint32_t accepted=0,sealed=0,spans=0,recovery_required=0;
};
static_assert(sizeof(Packet)==32,"bounded pointer-free QGWD inspection packet");
static inline bool Request(const Packet& p){
 return p.magic==Magic && p.version==Version && (p.operation==Begin || p.operation==Seal)
  && !p.reserved && !p.accepted && !p.sealed && !p.spans && !p.recovery_required;
}
}
#endif
