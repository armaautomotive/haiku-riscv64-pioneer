/* SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <memory>
#include <type_traits>
enum {B_DEVICE_OP_CODES_END=0x1000};
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentOutputValidationProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnOutputValidationProtocol.h"
using Reply=tt_q38_resident_output_validation;
static_assert(std::is_standard_layout<Reply>::value && std::is_trivially_copyable<Reply>::value,"fixed wire DATA only");
static_assert(TT_PCI_PROBE_Q38_RESIDENT_OUTPUT_VALIDATE!=TT_PCI_PROBE_Q38_GDN_OUTPUT_VALIDATE
 && TT_PCI_PROBE_Q38_RESIDENT_OUTPUT_VALIDATION_SUPPORT!=TT_PCI_PROBE_Q38_GDN_OUTPUT_VALIDATION_SUPPORT,"NO old DEBUG55 alias");
int main(){
 auto r=std::make_unique<Reply>();r->metadata.version=1;unsigned accepted=0,refused=0;
 for(unsigned token:{0U,1U,248319U}){r->metadata.token=token;assert(PioneerQ38ResidentOutputValidationRequest(*r));accepted++;}
 r->metadata.token=0;
 for(unsigned version:{0U,2U,UINT32_MAX}){r->metadata.version=version;assert(!PioneerQ38ResidentOutputValidationRequest(*r));refused++;}
 r->metadata.version=1;
 for(unsigned token:{248320U,UINT32_MAX}){r->metadata.token=token;assert(!PioneerQ38ResidentOutputValidationRequest(*r));refused++;}
 r->metadata.token=0;
 for(unsigned position:{1U,8191U,8192U,UINT32_MAX}){r->metadata.position=position;assert(!PioneerQ38ResidentOutputValidationRequest(*r));refused++;}
 r->metadata.position=0;
 auto* bytes=reinterpret_cast<unsigned char*>(r.get());
 for(size_t at=offsetof(tt_q38_embedding_produce,reserved);at<sizeof(r->metadata);at++){
  bytes[at]^=1;assert(!PioneerQ38ResidentOutputValidationRequest(*r));bytes[at]^=1;refused++;
 }
 for(size_t at=sizeof(r->metadata);at<offsetof(Reply,values);at+=4){
  bytes[at]=1;assert(!PioneerQ38ResidentOutputValidationRequest(*r));bytes[at]=0;refused++;
 }
 const unsigned offsets[]={TT_Q38_RESIDENT_OUTPUT_VALIDATION_CONV,TT_Q38_RESIDENT_OUTPUT_VALIDATION_HISTORY,
  TT_Q38_RESIDENT_OUTPUT_VALIDATION_GATED,TT_Q38_RESIDENT_OUTPUT_VALIDATION_OPERANDS,
  TT_Q38_RESIDENT_OUTPUT_VALIDATION_LINES,TT_Q38_RESIDENT_OUTPUT_VALIDATION_SCALES,
  TT_Q38_RESIDENT_OUTPUT_VALIDATION_RESULT,TT_Q38_RESIDENT_OUTPUT_VALIDATION_CURRENT,
  TT_Q38_RESIDENT_OUTPUT_VALIDATION_NEXT,TT_Q38_RESIDENT_OUTPUT_VALIDATION_WORDS};
 const unsigned lengths[]={10240,30720,6144,3072,768,192,5120,786432,786432};
 assert(!offsets[0] && offsets[9]==1629120);unsigned pages=0;
 for(unsigned region=0;region<9;region++){
  assert(offsets[region]+lengths[region]==offsets[region+1]);
  pages+=(lengths[region]+1023)/1024;
  for(unsigned within:{0U,lengths[region]/2,lengths[region]-1}){
   r->values[offsets[region]+within]=0x80000000;
   assert(!PioneerQ38ResidentOutputValidationRequest(*r));r->values[offsets[region]+within]=0;refused++;
  }
 }
 assert(pages==unsigned(TT_Q38_RESIDENT_OUTPUT_VALIDATION_PAGES));
 assert(PioneerQ38ResidentOutputValidationRequest(*r));
 printf("PASS combined-output DEBUG wire shape:accepted%u/refused%u/ALL metadata+header fields/ALL9regions first+middle+last/position0 only/6516736byte pointer-free HEAP reply/distinct old ABI;NO driver/IO/native grant\n",accepted,refused);
}
