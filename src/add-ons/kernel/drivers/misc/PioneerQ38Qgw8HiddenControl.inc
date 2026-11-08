/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_QGW8_HIDDEN_NATIVE_129
#include "../../../../../tools/pioneer/PioneerQ38Qgw8HiddenNativeRole.h"
#include "PioneerQ38Qgw8HiddenProtocol.h"
// Same FD/team/thread global call lock as START/HOLD/READ/FINISH. The
// underlying reader admits only the actual held, drained fixed terminal.
[[maybe_unused]] static __attribute__((noinline)) status_t
Q38Qgw8HiddenControl(PioneerProbeCookie* cookie,void* data,size_t length){
 namespace P=PioneerQ38Qgw8Hidden;
 if(geteuid()!=0)return B_NOT_ALLOWED;
 if(!cookie || cookie->magic!=0x50313030 || !data || length!=sizeof(P::Packet))return B_BAD_VALUE;
 thread_info thread={};status_t status=get_thread_info(find_thread(NULL),&thread);
 if(status!=B_OK || thread.team!=cookie->team)return status!=B_OK?status:B_NOT_ALLOWED;
 auto* packet=new(std::nothrow) P::Packet;if(!packet)return B_NO_MEMORY;
 status=user_memcpy(packet,data,sizeof(*packet));
 if(status!=B_OK || !P::RequestShape(*packet)){delete packet;return status!=B_OK?status:B_BAD_VALUE;}
 PioneerQ38TokenCallScope scope(cookie);auto* c=scope.Context();
 if(!c){status=scope.Status()!=B_OK?scope.Status():B_NOT_ALLOWED;delete packet;return status;}
 auto& reply=packet->reply;const auto& request=packet->request;
 reply.version=P::Version;reply.part=request.part;PioneerQ38TokenHiddenReadResult result;
 const bool okay=ReadQ38TokenNativeHiddenChunk(*c,request.part,packet->words,request.words,result);
 static_assert(sizeof(reply.detail)==sizeof(result),"whole diagnostic integer observation");
 memcpy(reply.detail,&result,sizeof(result));reply.started=result.started;
 reply.ready=okay && result.ready;reply.unsafe=result.unsafe;reply.words=reply.ready?result.wordCount:0;
 status=result.unsafe?B_DEV_NOT_READY:reply.ready?B_OK:B_NOT_ALLOWED;reply.status=status;bool retained=false;
 if(reply.ready && !P::ReplyShape(*packet)){
  // The underlying cursor has advanced; unexpected metadata is uncertain.
  tt_resident_session_result reservation={};RetainQ38DispatchOwner(*c,reservation);
  // The following copy failure need not retain this same owner twice.
  retained=true;
  reply.ready=0;reply.unsafe=1;reply.words=0;status=reply.status=B_DEV_NOT_READY;
 }
 if(!reply.ready)memset(packet->words,0,sizeof(packet->words));
 const bool retainOnCopyFailure=reply.ready || reply.unsafe;
 const status_t copied=user_memcpy(data,packet,sizeof(*packet));delete packet;
 if(copied!=B_OK){if(retainOnCopyFailure && !retained)Q38TokenControlReplyFailure(scope);return copied;}
 return status;
} // No retry, implicit END, CPU model product or Work activation.
#endif
