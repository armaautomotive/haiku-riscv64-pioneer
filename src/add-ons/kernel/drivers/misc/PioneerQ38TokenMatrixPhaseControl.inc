/* SPDX-License-Identifier: MIT */
#include "../../../../../tools/pioneer/PioneerQ38TokenMatrixPhaseProgramsRole.h"
#include "PioneerQ38TokenMatrixPhasePacket.h"
#if !defined(PIONEER_Q38_TOKEN_MATRIX_PHASE_BENCHMARK_NATIVE) || PIONEER_Q38_TOKEN_MATRIX_PHASE_BENCHMARK_NATIVE != 1
#error Only the distinct public sampler diagnostic may route this read
#endif
static_assert(PioneerQ38TokenMatrixPhaseDataSelection==3 && PioneerQ38TokenMatrixPhaseReadSelection==2,
 "actual public sampler+held reader required");
[[maybe_unused]] static __attribute__((noinline)) status_t
Q38TokenMatrixPhaseControl(PioneerProbeCookie* cookie,void* data,size_t length){
 namespace P=PioneerQ38TokenMatrixPhaseControl;
 if(geteuid()!=0)return B_NOT_ALLOWED;
 if(!cookie || cookie->magic!=0x50313030 || !data || length!=sizeof(P::Packet))return B_BAD_VALUE;
 thread_info thread={};status_t status=get_thread_info(find_thread(NULL),&thread);
 if(status!=B_OK || thread.team!=cookie->team)return status!=B_OK?status:B_NOT_ALLOWED;
 auto* packet=new(std::nothrow) P::Packet;if(!packet)return B_NO_MEMORY;
 status=user_memcpy(packet,data,sizeof(*packet));
 if(status!=B_OK || !P::RequestShape(*packet)){delete packet;return status!=B_OK?status:B_BAD_VALUE;}
 PioneerQ38TokenCallScope scope(cookie);auto* c=scope.Context();
 if(!c){status=scope.Status()!=B_OK?scope.Status():B_NOT_ALLOWED;delete packet;return status;}
 PioneerQ38TokenMatrixPhaseResult result;
 bool okay=ReadQ38TokenNativeMatrixPhase(*c,packet->request.worker,packet->words,result),replyFailureRetained=false;
 auto& reply=packet->reply;reply.version=P::Version;reply.operation=P::Read;reply.worker=packet->request.worker;
 if(okay && result.ready){
  reply.stopNonce=c->q38TokenNative->MatrixPhaseStopNonce(reply.worker);
  if(!reply.stopNonce || reply.stopNonce>=0x7fffffff
   || PioneerQ38TokenMatrixPhaseData::Classify(packet->words,reply.worker,reply.stopNonce)!=result.metricState){
   // A missing sealed journal is not repaired from the candidate record.
   Q38TokenControlReplyFailure(scope);replyFailureRetained=true;okay=false;reply.stopNonce=0;
   result.unsafe=1;result.ready=result.valid=0;result.phase=3;
   result.metricState=result.nonce=result.inputs=result.groups=0;
  }
 }
 reply.started=result.started;reply.ready=okay && result.ready;reply.unsafe=result.unsafe;
 status=result.unsafe?B_DEV_NOT_READY:okay && result.ready?B_OK:B_NOT_ALLOWED;
 reply.status=status;static_assert(sizeof(reply.detail)==sizeof(result),"fixed sampler observation detail");
 memcpy(reply.detail,&result,sizeof(result));
 if(!okay || !result.ready)memset(packet->words,0,sizeof(packet->words));
 if(!P::ReplyShape(*packet)){
  if(!replyFailureRetained)Q38TokenControlReplyFailure(scope);
  delete packet;return B_DEV_NOT_READY; // no malformed internal reply is copied
 }
 const status_t copied=user_memcpy(data,packet,sizeof(*packet));delete packet;
 if(copied!=B_OK){if((okay || result.unsafe) && !replyFailureRetained)Q38TokenControlReplyFailure(scope);return copied;}
 return status;
} // One fixed held peer read;no commands,reset,DMA,restore,END or Work grant.
