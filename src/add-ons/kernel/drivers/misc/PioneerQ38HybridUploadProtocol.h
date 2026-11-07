/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_HYBRID_UPLOAD_PROTOCOL_H
#define PIONEER_Q38_HYBRID_UPLOAD_PROTOCOL_H
#include <stdint.h>
#define TT_PCI_PROBE_Q38_HYBRID_UPLOAD_CONTROL (B_DEVICE_OP_CODES_END+0x8830)
namespace PioneerQ38HybridUpload {
enum {Version=1,Begin=1,Seal=2,Magic=0x31554851};
struct Packet {
 uint32_t magic=Magic,version=Version,operation=0,reserved=0;
 uint32_t accepted=0,sealed=0,spans=0,recovery_required=0;
};
static_assert(sizeof(Packet)==32,"bounded pointer-free hybrid upload control");
static inline bool Request(const Packet& p){
 return p.magic==Magic && p.version==Version && (p.operation==Begin || p.operation==Seal)
  && !p.reserved && !p.accepted && !p.sealed && !p.spans && !p.recovery_required;
}
}
#endif
