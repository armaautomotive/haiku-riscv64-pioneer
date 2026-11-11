/* SPDX-License-Identifier: MIT */
// Fixed two-token client protocol only; no driver, card, or model.
#include <cassert>
#define B_DEVICE_OP_CODES_END 1000
#define PIONEER_Q38_QGW8_FULL_SESSION_INSPECTION 1
#define PIONEER_Q38_QGW8_USER_SESSION_INSPECTION 1
#define PIONEER_Q38_QGWD_UPLOAD_SESSION_INSPECTION 1
#define PIONEER_Q38_QGWD_FULL_SESSION_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_VERIFIED_MODEL_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_QGWD_CLIENT_141 1
#define PIONEER_Q38_QGWD_TWO_CLIENT_142 1
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38HybridFullProtocol.h"
int main(){
 namespace F=PioneerQ38FullOffload;namespace P=PioneerQ38TokenControl;
 static_assert(F::BuildTag==UINT64_C(0x2026111100010142) && F::MaxPrompt==1
  && F::MaxOutput==2 && F::MaxPositions==2,"isolated bounded diagnostic");
 P::Packet p={};p.request.magic=P::Magic;p.request.version=P::Version;p.request.operation=P::Start;
 p.request.prompts=1;p.request.limit=2;p.request.stopEos=1;p.request.eos0=PioneerQ38FullOffloadEog::Eos0;
 p.request.eos1=PioneerQ38FullOffloadEog::Eos1;p.prompt[0]=248319;
 assert(F::RequestShape(p));p.prompt[0]=17;assert(!F::RequestShape(p));p.prompt[0]=248319;
 p.request.limit=1;assert(!F::RequestShape(p));p.request.limit=3;assert(!F::RequestShape(p));p.request.limit=2;
 p.request.prompts=2;assert(!F::RequestShape(p));p.request.prompts=1;
 p.request.stopEos=0;assert(!F::RequestShape(p));p.request.stopEos=1;
 p.request.eos1++;assert(!F::RequestShape(p));
}
