/* SPDX-License-Identifier: MIT */
#include "pioneer_q38_token_native_control_begin_role.h"
static PioneerQ38TokenBackendResult CheckNativeControlBegin(Fixture& f){
 namespace P=PioneerQ38TokenControl;auto packet=std::make_unique<P::Packet>();
 packet->request.magic=P::Magic;packet->request.version=P::Version;packet->request.operation=P::Start;
 packet->request.prompts=2;packet->request.limit=3;packet->request.eos0=248318;packet->request.eos1=248317;
 packet->prompt[0]=248319;packet->prompt[1]=17;
 auto input=std::make_unique<P::Packet>(*packet);
 tokenControlUid=tokenControlCopies=tokenControlFailCopy=0;tokenControlLoseTeam=tokenControlMutateInput=false;
 tokenControlTouches=&f.io.touches;
 const status_t status=Q38TokenNativeControl(&f.cookie,packet.get(),sizeof(*packet));
 assert(status==B_OK && tokenControlCopies==2 && packet->reply.version==P::Version && packet->reply.operation==P::Start
  && packet->reply.started && packet->reply.ready && !packet->reply.unsafe && !packet->reply.complete
  && packet->reply.stage==8 && packet->reply.status==B_OK && !packet->reply.records
  && !memcmp(&packet->request,&input->request,sizeof(packet->request))
  && !memcmp(packet->prompt,input->prompt,sizeof(packet->prompt)));
 for(auto word:packet->reply.reserved)assert(!word);
 for(auto word:packet->records)assert(!word);
 PioneerQ38TokenBackendResult result;memcpy(&result,packet->reply.detail,sizeof(result));
 assert(!lockHeld && lockAcquires==lockReleases);
 puts("EXECUTED private control Start/one immutable49344B user snapshot/SAME genuine scoped backend8 stages/ROOT startup ACK/reply under SAME real lock;NO public ABI,token,END,physical,Work or TPS grant");
 return result;
}
