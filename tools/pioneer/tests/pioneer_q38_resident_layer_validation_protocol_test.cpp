/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdio>
#include <cstdint>
#include <memory>
#include <type_traits>
#define PIONEER_Q38_GDN_MUTABLE_STATE 1 // canonical DATA geometry ONLY
enum {B_DEVICE_OP_CODES_END=0x1000};
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentLayerValidationProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentOutputValidationProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentLayerReadbackPlan.h"
using Reply=tt_q38_resident_layer_validation;
static_assert(std::is_standard_layout<Reply>::value && std::is_trivially_copyable<Reply>::value,"fixed HEAP wire DATA");
static_assert(TT_PCI_PROBE_Q38_RESIDENT_LAYER_VALIDATE!=TT_PCI_PROBE_Q38_RESIDENT_OUTPUT_VALIDATE
 && TT_PCI_PROBE_Q38_RESIDENT_LAYER_VALIDATION_SUPPORT!=TT_PCI_PROBE_Q38_RESIDENT_OUTPUT_VALIDATION_SUPPORT
 && unsigned(TT_Q38_RESIDENT_LAYER_VALIDATION_NAMESPACE)!=unsigned(TT_Q38_RESIDENT_OUTPUT_VALIDATION_NAMESPACE)
 && unsigned(TT_Q38_RESIDENT_LAYER_VALIDATION_WORDS)==unsigned(PioneerQ38ResidentLayerReadbackPlan::Words)
 && unsigned(TT_Q38_RESIDENT_LAYER_VALIDATION_PAGES)==unsigned(PioneerQ38ResidentLayerReadbackPlan::Pages),"new69 NEVER aliases old68,exact private QLV1 data only");
int main(){
 auto r=std::make_unique<Reply>();r->metadata.version=1;assert(PioneerQ38ResidentLayerValidationRequest(*r));unsigned refused=0;
 for(unsigned token:{1U,248319U,248320U,UINT32_MAX}){r->metadata.token=token;assert(!PioneerQ38ResidentLayerValidationRequest(*r));refused++;}r->metadata.token=0;
 for(unsigned position:{1U,8191U,8192U,UINT32_MAX}){r->metadata.position=position;assert(!PioneerQ38ResidentLayerValidationRequest(*r));refused++;}r->metadata.position=0;
 auto* bytes=reinterpret_cast<unsigned char*>(r.get());
 // Every fixed header bit except version's already-accepted value is
 // forbidden as input. No success/owner/metadata flag can be supplied.
 for(unsigned word=0;word<64;word++)for(unsigned bit=0;bit<32;bit++){
  bytes[word*4+bit/8]^=uint8_t(1U<<(bit%8));assert(!PioneerQ38ResidentLayerValidationRequest(*r));
  bytes[word*4+bit/8]^=uint8_t(1U<<(bit%8));refused++;
 }
 const unsigned offsets[]={0,5120,15360,46080,52224,57344,843776},lengths[]={5120,10240,30720,6144,5120,786432,786432};
 unsigned words=0;for(unsigned region=0;region<7;region++){
  assert(offsets[region]==words);words+=lengths[region];
  for(unsigned i:{0U,lengths[region]/2,lengths[region]-1}){
   r->values[offsets[region]+i]=0x80000000;assert(!PioneerQ38ResidentLayerValidationRequest(*r));r->values[offsets[region]+i]=0;refused++;
  }
 }
 assert(words==1630208 && PioneerQ38ResidentLayerValidationRequest(*r));
 printf("PASS NEW DEBUG69 namespace20/fixed6521088-byte pointer-free HEAP reply/ALL64header bits+7regions/%urefusals/ONLYtoken0position0layer0/old68incompatible;shape NOT native or cleanup authority\n",refused);
}
