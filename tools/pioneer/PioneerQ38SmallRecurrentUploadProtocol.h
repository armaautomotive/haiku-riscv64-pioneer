/* SPDX-License-Identifier: MIT */
#include "PioneerQ38RecurrentKernelCompileRole.h"
#if !defined(PIONEER_Q38_SMALL_RECURRENT_SPAN_INSPECTION) || PIONEER_Q38_SMALL_RECURRENT_SPAN_INSPECTION != 1 \
 || ((defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT)) && !defined(PIONEER_Q38_RECURRENT_KERNEL_COMPILE_ONLY)) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Small recurrent upload control is non-loadable inspection only
#endif
#ifndef PIONEER_Q38_SMALL_RECURRENT_UPLOAD_PROTOCOL_H
#define PIONEER_Q38_SMALL_RECURRENT_UPLOAD_PROTOCOL_H
#include <stdint.h>
namespace PioneerQ38SmallRecurrentUpload {
enum : uint32_t {Version=1,Begin=1,Seal=2,Magic=0x53554851};
struct Packet {
 uint32_t magic=Magic,version=Version,operation=0,reserved=0;
 uint32_t accepted=0,sealed=0,spans=0,recoveryRequired=0;
};
static_assert(sizeof(Packet)==32,"bounded pointer-free small recurrent control");
static inline bool Request(const Packet& p){
 return p.magic==Magic && p.version==Version && (p.operation==Begin || p.operation==Seal)
  && !p.reserved && !p.accepted && !p.sealed && !p.spans && !p.recoveryRequired;
}
}
#endif
