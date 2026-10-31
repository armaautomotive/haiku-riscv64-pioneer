/* SPDX-License-Identifier: MIT */
// Pure bounded DATA grammar,not a native/physical/Work capability.
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenValidationProtocol.h"
#include <cassert>
#include <cstdio>
#include <memory>
namespace P=PioneerQ38TokenControl;
namespace V=PioneerQ38TokenValidation;
int main(){
 auto p=std::make_unique<P::Packet>();unsigned positives=0,refusals=0;
 p->request.magic=P::Magic;p->request.version=P::Version;p->request.operation=P::Start;
 p->request.prompts=2;p->request.limit=3;p->prompt[0]=248319;p->prompt[1]=17;
 assert(P::RequestShape(*p) && V::RequestShape(*p));positives++;
 auto* bytes=reinterpret_cast<unsigned char*>(p.get());
 for(unsigned word=0;word<sizeof(*p)/4;word++){
  unsigned bit=word%32,at=word*4+bit/8,mask=1U<<(bit%8);bytes[at]^=mask;
  assert(!V::RequestShape(*p));bytes[at]^=mask;assert(V::RequestShape(*p));refusals++;
 }
 p->request.limit=4;assert(P::RequestShape(*p) && !V::RequestShape(*p));refusals++;
 p->request.limit=3;p->prompt[1]=18;assert(P::RequestShape(*p) && !V::RequestShape(*p));refusals++;
 *p={};p->request.magic=P::Magic;p->request.version=P::Version;
 for(unsigned op=P::Observe;op<=P::Finish;op++){
  p->request.operation=op;p->request.records=0;assert(V::RequestShape(*p));positives++;
 }
 p->request.operation=P::Read;
 for(unsigned records=1;records<=3;records++){p->request.records=records;assert(V::RequestShape(*p));positives++;}
 for(unsigned records:{4U,64U}){p->request.records=records;assert(P::RequestShape(*p) && !V::RequestShape(*p));refusals++;}
 assert(positives==9 && refusals==12340);
 std::printf("PASS fixed tag74 DATA packet/%u positives/%u refusals/all packet words mutated/fixed2prompt+3position/noEOS+bounded3record trial;NOT native execution,ownership,physical,Work or TPS proof\n",positives,refusals);
}
