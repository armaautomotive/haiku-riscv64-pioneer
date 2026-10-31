/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_TOKEN_NATIVE_CONTEXT
#include "PioneerQ38TokenNativeRole.h"
#include "PioneerQ38TokenControlPacket.h"
#ifdef PIONEER_Q38_TOKEN_VALIDATION_NATIVE
#include "PioneerQ38TokenValidationProtocol.h"
#endif
#ifdef PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_NATIVE
#include "PioneerQ38TokenHiddenValidationProtocol.h"
#endif
// Shared native adapter: private NODRV tests, or the separate tag74 bounded
// validation ioctl. Neither selection grants an Assistant/Work capability.
// One genuine FD/team/thread lock covers execution AND reply copy. A failed
// reply cannot race disposal/replacement of its context by another operation.
template<class Result> static void Q38TokenControlDetail(PioneerQ38TokenControl::Reply& reply,const Result& result){
 static_assert(sizeof(Result)==64,"only fixed integer diagnostic records");
 memcpy(reply.detail,&result,64);
}
[[maybe_unused]] static __attribute__((noinline)) void Q38TokenControlReplyFailure(PioneerQ38TokenCallScope& scope){
 auto* c=scope.Context();if(!c)return; // actual successful Finish may already have destroyed it
 tt_resident_session_result retained={};RetainQ38DispatchOwner(*c,retained);
} // CPU metadata ONLY,never cancel/retry/END/reset/free or subsequent device IO

[[maybe_unused]] static __attribute__((noinline)) status_t
Q38TokenNativeControl(PioneerProbeCookie* cookie,void* data,size_t length){
 namespace P=PioneerQ38TokenControl;
 if(geteuid()!=0)return B_NOT_ALLOWED;
 if(!cookie || cookie->magic!=0x50313030 || !data || length!=sizeof(P::Packet))return B_BAD_VALUE;
 thread_info thread={};status_t status=get_thread_info(find_thread(NULL),&thread);
 if(status!=B_OK || thread.team!=cookie->team)return status!=B_OK?status:B_NOT_ALLOWED;
 auto* packet=new(std::nothrow) P::Packet;if(!packet)return B_NO_MEMORY;
 status=user_memcpy(packet,data,sizeof(*packet));
 if(status!=B_OK || !P::RequestShape(*packet)
#ifdef PIONEER_Q38_TOKEN_VALIDATION_NATIVE
  || !PioneerQ38TokenValidation::RequestShape(*packet)
#endif
#ifdef PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_NATIVE
  || !PioneerQ38TokenHiddenValidation::ControlRequestShape(*packet)
#endif
 ){delete packet;return status!=B_OK?status:B_BAD_VALUE;}
 PioneerQ38TokenCallScope scope(cookie);auto& reply=packet->reply;const auto& request=packet->request;
 reply.version=P::Version;reply.operation=request.operation;status=B_NOT_ALLOWED;bool retainOnCopyFailure=false;
 if(auto* c=scope.Context()){
  if(request.operation==P::Start){
   const auto result=BeginQ38TokenNativeBackendScoped(scope,0,packet->prompt,request.prompts,request.limit,
    request.stopEos!=0,request.eos0,request.eos1);Q38TokenControlDetail(reply,result);
   reply.started=result.started;reply.ready=result.ready;reply.unsafe=result.unsafe;reply.complete=result.complete;reply.stage=result.stage;
   status=result.unsafe?B_DEV_NOT_READY:result.ready?B_OK:B_NOT_ALLOWED;
   retainOnCopyFailure=result.ready || result.unsafe || (result.started && c->q38TokenNative);
  }else if(request.operation==P::Finish){
   const auto result=FinishQ38TokenNativeBackendScoped(scope);Q38TokenControlDetail(reply,result);
   // c may now dangle. Do not dereference it again,including on reply failure.
   reply.started=result.started;reply.ready=result.ready;reply.unsafe=result.unsafe;reply.complete=result.complete;reply.stage=result.stage;
   status=result.unsafe?B_DEV_NOT_READY:result.complete?B_OK:B_NOT_ALLOWED;
   retainOnCopyFailure=result.complete || result.unsafe;
  }else if(request.operation==P::Observe){
   PioneerQ38TokenTerminalResult result;const bool okay=ObserveQ38TokenNativeTerminal(*c,result);Q38TokenControlDetail(reply,result);
   reply.started=result.started;reply.ready=okay && result.ready;reply.unsafe=result.unsafe;reply.stage=result.phase;
   status=result.unsafe?B_DEV_NOT_READY:okay?B_OK:B_NOT_ALLOWED;retainOnCopyFailure=okay || result.unsafe;
  }else if(request.operation==P::Cancel){
   PioneerQ38TokenExecutionResult result;const bool okay=CancelQ38TokenNativeRoot(*c,result);Q38TokenControlDetail(reply,result);
   reply.started=result.started;reply.ready=okay && result.cancelRequested;reply.unsafe=result.unsafe;reply.stage=result.phase;
   status=result.unsafe?B_DEV_NOT_READY:okay?B_OK:B_NOT_ALLOWED;retainOnCopyFailure=okay || result.unsafe;
  }else if(request.operation==P::Hold){
   PioneerQ38TokenHoldResult result;const bool okay=HoldQ38TokenNativeStopped(*c,result);Q38TokenControlDetail(reply,result);
   reply.started=result.started;reply.ready=okay && result.ready;reply.unsafe=result.unsafe;reply.stage=result.phase;
   status=result.unsafe?B_DEV_NOT_READY:okay?B_OK:B_NOT_ALLOWED;retainOnCopyFailure=okay || result.unsafe;
  }else if(request.operation==P::Read){
   PioneerQ38TokenReadResult result;const bool okay=ReadQ38TokenNativeRecords(*c,request.records,
    request.records?packet->records:NULL,result);Q38TokenControlDetail(reply,result);
   reply.started=result.started;reply.ready=okay && result.ready;reply.unsafe=result.unsafe;reply.stage=result.phase;
   reply.records=okay && result.ready?result.recordCount:0;
   if(!okay || !result.ready)memset(packet->records,0,sizeof(packet->records)); // no unvalidated partial output
   status=result.unsafe?B_DEV_NOT_READY:okay?B_OK:B_NOT_ALLOWED;retainOnCopyFailure=okay || result.unsafe;
  }
 }else{
  // Admission failed before an operation. Do not copy an unlocked reply or
  // act on a context that may now belong to another caller.
  status=scope.Status()!=B_OK?scope.Status():B_NOT_ALLOWED;delete packet;return status;
 }
 reply.status=status;
 const status_t copied=user_memcpy(data,packet,sizeof(*packet));delete packet;
 if(copied!=B_OK){if(retainOnCopyFailure)Q38TokenControlReplyFailure(scope);return copied;}
 return status;
} // no CPU arithmetic/reference,implicit cleanup,public Work or loadable-driver activation
#endif
