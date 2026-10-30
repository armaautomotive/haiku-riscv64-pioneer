/* SPDX-License-Identifier: MIT */
#include "pioneer_q38_token_native_control_operations_role.h"
template<class Result> static bool CheckNativeControlOperation(Fixture& f,unsigned operation,bool ready,Result& result){
 namespace P=PioneerQ38TokenControl;static_assert(sizeof(Result)==64,"actual typed native diagnostic");
 assert(operation>=P::Observe && operation<=P::Read && !lockHeld);
 auto packet=std::make_unique<P::Packet>();packet->request.magic=P::Magic;packet->request.version=P::Version;
 packet->request.operation=operation; // Read requests the genuinely empty frontier;never supplies a committed token
 auto input=std::make_unique<P::Packet>(*packet);
 tokenControlUid=tokenControlCopies=tokenControlFailCopy=0;tokenControlLoseTeam=tokenControlMutateInput=false;
 tokenControlTouches=&f.io.touches;
 const status_t status=Q38TokenNativeControl(&f.cookie,packet.get(),sizeof(*packet));
 assert(status==B_OK && tokenControlCopies==2 && !lockHeld && lockAcquires==lockReleases
  && packet->reply.version==P::Version && packet->reply.operation==operation && packet->reply.status==status
  && packet->reply.started && packet->reply.ready==unsigned(ready) && !packet->reply.unsafe
  && !packet->reply.complete && !packet->reply.records
  && !memcmp(&packet->request,&input->request,sizeof(packet->request))
  && !memcmp(packet->prompt,input->prompt,sizeof(packet->prompt)));
 for(auto word:packet->reply.reserved)assert(!word);
 for(auto word:packet->records)assert(!word);
 memcpy(&result,packet->reply.detail,sizeof(result));
 assert(result.started==packet->reply.started && result.unsafe==packet->reply.unsafe && result.phase==packet->reply.stage);
 tokenControlOperationCounts[operation]++;
 printf("EXECUTED private control operation%u/ready%u/actual native owner+phase+FD scope/input snapshot and reply under SAME real lock;NO supplied terminal,token,drain,cleanup or public/Work/physical/TPS grant\n",operation,unsigned(ready));
 return true;
}
