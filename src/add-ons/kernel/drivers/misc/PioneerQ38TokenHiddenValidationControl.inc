/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_NATIVE
#include "PioneerQ38TokenHiddenValidationRole.h"
#include "PioneerQ38TokenHiddenValidationProtocol.h"
// Execution and user reply copy share the SAME genuine FD/team/thread lock.
// A copy fault retains CPU ownership metadata only;never retry/END/reset/IO.
[[maybe_unused]] static __attribute__((noinline)) status_t
Q38TokenHiddenValidationControl(PioneerProbeCookie* cookie,void* data,size_t length){
 namespace P=PioneerQ38TokenHiddenValidation;
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
 reply.version=P::Version;reply.part=request.part;
 PioneerQ38TokenHiddenReadResult result;
 const bool okay=ReadQ38TokenNativeHiddenChunk(*c,request.part,packet->words,request.words,result);
 memcpy(reply.detail,&result,sizeof(result));
 reply.started=result.started;reply.ready=okay && result.ready;reply.unsafe=result.unsafe;
 reply.words=reply.ready?result.wordCount:0;
 if(!reply.ready)memset(packet->words,0,sizeof(packet->words)); // never publish a partial failed part
 status=result.unsafe?B_DEV_NOT_READY:okay?B_OK:B_NOT_ALLOWED;reply.status=status;
 const bool retainOnCopyFailure=okay || result.unsafe;
 const status_t copied=user_memcpy(data,packet,sizeof(*packet));delete packet;
 if(copied!=B_OK){if(retainOnCopyFailure)Q38TokenControlReplyFailure(scope);return copied;}
 return status;
} // fixed canonical hidden DATA only;no following cleanup or implicit fallback
#endif
