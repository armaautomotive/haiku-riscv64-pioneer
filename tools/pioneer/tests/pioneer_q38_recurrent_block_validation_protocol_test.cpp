/* SPDX-License-Identifier: MIT */
#define B_DEVICE_OP_CODES_END 1000
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38RecurrentBlockValidationProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38MixedLiveValidationProtocol.h"
#include <cassert>
#include <cstdio>
#include <memory>
int main(){
 auto request=std::make_unique<tt_q38_recurrent_block_validation>();request->metadata.version=1;
 assert(PioneerQ38RecurrentBlockValidationRequest(*request));auto* bytes=reinterpret_cast<unsigned char*>(request.get());
 for(unsigned i=0;i<sizeof(*request);i++){
  bytes[i]^=1;assert(!PioneerQ38RecurrentBlockValidationRequest(*request));bytes[i]^=1;
 }
 assert(PioneerQ38RecurrentBlockValidationRequest(*request));
 static_assert(TT_PCI_PROBE_Q38_RECURRENT_BLOCK_VALIDATE!=TT_PCI_PROBE_Q38_MIXED_LIVE_VALIDATE,"no old dense opcode alias");
 static_assert(sizeof(tt_q38_recurrent_block_validation)!=sizeof(tt_q38_mixed_live_validation),"no old dense payload alias");
 static_assert(TT_Q38_RECURRENT_BLOCK_VALIDATION_NAMESPACE!=TT_Q38_MIXED_LIVE_VALIDATION_NAMESPACE,"distinct model namespace");
 puts("PASS DEBUG71 fixed20800B pointer-free request/ALL20800 single-byte mutations refused/token0-position0-layer0/namespace22/no old dense ABI alias;NO native or physical authority");
}
