/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_FULL_OFFLOAD_PHASE_PROFILE
#include "PioneerQ38FullOffloadPhaseProfileRole.h"
#else
#include "PioneerQ38TokenPhaseProfileRole.h"
#endif
#include "PioneerQ38TokenPhaseProfilePacket.h"
// SAME real FD/team/thread lock covers held read AND reply copy.
[[maybe_unused]] static __attribute__((noinline)) status_t
Q38TokenPhaseProfileControl(PioneerProbeCookie* cookie,void* data,size_t length){
 namespace P=PioneerQ38TokenPhaseProfileControl;
 if(geteuid()!=0)return B_NOT_ALLOWED;
 if(!cookie || cookie->magic!=0x50313030 || !data || length!=sizeof(P::Packet))return B_BAD_VALUE;
 thread_info thread={};status_t status=get_thread_info(find_thread(NULL),&thread);
 if(status!=B_OK || thread.team!=cookie->team)return status!=B_OK?status:B_NOT_ALLOWED;
 auto* packet=new(std::nothrow) P::Packet;if(!packet)return B_NO_MEMORY;
 status=user_memcpy(packet,data,sizeof(*packet));
 if(status!=B_OK || !P::RequestShape(*packet)){delete packet;return status!=B_OK?status:B_BAD_VALUE;}
 PioneerQ38TokenCallScope scope(cookie);auto* c=scope.Context();
 if(!c){status=scope.Status()!=B_OK?scope.Status():B_NOT_ALLOWED;delete packet;return status;}
 PioneerQ38TokenPhaseProfileResult result;
 const bool okay=ReadQ38TokenNativePhaseProfile(*c,packet->words,result);
 auto& reply=packet->reply;reply.version=P::Version;reply.operation=P::Read;
 reply.started=result.started;reply.ready=okay && result.ready;reply.unsafe=result.unsafe;
 status=result.unsafe?B_DEV_NOT_READY:okay && result.ready?B_OK:B_NOT_ALLOWED;
 reply.status=status;static_assert(sizeof(reply.detail)==sizeof(result),"fixed held observation detail");
 memcpy(reply.detail,&result,sizeof(result));
 if(!okay || !result.ready)memset(packet->words,0,sizeof(packet->words));
 const status_t copied=user_memcpy(data,packet,sizeof(*packet));delete packet;
 if(copied!=B_OK){if(okay || result.unsafe)Q38TokenControlReplyFailure(scope);return copied;}
 return status;
} // invalid timing DATA is NOT unsafe/END permission;normal explicit cleanup remains required.
