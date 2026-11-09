/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_QGW8_TRACE_NATIVE_PROTOCOL_H
#define PIONEER_Q38_QGW8_TRACE_NATIVE_PROTOCOL_H
#include "PioneerQ38TokenControlPacket.h"
// Isolated fixed-token trace DATA, not Work or a general inference ABI.
#define TT_PCI_PROBE_Q38_QGW8_TRACE_READ (B_DEVICE_OP_CODES_END+0x8833)
namespace PioneerQ38Qgw8TraceNative {
inline bool ControlRequestShape(const PioneerQ38TokenControl::Packet& p){
 namespace C=PioneerQ38TokenControl;
 if(!C::RequestShape(p))return false;
 const auto& r=p.request;
 if(r.operation==C::Start)return r.prompts==1 && r.limit==1 && !r.stopEos && p.prompt[0]==248319;
 if(r.operation==C::Cancel)return false;
 return r.operation!=C::Read || r.records==1;
}
}
#endif
