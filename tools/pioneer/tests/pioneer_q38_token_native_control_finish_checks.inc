/* SPDX-License-Identifier: MIT */
#include "pioneer_q38_token_native_control_finish_role.h"
static PioneerQ38TokenBackendResult CheckNativeControlFinishCall(Fixture& f,bool loseReply,status_t& status){
 namespace P=PioneerQ38TokenControl;auto packet=std::make_unique<P::Packet>();
 packet->request.magic=P::Magic;packet->request.version=P::Version;packet->request.operation=P::Finish;
 auto input=std::make_unique<P::Packet>(*packet);
 tokenControlUid=tokenControlCopies=0;tokenControlFailCopy=loseReply?2:0;
 tokenControlLoseTeam=tokenControlMutateInput=false;tokenControlTouches=&f.io.touches;
 status=Q38TokenNativeControl(&f.cookie,packet.get(),sizeof(*packet));
 assert(tokenControlCopies==2 && !lockHeld && lockAcquires==lockReleases);
 PioneerQ38TokenBackendResult result;
 if(loseReply){
  assert(status==-6 && !memcmp(packet.get(),input.get(),sizeof(*packet)) && f.io.touches==tokenControlFailureTouches);
 }else{
  assert(packet->reply.version==P::Version && packet->reply.operation==P::Finish && packet->reply.status==status
   && !packet->reply.records && !memcmp(&packet->request,&input->request,sizeof(packet->request))
   && !memcmp(packet->prompt,input->prompt,sizeof(packet->prompt)));
  for(auto word:packet->reply.reserved)assert(!word);
  for(auto word:packet->records)assert(!word);
  memcpy(&result,packet->reply.detail,sizeof(result));
  assert(packet->reply.started==result.started && packet->reply.stage==result.stage && packet->reply.ready==result.ready
   && packet->reply.unsafe==result.unsafe && packet->reply.complete==result.complete);
 }
 return result; // a lost reply returns ONLY pristine TEST diagnostics,never a supplied completion
}
