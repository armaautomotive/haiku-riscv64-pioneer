/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4
#include "PioneerQ38PreparedSourceProtocol.h"
static __attribute__((noinline)) status_t
Q38PreparedSourceControl(PioneerProbeCookie* cookie,void* data,size_t length){
 namespace P=PioneerQ38PreparedSource;
 if(geteuid()!=0)return B_NOT_ALLOWED;
 if(!cookie || cookie->magic!=0x50313030 || !data || length!=sizeof(P::Packet))return B_BAD_VALUE;
 thread_info thread={};status_t status=get_thread_info(find_thread(NULL),&thread);
 if(status!=B_OK || thread.team!=cookie->team)return status!=B_OK?status:B_NOT_ALLOWED;
 auto* packet=new(std::nothrow) P::Packet;if(!packet)return B_NO_MEMORY;
 status=user_memcpy(packet,data,sizeof(*packet));
 if(status!=B_OK || !P::RequestShape(*packet)){delete packet;return status!=B_OK?status:B_BAD_VALUE;}
 PioneerQ38TokenCallScope scope(cookie);auto* c=scope.Context();bool okay=false;
 const auto& q=packet->request;auto& r=packet->reply;r.operation=q.operation;
 if(c && !c->q38TokenNative && c->io.TokenModelSourceScope(true)){
  if(q.operation==P::Begin && !c->q38PreparedSource){
   c->q38PreparedSource=new(std::nothrow) PioneerQ38PreparedNativeSource;
   if(c->q38PreparedSource)okay=c->q38PreparedSource->Begin(c,cookie,c->hashes,c->initialized,cookie->team,sReloadGeneration,
    c->banks,c->baseline.baseline.baseline.baseline.dram,q.data+16,P::Parts,PioneerResidentBanks::Pages);
  }else if(q.operation==P::Phase && c->q38PreparedSource)okay=c->q38PreparedSource->Accept(q.index,q.data,q.words);
  else if(q.operation==P::Seal && c->q38PreparedSource)okay=c->q38PreparedSource->Seal();
  if(c->q38PreparedSource){r.ready=c->q38PreparedSource->Ready();r.phases=c->q38PreparedSource->AcceptedPhases();r.parts=P::Parts;}
 }
 r.accepted=okay;r.status=okay?B_OK:B_NOT_ALLOWED;
 const status_t copied=user_memcpy(data,packet,sizeof(*packet));status=r.status;delete packet;
 if(copied!=B_OK){if(okay && c && c->q38PreparedSource)c->q38PreparedSource->Abandon();return copied;}
 return status;
} // SAME FD/team/thread lock. Metadata only:no NoC packet,code install,launch/reset or cleanup grant.
#endif
