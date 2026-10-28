/* SPDX-License-Identifier: MIT */
#define B_DEVICE_OP_CODES_END 1000
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38RecurrentPrefixValidationProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38RecurrentBlockValidationProtocol.h"
#include <cassert>
#include <cstdio>
#include <memory>
int main(){
 auto r=std::make_unique<tt_q38_recurrent_prefix_validation>();r->metadata.version=1;
 assert(PioneerQ38RecurrentPrefixValidationRequest(*r));auto* bytes=reinterpret_cast<unsigned char*>(r.get());
 for(unsigned i=0;i<sizeof(*r);i++){
  bytes[i]^=1;assert(!PioneerQ38RecurrentPrefixValidationRequest(*r));bytes[i]^=1;
 }
 assert(PioneerQ38RecurrentPrefixValidationRequest(*r));
 static_assert(TT_PCI_PROBE_Q38_RECURRENT_PREFIX_VALIDATE!=TT_PCI_PROBE_Q38_RECURRENT_BLOCK_VALIDATE,"no old opcode alias");
 static_assert(TT_PCI_PROBE_Q38_RECURRENT_PREFIX_VALIDATION_SUPPORT!=TT_PCI_PROBE_Q38_RECURRENT_BLOCK_VALIDATION_SUPPORT,"no old support alias");
 static_assert(sizeof(*r)!=sizeof(tt_q38_recurrent_block_validation),"no old payload alias");
 static_assert(TT_Q38_RECURRENT_PREFIX_VALIDATION_NAMESPACE==24 && TT_Q38_RECURRENT_PREFIX_VALIDATION_BUILD_TAG==UINT64_C(0x2026102800010073),"separate RESERVED diagnostic identity");
 puts("PASS RESERVED prefix DEBUG73/namespace24/20864B pointer-free fixed token0-position0 request/ALL20864 single-byte mutations refused/distinct96word header+complete-END fields;DATA ONLY,NO driver selection,native or physical authority");
}
