/* SPDX-License-Identifier: MIT */
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38Qgw8HiddenProtocol.h"
#include <cassert>
#include <cstdio>
#include <memory>

int main(){
 namespace P=PioneerQ38Qgw8Hidden;
 namespace C=PioneerQ38TokenControl;
 auto control=std::make_unique<C::Packet>();auto& start=control->request;
 start.magic=C::Magic;start.version=C::Version;start.operation=C::Start;start.prompts=start.limit=1;control->prompt[0]=P::Input;
 assert(P::ControlRequestShape(*control));
 control->prompt[0]--;assert(!P::ControlRequestShape(*control));control->prompt[0]=P::Input;
 start.limit=2;assert(!P::ControlRequestShape(*control));start.limit=1;
 start.prompts=0;assert(!P::ControlRequestShape(*control));start.prompts=1;
 *control=C::Packet{};control->request.magic=C::Magic;control->request.version=C::Version;
 control->request.operation=C::Read;control->request.records=1;assert(P::ControlRequestShape(*control));
 control->request.records=0;assert(!P::ControlRequestShape(*control));
 control->request.records=2;assert(!P::ControlRequestShape(*control));
 control->request.operation=C::Cancel;control->request.records=0;assert(!P::ControlRequestShape(*control));
 for(unsigned part=0;part<P::Parts;part++){
  auto packet=std::make_unique<P::Packet>();auto& q=packet->request;
  q.magic=P::Magic;q.version=P::Version;q.part=part;q.words=P::PartWords(part);
  assert(P::RequestShape(*packet) && !P::ReplyShape(*packet));
  auto changed=*packet;changed.request.reserved[5]=1;assert(!P::RequestShape(changed));
  changed=*packet;changed.request.part=2;assert(!P::RequestShape(changed));
  changed=*packet;changed.request.words++;assert(!P::RequestShape(changed));
  changed=*packet;changed.reply.ready=1;assert(!P::RequestShape(changed));
  changed=*packet;changed.words[0]=1;assert(!P::RequestShape(changed));
  auto& r=packet->reply;r.version=P::Version;r.part=part;r.started=r.ready=1;r.words=q.words;
  const uint32_t detail[]={1,1,4,0,0,0,1,part,part*P::ChunkWords,q.words,0,64,1,1,1,q.words*4};
  for(unsigned i=0;i<16;i++)r.detail[i]=detail[i];
  for(unsigned i=0;i<q.words;i++)packet->words[i]=i?i:0x7fc00000U;
  assert(P::ReplyShape(*packet)); // raw nonfinite bits remain diagnostic DATA
  for(unsigned i=0;i<16;i++){changed=*packet;changed.reply.detail[i]^=1;assert(!P::ReplyShape(changed));}
  for(unsigned i=0;i<9;i++){changed=*packet;changed.reply.reserved[i]=1;assert(!P::ReplyShape(changed));}
  changed=*packet;changed.reply.unsafe=1;assert(!P::ReplyShape(changed));
  changed=*packet;changed.reply.status=-1;assert(!P::ReplyShape(changed));
  if(part){changed=*packet;changed.words[q.words]=1;assert(!P::ReplyShape(changed));}
 }
 puts("PASS fixed QGW8 hidden DATA packet shape and malformed replies; no device or authority");
}
