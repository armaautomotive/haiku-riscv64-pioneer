/* SPDX-License-Identifier: MIT */
#include "PioneerQ38MixedLiveFinalReleaseRole.h"
// Inside the actual bank-END branch,with the global semaphore held.
// A compute-call RAII object must NOT outlive successful END:END destroys
// its context. Establish only the FD/team/thread seal here;all native,
// clock,source,hardware and disposal proofs still belong to actual END.
auto* mixedEndContext=cookie?cookie->context:NULL;
const thread_id mixedEndThread=find_thread(NULL);thread_info mixedEndInfo={};
if(!mixedEndContext || cookie->magic!=0x50313030 || sRuntimeOwner!=cookie
 || !sBankStorageOnly || !mixedEndContext->bankStorageOnly || mixedEndContext->io.q38MutableContext!=mixedEndContext
 || mixedEndContext->q38GdnCallThread!=-1 || mixedEndThread<0
 || get_thread_info(mixedEndThread,&mixedEndInfo)!=B_OK || mixedEndInfo.team!=cookie->team){
 status=B_NOT_ALLOWED;
}else{
 mixedEndContext->q38GdnCallThread=mixedEndThread;
 EndResidentSession(cookie,request.result,true);
 if(cookie->context==mixedEndContext){
  if(sRuntimeOwner!=cookie || cookie->magic!=0x50313030
   || mixedEndContext->q38GdnCallThread!=mixedEndThread || find_thread(NULL)!=mixedEndThread){
   tt_resident_session_result retained={};RetainQ38DispatchOwner(*mixedEndContext,retained);status=B_DEV_NOT_READY;
  }else mixedEndContext->q38GdnCallThread=-1; // call seal ONLY;never clear retained/unsafe/clock state
 }else if(cookie->context){
  tt_resident_session_result retained={};RetainQ38DispatchOwner(*cookie->context,retained);status=B_DEV_NOT_READY;
 }
} // cookie->context NULL after successful destruction is NEVER dereferenced
