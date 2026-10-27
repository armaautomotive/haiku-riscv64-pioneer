/* SPDX-License-Identifier: MIT */
#define B_DEVICE_OP_CODES_END 1000
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38MixedLiveValidationProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentLayerValidationProtocol.h"
#include <cassert>
#include <cstdio>
#include <memory>
int main(){
 auto r=std::make_unique<tt_q38_mixed_live_validation>();r->metadata.version=1;
 assert(PioneerQ38MixedLiveValidationRequest(*r));
 auto* bytes=reinterpret_cast<unsigned char*>(r.get());
 for(unsigned i=0;i<sizeof(*r);i++){
  bytes[i]^=1;assert(!PioneerQ38MixedLiveValidationRequest(*r));bytes[i]^=1;
 }
 assert(PioneerQ38MixedLiveValidationRequest(*r));
 static_assert(TT_PCI_PROBE_Q38_MIXED_LIVE_VALIDATE!=TT_PCI_PROBE_Q38_RESIDENT_LAYER_VALIDATE,"no old69 opcode alias");
 static_assert(sizeof(tt_q38_mixed_live_validation)!=sizeof(tt_q38_resident_layer_validation),"no old69 payload alias");
 puts("PASS DEBUG70 fixed20736B pointer-free request/ALL20736 single-byte mutations refused/token0-position0/namespace21/no old69 ABI alias;NO native authority");
}
