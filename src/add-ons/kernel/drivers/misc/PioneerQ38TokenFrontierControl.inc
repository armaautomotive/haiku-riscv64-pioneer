/* SPDX-License-Identifier: MIT */
#include "PioneerQ38TokenFrontierNativeRole.h"
#include "PioneerQ38TokenFrontierPacket.h"
// Exact private adapter only. No ProbeControl route or physical driver grant.
// The SAME real FD/team/thread lock covers the counter read AND reply copy.
[[maybe_unused]] static __attribute__((noinline)) status_t
Q38TokenFrontierControl(PioneerProbeCookie* cookie,void* data,size_t length){
 namespace P=PioneerQ38TokenFrontierControl;
 if(geteuid()!=0)return B_NOT_ALLOWED;
 if(!cookie || cookie->magic!=0x50313030 || !data || length!=sizeof(P::Packet))return B_BAD_VALUE;
 thread_info thread={};status_t status=get_thread_info(find_thread(NULL),&thread);
 if(status!=B_OK || thread.team!=cookie->team)return status!=B_OK?status:B_NOT_ALLOWED;
 P::Packet packet;status=user_memcpy(&packet,data,sizeof(packet));
 if(status!=B_OK || !P::RequestShape(packet))return status!=B_OK?status:B_BAD_VALUE;
 PioneerQ38TokenCallScope scope(cookie);auto* c=scope.Context();
 if(!c)return scope.Status()!=B_OK?scope.Status():B_NOT_ALLOWED; // no unlocked reply
 PioneerQ38TokenFrontierResult result;const bool okay=ObserveQ38TokenNativeFrontier(*c,result);
 auto& reply=packet.reply;reply.version=P::Version;reply.operation=P::Observe;
 reply.started=result.started;reply.ready=okay && result.ready;reply.unsafe=result.unsafe;
 status=result.unsafe?B_DEV_NOT_READY:okay && result.ready?B_OK:B_NOT_ALLOWED;
 reply.status=status;static_assert(sizeof(reply.detail)==sizeof(result),"exact private integer result");
 memcpy(reply.detail,&result,sizeof(result));
 const status_t copied=user_memcpy(data,&packet,sizeof(packet));
 if(copied!=B_OK){if(okay || result.unsafe)Q38TokenControlReplyFailure(scope);return copied;}
 return status;
} // no request-supplied count,generation,prompt,ready,cleanup or retry command
