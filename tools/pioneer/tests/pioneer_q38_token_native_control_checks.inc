/* SPDX-License-Identifier: MIT */
#include "pioneer_q38_token_native_control_role.h"
static void TokenControlRequest(PioneerQ38TokenControl::Packet& p,unsigned operation){
 namespace P=PioneerQ38TokenControl;memset(&p,0,sizeof(p));p.request.magic=P::Magic;p.request.version=P::Version;
 p.request.operation=operation;
 if(operation==P::Start){p.request.prompts=2;p.request.limit=3;p.request.eos0=248318;p.request.eos1=248317;p.prompt[0]=248319;p.prompt[1]=17;}
 tokenControlUid=tokenControlCopies=tokenControlFailCopy=0;tokenControlLoseTeam=tokenControlMutateInput=false;
}
static void CheckNativeControlContract(Fixture& f){
 namespace P=PioneerQ38TokenControl;auto p=std::make_unique<P::Packet>();auto& c=*f.c;auto& io=f.io;
 const unsigned before=io.touches,pci=io.nativePci.events,arc=c.q38ClockIO.rawEvents;const uint64 epoch=sQ38TokenModelEpoch;
 tokenControlTouches=&io.touches;unsigned checks=0;
 auto unchanged=[&](){assert(!c.q38TokenNative && !c.q38Dispatch && io.touches==before && io.nativePci.events==pci
  && c.q38ClockIO.rawEvents==arc && sQ38TokenModelEpoch==epoch && !sColdRecoveryRequired && !c.dispatchOwnerRetained
  && !lockHeld && lockAcquires==lockReleases);checks++;};
 TokenControlRequest(*p,P::Start);tokenControlUid=1;
 assert(Q38TokenNativeControl(&f.cookie,p.get(),sizeof(*p))==B_NOT_ALLOWED && !tokenControlCopies);unchanged();
 TokenControlRequest(*p,P::Start);f.cookie.magic^=1;
 assert(Q38TokenNativeControl(&f.cookie,p.get(),sizeof(*p))==B_BAD_VALUE && !tokenControlCopies);f.cookie.magic^=1;unchanged();
 currentTeam++;assert(Q38TokenNativeControl(&f.cookie,p.get(),sizeof(*p))==B_NOT_ALLOWED && !tokenControlCopies);currentTeam--;unchanged();
 assert(Q38TokenNativeControl(&f.cookie,nullptr,sizeof(*p))==B_BAD_VALUE && !tokenControlCopies);unchanged();
 for(size_t length:{size_t(0),sizeof(*p)-1,sizeof(*p)+1}){
  assert(Q38TokenNativeControl(&f.cookie,p.get(),length)==B_BAD_VALUE && !tokenControlCopies);unchanged();
 }
 for(unsigned variant=0;variant<4;variant++){
  TokenControlRequest(*p,P::Start);
  if(variant==0)p->reply.ready=1;
  if(variant==1)p->records[4095]=1;
  if(variant==2)p->request.operation=7;
  if(variant==3)p->prompt[0]=P::Vocabulary;
  const auto snapshot=std::make_unique<P::Packet>(*p);
  assert(Q38TokenNativeControl(&f.cookie,p.get(),sizeof(*p))==B_BAD_VALUE && tokenControlCopies==1
   && !memcmp(p.get(),snapshot.get(),sizeof(*p)));unchanged();
 }
 TokenControlRequest(*p,P::Start);tokenControlFailCopy=1;
 assert(Q38TokenNativeControl(&f.cookie,p.get(),sizeof(*p))==-6 && tokenControlCopies==1);unchanged();
 for(unsigned operation:{unsigned(P::Observe),unsigned(P::Cancel),unsigned(P::Hold),unsigned(P::Read),unsigned(P::Finish)}){
  TokenControlRequest(*p,operation);
  assert(Q38TokenNativeControl(&f.cookie,p.get(),sizeof(*p))==B_NOT_ALLOWED && tokenControlCopies==2
   && p->reply.version==1 && p->reply.operation==operation && p->reply.status==B_NOT_ALLOWED
   && !p->reply.started && !p->reply.ready && !p->reply.complete && !p->reply.unsafe);unchanged();
 }
 TokenControlRequest(*p,P::Observe);tokenControlFailCopy=2;
 assert(Q38TokenNativeControl(&f.cookie,p.get(),sizeof(*p))==-6 && tokenControlCopies==2);unchanged();
 TokenControlRequest(*p,P::Start);tokenControlLoseTeam=true;
 assert(Q38TokenNativeControl(&f.cookie,p.get(),sizeof(*p))==B_NOT_ALLOWED && tokenControlCopies==1
  && !p->reply.started && !p->reply.ready);currentTeam--;unchanged();
 printf("PASS private native control/%u privilege+FD+team+size+shape+copy+unbound-state+snapshot-race checks/zero device IO and publication/SAME lock held through replies/no refusal poison;NOT model execution/public ABI/physical/Work/TPS qualification\n",checks);
}
static void CheckNativeControlInitFault(Fixture& f,unsigned fault,uint64 epoch){
 namespace P=PioneerQ38TokenControl;auto p=std::make_unique<P::Packet>();TokenControlRequest(*p,P::Start);
 auto& c=*f.c;auto& io=f.io;tokenControlTouches=&io.touches;
 if(fault==0){io.failRun=1;tokenControlMutateInput=true;}
 if(fault==1)io.loseTeamRun=1;
 if(fault==2)io.loseGenerationRun=1;
 if(fault==3){io.noCompletion=true;tokenControlFailCopy=2;}
 const status_t result=Q38TokenNativeControl(&f.cookie,p.get(),sizeof(*p));
 assert(tokenControlCopies==2 && result==(fault==3?-6:B_DEV_NOT_READY) && c.q38TokenNative && c.q38TokenNative->Unsafe()
  && !c.q38TokenNative->Closed() && sQ38TokenModelEpoch==epoch+1 && io.rootStarts==1 && !io.rootStops && !io.rootHolds
  && !io.runtimePackets && !io.bootstrapArms && !io.executionStarts && !io.executionCpu->steps);
 if(fault<3)assert(p->reply.operation==P::Start && p->reply.started && p->reply.stage==2 && p->reply.unsafe
  && !p->reply.ready && !p->reply.complete && p->reply.status==B_DEV_NOT_READY && p->reply.detail[2]==2);
 else assert(io.touches==tokenControlFailureTouches && !p->reply.started && !p->reply.unsafe);
 for(const auto& peer:io.bootstrapPeer)assert(!peer->steps && !peer->deviceWrites);
 const unsigned touches=io.touches;f.Unknown();assert(io.touches==touches && !lockHeld && lockAcquires==lockReleases);
 printf("PASS actual private control early-init fault%u/immutable kernel input snapshot/real scoped Begin stage2 uncertainty/locked reply%s/ALL owners retained/no later device IO,bootstrap,model start,cleanup,free or retry;TEST ledger/OS/MMIO only,NOT original model/physical/Work/TPS qualification\n",fault,fault==3?" COPY FAILED;CPU-only retention":" delivered");
}
