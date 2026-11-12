/* SPDX-License-Identifier: MIT */
// Candidate response validation only. No driver, model, or device.
#define B_DEVICE_OP_CODES_END 1000
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38QgwdResidentProtocol.h"
#include <cassert>
#include <cstdio>
int main(){
 namespace R=PioneerQ38QgwdResident;
 R::Support support;assert(R::SupportRequest(support) && !R::Supported(support));
 support={55,1,1,1,1,1,{}};assert(R::Supported(support));
 support.reserved[1]=1;assert(!R::Supported(support));
 for(unsigned op=R::RequestClose;op<=R::ModelEnd;op++){
  R::Packet p;p.request.operation=op;assert(R::RequestShape(p));
  auto& r=p.reply;r.operation=op;r.started=r.ready=1;r.hostsReleased=3;
  if(op==R::RequestClose){r.stage=10;r.reservationRetained=1;}
  else if(op==R::NextRequest){r.stage=11;r.reservationRetained=r.oldContextRetired=1;}
  else{r.stage=12;r.modelEnded=1;}
  assert(R::SuccessfulReply(r,op));
  auto bad=r;bad.unsafe=1;assert(!R::SuccessfulReply(bad,op));
  bad=r;bad.operation=op%3+1;assert(!R::SuccessfulReply(bad,op));
  bad=r;bad.stage++;assert(!R::SuccessfulReply(bad,op));
  bad=r;bad.hostsReleased=2;assert(!R::SuccessfulReply(bad,op));
  bad=r;bad.reserved[4]=1;assert(!R::SuccessfulReply(bad,op));
  bad=r;bad.reservationRetained^=1;assert(!R::SuccessfulReply(bad,op));
  bad=r;bad.oldContextRetired^=1;assert(!R::SuccessfulReply(bad,op));
  bad=r;bad.modelEnded^=1;assert(!R::SuccessfulReply(bad,op));
 }
 puts("PASS resident protocol exact support and reply shape; NO DEVICE OR MODEL");
}
