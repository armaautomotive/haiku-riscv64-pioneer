/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TOKEN_VALIDATION_PROTOCOL_H
#define PIONEER_Q38_TOKEN_VALIDATION_PROTOCOL_H
#include "PioneerQ38TokenControlPacket.h"
// Distinct bounded validation route ONLY. This DATA header cannot activate
// a module,owner,upload,RUN,STOP,release or Work capability.
#define TT_PCI_PROBE_Q38_TOKEN_VALIDATE (B_DEVICE_OP_CODES_END+0x6603)
namespace PioneerQ38TokenValidation {
static constexpr uint64_t BuildTag=UINT64_C(0x2026103000010074);
enum {Prompts=2,Positions=3,FirstToken=248319,SecondToken=17};
static inline bool RequestShape(const PioneerQ38TokenControl::Packet& p){
 namespace P=PioneerQ38TokenControl;
 if(!P::RequestShape(p))return false;
 const auto& r=p.request;
 if(r.operation==P::Start)return r.prompts==Prompts && r.limit==Positions && !r.stopEos
  && !r.eos0 && !r.eos1 && p.prompt[0]==FirstToken && p.prompt[1]==SecondToken;
 return r.operation!=P::Read || r.records<=Positions;
} // fixed original three-position trial,not arbitrary prompts/generation
}
#endif
