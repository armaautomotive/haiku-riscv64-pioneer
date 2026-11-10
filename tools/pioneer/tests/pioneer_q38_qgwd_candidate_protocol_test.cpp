/* SPDX-License-Identifier: MIT */
// Candidate protocol shape only: no driver, card or model execution.
#include <cassert>
#define B_DEVICE_OP_CODES_END 1000
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38HybridFullProtocol.h"
int main(){
 namespace F=PioneerQ38FullOffload;namespace P=PioneerQ38TokenControl;
 static_assert(F::BuildTag==UINT64_C(0x2026111000010141) && F::MaxPrompt==1 && F::MaxOutput==1 && F::MaxPositions==1,
  "QGWD diagnostic is a distinct one-token capability");
 P::Packet p={};p.request.magic=P::Magic;p.request.version=P::Version;p.request.operation=P::Start;
 p.request.prompts=p.request.limit=p.request.stopEos=1;p.request.eos0=PioneerQ38FullOffloadEog::Eos0;
 p.request.eos1=PioneerQ38FullOffloadEog::Eos1;p.prompt[0]=248319;
 assert(F::RequestShape(p));p.prompt[0]=17;assert(!F::RequestShape(p));p.prompt[0]=248319;
 p.request.limit=2;assert(!F::RequestShape(p));p.request.limit=1;
 p.request.prompts=2;assert(!F::RequestShape(p));p.request.prompts=1;
 p.request.stopEos=0;assert(!F::RequestShape(p));p.request.stopEos=1;
 p.request.eos0++;assert(!F::RequestShape(p));p.request.eos0=PioneerQ38FullOffloadEog::Eos0;
 p.request.operation=P::Observe;p.request.prompts=p.request.limit=p.request.stopEos=p.request.eos0=p.request.eos1=0;
 p.prompt[0]=0;assert(F::RequestShape(p));
}
